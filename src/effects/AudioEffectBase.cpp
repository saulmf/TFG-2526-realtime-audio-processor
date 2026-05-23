#include "AudioEffectBase.h"


AudioEffectBase::AudioEffectBase() = default;


// APVTS initialisation

void AudioEffectBase::initialiseAPVTS(
    juce::AudioProcessorValueTreeState::ParameterLayout layout) {
    jassert(m_apvts == nullptr); // must be called exactly once

    m_apvts = std::make_unique<juce::AudioProcessorValueTreeState>(
        m_dummyProcessor,
        nullptr, // no UndoManager
        "Parameters", // root ValueTree identifier
        std::move(layout));
}


// IAudioEffect - lifecycle

void AudioEffectBase::prepare(const juce::dsp::ProcessSpec &spec) {
    m_currentSpec = spec;
    prepareToProcess(spec);
}

void AudioEffectBase::reset() {
    resetState();
}

void AudioEffectBase::process(juce::AudioBuffer<float> &buffer) {
    // -------------------------------------------------------------------------
    // AUDIO THREAD - this method is final and must remain allocation-free.
    //
    // Null Object / bypass pattern: if disabled, return immediately and leave the buffer unmodified.
    // The change is visible within the current block because m_enabled is atomic.
    // -------------------------------------------------------------------------

    if (!m_enabled.load(std::memory_order_relaxed))
        return;

    processBlock(buffer);
}

// IAudioEffect - enable / disable

bool AudioEffectBase::isEnabled() const {
    return m_enabled.load(std::memory_order_relaxed);
}

void AudioEffectBase::setEnabled(bool enabled) {
    m_enabled.store(enabled, std::memory_order_relaxed);
}


// IAudioEffect - parameter access

juce::AudioProcessorValueTreeState &AudioEffectBase::getAPVTS() {
    jassert(m_apvts != nullptr); // initialiseAPVTS() must have been called first
    return *m_apvts;
}
