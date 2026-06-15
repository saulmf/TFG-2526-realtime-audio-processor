#pragma once

#include <juce_dsp/juce_dsp.h>

#include "../AudioEffectBase.h"

/**
 * HardClippingEffect - hard-clipping distortion with a tone filter.
 *
 * DSP chain (juce::dsp::ProcessorChain):
 *  - [0] Gain                   - pre-gain controlled by the Gain parameter.
 *  - [1] WaveShaper (hard clip) - clips at +1.0 and -1.0, producing a harsher character than the soft-clipping Overdrive.
 *  - [2] StateVariableTPTFilter (LP) - tone control; allocation-free per-block update.
 *  - [3] Gain                        - output level.
 *
 * Compared to OverdriveEffect: identical chain layout, but the waveshaper uses hard clipping (jlimit) instead of tanh,
 * giving a more aggressive, "transistor" distortion character.
 * Pre-gain range is also wider (1–61x) to reflect that hard clipping requires more headroom to produce the full distortion spectrum.
 *
 * @note All per-block parameter reads are via cached std::atomic<float>* pointers.
 * @note All per-block DSP updates are allocation-free (real-time safe).
 */
class HardClippingEffect : public AudioEffectBase {
public:
    static constexpr const char *TYPE_ID = "hardclip"; ///< Stable type identifier used by EffectFactory and PresetManager.

    /** Creates the effect, initialises the APVTS, and caches raw parameter pointers. */
    HardClippingEffect();

    /** @copydoc IAudioEffect::getTypeId() */
    juce::String getTypeId() const override { return TYPE_ID; }

    /** @copydoc IAudioEffect::getName() */
    juce::String getName() const override { return "Hard Clipping"; }

    /** @copydoc IAudioEffect::getState() */
    juce::ValueTree getState() const override;

    /** @copydoc IAudioEffect::setState()
        @param state ValueTree as returned by getState(). */
    void setState(const juce::ValueTree &state) override;

protected:
    /**
     * Applies hard-clipping (jlimit ±1.0) with pre-gain, low-pass tone filter, and output level.
     * @param buffer Buffer to process in place.
     * @warning Audio thread only. No allocation, no locking.
     */
    void processBlock(juce::AudioBuffer<float> &buffer) override;

    /**
     * Sets the hard-clip waveshaper function and configures the tone filter as low-pass,
     * then prepares the full DSP chain.
     * @param spec Sample rate and block size confirmed by the audio device.
     */
    void prepareToProcess(const juce::dsp::ProcessSpec &spec) override;

    /** Resets all DSP chain state. */
    void resetState() override;

private:
    /** Builds the APVTS parameter layout: Gain, Level, and Tone. */
    static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout();

    // -------------------------------------------------------------------------
    // DSP chain
    // Index 0: pre-gain (gain amount)
    // Index 1: waveshaper (hard clip at +1.0 and -1.0)
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

    // Raw parameter pointers - cached in constructor, read on the audio thread
    std::atomic<float> *m_gainParam{nullptr};
    std::atomic<float> *m_levelParam{nullptr};
    std::atomic<float> *m_toneParam{nullptr};

    // Gain maps 0–1 -> pre-gain 1–61
    // (wider range than overdrive; hard clipping needs more headroom to reach the full distortion spectrum)
    static constexpr float k_minPreGain{1.0f};
    static constexpr float k_maxPreGain{61.0f};

    // Tone maps 0–1 -> low-pass cutoff 300–8000 Hz
    static constexpr float k_minToneHz{300.0f};
    static constexpr float k_maxToneHz{8000.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HardClippingEffect)
};
