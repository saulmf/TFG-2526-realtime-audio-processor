#include "FuzzEffect.h"

// Parameter layout

juce::AudioProcessorValueTreeState::ParameterLayout
FuzzEffect::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter> > params;

    // Fuzz - controls how hard the signal is pushed into the hard clipper.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fuzz", "Fuzz",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    // Level - output gain after the distortion stage.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "level", "Level",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.8f));

    return {params.begin(), params.end()};
}


// Constructor

FuzzEffect::FuzzEffect()
    : AudioEffectBase() {
    initialiseAPVTS(createParameterLayout());

    m_fuzzParam = getAPVTS().getRawParameterValue("fuzz");
    m_levelParam = getAPVTS().getRawParameterValue("level");

    jassert(m_fuzzParam != nullptr);
    jassert(m_levelParam != nullptr);

    DBG("[FuzzEffect] Constructed. Parameters: fuzz="
        + juce::String(m_fuzzParam->load())
        + " level=" + juce::String(m_levelParam->load()));
}


// AudioEffectBase hooks

void FuzzEffect::prepareToProcess(const juce::dsp::ProcessSpec &spec) {
    // Hard clipping: clamp to +1.0 or -1.0.
    // The very high pre-gain pushes the signal well past this threshold,
    // producing a near-square-wave output characteristic of classic fuzz pedals.
    m_chain.get<1>().functionToUse = [](float x) -> float {
        return juce::jlimit(-1.0f, 1.0f, x);
    };

    m_chain.prepare(spec);
}

void FuzzEffect::resetState() {
    m_chain.reset();
}

void FuzzEffect::processBlock(juce::AudioBuffer<float> &buffer) {

    // AUDIO THREAD - no allocation, no locking, no logging.
    // Both parameter updates below are allocation-free.

    const float fuzz = m_fuzzParam->load(std::memory_order_relaxed);
    const float level = m_levelParam->load(std::memory_order_relaxed);

    // Pre-gain: map fuzz 0–1 to the gain range that drives the signal hard into the clipper.
    // At minimum (5x) the signal is already clipped; at maximum (100x) the output is near-square-wave.
    m_chain.get<0>().setGainLinear(
        juce::jmap(fuzz, k_minPreGain, k_maxPreGain));

    // Output gain: direct 0–1 linear level.
    m_chain.get<2>().setGainLinear(level);

    // Run the full DSP chain in-place.
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing ctx(block);
    m_chain.process(ctx);
}


// Preset serialization

juce::ValueTree FuzzEffect::getState() const {
    juce::ValueTree state("Parameters");
    state.setProperty("fuzz", m_fuzzParam->load(), nullptr);
    state.setProperty("level", m_levelParam->load(), nullptr);
    return state;
}

void FuzzEffect::setState(const juce::ValueTree &state) {
    if (state.hasProperty("fuzz"))
        getAPVTS().getParameter("fuzz")->setValueNotifyingHost(
            state.getProperty("fuzz"));

    if (state.hasProperty("level"))
        getAPVTS().getParameter("level")->setValueNotifyingHost(
            state.getProperty("level"));
}
