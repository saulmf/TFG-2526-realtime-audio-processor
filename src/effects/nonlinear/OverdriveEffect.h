#pragma once

#include <juce_dsp/juce_dsp.h>

#include "../AudioEffectBase.h"

/**
 * OverdriveEffect - soft-clipping distortion with a tone filter.
 *
 * DSP chain (juce::dsp::ProcessorChain):
 * - [0] Gain                        - pre-gain controlled by the Drive parameter.
 * - [1] WaveShaper (std::tanh)      - smooth soft-clipping nonlinearity.
 * - [2] StateVariableTPTFilter (LP) - tone control; allocation-free per-block update.
 * - [3] Gain                        - output level.
 *
 * @note All per-block parameter reads are via cached std::atomic<float>* pointers.
 * @note All per-block DSP updates are allocation-free (real-time safe).
 */
class OverdriveEffect : public AudioEffectBase {
public:
    static constexpr const char *TYPE_ID = "overdrive";

    OverdriveEffect();

    juce::String getTypeId() const override { return TYPE_ID; }
    juce::String getName() const override { return "Overdrive"; }

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
    // Index 0: pre-gain (drive)
    // Index 1: waveshaper (soft clip)
    // Index 2: tone filter (low-pass, StateVariableTPT - no alloc on audio thread)
    // Index 3: output gain (level)
    // -------------------------------------------------------------------------
    using Chain = juce::dsp::ProcessorChain<
        juce::dsp::Gain<float>,
        juce::dsp::WaveShaper<float>,
        juce::dsp::StateVariableTPTFilter<float>,
        juce::dsp::Gain<float>
    >;

    Chain m_chain;

    // Raw parameter pointers - cached in constructor, read on the audio thread.
    std::atomic<float> *m_driveParam{nullptr};
    std::atomic<float> *m_levelParam{nullptr};
    std::atomic<float> *m_toneParam{nullptr};

    // Drive maps 0–1 -> pre-gain 1–41 (approx 0–32 dB of input boost).
    static constexpr float k_minPreGain{1.0f};
    static constexpr float k_maxPreGain{41.0f};

    // Tone maps 0–1 -> low-pass cutoff 300–8000 Hz
    static constexpr float k_minToneHz{300.0f};
    static constexpr float k_maxToneHz{8000.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverdriveEffect)
};
