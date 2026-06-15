#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../ApplicationController.h"

/**
 * StatusBar - thin strip at the bottom of the window.
 *
 * Displays the current audio device configuration:
 * - Sample rate (Hz)
 * - Buffer size (Samples)
 */
class StatusBar : public juce::Component {
public:
    /** Creates the status bar and performs an initial refresh to show current settings. */
    explicit StatusBar(ApplicationController &controller);

    /** Reads the current sample rate and buffer size from the controller and schedules a repaint. */
    void refresh();

    /** Draws the background and the right-aligned configuration text. */
    void paint(juce::Graphics &g) override;

    static constexpr int k_height = 20;

private:
    ApplicationController &m_controller;
    juce::String m_text;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)
};
