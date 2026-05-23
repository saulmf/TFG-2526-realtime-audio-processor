#include "AudioEngine.h"

// Static constants

const juce::Array<int> AudioEngine::k_validBufferSizes{64, 128, 256, 512, 1024};

const juce::Array<double> AudioEngine::k_validSampleRates{44100.0, 48000.0, 96000.0};


AudioEngine::AudioEngine() {
    // Initialize device manager - 1 input channel and 1 output channel for mono processing
    const juce::String error = m_deviceManager.initialiseWithDefaultDevices(1, 1);

    if (error.isNotEmpty())
    DBG("AudioEngine: DeviceManager initialisation warning: " + error);
}

AudioEngine::~AudioEngine() {
    stop();
    m_deviceManager.removeAudioCallback(this);
}

// Device configuration

juce::StringArray AudioEngine::getAvailableInputDevices() const {
    juce::StringArray inputNames;

    if (auto *deviceType = m_deviceManager.getCurrentDeviceTypeObject())
        inputNames = deviceType->getDeviceNames(true);

    return inputNames;
}

juce::StringArray AudioEngine::getAvailableOutputDevices() const {
    juce::StringArray outputNames;

    if (auto *deviceType = m_deviceManager.getCurrentDeviceTypeObject())
        outputNames = deviceType->getDeviceNames(false);

    return outputNames;
}

juce::String AudioEngine::getCurrentInputDeviceName() const {
    return m_inputDeviceName;
}

juce::String AudioEngine::getCurrentOutputDeviceName() const {
    return m_outputDeviceName;
}

bool AudioEngine::setInputDevice(const juce::String &deviceName) {
    if (getAvailableInputDevices().contains(deviceName)) {
        m_inputDeviceName = deviceName;
        return true;
    }

    DBG("AudioEngine::setInputDevice - device not found: " + deviceName);
    return false;
}

bool AudioEngine::setOutputDevice(const juce::String &deviceName) {
    if (getAvailableOutputDevices().contains(deviceName)) {
        m_outputDeviceName = deviceName;
        return true;
    }

    DBG("AudioEngine::setOutputDevice - device not found: " + deviceName);
    return false;
}

bool AudioEngine::setSampleRate(double sampleRate) {
    if (k_validSampleRates.contains(sampleRate)) {
        m_currentSampleRate = sampleRate;
        return true;
    }

    DBG("AudioEngine::setSampleRate - invalid rate: " + juce::String(sampleRate));
    return false;
}

bool AudioEngine::setBufferSize(int samplesBufferSize) {
    if (k_validBufferSizes.contains(samplesBufferSize)) {
        m_currentBufferSize = samplesBufferSize;
        return true;
    }

    DBG("AudioEngine::setBufferSize - invalid size: " + juce::String(samplesBufferSize));
    return false;
}

// Session control

juce::String AudioEngine::start() {
    if (m_isRunning.load())
        return {};

    if (m_inputDeviceName.isEmpty())
        return "No input device selected. Please select an audio input device.";

    if (m_outputDeviceName.isEmpty())
        return "No output device selected. Please select an audio output device.";

    const juce::String error = applyDeviceSettings();

    if (error.isNotEmpty())
        return error;

    m_deviceManager.addAudioCallback(this);
    m_isRunning.store(true);

    return {};
}

void AudioEngine::stop() {
    if (!m_isRunning.load())
        return;

    m_deviceManager.removeAudioCallback(this);
    m_isRunning.store(false);
}


// Private helpers
juce::String AudioEngine::applyDeviceSettings() {
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    m_deviceManager.getAudioDeviceSetup(setup);

    setup.inputDeviceName = m_inputDeviceName;
    setup.outputDeviceName = m_outputDeviceName;
    setup.sampleRate = m_currentSampleRate;
    setup.bufferSize = m_currentBufferSize;
    setup.useDefaultInputChannels = false;
    setup.useDefaultOutputChannels = false;
    setup.inputChannels.setRange(0, 1, true); // mono input - channel 0
    setup.outputChannels.setRange(0, 2, true); // stereo output - channels 0 and 1

    return m_deviceManager.setAudioDeviceSetup(setup, true);
}

// juce::AudioIODeviceCallback
void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice *device) {
    const double sampleRate = device->getCurrentSampleRate();
    const int bufferSize = device->getCurrentBufferSizeSamples();

    // Pre-allocate the work buffer here (not inside the callback)
    m_workBuffer.setSize(1, bufferSize, false, true, false);

    // Prepare the effect chain with the confirmed device parameters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(bufferSize);
    spec.numChannels = 1;

    m_effectChain.prepare(spec);

    DBG("AudioEngine: session started - "
        + juce::String(sampleRate) + " Hz, "
        + juce::String(bufferSize) + " samples/block");
}

void AudioEngine::audioDeviceStopped() {
    m_effectChain.reset();
    m_inputLevel.store(0.0f, std::memory_order_relaxed);
    m_outputLevel.store(0.0f, std::memory_order_relaxed);
    DBG("AudioEngine: session stopped.");
}

void AudioEngine::audioDeviceError(const juce::String &errorMessage) {
    // Log the error:
    // The GUI layer should poll isRunning() or use a listener to detect the condition and inform the user
    DBG("AudioEngine: device error - " + errorMessage);
    m_isRunning.store(false);
}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float *const*inputChannelData,
    int numInputChannels,
    float *const*outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext &context) {
    // Suppress unused parameter warnings (JUCE requires this signature)
    juce::ignoreUnused(context);

    // Safety guard - should never happen if start() validated correctly
    if (numInputChannels == 0 || numOutputChannels == 0) {
        for (int ch = 0; ch < numOutputChannels; ++ch)
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
        return;
    }

    // -----------------------------------------------------------------------
    // REAL-TIME AUDIO THREAD - rules enforced from here:
    //   - No memory allocation (no new, no malloc, no std::vector::push_back)
    //   - No blocking calls (no mutexes, no file I/O, no logging to disk)
    //   - No DBG() calls (they allocate internally)
    // -----------------------------------------------------------------------

    // Measure raw input peak before processing (no allocation, no blocking)
    float inputPeak = 0.0f;
    const float *rawIn = inputChannelData[0];
    for (int i = 0; i < numSamples; ++i)
        inputPeak = juce::jmax(inputPeak, std::abs(rawIn[i]));
    m_inputLevel.store(inputPeak, std::memory_order_relaxed);

    // Copy mono input (channel 0) into the pre-allocated work buffer
    m_workBuffer.copyFrom(0, 0, inputChannelData[0], numSamples);

    // Process through the effect chain
    m_effectChain.process(m_workBuffer);

    // Apply master volume before output (output meter reflects the actual device level)
    m_workBuffer.applyGain(m_masterVolume.load(std::memory_order_relaxed));

    // Measure output peak after processing
    float outputPeak = 0.0f;
    const float *processed = m_workBuffer.getReadPointer(0);
    for (int i = 0; i < numSamples; ++i)
        outputPeak = juce::jmax(outputPeak, std::abs(processed[i]));
    m_outputLevel.store(outputPeak, std::memory_order_relaxed);

    // Write the processed mono signal to all output channels
    for (int ch = 0; ch < numOutputChannels; ++ch) {
        juce::FloatVectorOperations::copy(outputChannelData[ch],
                                          m_workBuffer.getReadPointer(0),
                                          numSamples);
    }
}
