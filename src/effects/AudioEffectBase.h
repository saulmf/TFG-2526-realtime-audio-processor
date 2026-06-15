#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "IAudioEffect.h"

/**
 * AudioEffectBase is the abstract base class for all concrete effects.
 * It implements IAudioEffect using the Template Method pattern:
 * - process() final: enforces bypass, then calls processBlock().
 * - prepare() final: stores spec, then calls prepareToProcess().
 * - reset() final: delegates to resetState().
 *
 * Concrete effects inherit from this class and implement only:
 * - processBlock() - the DSP algorithm (audio thread).
 * - prepareToProcess() - DSP object initialization (GUI thread, before session).
 * - resetState() - clear DSP state without changing parameters.
 * - createParameterLayout() - static factory for the APVTS parameter set.
 * - getState() / setState() - parameter serialization / deserialization.
 * - getTypeId() / getName() - stable ID and display name.
 *
 * APVTS ownership:
 *   juce::AudioProcessorValueTreeState requires a juce::AudioProcessor& owner.
 *   In a standalone application there is no plugin host to provide one, so AudioEffectBase owns
 *   a private DummyProcessor for this purpose.
 *   Effects are therefore fully self-contained - no external processor needs to be passed in.
 *
 * Constructor protocol:
 *   Subclass constructors MUST call initialiseAPVTS(createParameterLayout()) as their first statement,
 *   then cache raw parameter pointers via getAPVTS().getRawParameterValue().
 */
class AudioEffectBase : public IAudioEffect {
public:
    ~AudioEffectBase() override = default;


    // IAudioEffect - lifecycle (Template Method, final)

    /** Stores spec and delegates to prepareToProcess(). GUI thread only.
        @param spec Sample rate, block size, and channel count for the upcoming session. */
    void prepare(const juce::dsp::ProcessSpec &spec) final;

    /** Delegates to resetState(). Clears DSP state, preserves parameters. */
    void reset() final;

    /**
     * Checks the enabled flag; if disabled, returns immediately (Null Object bypass).
     * Otherwise, delegates to processBlock().
     *
     * @param buffer Buffer to process in place.
     * @warning Audio thread only.
     */
    void process(juce::AudioBuffer<float> &buffer) final;


    // IAudioEffect - enable / disable (final)

    /** @copydoc IAudioEffect::isEnabled() */
    bool isEnabled() const final;

    /** @copydoc IAudioEffect::setEnabled() */
    void setEnabled(bool enabled) final;


    // IAudioEffect - parameter access (final)

    /** @copydoc IAudioEffect::getAPVTS() */
    juce::AudioProcessorValueTreeState &getAPVTS() final;

protected:
    /** Default constructor. Defined in the .cpp to prevent implicit deletion by the compiler. */
    AudioEffectBase();


    // Template method hooks - subclasses must implement these

    /**
     * DSP processing for one buffer.
     *
     * @param buffer Buffer to process in place; channel count and size are guaranteed to match the spec.
     * @warning Audio thread only. No allocation, no locking.
     */
    virtual void processBlock(juce::AudioBuffer<float> &buffer) = 0;

    /** Allocates and initialises all DSP objects for the given spec.
     * @param spec Sample rate, block size, and channel count confirmed by the audio device.
     * @note GUI thread.
     */
    virtual void prepareToProcess(const juce::dsp::ProcessSpec &spec) = 0;

    /**
     * Clear internal DSP state (delay lines, filter histories, etc.).
     *
     */
    virtual void resetState() = 0;


    // APVTS initialisation - call from subclass constructor

    /**
     * Constructs the APVTS with the given parameter layout.
     *
     * @param layout Parameter layout built by the concrete class's createParameterLayout().
     * @note Must be called exactly once, as the first statement of every concrete effect's constructor,
     * before caching any raw parameter pointers.
     */
    void initialiseAPVTS(juce::AudioProcessorValueTreeState::ParameterLayout layout);

private:
    /**
     * Minimal AudioProcessor used solely as an owner for the APVTS.
     * It has no audio buses and performs no processing.
     * It exists only because juce::AudioProcessorValueTreeState requires an AudioProcessor& owner.
     */
    class DummyProcessor final : public juce::AudioProcessor {
    public:
        DummyProcessor() : AudioProcessor(BusesProperties()) {}

        const juce::String getName() const override { return {}; } ///< Returns an empty name; this processor is never presented to the user.

        void prepareToPlay(double, int) override {}   ///< No-op: this processor is never used for audio.
        void releaseResources() override {}           ///< No-op: no resources to release.
        void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override {} ///< No-op: audio processing is handled by the concrete effect.

        bool isBusesLayoutSupported(const BusesLayout &) const override { return true; } ///< Accepts any layout since it never processes audio.

        bool acceptsMidi() const override { return false; } ///< MIDI not used.
        bool producesMidi() const override { return false; } ///< MIDI not used.

        double getTailLengthSeconds() const override { return 0.0; } ///< No tail.

        bool hasEditor() const override { return false; }             ///< No plugin editor in standalone mode.
        juce::AudioProcessorEditor *createEditor() override { return nullptr; } ///< Returns nullptr; standalone app, no plugin editor needed.

        int getNumPrograms() override { return 1; }      ///< Single dummy program required by the interface.
        int getCurrentProgram() override { return 0; }   ///< Always returns 0 (only one dummy program exists).
        void setCurrentProgram(int) override {}          ///< No-op.
        const juce::String getProgramName(int) override { return {}; } ///< Returns empty string.
        void changeProgramName(int, const juce::String &) override {}  ///< No-op.

        void getStateInformation(juce::MemoryBlock &) override {}    ///< State is managed by the APVTS, not stored here.
        void setStateInformation(const void *, int) override {}      ///< State is managed by the APVTS, not stored here.

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DummyProcessor)
    };


    DummyProcessor m_dummyProcessor;

    // Heap-allocated so it can be constructed after m_dummyProcessor by initialiseAPVTS(),
    // which is called from the subclass constructor.
    std::unique_ptr<juce::AudioProcessorValueTreeState> m_apvts;

    std::atomic<bool> m_enabled{true};
    juce::dsp::ProcessSpec m_currentSpec{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEffectBase)
};
