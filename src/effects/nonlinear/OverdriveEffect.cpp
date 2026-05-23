#include "OverdriveEffect.h"


// Parameter layout

juce::AudioProcessorValueTreeState::ParameterLayout
OverdriveEffect::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter> > params;

    // Drive - controls how hard the signal is pushed into the waveshaper.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive",
        juce::NormalisableRange(0.0f, 1.0f, 0.01f),
        0.5f));

    // Level - output gain after the distortion stage.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "level", "Level",
        juce::NormalisableRange(0.0f, 1.0f, 0.01f),
        0.8f));

    // Tone - low-pass filter cutoff; higher = brighter, lower = darker.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "tone", "Tone",
        juce::NormalisableRange(0.0f, 1.0f, 0.01f),
        0.5f));

    return {params.begin(), params.end()};
}


// Constructor

OverdriveEffect::OverdriveEffect()
    : AudioEffectBase() {
    initialiseAPVTS(createParameterLayout());

    m_driveParam = getAPVTS().getRawParameterValue("drive");
    m_levelParam = getAPVTS().getRawParameterValue("level");
    m_toneParam = getAPVTS().getRawParameterValue("tone");

    jassert(m_driveParam != nullptr);
    jassert(m_levelParam != nullptr);
    jassert(m_toneParam != nullptr);

    DBG("[OverdriveEffect] Constructed. Parameters: drive="
        + juce::String(m_driveParam->load())
        + " level=" + juce::String(m_levelParam->load())
        + " tone=" + juce::String(m_toneParam->load()));
}


// AudioEffectBase hooks

void OverdriveEffect::prepareToProcess(const juce::dsp::ProcessSpec &spec) {
    // Set the waveshaper function (std::tanh for smooth soft clipping).
    // Setting a std::function allocates, so this must happen here on the GUI thread - never inside processBlock.
    m_chain.get<1>().functionToUse = [](float x) {
        return std::tanh(x);
    };

    // Configure the tone filter as a low-pass (brightness control)
    m_chain.get<2>().setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    m_chain.prepare(spec);
}

void OverdriveEffect::resetState() {
    m_chain.reset();
}

void OverdriveEffect::processBlock(juce::AudioBuffer<float> &buffer) {
    // AUDIO THREAD - no allocation, no locking, no logging.
    // All three parameter updates below are allocation-free.

    const float drive = m_driveParam->load(std::memory_order_relaxed);
    const float level = m_levelParam->load(std::memory_order_relaxed);
    const float tone = m_toneParam->load(std::memory_order_relaxed);

    // Pre-gain: map drive 0–1 to a linear gain range that pushes the signal
    //  well into the tanh nonlinearity at high drive values.
    m_chain.get<0>().setGainLinear(
        juce::jmap(drive, k_minPreGain, k_maxPreGain));

    // Tone: map 0–1 to a low-pass cutoff frequency range.
    m_chain.get<2>().setCutoffFrequency(
        juce::jmap(tone, k_minToneHz, k_maxToneHz));

    // Output gain: direct 0–1 linear level.
    m_chain.get<3>().setGainLinear(level);

    // Run the full DSP chain in-place.
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing ctx(block);
    m_chain.process(ctx);
}


// Preset serialization

juce::ValueTree OverdriveEffect::getState() const {
    juce::ValueTree state("Parameters");
    state.setProperty("drive", m_driveParam->load(), nullptr);
    state.setProperty("level", m_levelParam->load(), nullptr);
    state.setProperty("tone", m_toneParam->load(), nullptr);
    return state;
}

void OverdriveEffect::setState(const juce::ValueTree &state) {
    if (state.hasProperty("drive"))
        getAPVTS().getParameter("drive")->setValueNotifyingHost(
            state.getProperty("drive"));

    if (state.hasProperty("level"))
        getAPVTS().getParameter("level")->setValueNotifyingHost(
            state.getProperty("level"));

    if (state.hasProperty("tone"))
        getAPVTS().getParameter("tone")->setValueNotifyingHost(
            state.getProperty("tone"));
}
