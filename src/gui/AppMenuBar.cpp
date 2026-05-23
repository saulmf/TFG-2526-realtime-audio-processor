#include "AppMenuBar.h"

AppMenuBar::AppMenuBar(ApplicationController &controller)
    : m_controller(controller) {
    addAndMakeVisible(m_menuBarComponent);
}

AppMenuBar::~AppMenuBar() {
    m_menuBarComponent.setModel(nullptr);
}

void AppMenuBar::resized() {
    m_menuBarComponent.setBounds(getLocalBounds());
}


// MenuBarModel

juce::StringArray AppMenuBar::getMenuBarNames() {
    return {TRANS("File"), TRANS("Audio"), TRANS("Help")};
}

juce::PopupMenu AppMenuBar::getMenuForIndex(int menuIndex, const juce::String &) {
    const bool running = m_controller.isRunning();

    juce::PopupMenu menu;

    if (menuIndex == 0) // File
    {
        menu.addItem(fileNewChain, TRANS("New Chain"), !running);
        menu.addSeparator();
        menu.addItem(fileSavePreset, TRANS("Save Preset..."), !running && m_controller.getNumEffects() > 0);
        menu.addItem(fileLoadPreset, TRANS("Load Preset..."), !running);
    } else if (menuIndex == 1) // Audio
    {
        // Buffer Size submenu
        juce::PopupMenu bufMenu;
        const int currentBuf = m_controller.getCurrentBufferSize();
        int id = bufSize64;
        for (int size: m_controller.getValidBufferSizes()) {
            bufMenu.addItem(id, juce::String(size) + " " + TRANS("samples"), !running, size == currentBuf);
            ++id;
        }
        menu.addSubMenu(TRANS("Buffer Size"), bufMenu, !running);

        // Sample Rate submenu
        juce::PopupMenu srMenu;
        const double currentSR = m_controller.getCurrentSampleRate();
        id = sampleRate44k;
        for (double rate: m_controller.getValidSampleRates()) {
            srMenu.addItem(id, juce::String(static_cast<int>(rate)) + " Hz",
                           !running, juce::approximatelyEqual(rate, currentSR));
            ++id;
        }
        menu.addSubMenu(TRANS("Sample Rate"), srMenu, !running);
    } else if (menuIndex == 2) // Help
    {
        menu.addItem(helpHowToUse, TRANS("How to use..."));
        menu.addSeparator();
        menu.addItem(helpAbout, TRANS("About..."));
    }

    return menu;
}

void AppMenuBar::menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/) {
    if (menuItemID == fileNewChain) {
        handleNewChain();
        return;
    }
    if (menuItemID == fileSavePreset) {
        handleSavePreset();
        return;
    }
    if (menuItemID == fileLoadPreset) {
        handleLoadPreset();
        return;
    }
    if (menuItemID == helpHowToUse) {
        handleHowToUse();
        return;
    }
    if (menuItemID == helpAbout) {
        handleAbout();
        return;
    }

    // Buffer size selection
    if (menuItemID >= bufSize64 && menuItemID <= bufSize1024) {
        const int idx = menuItemID - bufSize64;
        const auto &sizes = m_controller.getValidBufferSizes();
        if (idx < sizes.size()) {
            m_controller.setBufferSize(sizes[idx]);
            if (onSettingsChanged) onSettingsChanged();
        }
        return;
    }

    // Sample rate selection
    if (menuItemID >= sampleRate44k && menuItemID <= sampleRate96k) {
        const int idx = menuItemID - sampleRate44k;
        const auto &rates = m_controller.getValidSampleRates();
        if (idx < rates.size()) {
            m_controller.setSampleRate(rates[idx]);
            if (onSettingsChanged) onSettingsChanged();
        }
    }
}


void AppMenuBar::handleNewChain() {
    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::QuestionIcon,
        TRANS("New Chain"),
        TRANS("Clear the current effect chain? This cannot be undone."),
        TRANS("Clear"),
        TRANS("Cancel"),
        nullptr,
        juce::ModalCallbackFunction::create([this](int result) {
            if (result == 1) {
                m_controller.clearChain();
                if (onChainChanged) onChainChanged();
            }
        }));
}

