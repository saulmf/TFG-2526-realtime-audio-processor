#pragma once

#include <juce_core/juce_core.h>

#include "audio/AudioEngine.h"
#include "audio/PresetManager.h"
#include "effects/IAudioEffect.h"

/**
 * ApplicationController is the facade between the GUI layer and the audio subsystem.
 *
 * The GUI exclusively interacts with this class (never touches AudioEngine, EffectChain, EffectFactory, or PresetManager).
 *
 * Responsibilities:
 *   - Exposes a high-level, GUI-oriented API.
 *   - Coordinates multi-subsystem operations (EffectFactory, EffectChain, PresetManager...)
 *   - Owns the PresetManager (stateless, lives here)
 *   - Holds a reference to AudioEngine (owned by main.cpp)
 *
 * Thread safety: all methods must be called from the GUI thread only, except where noted.
 */
class ApplicationController {
public:
    explicit ApplicationController(AudioEngine &audioEngine);

    ~ApplicationController() = default;

    // Session control

    /**
     * Starts the real-time audio session.
     * @return Empty string on success, or a descriptive error message on failure.
     */
    juce::String start();

    /** Stops the real-time audio session. The effect chain is preserved. */
    void stop();

    /** Returns true if the audio session is currently running. */
    bool isRunning() const;

    // Device configuration - call before start()

    /** Returns all audio input device names available on the host system.
        @return Empty array if no input devices are detected. */
    [[nodiscard]] juce::StringArray getAvailableInputDevices() const;

    /** Returns all audio output device names available on the host system.
        @return Empty array if no output devices are detected. */
    [[nodiscard]] juce::StringArray getAvailableOutputDevices() const;

    /** Returns the currently selected input device name.
        @return Empty string if no input device has been selected. */
    [[nodiscard]] juce::String getCurrentInputDeviceName() const;

    /** Returns the currently selected output device name.
        @return Empty string if no output device has been selected. */
    [[nodiscard]] juce::String getCurrentOutputDeviceName() const;

    /** Selects the audio input device by name.
        @param deviceName Name as returned by getAvailableInputDevices().
        @return true on success, false if the device was not found or could not be opened. */
    bool setInputDevice(const juce::String &deviceName);

    /** Selects the audio output device by name.
        @param deviceName Name as returned by getAvailableOutputDevices().
        @return true on success, false if the device was not found or could not be opened. */
    bool setOutputDevice(const juce::String &deviceName);

    /** Sets the desired sample rate. Must be one of the values in getValidSampleRates().
        @param sampleRate Target sample rate in Hz.
        @return true if the value was accepted; false if it is not a supported rate. */
    bool setSampleRate(double sampleRate);

    /** Sets the desired buffer size in samples. Must be one of the values in getValidBufferSizes().
        @param bufferSizeInSamples Target block size.
        @return true if the value was accepted; false if it is not a supported size. */
    bool setBufferSize(int bufferSizeInSamples);

    /** Returns the sample rate currently in use (or the pending value if the session has not started). */
    [[nodiscard]] double getCurrentSampleRate() const;

    /** Returns the buffer size in samples currently in use (or the pending value if the session has not started). */
    [[nodiscard]] int getCurrentBufferSize() const;

    /**
     * Returns the most recent raw input peak in the range [0, 1+].
     * Can exceed 1.0 when the input device is already clipping.
     */
    [[nodiscard]] float getInputLevel() const;

    /**
     * Returns the most recent raw output peak in the range [0, 1+].
     * Can exceed 1.0 when effects boost the signal.
     */
    [[nodiscard]] float getOutputLevel() const;

    /** Sets the master output gain.
        @param v Volume in the range [0, 1]; values outside this range are clamped. */
    void setMasterVolume(float v);

    /** Returns the current master volume in the range [0, 1]. */
    [[nodiscard]] float getMasterVolume() const;

    /** Returns the list of supported buffer sizes, in samples. */
    [[nodiscard]] static const juce::Array<int> &getValidBufferSizes();

    /** Returns the list of supported sample rates, in Hz. */
    [[nodiscard]] static const juce::Array<double> &getValidSampleRates();


    // Effect chain management

    /**
     * Creates the effect with the given type ID and appends it to the end of the chain.
     * Does nothing if the type ID is not registered.
     * @param typeId Registered type ID of the effect to create (e.g. "overdrive").
     */
    void addEffect(const juce::String &typeId);

    /** Removes the effect at the given index from the chain.
        @param index Zero-based position of the effect to remove. */
    void removeEffect(int index);

    /** Removes all effects from the chain. */
    void clearChain();

    /** Moves the effect at fromIndex to toIndex, shifting intermediate effects accordingly.
        @param fromIndex Current position of the effect to move.
        @param toIndex Target position. */
    void moveEffect(int fromIndex, int toIndex);

    /** Enables or disables the effect at the given index without removing it from the chain.
        @param index Zero-based position of the effect.
        @param enabled Pass true to enable processing, false to bypass. */
    void setEffectEnabled(int index, bool enabled);

    /** Returns the number of effects currently in the chain. */
    [[nodiscard]] int getNumEffects() const;

    /**
     * Returns a raw non-owning pointer to the effect at the given index.
     * @param index Zero-based position in the chain.
     * @return Pointer to the effect, or nullptr if the index is out of range.
     */
    [[nodiscard]] IAudioEffect *getEffect(int index);

    /**
     * Returns the type IDs of all registered effects, sorted alphabetically.
     * @return StringArray of type IDs; used to populate the add-effect list in the GUI.
     */
    [[nodiscard]] juce::StringArray getAvailableEffectTypes() const;

    // Maximum number of effects allowed in the chain (2 rows x 4 columns)
    static constexpr int k_maxEffects = 8;


    // Preset management

    /** Saves the current chain state to an XML file.
        @param file Destination file; created or overwritten if it already exists.
        @return true on success, false if the file could not be written. */
    bool savePreset(const juce::File &file);

    /**
     * Loads and restores a chain state from an XML file.
     * @param file Source XML preset file.
     * @param outSkippedTypeIds Populated with any type IDs present in the file but not recognised by the factory.
     * @return true on success, false if the file is missing or malformed.
     */
    bool loadPreset(const juce::File &file, juce::StringArray &outSkippedTypeIds);

private:
    AudioEngine &m_audioEngine;
    PresetManager m_presetManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApplicationController)
};
