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

    /** Returns all audio input device names available on the host system.
        @return Empty array if no input devices are detected. */
    juce::StringArray getAvailableInputDevices() const;

    /** Returns all audio output device names available on the host system.
        @return Empty array if no output devices are detected. */
    juce::StringArray getAvailableOutputDevices() const;

    /** Returns the currently selected input device name.
        @return Empty string if no input device has been selected. */
    juce::String getCurrentInputDeviceName() const;

    /** Returns the currently selected output device name.
        @return Empty string if no output device has been selected. */
    juce::String getCurrentOutputDeviceName() const;

    /** Selects the audio input device by name.
        @param deviceName Name as returned by getAvailableInputDevices().
        @return true on success, false if the device was not found or could not be opened. */
    bool setInputDevice(const juce::String &deviceName);

    /** Selects the audio output device by name.
        @param deviceName Name as returned by getAvailableOutputDevices().
        @return true on success, false if the device was not found or could not be opened. */
    bool setOutputDevice(const juce::String &deviceName);

    /** Sets the desired sample rate. Must be one of 44100, 48000, or 96000.
        @param sampleRate Target sample rate in Hz.
        @return true if the value was accepted; false if it is not in k_validSampleRates. */
    bool setSampleRate(double sampleRate);

    /** Sets the desired buffer size in samples. Must be one of the values in k_validBufferSizes.
        @param bufferSizeInSamples Target block size.
        @return true if the value was accepted; false if it is not in k_validBufferSizes. */
    bool setBufferSize(int bufferSizeInSamples);

    /** Returns the sample rate currently in use (or the pending value if the session has not started). */
    double getCurrentSampleRate() const { return m_currentSampleRate; }

    /** Returns the buffer size in samples currently in use (or the pending value if the session has not started). */
    int getCurrentBufferSize() const { return m_currentBufferSize; }

    static constexpr int k_defaultBufferSize{256};
    static constexpr double k_defaultSampleRate{44100.0};

    static const juce::Array<int> k_validBufferSizes;
    static const juce::Array<double> k_validSampleRates;


    // Session control - GUI thread only

    /**
     * Starts the real-time audio session.
     * Validates that input and output devices are selected before opening the device stream.
     * @return Empty string on success, or a descriptive error message on failure.
     */
    juce::String start();

    /** Stops the real-time audio session and closes the device stream. */
    void stop();

    /** Returns true if the audio session is currently active. */
    bool isRunning() const { return m_isRunning.load(); }

    // Subsystem access - GUI thread only

    /** Returns a reference to the effect chain owned by this engine. */
    EffectChain &getEffectChain() { return m_effectChain; }

    /** @overload */
    const EffectChain &getEffectChain() const { return m_effectChain; }

    /** Returns a reference to the effect factory owned by this engine. */
    EffectFactory &getEffectFactory() { return m_effectFactory; }

    /** @overload */
    const EffectFactory &getEffectFactory() const { return m_effectFactory; }

    /**
     * Returns the most recent raw input peak in the range [0, 1+].
     * Written by the audio thread, read by the GUI thread via relaxed atomic load.
     */
    float getInputLevel() const noexcept { return m_inputLevel.load(std::memory_order_relaxed); }

    /**
     * Returns the most recent raw output peak in the range [0, 1+].
     * Written by the audio thread, read by the GUI thread via relaxed atomic load.
     */
    float getOutputLevel() const noexcept { return m_outputLevel.load(std::memory_order_relaxed); }

    /**
     * Sets the master output gain applied after the effect chain.
     * @param v Volume in the range [0, 1]; values outside this range are clamped.
     * @note Written by the GUI thread, read by the audio thread via relaxed atomic store.
     */
    void setMasterVolume(float v) noexcept {
        m_masterVolume.store(juce::jlimit(0.0f, 1.0f, v), std::memory_order_relaxed);
    }

    /** Returns the current master volume in the range [0, 1]. */
    float getMasterVolume() const noexcept { return m_masterVolume.load(std::memory_order_relaxed); }


    // juce::AudioIODeviceCallback - audio thread only, do not call directly

    /**
     * Main real-time audio callback.
     * Reads mono input, runs it through the effect chain, applies master volume, and writes to all output channels.
     * @warning Audio thread only. No allocation, no locking, no logging inside this method.
     */
    void audioDeviceIOCallbackWithContext(
        const float *const*inputChannelData,
        int numInputChannels,
        float *const*outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext &context) override;

    /**
     * Called by JUCE just before the audio device opens the stream.
     * Pre-allocates the work buffer and prepares the effect chain with the confirmed device parameters.
     * @param device The device that is about to start; used to read confirmed sample rate and block size.
     */
    void audioDeviceAboutToStart(juce::AudioIODevice *device) override;

    /**
     * Called by JUCE when the audio device stream closes.
     * Resets the effect chain state and clears the level meters.
     */
    void audioDeviceStopped() override;

    /**
     * Called by JUCE on a device error.
     * Logs the error and marks the session as stopped so the GUI can react.
     * @param errorMessage Descriptive description of the device error.
     */
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
