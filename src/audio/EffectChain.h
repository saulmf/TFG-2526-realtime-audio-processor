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
     */
    void process(juce::AudioBuffer<float> &buffer);

    // Chain modification - GUI thread only, engine must be stopped

    /**
     * Appends an effect at the end of the chain.
     * If position is specified and valid, inserts at that index instead.
     * Takes ownership of the effect via unique_ptr.
     */
    void addEffect(std::unique_ptr<IAudioEffect> effect, int position = -1);

    /**
     * Removes the effect at the given index from the chain.
     * Remaining effects preserve their relative order.
     */
    void removeEffect(int index);

    /**
     * Moves the effect at fromIndex to toIndex, shifting intermediate effects accordingly.
     */
    void moveEffect(int fromIndex, int toIndex);

    // Per-effect control - safe to call from GUI thread during a session

    /**
     * Enables or disables the effect at the given index without removing it from the chain.
     * The change takes effect within the current or next processing block.
     */
    void setEffectEnabled(int index, bool enabled);

    // Queries - GUI thread

    int getNumEffects() const;

    bool isEmpty() const;

    /**
     * Returns a raw non-owning pointer to the effect at the given index, or nullptr if the index is out of range.
     * The pointer is valid only as long as the effect remains in the chain.
     */
    IAudioEffect *getEffect(int index);

    const IAudioEffect *getEffect(int index) const;

    // Preset serialization

    /**
     * Serializes the full chain state (effect order, enabled/disabled states, and all parameter values) into a ValueTree.
     */
    juce::ValueTree getState() const;

    /**
     * Reconstructs the chain from a previously serialised ValueTree.
     * The EffectFactory is used to instantiate each effect by type ID.
     * Completely replaces the current chain contents.
     * Returns the type IDs of any effects that were skipped because they were not recognised by the factory.
     */
    juce::StringArray setState(const juce::ValueTree &state, const class EffectFactory &factory);

private:
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
