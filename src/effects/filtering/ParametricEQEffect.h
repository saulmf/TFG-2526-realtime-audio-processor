#pragma once

#include <juce_dsp/juce_dsp.h>

#include "../AudioEffectBase.h"

/**
 * ParametricEQEffect - three-band parametric equalizer.
 *
 * Each band is a peak/bell filter with independent Frequency, Gain, and Q parameters. All three bands are applied in series.
 *
 * DSP chain (juce::dsp::ProcessorChain):
 * - [0] ProcessorDuplicator<IIR::Filter> - low band (default centre  200 Hz)
 * - [1] ProcessorDuplicator<IIR::Filter> - mid band (default centre 1000 Hz)
 * - [2] ProcessorDuplicator<IIR::Filter> - high band (default centre 5000 Hz)
 *
 * ProcessorDuplicator wraps a mono IIR::Filter so the same coefficients are
 * shared across all channels while per-channel filter state is kept separate.
 *
 * @note Coefficient update note: juce::dsp::IIR::Coefficients::makePeakFilter allocates a small temporary object (reference-counted).
 * This is the standard JUCE EQ pattern and cannot be avoided without computing biquad coefficients manually.
 * The allocation is fixed-size and immediately freed after the coefficient values are copied into the pre-existing state object.
 *
 * @note All per-block parameter reads are via cached std::atomic<float>* pointers.
 */
class ParametricEQEffect : public AudioEffectBase {
public:
    static constexpr const char *TYPE_ID = "parametriceq"; ///< Stable type identifier used by EffectFactory and PresetManager.

    // Exposed so the anonymous-namespace helpers in the .cpp can reference them
    static constexpr float k_minFreqHz{20.0f};   ///< Minimum band centre frequency (Hz).
    static constexpr float k_maxFreqHz{20000.0f}; ///< Maximum band centre frequency (Hz).
    static constexpr float k_minQ{0.1f};          ///< Minimum Q (widest bandwidth).
    static constexpr float k_maxQ{10.0f};          ///< Maximum Q (narrowest bandwidth).

    /** Creates the effect, initialises the APVTS with nine parameters, and caches raw parameter pointers. */
    ParametricEQEffect();

    /** @copydoc IAudioEffect::getTypeId() */
    juce::String getTypeId() const override { return TYPE_ID; }

    /** @copydoc IAudioEffect::getName() */
    juce::String getName() const override { return "Parametric EQ"; }

    /** @copydoc IAudioEffect::getState() */
    juce::ValueTree getState() const override;

    /** @copydoc IAudioEffect::setState()
        @param state ValueTree as returned by getState(). */
    void setState(const juce::ValueTree &state) override;

protected:
    /**
     * Recalculates IIR peak-filter coefficients for each of the three bands, then processes the buffer.
     * @param buffer Buffer to process in place.
     * @note Coefficient calculation involves a small, fixed-size allocation (standard JUCE EQ pattern).
     * @warning Audio thread only.
     */
    void processBlock(juce::AudioBuffer<float> &buffer) override;

    /**
     * Initialises all three IIR band filters with flat (0 dB) default coefficients
     * and prepares the DSP chain.
     * @param spec Sample rate and block size confirmed by the audio device.
     */
    void prepareToProcess(const juce::dsp::ProcessSpec &spec) override;

    /** Resets the filter history for all three bands. */
    void resetState() override;

private:
    /** Builds the APVTS parameter layout: Freq, Gain, and Q for each of the three bands (nine parameters total). */
    static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout();

    // Convenience: mono IIR filter duplicated across channels
    using BandFilter = juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>
    >;

    // -------------------------------------------------------------------------
    // DSP chain
    // Index 0: low band filter
    // Index 1: mid band filter
    // Index 2: high band filter
    // -------------------------------------------------------------------------
    using Chain = juce::dsp::ProcessorChain<BandFilter, BandFilter, BandFilter>;

    Chain m_chain;
    double m_sampleRate{44100.0}; // cached from prepareToProcess for use in processBlock

    // ---- Band 1 (low) -------------------------------------------------------
    std::atomic<float> *m_freq1Param{nullptr};
    std::atomic<float> *m_gain1Param{nullptr};
    std::atomic<float> *m_q1Param{nullptr};

    // ---- Band 2 (mid) -------------------------------------------------------
    std::atomic<float> *m_freq2Param{nullptr};
    std::atomic<float> *m_gain2Param{nullptr};
    std::atomic<float> *m_q2Param{nullptr};

    // ---- Band 3 (high) ------------------------------------------------------
    std::atomic<float> *m_freq3Param{nullptr};
    std::atomic<float> *m_gain3Param{nullptr};
    std::atomic<float> *m_q3Param{nullptr};

    // Gain range per band (dB)
    static constexpr float k_minGainDb{-24.0f};
    static constexpr float k_maxGainDb{24.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParametricEQEffect)
};
