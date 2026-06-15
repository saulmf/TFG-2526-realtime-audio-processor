#include "EffectChain.h"
#include "EffectFactory.h"
#include "../effects/IAudioEffect.h"

// Lifecycle

void EffectChain::prepare(const juce::dsp::ProcessSpec &spec) {
    m_currentSpec = spec;

    for (auto &effect: m_effects)
        effect->prepare(spec);

    m_isPrepared = true;
}

void EffectChain::reset() {
    for (auto &effect: m_effects)
        effect->reset();
}

void EffectChain::process(juce::AudioBuffer<float> &buffer) {
    // -------------------------------------------------------------------------
    // AUDIO THREAD - no allocation, no locking, no logging.
    // Each effect handles its own bypass internally (AudioEffectBase::process).
    // -------------------------------------------------------------------------

    for (auto &effect: m_effects)
        effect->process(buffer);
}

// Chain modification

void EffectChain::addEffect(std::unique_ptr<IAudioEffect> effect) {
    jassert(effect != nullptr);

    // If the engine is already running and prepared,
    // bring the new effect up to speed before inserting it into the chain.
    if (m_isPrepared) {
        effect->prepare(m_currentSpec);
    }

    m_effects.push_back(std::move(effect));
}

void EffectChain::removeEffect(int index) {
    if (!isValidIndex(index)) {
        jassertfalse;
        return;
    }

    m_effects.erase(m_effects.begin() + index); // Remaining effects maintain their relative order
}

void EffectChain::moveEffect(int fromIndex, int toIndex) {
    if (!isValidIndex(fromIndex) || !isValidIndex(toIndex)) {
        jassertfalse;
        return;
    }

    if (fromIndex == toIndex)
        return;

    // Rotate the element into its new position without any re-allocation using std::rotate
    // (no-copy way to move one element within a vector while preserving the order of all others)
    if (fromIndex < toIndex) {
        std::rotate(m_effects.begin() + fromIndex,
                    m_effects.begin() + fromIndex + 1,
                    m_effects.begin() + toIndex + 1);
    } else {
        std::rotate(m_effects.begin() + toIndex,
                    m_effects.begin() + fromIndex,
                    m_effects.begin() + fromIndex + 1);
    }
}

// Per-effect control

void EffectChain::setEffectEnabled(int index, bool enabled) {
    if (!isValidIndex(index)) {
        jassertfalse;
        return;
    }

    m_effects[static_cast<std::size_t>(index)]->setEnabled(enabled);
    // The atomic write inside setEnabled() is immediately visible to the audio thread on its next process() call
}

// Queries

int EffectChain::getNumEffects() const {
    return static_cast<int>(m_effects.size());
}

bool EffectChain::isEmpty() const {
    return m_effects.empty();
}

IAudioEffect *EffectChain::getEffect(int index) {
    if (!isValidIndex(index))
        return nullptr;

    return m_effects[static_cast<std::size_t>(index)].get();
}

const IAudioEffect *EffectChain::getEffect(int index) const {
    if (!isValidIndex(index))
        return nullptr;

    return m_effects[static_cast<std::size_t>(index)].get();
}

// Preset serialization

juce::ValueTree EffectChain::getState() const {
    juce::ValueTree chainState(k_chainTag);

    for (const auto &effect: m_effects) {
        // Each effect serializes its own parameters into a ValueTree subtree.
        juce::ValueTree effectNode(k_effectTag);

        // We wrap it with the chain-level metadata (typeId, enabled).
        effectNode.setProperty(k_typeAttr, effect->getTypeId(), nullptr);
        effectNode.setProperty(k_enabledAttr, effect->isEnabled(), nullptr);

        // Append the effect's own parameter state as a child
        effectNode.appendChild(effect->getState(), nullptr);
        chainState.appendChild(effectNode, nullptr);
    }

    return chainState;
}

juce::StringArray EffectChain::setState(const juce::ValueTree &state,
                                        const EffectFactory &factory) {
    juce::StringArray skipped;

    if (!state.hasType(k_chainTag)) {
        jassertfalse;
        return skipped;
    }

    // Clear the current chain before rebuilding
    m_effects.clear();

    for (int i = 0; i < state.getNumChildren(); ++i) {
        const juce::ValueTree effectNode = state.getChild(i);

        if (!effectNode.hasType(k_effectTag))
            continue;

        const juce::String typeId = effectNode
                .getProperty(k_typeAttr)
                .toString();

        // Reconstruct the effect instance via the factory
        auto effect = factory.create(typeId);

        if (effect == nullptr) {
            DBG("EffectChain::setState - unknown effect type: " + typeId);
            skipped.add(typeId);
            continue;
        }

        // Restore enabled/disabled state
        const bool enabled = effectNode.getProperty(k_enabledAttr, true);
        effect->setEnabled(enabled);

        // Restore parameter values from the child ValueTree
        const juce::ValueTree paramState = effectNode.getChild(0);
        if (paramState.isValid())
            effect->setState(paramState);

        // Prepare the effect if the chain is already prepared
        if (m_isPrepared)
            effect->prepare(m_currentSpec);

        m_effects.push_back(std::move(effect));
    }

    return skipped;
}

// Private helpers

bool EffectChain::isValidIndex(int index) const {
    return index >= 0 && index < static_cast<int>(m_effects.size());
}
