#include "DigitalDelayEffect.h"

// Parameter layout

juce::AudioProcessorValueTreeState::ParameterLayout
DigitalDelayEffect::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter> > params;

    // Delay time in milliseconds.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delayTime", "Delay Time",
        juce::NormalisableRange(k_minDelayMs, k_maxDelayMs, 1.0f),
        350.0f));

    // Feedback - how much of the delayed signal is fed back into the input..
    // Hard-capped below 1.0 to prevent infinite self-oscillation.
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "feedback", "Feedback",
        juce::NormalisableRange(0.0f, k_maxFeedback, 0.01f),
        0.4f));

    // Mix - wet proportion (0 = dry only, 1 = wet only).
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange(0.0f, 1.0f, 0.01f),
        0.3f));

    return {params.begin(), params.end()};
}


// Constructor

DigitalDelayEffect::DigitalDelayEffect()
    : AudioEffectBase() {
    initialiseAPVTS(createParameterLayout());

    m_delayTimeParam = getAPVTS().getRawParameterValue("delayTime");
    m_feedbackParam = getAPVTS().getRawParameterValue("feedback");
    m_mixParam = getAPVTS().getRawParameterValue("mix");

    jassert(m_delayTimeParam != nullptr);
    jassert(m_feedbackParam != nullptr);
    jassert(m_mixParam != nullptr);

    DBG("[DigitalDelayEffect] Constructed. delayTime="
        + juce::String(m_delayTimeParam->load(), 0) + " ms"
        + "  feedback=" + juce::String(m_feedbackParam->load(), 2)
        + "  mix=" + juce::String(m_mixParam->load(), 2));
}


// AudioEffectBase hooks

void DigitalDelayEffect::prepareToProcess(const juce::dsp::ProcessSpec &spec) {
    m_sampleRate = spec.sampleRate;

    // Pre-allocate the delay buffer for the worst-case delay at the highest supported sample rate.
    // (must happen here - never inside processBlock).
    m_maxDelaySamples = static_cast<int>(k_maxDelayMs * 0.001 * spec.sampleRate) + 1;
    m_delayLine.setMaximumDelayInSamples(m_maxDelaySamples);
    m_delayLine.prepare(spec);

    m_mixer.prepare(spec);
}

void DigitalDelayEffect::resetState() {
    m_delayLine.reset();
    m_mixer.reset();
}

void DigitalDelayEffect::processBlock(juce::AudioBuffer<float> &buffer) {
    /*
     * AUDIO THREAD - no allocation, no locking, no logging.
     * Signal flow per sample:
     *  1. Read delayed output from the delay line (popSample).
     *   2. Write (dry + feedback * delayed) into the delay line (pushSample).
     *   3. Buffer carries the pure wet signal after the loop.
     *   4. DryWetMixer blends the saved dry signal back in.
     *
     * DelayLine::setDelay() - updates read-pointer offset, allocation-free.
     * DelayLine::pushSample() - writes one sample, no allocation.
     * DelayLine::popSample() - reads one interpolated sample, no allocation.
     * DryWetMixer operations - allocation-free once prepared.
    */

    const float delayMs = m_delayTimeParam->load(std::memory_order_relaxed);
    const float feedback = m_feedbackParam->load(std::memory_order_relaxed);
    const float mix = m_mixParam->load(std::memory_order_relaxed);

    const float delaySamples = juce::jlimit(
        1.0f,
        static_cast<float>(m_maxDelaySamples),
        delayMs * 0.001f * static_cast<float>(m_sampleRate));

    m_delayLine.setDelay(delaySamples);

    juce::dsp::AudioBlock<float> block(buffer);

    // Save the dry signal before we overwrite the buffer with the wet signal.
    m_mixer.setWetMixProportion(mix);
    m_mixer.pushDrySamples(block);

    // Build the wet signal in-place using the feedback delay loop.
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float *data = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            const float dry = data[i];
            const float delayed = m_delayLine.popSample(ch);

            m_delayLine.pushSample(ch, dry + feedback * delayed);

            data[i] = delayed; // pure wet output; mixer re-adds dry below
        }
    }

    // Blend dry (saved above) and wet (buffer now contains delayed signal).
    m_mixer.mixWetSamples(block);
}


// Preset serialization

juce::ValueTree DigitalDelayEffect::getState() const {
    juce::ValueTree state("Parameters");
    state.setProperty("delayTime", m_delayTimeParam->load(), nullptr);
    state.setProperty("feedback", m_feedbackParam->load(), nullptr);
    state.setProperty("mix", m_mixParam->load(), nullptr);
    return state;
}

void DigitalDelayEffect::setState(const juce::ValueTree &state) {
    auto restore = [&](const char *id) {
        if (!state.hasProperty(id)) return;
        auto *p = getAPVTS().getParameter(id);
        if (p != nullptr)
            p->setValueNotifyingHost(
                p->convertTo0to1(state.getProperty(id)));
    };

    restore("delayTime");
    restore("feedback");
    restore("mix");
}
