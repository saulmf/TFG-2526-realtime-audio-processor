#pragma once

#include <array>
#include <juce_dsp/juce_dsp.h>

#include "../AudioEffectBase.h"

/**
 * LowPassFilterEffect - Butterworth-style low-pass filter with variable order.
 *
 * Implements variable rolloff by cascading up to k_maxStages second-order StateVariableTPTFilter stages.
 * Each stage adds 12 dB/oct of attenuation:
 *   Order 1 -> 12 dB/oct (1 stage)
 *   Order 2 -> 24 dB/oct (2 stages)
 *   Order 3 -> 36 dB/oct (3 stages)
 *   Order 4 -> 48 dB/oct (4 stages)
 *
 * All k_maxStages filters are prepared and maintained; processBlock activates only the first 'order' of them on each buffer.
 * (Avoids any runtime allocation - the cascade depth changes without touching DSP object lifetimes).
 *
 * StateVariableTPTFilter::setCutoffFrequency() is allocation-free, so live cutoff changes are fully real-time safe.
 */
class LowPassFilterEffect : public AudioEffectBase {
public:
    static constexpr const char *TYPE_ID = "lowpass";
    static constexpr int k_maxStages = 4;

    LowPassFilterEffect();

    juce::String getTypeId() const override { return TYPE_ID; }
    juce::String getName() const override { return "Low-Pass Filter"; }

    juce::ValueTree getState() const override;

    void setState(const juce::ValueTree &state) override;

protected:
    void processBlock(juce::AudioBuffer<float> &buffer) override;

    void prepareToProcess(const juce::dsp::ProcessSpec &spec) override;

    void resetState() override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout();

    // k_maxStages cascadeable LP stages - all prepared; only 'order' are active
    std::array<juce::dsp::StateVariableTPTFilter<float>, k_maxStages> m_filters;

    // Raw parameter pointers - cached in constructor, read on the audio thread
    std::atomic<float> *m_cutoffParam{nullptr};
    std::atomic<float> *m_orderParam{nullptr}; // int 1–4, stored as float by APVTS

    // Cutoff range (Hz) - skewed range gives log-like feel on the control
    static constexpr float k_minCutoffHz{20.0f};
    static constexpr float k_maxCutoffHz{20000.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LowPassFilterEffect)
};
