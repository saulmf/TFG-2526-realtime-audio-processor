#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "audio/EffectChain.h"

#include "effects/nonlinear/OverdriveEffect.h"
#include "effects/nonlinear/HardClippingEffect.h"
#include "effects/nonlinear/FuzzEffect.h"
#include "effects/filtering/ParametricEQEffect.h"
#include "effects/filtering/HighPassFilterEffect.h"
#include "effects/filtering/LowPassFilterEffect.h"
#include "effects/timebased/DigitalDelayEffect.h"
#include "effects/timebased/AlgorithmicReverbEffect.h"

/**
 * Automates PT-01, PT-02 and PT-06 from the Performance Testing plan: callback
 * execution time of EffectChain::process() under an empty chain, a full
 * 8-effect chain, and a minimum-buffer stress configuration.
 */
class PerformanceTests final : public juce::UnitTest
{
public:
    PerformanceTests() : juce::UnitTest("Performance", "Performance") {}

    void runTest() override
    {
        testPT01_EmptyChainCallbackTime();
        testPT02_FullChainCallbackTime();
        testPT06_MinimumBufferStressTest();
    }

private:
    static constexpr int k_steadyStateCallbacks = 10000;
    static constexpr int k_warmupCallbacks = 100;

    struct TimingStats
    {
        double meanUs;
        double maxUs;
        int dropouts; // callbacks whose execution time exceeded the buffer period
    };

    static void addAllEffects(EffectChain &chain)
    {
        chain.addEffect(std::make_unique<OverdriveEffect>());
        chain.addEffect(std::make_unique<HardClippingEffect>());
        chain.addEffect(std::make_unique<FuzzEffect>());
        chain.addEffect(std::make_unique<ParametricEQEffect>());
        chain.addEffect(std::make_unique<HighPassFilterEffect>());
        chain.addEffect(std::make_unique<LowPassFilterEffect>());
        chain.addEffect(std::make_unique<DigitalDelayEffect>());
        chain.addEffect(std::make_unique<AlgorithmicReverbEffect>());
    }

    /**
     * Runs numCallbacks process() calls.
     *
     * Returns:
     * - mean execution time in us
     * - max execution time in us
     * - count of callbacks that overran bufferPeriodUs.
     *
     * The buffer is refilled with noise before each call (outside the timed region), so data-dependent effects
     * (like clipping) aren't benchmarked against silence.
    */
    static TimingStats measureCallbackTimes(EffectChain &chain, juce::AudioBuffer<float> &buffer,
                                              int numCallbacks, double bufferPeriodUs)
    {
        const double ticksPerSecond = static_cast<double>(juce::Time::getHighResolutionTicksPerSecond());
        juce::Random rng(42);

        double sumUs = 0.0;
        double maxUs = 0.0;
        int dropouts = 0;

        for (int i = 0; i < numCallbacks; ++i)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto *data = buffer.getWritePointer(ch);
                for (int s = 0; s < buffer.getNumSamples(); ++s)
                    data[s] = rng.nextFloat() * 2.0f - 1.0f;
            }

            const auto start = juce::Time::getHighResolutionTicks();
            chain.process(buffer);
            const auto end = juce::Time::getHighResolutionTicks();

            const double us = static_cast<double>(end - start) / ticksPerSecond * 1.0e6;
            sumUs += us;
            maxUs = juce::jmax(maxUs, us);
            if (us > bufferPeriodUs)
                ++dropouts;
        }

        return { sumUs / static_cast<double>(numCallbacks), maxUs, dropouts };
    }

    static double bufferPeriodUs(const juce::dsp::ProcessSpec &spec)
    {
        return static_cast<double>(spec.maximumBlockSize) / spec.sampleRate * 1.0e6;
    }

    void logStats(const TimingStats &stats)
    {
        logMessage("  Mean: " + juce::String(stats.meanUs, 2) + " us, Max: " + juce::String(stats.maxUs, 2)
                  + " us, Dropouts: " + juce::String(stats.dropouts));
    }

    /** PT-01: callback time with an empty chain (44100Hz / 256 samples) must stay under 1ms. */
    void testPT01_EmptyChainCallbackTime()
    {
        beginTest("PT-01 Callback time, empty chain");

        const juce::dsp::ProcessSpec spec{44100.0, 256, 1};
        EffectChain chain;
        chain.prepare(spec);

        juce::AudioBuffer<float> buffer(static_cast<int>(spec.numChannels),
                                        static_cast<int>(spec.maximumBlockSize));

        measureCallbackTimes(chain, buffer, k_warmupCallbacks, bufferPeriodUs(spec));
        const auto stats = measureCallbackTimes(chain, buffer, k_steadyStateCallbacks, bufferPeriodUs(spec));
        logStats(stats);

        expect(stats.meanUs < 1000.0, "Mean callback time exceeds 1 ms threshold");
        expect(stats.maxUs < 1000.0, "Max callback time exceeds 1 ms threshold");
    }

    /** PT-02: callback time with a full 8-effect chain (44100Hz / 256 samples) must stay under 5.8ms. */
    void testPT02_FullChainCallbackTime()
    {
        beginTest("PT-02 Callback time, full chain");

        const juce::dsp::ProcessSpec spec{44100.0, 256, 1};
        EffectChain chain;
        addAllEffects(chain);
        chain.prepare(spec);

        juce::AudioBuffer<float> buffer(static_cast<int>(spec.numChannels),
                                        static_cast<int>(spec.maximumBlockSize));

        measureCallbackTimes(chain, buffer, k_warmupCallbacks, bufferPeriodUs(spec));
        const auto stats = measureCallbackTimes(chain, buffer, k_steadyStateCallbacks, bufferPeriodUs(spec));
        logStats(stats);

        expect(stats.meanUs < 5800.0, "Mean callback time exceeds 5.8 ms threshold");
        expect(stats.maxUs < 5800.0, "Max callback time exceeds 5.8 ms threshold");
    }

    /** PT-06: minimum buffer stress test (64 samples / 8 effects, 60s of real-time playback)
        must stay under 1.45ms with zero dropouts. */
    void testPT06_MinimumBufferStressTest()
    {
        beginTest("PT-06 Callback time, minimum buffer stress test");

        const juce::dsp::ProcessSpec spec{44100.0, 64, 1};
        EffectChain chain;
        addAllEffects(chain);
        chain.prepare(spec);

        juce::AudioBuffer<float> buffer(static_cast<int>(spec.numChannels),
                                        static_cast<int>(spec.maximumBlockSize));

        measureCallbackTimes(chain, buffer, k_warmupCallbacks, bufferPeriodUs(spec));

        // Number of callbacks needed to cover 60s of real-time playback at this buffer size.
        const int numCallbacks = static_cast<int>(60.0 * spec.sampleRate / spec.maximumBlockSize);
        const auto stats = measureCallbackTimes(chain, buffer, numCallbacks, bufferPeriodUs(spec));
        logStats(stats);

        expect(stats.meanUs < 1450.0, "Mean callback time exceeds 1.45 ms threshold");
        expectEquals(stats.dropouts, 0, "One or more callbacks overran the 64-sample buffer period");
    }
};

static PerformanceTests g_performanceTests;
