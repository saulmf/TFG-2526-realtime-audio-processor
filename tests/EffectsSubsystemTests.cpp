#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "effects/nonlinear/OverdriveEffect.h"
#include "effects/nonlinear/HardClippingEffect.h"
#include "effects/nonlinear/FuzzEffect.h"
#include "effects/filtering/ParametricEQEffect.h"
#include "effects/filtering/HighPassFilterEffect.h"
#include "effects/filtering/LowPassFilterEffect.h"
#include "effects/timebased/DigitalDelayEffect.h"
#include "effects/timebased/AlgorithmicReverbEffect.h"

class EffectsSubsystemTests final : public juce::UnitTest
{
public:
    EffectsSubsystemTests() : juce::UnitTest("Effects Subsystem", "AudioProcessor") {}

    void runTest() override
    {
        testUT01_BypassDisabled();
        testUT02_BypassEnabled();
        testUT03_StateRoundtrip();
        testUT04_AtomicPointerInit();
        testUT05_PrepareAndProcess();
        testUT06_ResetClearsDelay();
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

    /** Verifies that the raw denormalized value of paramId matches between two effects. */
    void checkParam(IAudioEffect& original,
                    IAudioEffect& restored,
                    const char*   paramId,
                    float         tolerance)
    {
        const float orig = original.getAPVTS().getRawParameterValue(paramId)->load();
        const float rest = restored.getAPVTS().getRawParameterValue(paramId)->load();
        expectWithinAbsoluteError(rest, orig, tolerance,
            juce::String(paramId) + " roundtrip mismatch");
    }

    /** UT-01: A disabled effect must not alter the buffer. */
    void testUT01_BypassDisabled()
    {
        beginTest("UT-01 Bypass: disabled effect leaves buffer unchanged");

        OverdriveEffect effect;
        effect.getAPVTS().getParameter("drive")->setValueNotifyingHost(1.0f);
        effect.setEnabled(false);
        effect.prepare(makeSpec());

        juce::AudioBuffer<float> buffer(1, 256);
        juce::FloatVectorOperations::fill(buffer.getWritePointer(0), 1.0f, buffer.getNumSamples());
        effect.process(buffer);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            expect(buffer.getSample(0, i) == 1.0f,
                   "Sample " + juce::String(i) + " was modified by a disabled effect");
    }

    /** UT-02: An enabled effect at maximum drive must alter at least one sample. */
    void testUT02_BypassEnabled()
    {
        beginTest("UT-02 Bypass: enabled effect modifies the buffer");

        OverdriveEffect effect;
        effect.getAPVTS().getParameter("drive")->setValueNotifyingHost(1.0f);
        effect.setEnabled(true);
        effect.prepare(makeSpec());

        juce::AudioBuffer<float> buffer(1, 256);
        juce::FloatVectorOperations::fill(buffer.getWritePointer(0), 0.5f, buffer.getNumSamples());
        effect.process(buffer);

        bool anyDiffers = false;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (buffer.getSample(0, i) != 0.5f)
            {
                anyDiffers = true;
                break;
            }
        }

        expect(anyDiffers, "Enabled effect did not modify any samples");
    }

