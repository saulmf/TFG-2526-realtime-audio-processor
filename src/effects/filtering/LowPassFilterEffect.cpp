#include "LowPassFilterEffect.h"

// Parameter layout

juce::AudioProcessorValueTreeState::ParameterLayout
LowPassFilterEffect::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter> > params;

    // Cutoff frequency - skewed so the knob feels logarithmic
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "cutoff", "Cutoff",
        juce::NormalisableRange<float>(k_minCutoffHz, k_maxCutoffHz, 1.0f, 0.25f),
        1000.0f));

    // Order - each unit adds one 2nd-order stage (+12 dB/oct rolloff)
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "order", "Order", 1, k_maxStages, 2));

    return {params.begin(), params.end()};
}


// Constructor

LowPassFilterEffect::LowPassFilterEffect()
    : AudioEffectBase() {
    initialiseAPVTS(createParameterLayout());

    m_cutoffParam = getAPVTS().getRawParameterValue("cutoff");
    m_orderParam = getAPVTS().getRawParameterValue("order");

    jassert(m_cutoffParam != nullptr);
    jassert(m_orderParam != nullptr);

    DBG("[LowPassFilterEffect] Constructed. cutoff="
        + juce::String(m_cutoffParam->load(), 0) + " Hz"
        + "  order=" + juce::String(static_cast<int>(m_orderParam->load())));
}


// AudioEffectBase hooks

void LowPassFilterEffect::prepareToProcess(const juce::dsp::ProcessSpec &spec) {
    for (auto &f: m_filters) {
        f.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        f.prepare(spec);
    }
}

void LowPassFilterEffect::resetState() {
    for (auto &f: m_filters)
        f.reset();
}

void LowPassFilterEffect::processBlock(juce::AudioBuffer<float> &buffer) {
    // -------------------------------------------------------------------------
    // AUDIO THREAD - no allocation, no locking, no logging.
    // StateVariableTPTFilter::setCutoffFrequency() is allocation-free.
    // Only the first 'order' stages are processed; the rest are idle.
    // -------------------------------------------------------------------------

    const float cutoff = m_cutoffParam->load(std::memory_order_relaxed);
    const int order = juce::roundToInt(m_orderParam->load(std::memory_order_relaxed));

    juce::dsp::AudioBlock<float> block(buffer);

    for (int i = 0; i < order; ++i) {
        m_filters[i].setCutoffFrequency(cutoff);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        m_filters[i].process(ctx);
    }
}


// Preset serialization

juce::ValueTree LowPassFilterEffect::getState() const {
    juce::ValueTree state("Parameters");
    state.setProperty("cutoff", m_cutoffParam->load(), nullptr);
    state.setProperty("order", m_orderParam->load(), nullptr);
    return state;
}

void LowPassFilterEffect::setState(const juce::ValueTree &state) {
    auto restore = [&](const char *id) {
        if (!state.hasProperty(id)) return;
        auto *p = getAPVTS().getParameter(id);
        if (p != nullptr)
            p->setValueNotifyingHost(
                p->convertTo0to1(state.getProperty(id)));
    };

    restore("cutoff");
    restore("order");
}
