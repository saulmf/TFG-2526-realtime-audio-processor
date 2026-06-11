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
 * @note onChainChanged is fired after "New Chain" and after a successful "Load Preset" so the parent can refresh the EffectChainPanel.
 */
class AppMenuBar : public juce::Component,
                   public juce::MenuBarModel {
public:
    explicit AppMenuBar(ApplicationController &controller);

    ~AppMenuBar() override;

    void resized() override;

    std::function<void()> onChainChanged;
    std::function<void()> onSettingsChanged;
    std::function<void(const juce::String &)> onErrorMessage;

    static constexpr int k_height = 24;

    juce::StringArray getMenuBarNames() override;

    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String &menuName) override;

    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

private:
    void handleNewChain();

    void handleSavePreset();

    void handleLoadPreset();

    void handleHowToUse();

    void handleAbout();

    ApplicationController &m_controller;
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
