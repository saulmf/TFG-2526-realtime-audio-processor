#include "HardClippingEffect.h"

// Parameter layout

juce::AudioProcessorValueTreeState::ParameterLayout
HardClippingEffect::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter> > params;

    // Gain - controls how hard the signal is pushed into the hard clipper.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain", "Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    // Level - output gain after the distortion stage.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "level", "Level",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.8f));

    // Tone - low-pass filter cutoff; higher = brighter, lower = darker.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "tone", "Tone",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    return {params.begin(), params.end()};
}

// Constructor

HardClippingEffect::HardClippingEffect()
    : AudioEffectBase() {
    initialiseAPVTS(createParameterLayout());

    m_gainParam = getAPVTS().getRawParameterValue("gain");
    m_levelParam = getAPVTS().getRawParameterValue("level");
    m_toneParam = getAPVTS().getRawParameterValue("tone");

    jassert(m_gainParam != nullptr);
    jassert(m_levelParam != nullptr);
    jassert(m_toneParam != nullptr);

    DBG("[HardClippingEffect] Constructed. Parameters: gain="
        + juce::String(m_gainParam->load())
        + " level=" + juce::String(m_levelParam->load())
        + " tone=" + juce::String(m_toneParam->load()));
}


// AudioEffectBase hooks

void HardClippingEffect::prepareToProcess(const juce::dsp::ProcessSpec &spec) {
    // Hard clipping: clips the signal symmetrically at +1.0 and -1.0.
    // Unlike tanh, this produces a flat top on the waveform - the extra odd harmonics give it a harsher,
    // more "transistor distortion" character.
    m_chain.get<1>().functionToUse = [](float x) -> float {
        return juce::jlimit(-1.0f, 1.0f, x);
    };

    // Configure the tone filter as a low-pass (brightness control)
    m_chain.get<2>().setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    m_chain.prepare(spec);
}

void HardClippingEffect::resetState() {
    m_chain.reset();
}

void HardClippingEffect::processBlock(juce::AudioBuffer<float> &buffer) {

    // AUDIO THREAD - no allocation, no locking, no logging.
    // All three parameter updates below are allocation-free.

    const float gain = m_gainParam->load(std::memory_order_relaxed);
    const float level = m_levelParam->load(std::memory_order_relaxed);
    const float tone = m_toneParam->load(std::memory_order_relaxed);

    // Pre-gain: map gain 0–1 to a linear gain range that pushes
    //  the signal into the hard clipper across the full range of the parameter.
    m_chain.get<0>().setGainLinear(
        juce::jmap(gain, k_minPreGain, k_maxPreGain));

    // Tone: map 0–1 to a low-pass cutoff frequency range.
    m_chain.get<2>().setCutoffFrequency(
        juce::jmap(tone, k_minToneHz, k_maxToneHz));

    // Output gain: direct 0–1 linear level.
    m_chain.get<3>().setGainLinear(level);

    // Run the full DSP chain in-place.
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    m_chain.process(ctx);
}


// Preset serialization

juce::ValueTree HardClippingEffect::getState() const {
    juce::ValueTree state("Parameters");
    state.setProperty("gain", m_gainParam->load(), nullptr);
    state.setProperty("level", m_levelParam->load(), nullptr);
    state.setProperty("tone", m_toneParam->load(), nullptr);
    return state;
}

void HardClippingEffect::setState(const juce::ValueTree &state) {
    if (state.hasProperty("gain"))
        getAPVTS().getParameter("gain")->setValueNotifyingHost(
            state.getProperty("gain"));

    if (state.hasProperty("level"))
        getAPVTS().getParameter("level")->setValueNotifyingHost(
            state.getProperty("level"));

    if (state.hasProperty("tone"))
        getAPVTS().getParameter("tone")->setValueNotifyingHost(
            state.getProperty("tone"));
}
