#include "ParametricEQEffect.h"

// Helpers

namespace {
    // Builds a NormalisableRange for frequency with logarithmic skew so that
    // the control feels linear on a log scale (more resolution in the lows).
    juce::NormalisableRange<float> freqRange() {
        return {
            ParametricEQEffect::k_minFreqHz,
            ParametricEQEffect::k_maxFreqHz,
            1.0f, // step
            0.25f // skew: < 1.0 gives more resolution at the low end
        };
    }

    // Builds a NormalisableRange for Q with mild skew toward lower values
    // where most practical filter shapes live (0.1–3.0).
    juce::NormalisableRange<float> qRange() {
        return {
            ParametricEQEffect::k_minQ,
            ParametricEQEffect::k_maxQ,
            0.01f,
            0.5f
        };
    }
}

// Parameter layout

juce::AudioProcessorValueTreeState::ParameterLayout
ParametricEQEffect::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter> > params;

    // ---- Band 1 (low) -------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "freq1", "Low Freq", freqRange(), 200.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain1", "Low Gain",
        juce::NormalisableRange<float>(k_minGainDb, k_maxGainDb, 0.1f),
        0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "q1", "Low Q", qRange(), 0.7f));

    // ---- Band 2 (mid) -------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "freq2", "Mid Freq", freqRange(), 1000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain2", "Mid Gain",
        juce::NormalisableRange<float>(k_minGainDb, k_maxGainDb, 0.1f),
        0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "q2", "Mid Q", qRange(), 0.7f));

    // ---- Band 3 (high) ------------------------------------------------------
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "freq3", "High Freq", freqRange(), 5000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain3", "High Gain",
        juce::NormalisableRange<float>(k_minGainDb, k_maxGainDb, 0.1f),
        0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "q3", "High Q", qRange(), 0.7f));

    return {params.begin(), params.end()};
}

// Constructor

ParametricEQEffect::ParametricEQEffect()
    : AudioEffectBase() {
    initialiseAPVTS(createParameterLayout());

    m_freq1Param = getAPVTS().getRawParameterValue("freq1");
    m_gain1Param = getAPVTS().getRawParameterValue("gain1");
    m_q1Param = getAPVTS().getRawParameterValue("q1");

    m_freq2Param = getAPVTS().getRawParameterValue("freq2");
    m_gain2Param = getAPVTS().getRawParameterValue("gain2");
    m_q2Param = getAPVTS().getRawParameterValue("q2");

    m_freq3Param = getAPVTS().getRawParameterValue("freq3");
    m_gain3Param = getAPVTS().getRawParameterValue("gain3");
    m_q3Param = getAPVTS().getRawParameterValue("q3");

    jassert(m_freq1Param && m_gain1Param && m_q1Param);
    jassert(m_freq2Param && m_gain2Param && m_q2Param);
    jassert(m_freq3Param && m_gain3Param && m_q3Param);

    DBG("[ParametricEQEffect] Constructed. Bands: "
        + juce::String(m_freq1Param->load(), 0) + " Hz / "
        + juce::String(m_freq2Param->load(), 0) + " Hz / "
        + juce::String(m_freq3Param->load(), 0) + " Hz");
}


// AudioEffectBase hooks

void ParametricEQEffect::prepareToProcess(const juce::dsp::ProcessSpec &spec) {
    m_sampleRate = spec.sampleRate;

    // Initialize all three bands with their default coefficients so the chain
    // starts with a flat response (0 dB gain on all bands).
    auto makePeak = [&](double freq, double gainDb, double q) {
        return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            m_sampleRate, freq, q,
            juce::Decibels::decibelsToGain(static_cast<float>(gainDb)));
    };

    *m_chain.get<0>().state = *makePeak(200.0, 0.0, 0.7);
    *m_chain.get<1>().state = *makePeak(1000.0, 0.0, 0.7);
    *m_chain.get<2>().state = *makePeak(5000.0, 0.0, 0.7);

    m_chain.prepare(spec);
}

void ParametricEQEffect::resetState() {
    m_chain.reset();
}

void ParametricEQEffect::processBlock(juce::AudioBuffer<float> &buffer) {
    // -------------------------------------------------------------------------
    // AUDIO THREAD.
    // IIR coefficient updates use juce::dsp::IIR::Coefficients::makePeakFilter
    // which allocates a small temporary object.
    // (standard JUCE EQ pattern - the values are copied into the pre-existing state and the temporary is immediately freed).
    // No other allocations occur here.
    // -------------------------------------------------------------------------

    const float freq1 = m_freq1Param->load(std::memory_order_relaxed);
    const float gain1 = m_gain1Param->load(std::memory_order_relaxed);
    const float q1 = m_q1Param->load(std::memory_order_relaxed);

    const float freq2 = m_freq2Param->load(std::memory_order_relaxed);
    const float gain2 = m_gain2Param->load(std::memory_order_relaxed);
    const float q2 = m_q2Param->load(std::memory_order_relaxed);

    const float freq3 = m_freq3Param->load(std::memory_order_relaxed);
    const float gain3 = m_gain3Param->load(std::memory_order_relaxed);
    const float q3 = m_q3Param->load(std::memory_order_relaxed);

    *m_chain.get<0>().state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        m_sampleRate, freq1, q1, juce::Decibels::decibelsToGain(gain1));

    *m_chain.get<1>().state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        m_sampleRate, freq2, q2, juce::Decibels::decibelsToGain(gain2));

    *m_chain.get<2>().state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        m_sampleRate, freq3, q3, juce::Decibels::decibelsToGain(gain3));

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    m_chain.process(ctx);
}


// Preset serialization

juce::ValueTree ParametricEQEffect::getState() const {
    juce::ValueTree state("Parameters");

    state.setProperty("freq1", m_freq1Param->load(), nullptr);
    state.setProperty("gain1", m_gain1Param->load(), nullptr);
    state.setProperty("q1", m_q1Param->load(), nullptr);

    state.setProperty("freq2", m_freq2Param->load(), nullptr);
    state.setProperty("gain2", m_gain2Param->load(), nullptr);
    state.setProperty("q2", m_q2Param->load(), nullptr);

    state.setProperty("freq3", m_freq3Param->load(), nullptr);
    state.setProperty("gain3", m_gain3Param->load(), nullptr);
    state.setProperty("q3", m_q3Param->load(), nullptr);

    return state;
}

void ParametricEQEffect::setState(const juce::ValueTree &state) {
    auto restore = [&](const char *id) {
        if (!state.hasProperty(id)) return;
        auto *p = getAPVTS().getParameter(id);
        if (p != nullptr)
            p->setValueNotifyingHost(
                p->convertTo0to1(state.getProperty(id)));
    };

    restore("freq1");
    restore("gain1");
    restore("q1");
    restore("freq2");
    restore("gain2");
    restore("q2");
    restore("freq3");
    restore("gain3");
    restore("q3");
}
