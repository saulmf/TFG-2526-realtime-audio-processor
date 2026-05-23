#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "EffectChain.h"
#include "EffectFactory.h"

/**
 * AudioEngine manages the audio device lifecycle and the real-time processing callback.
 *
 * Thread safety:
 *   - All public configuration methods (setInputDevice, setSampleRate, etc.)
 *     must be called from the GUI thread only, and only while the engine is stopped.
 *   - audioDeviceIOCallbackWithContext() is called exclusively by the audio thread.
 *      No GUI thread methods may be called from within it.
 */
class AudioEngine : public juce::AudioIODeviceCallback {
public:
    AudioEngine();

    ~AudioEngine() override;

    // Device configuration - GUI thread only, call before start()

    /** Returns all audio input device names available on the host system. */
    juce::StringArray getAvailableInputDevices() const;

    /** Returns all audio output device names available on the host system. */
    juce::StringArray getAvailableOutputDevices() const;

    /** Returns the currently selected input device name, or empty if none. */
    juce::String getCurrentInputDeviceName() const;

    /** Returns the currently selected output device name, or empty if none. */
    juce::String getCurrentOutputDeviceName() const;

    /** Selects the audio input device by name. Returns true on success. */
    bool setInputDevice(const juce::String &deviceName);

    /** Selects the audio output device by name. Returns true on success. */
    bool setOutputDevice(const juce::String &deviceName);

    /** Sets the desired sample rate. Must be one of 44100, 48000, or 96000. */
    bool setSampleRate(double sampleRate);

    /** Sets the desired buffer size in samples. */
    bool setBufferSize(int bufferSizeInSamples);

    double getCurrentSampleRate() const { return m_currentSampleRate; }
    int getCurrentBufferSize() const { return m_currentBufferSize; }

    static constexpr int k_defaultBufferSize{256};
    static constexpr double k_defaultSampleRate{44100.0};

    static const juce::Array<int> k_validBufferSizes;
    static const juce::Array<double> k_validSampleRates;


    // Session control - GUI thread only

    /**
     * Starts the real-time audio session.
     * Validates that input and output devices are selected before opening the device stream.
     * Returns an empty string on success, or an error message on failure.
     */
    juce::String start();

    /** Stops the real-time audio session and closes the device stream. */
    void stop();

    bool isRunning() const { return m_isRunning.load(); }

    // Subsystem access - GUI thread only

    EffectChain &getEffectChain() { return m_effectChain; }
    const EffectChain &getEffectChain() const { return m_effectChain; }

    EffectFactory &getEffectFactory() { return m_effectFactory; }
    const EffectFactory &getEffectFactory() const { return m_effectFactory; }

    // Raw input/output peak levels in the range [0, 1+].
    // Written by the audio thread, read by the GUI thread - safe via std::atomic relaxed ordering.
    float getInputLevel() const noexcept { return m_inputLevel.load(std::memory_order_relaxed); }
    float getOutputLevel() const noexcept { return m_outputLevel.load(std::memory_order_relaxed); }

    // Master output volume [0, 1].
    // Written by the GUI thread, read by the audio thread - safe via std::atomic relaxed ordering.
    void setMasterVolume(float v) noexcept {
        m_masterVolume.store(juce::jlimit(0.0f, 1.0f, v), std::memory_order_relaxed);
    }

    float getMasterVolume() const noexcept { return m_masterVolume.load(std::memory_order_relaxed); }


    // juce::AudioIODeviceCallback - audio thread only, do not call directly
    void audioDeviceIOCallbackWithContext(
        const float *const*inputChannelData,
        int numInputChannels,
        float *const*outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext &context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override;

    void audioDeviceStopped() override;

    void audioDeviceError(const juce::String &errorMessage) override;

private:
    // Helpers

    /** Rebuilds the AudioDeviceSetup from current member state and applies it. */
    juce::String applyDeviceSettings();

    juce::AudioDeviceManager m_deviceManager;
    EffectFactory m_effectFactory;
    EffectChain m_effectChain;

    // Pre-allocated work buffer - never resized inside the audio callback
    juce::AudioBuffer<float> m_workBuffer;

    // Current configuration
    juce::String m_inputDeviceName{};
    juce::String m_outputDeviceName{};
    double m_currentSampleRate{k_defaultSampleRate};
    int m_currentBufferSize{k_defaultBufferSize};

    std::atomic<bool> m_isRunning{false};
    std::atomic<float> m_inputLevel{0.0f};
    std::atomic<float> m_outputLevel{0.0f};
    std::atomic<float> m_masterVolume{1.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
