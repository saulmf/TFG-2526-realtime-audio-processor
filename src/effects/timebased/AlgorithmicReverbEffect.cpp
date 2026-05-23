#include "AlgorithmicReverbEffect.h"

// Parameter layout

juce::AudioProcessorValueTreeState::ParameterLayout
AlgorithmicReverbEffect::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter> > params;

    // Room size - controls the size of the simulated space.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "roomSize", "Room Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    // Damping - high-frequency absorption; higher = darker, shorter decay.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "damping", "Damping",
        juce::NormalisableRange(0.0f, 1.0f, 0.01f),
        0.5f));

    // Width - stereo spread of the reverb tail.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "width", "Width",
        juce::NormalisableRange(0.0f, 1.0f, 0.01f),
        0.8f));

    // Mix - wet proportion (0 = dry only, 1 = wet only)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange(0.0f, 1.0f, 0.01f),
        0.35f));

    return {params.begin(), params.end()};
}

// Constructor

AlgorithmicReverbEffect::AlgorithmicReverbEffect()
    : AudioEffectBase() {
    initialiseAPVTS(createParameterLayout());

    m_roomSizeParam = getAPVTS().getRawParameterValue("roomSize");
    m_dampingParam = getAPVTS().getRawParameterValue("damping");
    m_widthParam = getAPVTS().getRawParameterValue("width");
    m_mixParam = getAPVTS().getRawParameterValue("mix");

    jassert(m_roomSizeParam != nullptr);
    jassert(m_dampingParam != nullptr);
    jassert(m_widthParam != nullptr);
    jassert(m_mixParam != nullptr);

    DBG("[AlgorithmicReverbEffect] Constructed. roomSize="
        + juce::String(m_roomSizeParam->load(), 2)
        + "  damping=" + juce::String(m_dampingParam->load(), 2)
        + "  width=" + juce::String(m_widthParam->load(), 2)
        + "  mix=" + juce::String(m_mixParam->load(), 2));
}


// AudioEffectBase hooks

void AlgorithmicReverbEffect::prepareToProcess(const juce::dsp::ProcessSpec &spec) {
    m_reverb.prepare(spec);
}

void AlgorithmicReverbEffect::resetState() {
    m_reverb.reset();
}

void AlgorithmicReverbEffect::processBlock(juce::AudioBuffer<float> &buffer) {
    // AUDIO THREAD - no allocation, no locking, no logging.
    // juce::dsp::Reverb::setParameters() writes the new values directly into
    // the pre-allocated comb and all-pass filter coefficients - no allocation.
    // Wet/dry blending is handled by wetLevel + dryLevel inside the reverb algorithm itself, so no external DryWetMixer is required.

    const float roomSize = m_roomSizeParam->load(std::memory_order_relaxed);
    const float damping = m_dampingParam->load(std::memory_order_relaxed);
    const float width = m_widthParam->load(std::memory_order_relaxed);
    const float mix = m_mixParam->load(std::memory_order_relaxed);

    juce::Reverb::Parameters params;
    params.roomSize = roomSize;
    params.damping = damping;
    params.width = width;
    params.wetLevel = mix;
    params.dryLevel = 1.0f - mix;
    params.freezeMode = 0.0f;

    m_reverb.setParameters(params);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing ctx(block);
    m_reverb.process(ctx);
}


// Preset serialization

juce::ValueTree AlgorithmicReverbEffect::getState() const {
    juce::ValueTree state("Parameters");
    state.setProperty("roomSize", m_roomSizeParam->load(), nullptr);
    state.setProperty("damping", m_dampingParam->load(), nullptr);
    state.setProperty("width", m_widthParam->load(), nullptr);
    state.setProperty("mix", m_mixParam->load(), nullptr);
    return state;
}

void AlgorithmicReverbEffect::setState(const juce::ValueTree &state) {
    if (state.hasProperty("roomSize"))
        getAPVTS().getParameter("roomSize")->setValueNotifyingHost(
            state.getProperty("roomSize"));

    if (state.hasProperty("damping"))
        getAPVTS().getParameter("damping")->setValueNotifyingHost(
            state.getProperty("damping"));

    if (state.hasProperty("width"))
        getAPVTS().getParameter("width")->setValueNotifyingHost(
            state.getProperty("width"));

    if (state.hasProperty("mix"))
        getAPVTS().getParameter("mix")->setValueNotifyingHost(
            state.getProperty("mix"));
}
