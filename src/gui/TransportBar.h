#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../ApplicationController.h"
#include "LevelMeter.h"

/**
 * TransportBar - fixed top bar for device selection and session transport.
 *
 * Displays:
 *   - Input / Output device combo boxes
 *   - Master volume control
 *   - Start / Stop session button and session status indicator
 *   - Input / Output level meters
 *
 * @note onSessionStateChanged is called after every Start or Stop action so that
 * sibling components can react to the new state.
 */
class TransportBar : public juce::Component {
public:
    explicit TransportBar(ApplicationController &controller);

    void resized() override;

    void paint(juce::Graphics &g) override;

    void refreshStatus();

    // Wired by the parent (MainWindow content component) to notify siblings when the session starts or stops.
    std::function<void()> onSessionStateChanged;

    static constexpr int k_height = 84;

private:
    void populateDeviceLists();

    ApplicationController &m_controller;

    // Device selection
    juce::Label m_inputLabel{{}, TRANS("Input:")};
    juce::Label m_outputLabel{{}, TRANS("Output:")};
    juce::ComboBox m_inputBox;
    juce::ComboBox m_outputBox;

    // Master volume
    juce::Label m_volLabel{{}, TRANS("Vol:")};
    juce::Slider m_volumeSlider;

    // Start / Stop button, status label and LED indicator
    juce::TextButton m_startStopButton{TRANS("Start session")};
    juce::Label m_statusLabel;
    juce::Rectangle<int> m_ledBounds;

    // Input / Output level meters
    juce::Label m_inputMeterLabel{{}, "IN"};
    juce::Label m_outputMeterLabel{{}, "OUT"};
    LevelMeter m_inputLevelMeter;
    LevelMeter m_outputLevelMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};
