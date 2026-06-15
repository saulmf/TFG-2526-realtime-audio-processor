#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_data_structures/juce_data_structures.h>

#include "../effects/IAudioEffect.h"

/**
 * EffectChain owns an ordered sequence of audio effects and processes
 * an AudioBuffer through them sequentially on each callback.
 *
 * This class implements the Chain of Responsibility pattern - each effect
 * in the chain receives the buffer in order and decides independently
 * whether to process it or pass it through (based on its enabled state).
 *
 * Thread safety:
 *   - All structural modification methods (add, remove, move) must be
 *     called from the GUI thread only, and only while the audio engine
 *     is stopped. Hot-swapping during a live session is not supported.
 *   - process() is called exclusively from the audio thread. No structural
 *     modifications may occur concurrently with process().
 *   - setEffectEnabled() is the only method safe to call from the GUI
 *     thread during an active session, as the enabled flag is atomic
 *     inside AudioEffectBase.
 */
class EffectChain {
public:
    EffectChain() = default;

    ~EffectChain() = default;

    // Lifecycle - called by AudioEngine, audio thread

    /**
     * Prepares all effects in the chain for processing.
     * Must be called before the first process() call.
     * Safe to call multiple times (e.g. if sample rate changes).
     * @param spec Sample rate, block size, and channel count for the upcoming session.
     */
    void prepare(const juce::dsp::ProcessSpec &spec);

    /**
     * Resets the internal state of all effects (clears delay lines, filter histories, etc.) without changing parameters.
     * Called by AudioEngine when the session stops.
     */
    void reset();

    /**
     * Processes the buffer through all effects in order.
     * Called exclusively from the audio thread.
     * No allocation, no locking, no logging inside this method.
     * @param buffer Buffer to process in place; passed unmodified to each enabled effect in sequence.
     */
    void process(juce::AudioBuffer<float> &buffer);

    // Chain modification - GUI thread only, engine must be stopped

    /**
     * Appends an effect at the end of the chain.
     * Takes ownership of the effect via unique_ptr.
     * @param effect Fully constructed effect to add; ownership is transferred.
     */
    void addEffect(std::unique_ptr<IAudioEffect> effect);

    /**
     * Removes the effect at the given index from the chain.
     * Remaining effects preserve their relative order.
     * @param index Zero-based position of the effect to remove.
     */
    void removeEffect(int index);

    /**
     * Moves the effect at fromIndex to toIndex, shifting intermediate effects accordingly.
     * @param fromIndex Current position of the effect to move.
     * @param toIndex Target position.
     */
    void moveEffect(int fromIndex, int toIndex);

    // Per-effect control - safe to call from GUI thread during a session

    /**
     * Enables or disables the effect at the given index without removing it from the chain.
     * The change takes effect within the current or next processing block.
     * @param index Zero-based position of the effect.
     * @param enabled Pass true to enable processing, false to bypass.
     */
    void setEffectEnabled(int index, bool enabled);

    // Queries - GUI thread

    /** Returns the number of effects currently in the chain. */
    int getNumEffects() const;

    /** Returns true if the chain contains no effects. */
    bool isEmpty() const;

    /**
     * Returns a raw non-owning pointer to the effect at the given index.
     * The pointer is valid only as long as the effect remains in the chain.
     * @param index Zero-based position in the chain.
     * @return Pointer to the effect, or nullptr if the index is out of range.
     */
    IAudioEffect *getEffect(int index);

    /** @overload */
    const IAudioEffect *getEffect(int index) const;

    // Preset serialization

    /**
     * Serializes the full chain state (effect order, enabled/disabled states, and all parameter values) into a ValueTree.
     * @return ValueTree that can be passed to setState() to fully reconstruct the chain.
     */
    juce::ValueTree getState() const;

    /**
     * Reconstructs the chain from a previously serialised ValueTree.
     * The EffectFactory is used to instantiate each effect by type ID.
     * Completely replaces the current chain contents.
     * @param state ValueTree as returned by getState().
     * @param factory Used to instantiate each effect by its stored type ID.
     * @return Type IDs of any effects that were skipped because they were not recognised by the factory.
     */
    juce::StringArray setState(const juce::ValueTree &state, const class EffectFactory &factory);

private:
    /** Returns true if @p index is within [0, numEffects). */
    bool isValidIndex(int index) const;

    std::vector<std::unique_ptr<IAudioEffect> > m_effects;
    juce::dsp::ProcessSpec m_currentSpec{};
    bool m_isPrepared{false};

    // Serialisation tag constants
    static constexpr const char *k_chainTag = "EffectChain";
    static constexpr const char *k_effectTag = "Effect";
    static constexpr const char *k_typeAttr = "typeId";
    static constexpr const char *k_enabledAttr = "enabled";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectChain)
};
