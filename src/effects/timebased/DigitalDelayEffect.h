#pragma once

#include <juce_dsp/juce_dsp.h>

#include "../AudioEffectBase.h"

/**
 * DigitalDelayEffect - tape-style digital delay with feedback.
 *
 * JUCE DSP modules used:
 *   - juce::dsp::DelayLine<float, Linear> - pre-allocated circular buffer with
 *     linear interpolation for smooth delay time changes (allocation-free once prepared)
 *   - juce::dsp::DryWetMixer<float> - dry/wet blend with latency compensation
 *
 * The feedback loop is implemented sample-by-sample inside processBlock.
 * Feedback is capped at k_maxFeedback (< 1.0) to prevent infinite resonance.
 *
 * @note All per-block parameter reads are via cached std::atomic<float>* pointers.
 * @note All per-block operations are allocation-free (real-time safe).
 */
class DigitalDelayEffect : public AudioEffectBase {
public:
    static constexpr const char *TYPE_ID = "digitaldelay"; ///< Stable type identifier used by EffectFactory and PresetManager.

    /** Creates the effect, initialises the APVTS, and caches raw parameter pointers. */
    DigitalDelayEffect();

    /** @copydoc IAudioEffect::getTypeId() */
    juce::String getTypeId() const override { return TYPE_ID; }

    /** @copydoc IAudioEffect::getName() */
    juce::String getName() const override { return "Digital Delay"; }

    /** @copydoc IAudioEffect::getState() */
    juce::ValueTree getState() const override;

    /** @copydoc IAudioEffect::setState()
        @param state ValueTree as returned by getState(). */
    void setState(const juce::ValueTree &state) override;

protected:
    /**
     * Runs the per-sample feedback loop and blends dry and wet signals via the DryWetMixer.
     * @param buffer Buffer to process in place.
     * @warning Audio thread only. No allocation (DelayLine and DryWetMixer operations are allocation-free once prepared).
     */
    void processBlock(juce::AudioBuffer<float> &buffer) override;

    /**
     * Pre-allocates the delay line for the worst-case delay at the current sample rate,
     * then prepares the dry/wet mixer.
     * @param spec Sample rate and block size confirmed by the audio device.
     */
    void prepareToProcess(const juce::dsp::ProcessSpec &spec) override;

    /** Clears the delay line buffer and the dry/wet mixer state. */
    void resetState() override;

private:
    /** Builds the APVTS parameter layout: Delay Time (ms), Feedback, and Mix. */
    static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout();

    // Delay line with linear interpolation - supports smooth delay time sweeps.
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> m_delayLine;

    // Handles dry/wet blend; pushDrySamples saves the input before we overwrite it.
    juce::dsp::DryWetMixer<float> m_mixer;

    // Cached from prepareToProcess - needed in processBlock to convert ms -> samples
    double m_sampleRate{44100.0};
    int m_maxDelaySamples{0};

    // Raw parameter pointers - cached in constructor, read on the audio thread.
    std::atomic<float> *m_delayTimeParam{nullptr};  // ms
    std::atomic<float> *m_feedbackParam{nullptr};   // 0–k_maxFeedback
    std::atomic<float> *m_mixParam{nullptr};        // 0–1 wet proportion

    static constexpr float k_minDelayMs{1.0f};
    static constexpr float k_maxDelayMs{2000.0f};
    static constexpr float k_maxFeedback{0.95f};    // < 1.0 to prevent runaway resonance

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DigitalDelayEffect)
};
