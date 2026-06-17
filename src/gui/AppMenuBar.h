#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../ApplicationController.h"

/**
 * AppMenuBar - application menu bar.
 *
 * Buffer Size and Sample Rate items are greyed out while the session is
 * running, since the audio device cannot be reconfigured mid-stream.
 *
 * Each top-level menu name is underlined at its first letter and can be opened with Alt+(that letter);
 * openMenu() is the entry point used for this by the parent's keyPressed() handler.
 * File actions also have a Ctrl shortcut (New Chain: Ctrl+N, Save Preset: Ctrl+S, Load Preset: Ctrl+O),
 * exposed as the trigger() methods.
 *
 * @note onChainChanged is fired after "New Chain" and after a successful "Load Preset" so the parent can refresh the EffectChainPanel.
 */
class AppMenuBar : public juce::Component,
                   public juce::MenuBarModel {
public:
    /** Creates the menu bar and adds the JUCE MenuBarComponent as a child. */
    explicit AppMenuBar(ApplicationController &controller);

    ~AppMenuBar() override;

    /** Fills the component with the MenuBarComponent. */
    void resized() override;

    /** Fired after a "New Chain" action or a successful "Load Preset" so the parent can refresh the EffectChainPanel. */
    std::function<void()> onChainChanged;

    /** Fired after a buffer size or sample rate change so the parent can update the StatusBar. */
    std::function<void()> onSettingsChanged;

    /** Fired with an error message when an action fails (e.g. preset file could not be written). */
    std::function<void(const juce::String &)> onErrorMessage;

    static constexpr int k_height = 24; ///< Fixed height of the menu bar in pixels.

    /** Returns the top-level menu names: File, Audio Settings, and Help. */
    juce::StringArray getMenuBarNames() override;

    /**
     * Builds and returns the popup menu for the given top-level index.
     * Audio settings items are greyed out while the session is running.
     * @param menuIndex Index of the top-level menu (0 = File, 1 = Audio Settings, 2 = Help).
     * @param menuName Display name of the menu (unused, present to satisfy the interface).
     * @return Fully populated PopupMenu for the requested index.
     */
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String &menuName) override;

    /** Dispatches the selected menu item to the appropriate private handler.
        @param menuItemID ID from the MenuIDs enum.
        @param topLevelMenuIndex Unused; present to satisfy the interface. */
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    /** Opens one of the top-level menus, as if it had been clicked. Used by Alt+letter access keys.
        @param menuIndex Index of the top-level menu (0 = File, 1 = Audio, 2 = Help). */
    void openMenu(int menuIndex);

    /** Triggers "New Chain" as if selected from the File menu. Used by the Ctrl+N shortcut. */
    void triggerNewChain();

    /** Triggers "Save Preset..." as if selected from the File menu. Used by the Ctrl+S shortcut. */
    void triggerSavePreset();

    /** Triggers "Load Preset..." as if selected from the File menu. Used by the Ctrl+O shortcut. */
    void triggerLoadPreset();

private:
    /** Clears the chain after asking the user for confirmation. */
    void handleNewChain();

    /** Opens a file chooser and saves the current chain state to an XML preset file. */
    void handleSavePreset();

    /** Opens a file chooser and loads a preset from an XML file, replacing the current chain. */
    void handleLoadPreset();

    /** Shows a brief how-to-use dialog. */
    void handleHowToUse();

    /** Shows the About dialog with version and author information. */
    void handleAbout();

    ApplicationController &m_controller;

    // Underlines the first letter of each top-level menu name to show its Alt access key.
    class MenuBarLookAndFeel;
    std::unique_ptr<MenuBarLookAndFeel> m_menuBarLookAndFeel;

    juce::MenuBarComponent m_menuBarComponent{this};
    std::unique_ptr<juce::FileChooser> m_fileChooser;

    enum MenuIDs {
        fileNewChain = 1001,
        fileSavePreset = 1002,
        fileLoadPreset = 1003,

        bufSize64 = 2001,
        bufSize128 = 2002,
        bufSize256 = 2003,
        bufSize512 = 2004,
        bufSize1024 = 2005,

        sampleRate44k = 2101,
        sampleRate48k = 2102,
        sampleRate96k = 2103,

        helpHowToUse = 3001,
        helpAbout = 3002,
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppMenuBar)
};
