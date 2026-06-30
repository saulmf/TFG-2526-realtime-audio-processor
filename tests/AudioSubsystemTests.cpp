#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "audio/AudioEngine.h"
#include "audio/EffectChain.h"
#include "audio/EffectFactory.h"
#include "audio/PresetManager.h"
#include "ApplicationController.h"

#include "effects/nonlinear/OverdriveEffect.h"
#include "effects/nonlinear/HardClippingEffect.h"
#include "effects/nonlinear/FuzzEffect.h"
#include "effects/filtering/ParametricEQEffect.h"
#include "effects/filtering/HighPassFilterEffect.h"
#include "effects/filtering/LowPassFilterEffect.h"
#include "effects/timebased/DigitalDelayEffect.h"
#include "effects/timebased/AlgorithmicReverbEffect.h"

class AudioSubsystemTests final : public juce::UnitTest
{
public:
    AudioSubsystemTests() : juce::UnitTest("Audio Subsystem", "AudioProcessor") {}

    void runTest() override
    {
        testUT07_RegisteredEffectCreation();
        testUT08_UnknownEffectCreation();
        testUT09_AddAndCount();
        testUT10_RemovePreservesOrder();
        testUT11_MoveEffect();
        testUT12_DisabledEffectBypass();
        testUT13_ChainState();
        testUT14_PresetSaveLoad();
        testUT15_StartWithoutDevice();
        testUT16_InvalidSampleRate();
        testUT17_InvalidBufferSize();
        testUT18_ChainFullLimit();
        testUT19_ClearChain();
    }

private:
    static juce::dsp::ProcessSpec makeSpec()
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = 44100.0;
        spec.maximumBlockSize = 256;
        spec.numChannels      = 1;
        return spec;
    }

    static void registerAllEffects(EffectFactory& factory)
    {
        factory.registerEffect(OverdriveEffect::TYPE_ID,
            []() { return std::make_unique<OverdriveEffect>(); });
        factory.registerEffect(HardClippingEffect::TYPE_ID,
            []() { return std::make_unique<HardClippingEffect>(); });
        factory.registerEffect(FuzzEffect::TYPE_ID,
            []() { return std::make_unique<FuzzEffect>(); });
        factory.registerEffect(ParametricEQEffect::TYPE_ID,
            []() { return std::make_unique<ParametricEQEffect>(); });
        factory.registerEffect(HighPassFilterEffect::TYPE_ID,
            []() { return std::make_unique<HighPassFilterEffect>(); });
        factory.registerEffect(LowPassFilterEffect::TYPE_ID,
            []() { return std::make_unique<LowPassFilterEffect>(); });
        factory.registerEffect(DigitalDelayEffect::TYPE_ID,
            []() { return std::make_unique<DigitalDelayEffect>(); });
        factory.registerEffect(AlgorithmicReverbEffect::TYPE_ID,
            []() { return std::make_unique<AlgorithmicReverbEffect>(); });
    }

    /** UT-07: create() with a registered typeId must return a valid instance. */
    void testUT07_RegisteredEffectCreation()
    {
        beginTest("UT-07 Registered effect creation");

        EffectFactory factory;
        factory.registerEffect(OverdriveEffect::TYPE_ID,
            []() { return std::make_unique<OverdriveEffect>(); });

        auto effect = factory.create(OverdriveEffect::TYPE_ID);
        expect(effect != nullptr, "Factory returned null for a registered typeId");
    }

    /** UT-08: create() with an unknown typeId must return nullptr. */
    void testUT08_UnknownEffectCreation()
    {
        beginTest("UT-08 Unknown effect creation");

        EffectFactory factory;
        auto effect = factory.create("nonexistent");
        expect(effect == nullptr, "Factory returned non-null for an unknown typeId");
    }

    /** UT-09: After adding three effects the chain reports three effects. */
    void testUT09_AddAndCount()
    {
        beginTest("UT-09 Add and count effects");

        EffectChain chain;
        chain.addEffect(std::make_unique<OverdriveEffect>());
        chain.addEffect(std::make_unique<FuzzEffect>());
        chain.addEffect(std::make_unique<HardClippingEffect>());

        expectEquals(chain.getNumEffects(), 3);
    }

    /** UT-10: Removing the middle effect must keep A before C. */
    void testUT10_RemovePreservesOrder()
    {
        beginTest("UT-10 Remove effect preserves order");

        EffectChain chain;
        chain.addEffect(std::make_unique<OverdriveEffect>());    // index 0: A
        chain.addEffect(std::make_unique<FuzzEffect>());         // index 1: B
        chain.addEffect(std::make_unique<HardClippingEffect>()); // index 2: C

        chain.removeEffect(1); // remove B

        expectEquals(chain.getNumEffects(), 2);
        expect(chain.getEffect(0)->getTypeId() == juce::String(OverdriveEffect::TYPE_ID),
               "Effect at index 0 should be A (Overdrive)");
        expect(chain.getEffect(1)->getTypeId() == juce::String(HardClippingEffect::TYPE_ID),
               "Effect at index 1 should be C (HardClipping)");
    }

    /** UT-11: Moving A from index 0 to the end must yield [B, C, A]. */
    void testUT11_MoveEffect()
    {
        beginTest("UT-11 Move effect reorders correctly");

        EffectChain chain;
        chain.addEffect(std::make_unique<OverdriveEffect>());    // index 0: A
        chain.addEffect(std::make_unique<FuzzEffect>());         // index 1: B
        chain.addEffect(std::make_unique<HardClippingEffect>()); // index 2: C

        chain.moveEffect(0, 2); // A moves to the end

        expectEquals(chain.getNumEffects(), 3);
        expect(chain.getEffect(0)->getTypeId() == juce::String(FuzzEffect::TYPE_ID),
               "Effect at index 0 should be B (Fuzz)");
        expect(chain.getEffect(1)->getTypeId() == juce::String(HardClippingEffect::TYPE_ID),
               "Effect at index 1 should be C (HardClipping)");
        expect(chain.getEffect(2)->getTypeId() == juce::String(OverdriveEffect::TYPE_ID),
               "Effect at index 2 should be A (Overdrive)");
    }

    /** UT-12: A single disabled effect in the chain must not alter the buffer. */
    void testUT12_DisabledEffectBypass()
    {
        beginTest("UT-12 Disabled effect bypass in chain");

        EffectChain chain;
        chain.addEffect(std::make_unique<OverdriveEffect>());
        chain.setEffectEnabled(0, false);

        chain.getEffect(0)->getAPVTS().getParameter("drive")->setValueNotifyingHost(1.0f);
        chain.prepare(makeSpec());

        juce::AudioBuffer<float> buffer(1, 256);
        juce::FloatVectorOperations::fill(buffer.getWritePointer(0), 1.0f, buffer.getNumSamples());
        chain.process(buffer);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            expect(buffer.getSample(0, i) == 1.0f,
                   "Sample " + juce::String(i) + " was modified by a disabled chain effect");
    }

    /** UT-13: getState/setState must preserve chain structure, enabled flags, and parameters. */
    void testUT13_ChainState()
    {
        beginTest("UT-13 Chain state roundtrip");

        EffectFactory factory;
        registerAllEffects(factory);

        EffectChain chain;
        chain.addEffect(std::make_unique<OverdriveEffect>());
        chain.addEffect(std::make_unique<FuzzEffect>());
        chain.setEffectEnabled(1, false);

        chain.getEffect(0)->getAPVTS().getParameter("drive")->setValueNotifyingHost(0.9f);
        const float savedDrive =
            chain.getEffect(0)->getAPVTS().getRawParameterValue("drive")->load();

        const juce::ValueTree state = chain.getState();

        // Clear chain manually
        chain.removeEffect(1);
        chain.removeEffect(0);
        expectEquals(chain.getNumEffects(), 0);

        chain.setState(state, factory);

        expectEquals(chain.getNumEffects(), 2, "Chain size not restored");
        expect(chain.getEffect(0)->getTypeId() == juce::String(OverdriveEffect::TYPE_ID),
               "Effect at index 0 should be Overdrive");
        expect(chain.getEffect(1)->getTypeId() == juce::String(FuzzEffect::TYPE_ID),
               "Effect at index 1 should be Fuzz");
        expect(!chain.getEffect(1)->isEnabled(),
               "Second effect should still be disabled");

        const float restoredDrive =
            chain.getEffect(0)->getAPVTS().getRawParameterValue("drive")->load();
        expectWithinAbsoluteError(restoredDrive, savedDrive, 0.005f,
            "Drive parameter not preserved by chain state restore");
    }

    /** UT-14: Saving and loading a preset must fully restore chain structure and parameters. */
    void testUT14_PresetSaveLoad()
    {
        beginTest("UT-14 Preset save/load check");

        EffectFactory factory;
        registerAllEffects(factory);

        EffectChain chain;
        chain.addEffect(std::make_unique<OverdriveEffect>());
        chain.addEffect(std::make_unique<DigitalDelayEffect>());
        chain.setEffectEnabled(0, false);

        chain.getEffect(0)->getAPVTS().getParameter("drive")->setValueNotifyingHost(0.7f);
        chain.getEffect(1)->getAPVTS().getParameter("mix")->setValueNotifyingHost(0.8f);
        const float savedDrive =
            chain.getEffect(0)->getAPVTS().getRawParameterValue("drive")->load();
        const float savedMix =
            chain.getEffect(1)->getAPVTS().getRawParameterValue("mix")->load();

        const juce::File tempFile =
            juce::File::getSpecialLocation(juce::File::tempDirectory)
                .getChildFile("ap_test_preset.xml");

        PresetManager pm;
        expect(pm.savePreset(chain, tempFile), "Preset save failed");

        EffectChain loaded;
        juce::StringArray skipped;
        expect(pm.loadPreset(loaded, factory, tempFile, skipped), "Preset load failed");
        expect(skipped.isEmpty(), "Unexpected skipped type IDs");

        expectEquals(loaded.getNumEffects(), 2, "Loaded chain has wrong effect count");
        expect(loaded.getEffect(0)->getTypeId() == juce::String(OverdriveEffect::TYPE_ID),
               "Effect 0 type mismatch after load");
        expect(loaded.getEffect(1)->getTypeId() == juce::String(DigitalDelayEffect::TYPE_ID),
               "Effect 1 type mismatch after load");
        expect(!loaded.getEffect(0)->isEnabled(),
               "Effect 0 enabled state not preserved");

        const float loadedDrive =
            loaded.getEffect(0)->getAPVTS().getRawParameterValue("drive")->load();
        const float loadedMix =
            loaded.getEffect(1)->getAPVTS().getRawParameterValue("mix")->load();
        expectWithinAbsoluteError(loadedDrive, savedDrive, 0.005f, "Drive not preserved after preset load");
        expectWithinAbsoluteError(loadedMix,   savedMix,   0.005f, "Mix not preserved after preset load");

        tempFile.deleteFile();
    }

    /** UT-15: start() without configured devices must return a non-empty error string. */
    void testUT15_StartWithoutDevice()
    {
        beginTest("UT-15 Start without configured device returns error");

        AudioEngine engine;
        ApplicationController controller(engine);

        const juce::String error = controller.start();
        expect(error.isNotEmpty(), "Expected a non-empty error when no devices are configured");
    }

    /** UT-16: setSampleRate() must reject an unsupported rate and leave the current rate unchanged. */
    void testUT16_InvalidSampleRate()
    {
        beginTest("UT-16 Invalid sample rate rejected");

        AudioEngine engine;
        ApplicationController controller(engine);

        const double prevRate = controller.getCurrentSampleRate();
        const bool accepted  = controller.setSampleRate(12345.0);

        expect(!accepted, "setSampleRate(12345) should return false");
        expectWithinAbsoluteError(controller.getCurrentSampleRate(), prevRate, 0.001,
            "Sample rate changed after being rejected");
    }

    /** UT-17: setBufferSize() must reject an unsupported size and leave the current size unchanged. */
    void testUT17_InvalidBufferSize()
    {
        beginTest("UT-17 Invalid buffer size rejected");

        AudioEngine engine;
        ApplicationController controller(engine);

        const int prevSize = controller.getCurrentBufferSize();
        const bool accepted = controller.setBufferSize(300);

        expect(!accepted, "setBufferSize(300) should return false");
        expectEquals(controller.getCurrentBufferSize(), prevSize,
            "Buffer size changed after being rejected");
    }

    /** UT-18: Attempting to exceed the eight-effect limit must leave the chain at eight. */
    void testUT18_ChainFullLimit()
    {
        beginTest("UT-18 Chain limited to eight effects");

        AudioEngine engine;
        registerAllEffects(engine.getEffectFactory());
        ApplicationController controller(engine);

        for (int i = 0; i < ApplicationController::k_maxEffects; ++i)
            controller.addEffect(OverdriveEffect::TYPE_ID);

        expectEquals(controller.getNumEffects(), ApplicationController::k_maxEffects,
            "Chain should have exactly eight effects");

        controller.addEffect(OverdriveEffect::TYPE_ID);
        expectEquals(controller.getNumEffects(), ApplicationController::k_maxEffects,
            "Chain must not exceed eight effects");
    }

    /** UT-19: clearChain() must remove all effects from the chain. */
    void testUT19_ClearChain()
    {
        beginTest("UT-19 Clear chain operation");

        AudioEngine engine;
        registerAllEffects(engine.getEffectFactory());
        ApplicationController controller(engine);

        controller.addEffect(OverdriveEffect::TYPE_ID);
        controller.addEffect(FuzzEffect::TYPE_ID);
        controller.addEffect(HardClippingEffect::TYPE_ID);
        expectEquals(controller.getNumEffects(), 3);

        controller.clearChain();
        expectEquals(controller.getNumEffects(), 0, "Chain must be empty after clearChain()");
    }
};

static AudioSubsystemTests g_audioSubsystemTests;
