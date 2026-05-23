#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../ApplicationController.h"
#include "UIRegistry.h"

/**
 * MainWindow is the application's top-level window.
 *
 * Receives ApplicationController and UIRegistry at construction time and
 * passes both down to child components as the UI is built out.
 *
 * @note Owns all GUI subcomponents via JUCE component ownership.
 *
 * @warning The GUI layer never accesses AudioEngine or any audio subsystem directly.
 */
class MainWindow : public juce::DocumentWindow {
public:
    MainWindow(const juce::String &name,
               ApplicationController &controller,
               UIRegistry &uiRegistry);

    void closeButtonPressed() override;

private:
    ApplicationController &m_controller;
    UIRegistry &m_uiRegistry;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