    /** UT-03: getState/setState roundtrip must preserve parameter values for every effect type. */
    void testUT03_StateRoundtrip()
    {
        beginTest("UT-03 Effect state roundtrip");

        // Overdrive: drive, level, tone in [0, 1]
        {
            OverdriveEffect e1;
            e1.getAPVTS().getParameter("drive")->setValueNotifyingHost(0.8f);
            e1.getAPVTS().getParameter("level")->setValueNotifyingHost(0.3f);
            e1.getAPVTS().getParameter("tone")->setValueNotifyingHost(0.7f);
            juce::ValueTree s = e1.getState();
            OverdriveEffect e2;
            e2.setState(s);
            checkParam(e1, e2, "drive", 0.005f);
            checkParam(e1, e2, "level", 0.005f);
            checkParam(e1, e2, "tone",  0.005f);
        }

        // HardClipping: gain, level, tone in [0, 1]
        {
            HardClippingEffect e1;
            e1.getAPVTS().getParameter("gain")->setValueNotifyingHost(0.9f);
            e1.getAPVTS().getParameter("level")->setValueNotifyingHost(0.4f);
            e1.getAPVTS().getParameter("tone")->setValueNotifyingHost(0.6f);
            juce::ValueTree s = e1.getState();
            HardClippingEffect e2;
            e2.setState(s);
            checkParam(e1, e2, "gain",  0.005f);
            checkParam(e1, e2, "level", 0.005f);
            checkParam(e1, e2, "tone",  0.005f);
        }

        // Fuzz: fuzz, level in [0, 1]
        {
            FuzzEffect e1;
            e1.getAPVTS().getParameter("fuzz")->setValueNotifyingHost(0.75f);
            e1.getAPVTS().getParameter("level")->setValueNotifyingHost(0.5f);
            juce::ValueTree s = e1.getState();
            FuzzEffect e2;
            e2.setState(s);
            checkParam(e1, e2, "fuzz",  0.005f);
            checkParam(e1, e2, "level", 0.005f);
        }

        // ParametricEQ: freq [20, 20000], gain [-24, 24], Q [0.1, 10] per band
        {
            ParametricEQEffect e1;
            e1.getAPVTS().getParameter("freq1")->setValueNotifyingHost(0.4f);
            e1.getAPVTS().getParameter("gain1")->setValueNotifyingHost(0.75f);
            e1.getAPVTS().getParameter("q1")->setValueNotifyingHost(0.6f);
            e1.getAPVTS().getParameter("freq2")->setValueNotifyingHost(0.5f);
            e1.getAPVTS().getParameter("gain2")->setValueNotifyingHost(0.3f);
            e1.getAPVTS().getParameter("q2")->setValueNotifyingHost(0.8f);
            e1.getAPVTS().getParameter("freq3")->setValueNotifyingHost(0.7f);
            e1.getAPVTS().getParameter("gain3")->setValueNotifyingHost(0.6f);
            e1.getAPVTS().getParameter("q3")->setValueNotifyingHost(0.4f);
            juce::ValueTree s = e1.getState();
            ParametricEQEffect e2;
            e2.setState(s);
            checkParam(e1, e2, "freq1", 1.0f);
            checkParam(e1, e2, "gain1", 0.1f);
            checkParam(e1, e2, "q1",    0.05f);
            checkParam(e1, e2, "freq2", 1.0f);
            checkParam(e1, e2, "gain2", 0.1f);
            checkParam(e1, e2, "q2",    0.05f);
            checkParam(e1, e2, "freq3", 1.0f);
            checkParam(e1, e2, "gain3", 0.1f);
            checkParam(e1, e2, "q3",    0.05f);
        }

        // HighPassFilter: cutoff [20, 20000] Hz (skewed), order int [1, 4]
        {
            HighPassFilterEffect e1;
            e1.getAPVTS().getParameter("cutoff")->setValueNotifyingHost(0.6f);
            e1.getAPVTS().getParameter("order")->setValueNotifyingHost(0.67f);
            juce::ValueTree s = e1.getState();
            HighPassFilterEffect e2;
            e2.setState(s);
            checkParam(e1, e2, "cutoff", 1.0f);
            checkParam(e1, e2, "order",  0.5f);
        }

        // LowPassFilter: cutoff [20, 20000] Hz (skewed), order int [1, 4]
        {
            LowPassFilterEffect e1;
            e1.getAPVTS().getParameter("cutoff")->setValueNotifyingHost(0.4f);
            e1.getAPVTS().getParameter("order")->setValueNotifyingHost(0.33f);
            juce::ValueTree s = e1.getState();
            LowPassFilterEffect e2;
            e2.setState(s);
            checkParam(e1, e2, "cutoff", 1.0f);
            checkParam(e1, e2, "order",  0.5f);
        }

        // DigitalDelay: delayTime [1, 2000] ms, feedback [0, 0.95], mix [0, 1]
        {
            DigitalDelayEffect e1;
            e1.getAPVTS().getParameter("delayTime")->setValueNotifyingHost(0.5f);
            e1.getAPVTS().getParameter("feedback")->setValueNotifyingHost(0.6f);
            e1.getAPVTS().getParameter("mix")->setValueNotifyingHost(0.8f);
            juce::ValueTree s = e1.getState();
            DigitalDelayEffect e2;
            e2.setState(s);
            checkParam(e1, e2, "delayTime", 1.0f);
            checkParam(e1, e2, "feedback",  0.005f);
            checkParam(e1, e2, "mix",       0.005f);
        }

        // AlgorithmicReverb: roomSize, damping, width, mix in [0, 1]
        {
            AlgorithmicReverbEffect e1;
            e1.getAPVTS().getParameter("roomSize")->setValueNotifyingHost(0.8f);
            e1.getAPVTS().getParameter("damping")->setValueNotifyingHost(0.4f);
            e1.getAPVTS().getParameter("width")->setValueNotifyingHost(0.6f);
            e1.getAPVTS().getParameter("mix")->setValueNotifyingHost(0.7f);
            juce::ValueTree s = e1.getState();
            AlgorithmicReverbEffect e2;
            e2.setState(s);
            checkParam(e1, e2, "roomSize", 0.005f);
            checkParam(e1, e2, "damping",  0.005f);
            checkParam(e1, e2, "width",    0.005f);
            checkParam(e1, e2, "mix",      0.005f);
        }
    }

