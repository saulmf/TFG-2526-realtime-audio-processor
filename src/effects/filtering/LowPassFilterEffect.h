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
    static constexpr const char *TYPE_ID = "lowpass"; ///< Stable type identifier used by EffectFactory and PresetManager.
    static constexpr int k_maxStages = 4;             ///< Maximum number of cascadeable filter stages (each adds 12 dB/oct).

    /** Creates the effect, initialises the APVTS, and caches raw parameter pointers. */
    LowPassFilterEffect();

    /** @copydoc IAudioEffect::getTypeId() */
    juce::String getTypeId() const override { return TYPE_ID; }

    /** @copydoc IAudioEffect::getName() */
    juce::String getName() const override { return "Low-Pass Filter"; }

    /** @copydoc IAudioEffect::getState() */
    juce::ValueTree getState() const override;

    /** @copydoc IAudioEffect::setState()
        @param state ValueTree as returned by getState(). */
    void setState(const juce::ValueTree &state) override;

protected:
    /**
     * Applies the first 'order' cascaded low-pass stages at the current cutoff frequency.
     * @param buffer Buffer to process in place.
     * @warning Audio thread only. No allocation (setCutoffFrequency is allocation-free).
     */
    void processBlock(juce::AudioBuffer<float> &buffer) override;

    /**
     * Configures all k_maxStages filters as low-pass and prepares each one.
     * All stages are always prepared; only the first 'order' are active in processBlock.
     * @param spec Sample rate and block size confirmed by the audio device.
     */
    void prepareToProcess(const juce::dsp::ProcessSpec &spec) override;

    /** Resets the state of all k_maxStages filter stages. */
    void resetState() override;

private:
    /** Builds the APVTS parameter layout: Cutoff (Hz) and Order (1–4). */
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
