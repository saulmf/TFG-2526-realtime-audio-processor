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
    static constexpr const char *TYPE_ID = "overdrive"; ///< Stable type identifier used by EffectFactory and PresetManager.

    /** Creates the effect, initialises the APVTS, and caches raw parameter pointers. */
    OverdriveEffect();

    /** @copydoc IAudioEffect::getTypeId() */
    juce::String getTypeId() const override { return TYPE_ID; }

    /** @copydoc IAudioEffect::getName() */
    juce::String getName() const override { return "Overdrive"; }

    /** @copydoc IAudioEffect::getState() */
    juce::ValueTree getState() const override;

    /** @copydoc IAudioEffect::setState()
        @param state ValueTree as returned by getState(). */
    void setState(const juce::ValueTree &state) override;

protected:
    /**
     * Applies soft-clipping (tanh) with pre-gain, low-pass tone filter, and output level.
     * @param buffer Buffer to process in place.
     * @warning Audio thread only. No allocation, no locking.
     */
    void processBlock(juce::AudioBuffer<float> &buffer) override;

    /**
     * Sets the tanh waveshaper function and configures the tone filter as low-pass,
     * then prepares the full DSP chain.
     * @param spec Sample rate and block size confirmed by the audio device.
     */
    void prepareToProcess(const juce::dsp::ProcessSpec &spec) override;

    /** Resets all DSP chain state (filter history, waveshaper state). */
    void resetState() override;

private:
    /** Builds the APVTS parameter layout: Drive, Level, and Tone. */
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
