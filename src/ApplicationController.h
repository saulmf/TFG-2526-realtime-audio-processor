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
     * Returns an empty string on success, or a human-readable error message.
     */
    juce::String start();

    /** Stops the real-time audio session. The effect chain is preserved. */
    void stop();

    /** Returns true if the audio session is currently running. */
    bool isRunning() const;

    // Device configuration - call before start()

    [[nodiscard]] juce::StringArray getAvailableInputDevices() const;

    [[nodiscard]] juce::StringArray getAvailableOutputDevices() const;

    [[nodiscard]] juce::String getCurrentInputDeviceName() const;

    [[nodiscard]] juce::String getCurrentOutputDeviceName() const;

    bool setInputDevice(const juce::String &deviceName);

    bool setOutputDevice(const juce::String &deviceName);

    bool setSampleRate(double sampleRate);

    bool setBufferSize(int bufferSizeInSamples);

    [[nodiscard]] double getCurrentSampleRate() const;

    [[nodiscard]] int getCurrentBufferSize() const;

    // Raw input/output peak levels [0, 1+] (can exceed 1.0 when effects boost the signal or the input is already clipping)
    [[nodiscard]] float getInputLevel() const;

    [[nodiscard]] float getOutputLevel() const;

    // Master output volume [0, 1]. Default 1.0 (full volume).
    void setMasterVolume(float v);

    [[nodiscard]] float getMasterVolume() const;

    [[nodiscard]] static const juce::Array<int> &getValidBufferSizes();

    [[nodiscard]] static const juce::Array<double> &getValidSampleRates();


    // Effect chain management

    /**
     * Creates the effect with the given type ID and appends it to the chain.
     * If position is a valid index, inserts at that position instead.
     * Does nothing if the type ID is not registered.
     */
    void addEffect(const juce::String &typeId, int position = -1);

    void removeEffect(int index);

    void clearChain();

    void moveEffect(int fromIndex, int toIndex);

    void setEffectEnabled(int index, bool enabled);

    [[nodiscard]] int getNumEffects() const;

    [[nodiscard]] IAudioEffect *getEffect(int index);

    /**
     * Returns the type IDs of all registered effects, sorted alphabetically.
     * (Used to populate the add-effect list in the GUI).
     */
    [[nodiscard]] juce::StringArray getAvailableEffectTypes() const;

    // Maximum number of effects allowed in the chain (2 rows x 4 columns)
    static constexpr int k_maxEffects = 8;


    // Preset management

    /** Saves the current chain state to an XML file. Returns true on success. */
    bool savePreset(const juce::File &file);

    /** Loads and restores a chain state from an XML file. Returns true on success. */
    bool loadPreset(const juce::File &file);

private:
    AudioEngine &m_audioEngine;
    PresetManager m_presetManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApplicationController)
};
