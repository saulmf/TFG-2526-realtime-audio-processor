#pragma once

#include <functional>
#include <memory>

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
 * The "Input:"/"Output:" labels and the Start/Stop button are underlined at
 * their first letter and reachable with Alt+I, Alt+O, and Alt+S respectively.
 *
 * @note onSessionStateChanged is called after every Start or Stop action so that
 * sibling components can react to the new state.
 */
class TransportBar : public juce::Component {
public:
    /** Creates the bar, sets up all controls, connects their callbacks, and populates the device lists. */
    explicit TransportBar(ApplicationController &controller);

    ~TransportBar() override;

    /** Lays out the device combo boxes, volume slider, start/stop button, level meters, and LED. */
    void resized() override;

    /** Draws the background, bottom separator line, and session LED indicator. */
    void paint(juce::Graphics &g) override;

    /** Updates the status label and repaints the LED to reflect the current session state. */
    void refreshStatus();

    /** Re-applies all translated strings after a runtime language change. */
    void refreshLanguage();

    /** Opens the input-device combo box's dropdown. Used by the Alt+I shortcut. */
    void focusInputDevice();

    /** Opens the output-device combo box's dropdown. Used by the Alt+O shortcut. */
    void focusOutputDevice();

    /** Triggers the Start/Stop session button, as if clicked. Used by the Alt+S shortcut. */
    void triggerStartStop();

    // Wired by the parent (MainWindow content component) to notify siblings when the session starts or stops.
    std::function<void()> onSessionStateChanged;

    // Fired with an error message when start() fails (no device selected, device error, etc.).
    std::function<void(const juce::String &)> onErrorMessage;

    static constexpr int k_height = 84;

private:
    /** Queries the controller for available I/O devices and populates both combo boxes. */
    void populateDeviceLists();

    // Underlines the first letter of the device labels and the start/stop button to show their Alt access key.
    class MnemonicLookAndFeel;
    std::unique_ptr<MnemonicLookAndFeel> m_mnemonicLookAndFeel;

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
