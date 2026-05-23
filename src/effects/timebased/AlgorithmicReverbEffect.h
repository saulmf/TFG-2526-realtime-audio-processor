#pragma once

#include <juce_dsp/juce_dsp.h>

#include "../AudioEffectBase.h"

/**
 * AlgorithmicReverbEffect - Schroeder/Moorer comb-filter reverb.
 *
 * Delegates entirely to juce::dsp::Reverb, which wraps JUCE's built-in Schroeder-style algorithmic reverb
 * (8 comb filters + 4 all-pass filters).
 *
 * juce::dsp::Reverb::setParameters() updates the internal comb/all-pass filter coefficients by writing to pre-allocated delay lines
 * (no allocation occurs, making live parameter changes fully real-time safe).
 *
 * Wet/dry blending is handled directly through juce::Reverb::Parameters
 * (wetLevel + dryLevel), so no separate DryWetMixer is needed.
 *
 * @note juce::dsp::Reverb is a stereo algorithm.
 * In mono mode (our standard signal path) it runs using only the left-channel comb bank; the Width parameter still
 * influences the internal comb filter mixing but has no audible stereo effect until a stereo output path is introduced.
 *
 * All per-block parameter reads are via cached std::atomic<float>* pointers.
 */
class AlgorithmicReverbEffect : public AudioEffectBase {
public:
    static constexpr const char *TYPE_ID = "reverb";

    AlgorithmicReverbEffect();

    juce::String getTypeId() const override { return TYPE_ID; }
    juce::String getName() const override { return "Reverb"; }

    juce::ValueTree getState() const override;

    void setState(const juce::ValueTree &state) override;

protected:
    void processBlock(juce::AudioBuffer<float> &buffer) override;

    void prepareToProcess(const juce::dsp::ProcessSpec &spec) override;

    void resetState() override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout();

    juce::dsp::Reverb m_reverb;

    // Raw parameter pointers - cached in constructor, read on the audio thread.
    std::atomic<float> *m_roomSizeParam{nullptr};
    std::atomic<float> *m_dampingParam{nullptr};
    std::atomic<float> *m_widthParam{nullptr};
    std::atomic<float> *m_mixParam{nullptr};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgorithmicReverbEffect)
};
