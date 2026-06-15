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
    static constexpr const char *TYPE_ID = "reverb"; ///< Stable type identifier used by EffectFactory and PresetManager.

    /** Creates the effect, initialises the APVTS, and caches raw parameter pointers. */
    AlgorithmicReverbEffect();

    /** @copydoc IAudioEffect::getTypeId() */
    juce::String getTypeId() const override { return TYPE_ID; }

    /** @copydoc IAudioEffect::getName() */
    juce::String getName() const override { return "Reverb"; }

    /** @copydoc IAudioEffect::getState() */
    juce::ValueTree getState() const override;

    /** @copydoc IAudioEffect::setState()
        @param state ValueTree as returned by getState(). */
    void setState(const juce::ValueTree &state) override;

protected:
    /**
     * Updates reverb parameters and processes the buffer through juce::dsp::Reverb.
     * Wet/dry blending is handled internally by the reverb via wetLevel and dryLevel.
     * @param buffer Buffer to process in place.
     * @warning Audio thread only. setParameters() writes to pre-allocated delay lines (no allocation).
     */
    void processBlock(juce::AudioBuffer<float> &buffer) override;

    /** Prepares the juce::dsp::Reverb object for the given sample rate and block size.
        @param spec Sample rate and block size confirmed by the audio device. */
    void prepareToProcess(const juce::dsp::ProcessSpec &spec) override;

    /** Resets the reverb algorithm state, clearing all comb and all-pass filter histories. */
    void resetState() override;

private:
    /** Builds the APVTS parameter layout: Room Size, Damping, Width, and Mix. */
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
