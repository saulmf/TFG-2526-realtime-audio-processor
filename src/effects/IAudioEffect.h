#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>

/**
 * IAudioEffect defines the contract that every audio effect in the chain must fulfil.
 * Pure abstract interface (no data members, no implementation, no constructors).
 *
 * Direct inheritance from IAudioEffect is reserved for AudioEffectBase.
 *
 * Implementors must guarantee:
 *   - prepare() is called before the first process() call.
 *   - process() is called exclusively from the audio thread.
 *   - process() performs no memory allocation, no blocking calls, and no logging.
 *   - getState() and setState() are called from the GUI thread only.
 */
class IAudioEffect {
public:
    virtual ~IAudioEffect() = default;


    // Lifecycle

    /**
     * Prepares the effect for processing with the given spec.
     *
     * Must be called before the first process() invocation and again whenever the sample rate or block size changes.
     *
     * Called from the GUI thread before the session starts.
     * @param spec Sample rate, block size, and channel count for the upcoming session.
     */
    virtual void prepare(const juce::dsp::ProcessSpec &spec) = 0;

    /**
     * Resets the effect's internal DSP state (delay lines, filter memories, envelope followers...) without altering parameters.
     *
     * Called when the audio session stops.
     */
    virtual void reset() = 0;


    // Processing - audio thread only

    /**
     * Processes the audio buffer in place through this effect.
     *
     * If the effect is disabled, the buffer passes through unmodified.
     *
     * @param buffer Buffer to process in place; channel count and size must match the spec given to prepare().
     * @note No memory allocation, locking, or blocking operations are permitted inside this method or any method it calls.
     */
    virtual void process(juce::AudioBuffer<float> &buffer) = 0;


    // Identity

    /**
     * Returns the unique string identifier for this effect type.
     *
     * Used by EffectFactory and PresetManager to serialize and reconstruct effects by type.
     *
     * @return Stable type ID string (e.g. "overdrive", "reverb").
     * @note Must be stable across versions (changing it breaks saved presets).
     */
    virtual juce::String getTypeId() const = 0;

    /**
     * Returns the descriptive display name for this effect.
     *
     * Used by the GUI to label effect strips in the chain panel.
     * @return Display name (e.g. "Overdrive", "Digital Delay").
     */
    virtual juce::String getName() const = 0;


    // Enable / disable

    /**
     * Returns true if the effect is currently enabled.
     *
     * A disabled effect passes the signal through unmodified.
     * @return true if active, false if bypassed.
     */
    virtual bool isEnabled() const = 0;

    /**
     * Enables or disables the effect.
     *
     * Must be implemented with an atomic flag so it is safe to call from the GUI thread while the audio thread is running.
     * @param enabled Pass true to enable processing, false to bypass.
     */
    virtual void setEnabled(bool enabled) = 0;


    // Parameter access

    /**
     * Returns a reference to the AudioProcessorValueTreeState that owns this effect's parameters.
     *
     * Used by the GUI to bind sliders and knobs to parameters in a thread-safe manner via APVTS attachments.
     * @return Reference to the effect's APVTS.
     */
    virtual juce::AudioProcessorValueTreeState &getAPVTS() = 0;


    // Preset serialization

    /**
     * Serializes the current parameter state of this effect into a ValueTree subtree.
     *
     * Called by EffectChain::getState().
     *
     * @return ValueTree containing the effect's type ID, enabled state, and all parameter values.
     * @warning Must be called from the GUI thread only.
     */
    virtual juce::ValueTree getState() const = 0;

    /**
     * Restores the parameter state of this effect from a previously serialised ValueTree.
     *
     * Called by EffectChain::setState().
     *
     * @param state ValueTree as returned by getState().
     * @warning Must be called from the GUI thread only.
     */
    virtual void setState(const juce::ValueTree &state) = 0;

protected:
    IAudioEffect() = default; ///< Protected to prevent direct instantiation; only subclasses may construct via this path.

public:
    IAudioEffect(const IAudioEffect &) = delete;             ///< Deleted: DSP state (buffers, atomics, APVTS) cannot be safely duplicated.
    IAudioEffect &operator=(const IAudioEffect &) = delete;  ///< Deleted: DSP state (buffers, atomics, APVTS) cannot be safely duplicated.
    IAudioEffect(IAudioEffect &&) = delete;                  ///< Deleted: effects are owned exclusively by EffectChain via unique_ptr; moving them would leave the chain in an inconsistent state.
    IAudioEffect &operator=(IAudioEffect &&) = delete;       ///< Deleted: effects are owned exclusively by EffectChain via unique_ptr.
};