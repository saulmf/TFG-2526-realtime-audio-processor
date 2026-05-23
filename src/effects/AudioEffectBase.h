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

    /** Stores spec and delegates to prepareToProcess(). GUI thread only. */
    void prepare(const juce::dsp::ProcessSpec &spec) final;

    /** Delegates to resetState(). Clears DSP state, preserves parameters. */
    void reset() final;

    /**
     * Checks the enabled flag; if disabled, returns immediately (Null Object bypass).
     * Otherwise, delegates to processBlock().
     *
     * @warning Audio thread only.
     */
    void process(juce::AudioBuffer<float> &buffer) final;


    // IAudioEffect - enable / disable (final)

    bool isEnabled() const final;

    void setEnabled(bool enabled) final;


    // IAudioEffect - parameter access (final)

    juce::AudioProcessorValueTreeState &getAPVTS() final;

protected:
    AudioEffectBase(); // defined in .cpp - user-provided to avoid implicit deletion


    // Template method hooks - subclasses must implement these

    /**
     * DSP processing for one buffer.
     *
     * @warning Audio thread only. No allocation, no locking.
     */
    virtual void processBlock(juce::AudioBuffer<float> &buffer) = 0;

    /** Allocate and initialize all DSP objects for the given spec.
     *
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
     * @note Must be called exactly once, as the first statement of every concrete effect's constructor,
     * before caching any raw parameter pointers.
     */
    void initialiseAPVTS(juce::AudioProcessorValueTreeState::ParameterLayout layout);

private:
    // DummyProcessor - minimal AudioProcessor used solely to own the APVTS.
    // It has no audio buses and performs no processing.
    // It exists only because juce::AudioProcessorValueTreeState requires an AudioProcessor& owner.

    class DummyProcessor final : public juce::AudioProcessor {
    public:
        DummyProcessor() : AudioProcessor(BusesProperties()) {
        }

        // Identity
        const juce::String getName() const override { return {}; }

        // Audio processing - never called
        void prepareToPlay(double, int) override {
        }

        void releaseResources() override {
        }

        void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override {
        }

        // Bus layout - accepts anything since it is never used for audio
        bool isBusesLayoutSupported(const BusesLayout &) const override { return true; }

        // MIDI - not used
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }

        // Tail
        double getTailLengthSeconds() const override { return 0.0; }

        // Editor - standalone app, no plugin editor needed
        bool hasEditor() const override { return false; }
        juce::AudioProcessorEditor *createEditor() override { return nullptr; }

        // Programs - single dummy program required by the interface
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }

        void setCurrentProgram(int) override {
        }

        const juce::String getProgramName(int) override { return {}; }

        void changeProgramName(int, const juce::String &) override {
        }

        // State - managed by the APVTS, not stored here
        void getStateInformation(juce::MemoryBlock &) override {
        }

        void setStateInformation(const void *, int) override {
        }

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