void AppMenuBar::handleSavePreset() {
    m_fileChooser = std::make_unique<juce::FileChooser>(
        TRANS("Save Preset"),
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.xml");

    m_fileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode
        | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser &fc) {
            const auto result = fc.getResult();
            if (result.getFullPathName().isEmpty())
                return;

            if (!m_controller.savePreset(result.withFileExtension("xml"))) {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,
                    TRANS("Error"),
                    TRANS("Failed to save preset."));
            }
        });
}

void AppMenuBar::handleLoadPreset() {
    m_fileChooser = std::make_unique<juce::FileChooser>(
        TRANS("Load Preset"),
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.xml");

    m_fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser &fc) {
            const auto result = fc.getResult();
            if (result.getFullPathName().isEmpty())
                return;

            if (m_controller.loadPreset(result)) {
                if (onChainChanged) onChainChanged();
            } else {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,
                    TRANS("Error"),
                    TRANS("Failed to load preset."));
            }
        });
}

namespace {
    // Custom component that renders help text with bold section headings.
    class HelpContent : public juce::Component {
    public:
        HelpContent() {
            const std::pair<juce::String, juce::String> sections[] =
            {
                {
                    TRANS("Starting a session:"),
                    TRANS("1. Select your audio input device (e.g. USB interface).") + "\n"
                    + TRANS("2. Select your audio output device.") + "\n"
                    + TRANS("3. Click \"Start session\". The LED turns green when active.")
                },

                {
                    TRANS("Adding effects:"),
                    TRANS(
                        "Click any empty slot (+) in the chain, or open \"Browse Effects\" at the bottom and click a pedal thumbnail. The effect is placed at that position.")
                },

                {
                    TRANS("Enabling / disabling an effect:"),
                    TRANS("Click the title bar of any effect card to toggle bypass. The LED turns red when bypassed.")
                },

                {
                    TRANS("Reordering effects:"),
                    TRANS("Drag an effect card by its title bar and drop it onto another slot.")
                },

                {
                    TRANS("Presets:"),
                    TRANS("Use File > Save Preset... / Load Preset... to store and recall the full effect chain.")
                },
            };

            const juce::Font headingFont{juce::FontOptions(13.5f).withStyle("Bold")};
            const juce::Font bodyFont{juce::FontOptions(13.0f)};

            juce::AttributedString attrStr;
            attrStr.setWordWrap(juce::AttributedString::byWord);
            attrStr.setLineSpacing(3.0f);

            for (const auto &[heading, body]: sections) {
                attrStr.append(heading + "\n", headingFont, juce::Colours::white);
                attrStr.append(body + "\n\n", bodyFont, juce::Colours::white.withAlpha(0.78f));
            }

            m_layout.createLayout(attrStr, k_textW);

            setSize(static_cast<int>(k_textW) + 2 * k_pad,
                    juce::roundToInt(m_layout.getHeight()) + 2 * k_pad);
        }

        void paint(juce::Graphics &g) override {
            g.fillAll(juce::Colour(0xFF252535));
            m_layout.draw(g, getLocalBounds().reduced(k_pad).toFloat());
        }

    private:
        static constexpr int k_pad = 18;
        static constexpr float k_textW = 430.0f;
        juce::TextLayout m_layout;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpContent)
    };
} // namespace

void AppMenuBar::handleHowToUse() {
    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned(new HelpContent());
    opts.dialogTitle = TRANS("How to use");
    opts.dialogBackgroundColour = juce::Colour(0xFF252535);
    opts.escapeKeyTriggersCloseButton = true;
    opts.useNativeTitleBar = false;
    opts.resizable = false;
    opts.launchAsync();
}

void AppMenuBar::handleAbout() {
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        TRANS("About Realtime Audio Processor"),
        "Realtime Audio Processor v1.0.0\n\n"
        + TRANS("Real-time multi-effects processor for electric guitar.") + "\n"
        + TRANS("Final Degree Project (TFG) - Software Engineering.") + "\n"
        + TRANS("Made by Saul Martin."));
}
