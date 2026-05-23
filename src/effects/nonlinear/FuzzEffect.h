#pragma once

#include <juce_dsp/juce_dsp.h>

#include "../AudioEffectBase.h"

/**
 * FuzzEffect - hard-clipping distortion with extreme gain staging.
 *
 * DSP chain (juce::dsp::ProcessorChain):
 *  - [0] Gain                   - pre-gain controlled by the Fuzz parameter.
 *  - [1] WaveShaper (hard clip) - hard clipping at +1.0 and -1.0 for square-wave character.
 *  - [2] Gain                   - output level.
 *
 * Fuzz differs from overdrive in two ways:
 *  - Much higher pre-gain range (5–100x vs 1–41x), producing near-square-wave output.
 *  - Hard clipping waveshaper instead of tanh, giving the characteristic fuzzy texture.
 *
 * @note All per-block parameter reads are via cached std::atomic<float>* pointers.
 * @note All per-block DSP updates are allocation-free (real-time safe).
 */
class FuzzEffect : public AudioEffectBase {
public:
    static constexpr const char *TYPE_ID = "fuzz";

    FuzzEffect();

    juce::String getTypeId() const override { return TYPE_ID; }
    juce::String getName() const override { return "Fuzz"; }

    juce::ValueTree getState() const override;

    void setState(const juce::ValueTree &state) override;

protected:
    void processBlock(juce::AudioBuffer<float> &buffer) override;

    void prepareToProcess(const juce::dsp::ProcessSpec &spec) override;

    void resetState() override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout();

    // -------------------------------------------------------------------------
    // DSP chain
    // Index 0: pre-gain (fuzz amount)
    // Index 1: waveshaper (hard clip)
    // Index 2: output gain (level)
    // -------------------------------------------------------------------------
    using Chain = juce::dsp::ProcessorChain<
        juce::dsp::Gain<float>,
        juce::dsp::WaveShaper<float>,
        juce::dsp::Gain<float>
    >;

    Chain m_chain;

    // Raw parameter pointers - cached in constructor, read on the audio thread.
    std::atomic<float> *m_fuzzParam{nullptr};
    std::atomic<float> *m_levelParam{nullptr};

    // Fuzz maps 0–1 -> pre-gain 5–100 (extreme saturation even at minimum).
    static constexpr float k_minPreGain{5.0f};
    static constexpr float k_maxPreGain{100.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FuzzEffect)
};