    /** UT-04: getRawParameterValue must return non-null for every declared parameter. */
    void testUT04_AtomicPointerInit()
    {
        beginTest("UT-04 Atomic parameter pointers are non-null after construction");

        {
            OverdriveEffect e;
            expect(e.getAPVTS().getRawParameterValue("drive") != nullptr);
            expect(e.getAPVTS().getRawParameterValue("level") != nullptr);
            expect(e.getAPVTS().getRawParameterValue("tone")  != nullptr);
        }
        {
            HardClippingEffect e;
            expect(e.getAPVTS().getRawParameterValue("gain")  != nullptr);
            expect(e.getAPVTS().getRawParameterValue("level") != nullptr);
            expect(e.getAPVTS().getRawParameterValue("tone")  != nullptr);
        }
        {
            FuzzEffect e;
            expect(e.getAPVTS().getRawParameterValue("fuzz")  != nullptr);
            expect(e.getAPVTS().getRawParameterValue("level") != nullptr);
        }
        {
            ParametricEQEffect e;
            for (const char* id : { "freq1","gain1","q1","freq2","gain2","q2","freq3","gain3","q3" })
                expect(e.getAPVTS().getRawParameterValue(id) != nullptr,
                       juce::String(id) + " pointer is null");
        }
        {
            HighPassFilterEffect e;
            expect(e.getAPVTS().getRawParameterValue("cutoff") != nullptr);
            expect(e.getAPVTS().getRawParameterValue("order")  != nullptr);
        }
        {
            LowPassFilterEffect e;
            expect(e.getAPVTS().getRawParameterValue("cutoff") != nullptr);
            expect(e.getAPVTS().getRawParameterValue("order")  != nullptr);
        }
        {
            DigitalDelayEffect e;
            expect(e.getAPVTS().getRawParameterValue("delayTime") != nullptr);
            expect(e.getAPVTS().getRawParameterValue("feedback")  != nullptr);
            expect(e.getAPVTS().getRawParameterValue("mix")       != nullptr);
        }
        {
            AlgorithmicReverbEffect e;
            expect(e.getAPVTS().getRawParameterValue("roomSize") != nullptr);
            expect(e.getAPVTS().getRawParameterValue("damping")  != nullptr);
            expect(e.getAPVTS().getRawParameterValue("width")    != nullptr);
            expect(e.getAPVTS().getRawParameterValue("mix")      != nullptr);
        }
    }

    /** UT-05: Each effect must prepare and process a buffer without error. */
    void testUT05_PrepareAndProcess()
    {
        beginTest("UT-05 Prepare operation check");

        const auto spec = makeSpec();
        juce::AudioBuffer<float> buffer(1, 256);
        buffer.clear();

        auto run = [&](IAudioEffect& e)
        {
            e.prepare(spec);
            e.process(buffer);
            expect(true);
        };

        { OverdriveEffect e;         run(e); }
        { HardClippingEffect e;      run(e); }
        { FuzzEffect e;              run(e); }
        { ParametricEQEffect e;      run(e); }
        { HighPassFilterEffect e;    run(e); }
        { LowPassFilterEffect e;     run(e); }
        { DigitalDelayEffect e;      run(e); }
        { AlgorithmicReverbEffect e; run(e); }
    }

    /** UT-06: After reset(), processing silence must produce a silent output. */
    void testUT06_ResetClearsDelay()
    {
        beginTest("UT-06 Reset operation clears delay state");

        DigitalDelayEffect effect;
        const auto spec = makeSpec();
        effect.prepare(spec);

        // Shortest delay so the impulse recirculates within one buffer period
        effect.getAPVTS().getParameter("delayTime")->setValueNotifyingHost(0.0f);
        effect.getAPVTS().getParameter("feedback")->setValueNotifyingHost(0.5f);
        effect.getAPVTS().getParameter("mix")->setValueNotifyingHost(1.0f);

        juce::AudioBuffer<float> impulse(1, 256);
        impulse.clear();
        impulse.setSample(0, 0, 1.0f);
        effect.process(impulse);

        effect.reset();

        juce::AudioBuffer<float> silence(1, 256);
        silence.clear();
        effect.process(silence);

        for (int i = 0; i < silence.getNumSamples(); ++i)
            expectWithinAbsoluteError(silence.getSample(0, i), 0.0f, 1e-6f,
                "Sample " + juce::String(i) + " non-zero after reset");
    }
};

static EffectsSubsystemTests g_effectsSubsystemTests;
