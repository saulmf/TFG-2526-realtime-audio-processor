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
    /**
     * Creates the window, builds the MainContentComponent, and makes it visible.
     * @param name Application name shown in the title bar.
     * @param controller Facade used by all child components to interact with the audio subsystem.
     * @param uiRegistry Visual descriptors for each registered effect type.
     */
    MainWindow(const juce::String &name,
               ApplicationController &controller,
               UIRegistry &uiRegistry);

    /** Forwards the close request to the JUCE application, triggering a clean shutdown. */
    void closeButtonPressed() override;

private:
    ApplicationController &m_controller;
    UIRegistry &m_uiRegistry;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
