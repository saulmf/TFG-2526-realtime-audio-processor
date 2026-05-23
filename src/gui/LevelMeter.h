#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * LevelMeter - horizontal input level bar with peak hold and clip indicator.
 *
 * Visual layout (left to right):
 *  - Green: -60 to -12 dBFS
 *  - Yellow: -12 to -3 dBFS
 *  - Red: -3 to 0 dBFS
 *
 * A thin white peak-hold marker stays at the recent maximum for approx. 1.5 s before decaying.
 * The CLIP LED turns red on any sample >= 0 dBFS and holds for approx. 2 s.
 */
class LevelMeter : public juce::Component,
                   public juce::SettableTooltipClient,
                   private juce::Timer {
public:
    LevelMeter();

    void setLevelSource(std::function<float()> source);

    void setActive(bool shouldBeActive);

    void paint(juce::Graphics &g) override;

private:
    void timerCallback() override;

    std::function<float()> m_levelSource;

    float m_displayLevel{0.0f};
    float m_peakHold{0.0f};
    int m_peakHoldFrames{0};
    int m_clipHoldFrames{0};
    bool m_active{false};

    static constexpr float k_minDb = -60.0f;
    static constexpr float k_decayFactor = 0.88f; // per tick at 30 Hz
    static constexpr int k_peakHoldFrames = 45; // approx. 1.5 s
    static constexpr int k_clipHoldFrames = 60; // approx. 2 s

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};
