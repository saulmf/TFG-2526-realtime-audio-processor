#include "MainWindow.h"
#include "AppMenuBar.h"
#include "TransportBar.h"
#include "EffectChainPanel.h"
#include "EffectBrowserPanel.h"
#include "StatusBar.h"

class NotificationPopup : public juce::Component, private juce::Timer {
public:
    NotificationPopup() {
        m_label.setJustificationType(juce::Justification::centred);
        m_label.setColour(juce::Label::textColourId, juce::Colours::white);
        m_label.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(m_label);
        setInterceptsMouseClicks(false, false);
        setVisible(false);
    }

    void show(const juce::String &msg, int durationMs = 3000) {
        m_label.setText(msg, juce::dontSendNotification);
        setVisible(true);
        toFront(false);
        stopTimer();
        startTimer(durationMs);
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(juce::Colour(0xF2252535));
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(juce::Colour(0xFFFBBF24));
        g.drawRoundedRectangle(bounds, 8.0f, 1.5f);
    }

    void resized() override {
        m_label.setBounds(getLocalBounds().reduced(12, 0));
    }

private:
    void timerCallback() override {
        stopTimer();
        setVisible(false);
    }

    juce::Label m_label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotificationPopup)
};

class MainContentComponent : public juce::Component {
public:
    MainContentComponent(ApplicationController &controller, UIRegistry &uiRegistry)
        : m_menuBar(controller),
          m_transportBar(controller),
          m_chainPanel(controller, uiRegistry),
          m_browserPanel(controller, uiRegistry),
          m_statusBar(controller) {
        addAndMakeVisible(m_menuBar);
        addAndMakeVisible(m_transportBar);
        addAndMakeVisible(m_chainPanel);
        addAndMakeVisible(m_browserPanel);
        addAndMakeVisible(m_statusBar);

        m_browserPanel.toFront(false);
        addChildComponent(m_errorPopup);

        // Route error messages from all child components to the notification popup
        auto showError = [this](const juce::String &msg) { m_errorPopup.show(msg); };
        m_menuBar.onErrorMessage       = showError;
        m_transportBar.onErrorMessage  = showError;
        m_browserPanel.onErrorMessage  = showError;
        m_chainPanel.onErrorMessage    = showError;

        // Rebuild chain display after new chain / preset load from menu
        m_menuBar.onChainChanged = [this] {
            m_chainPanel.refreshChain();
        };

        // Refresh status bar when audio settings change via the menu
        m_menuBar.onSettingsChanged = [this] {
            m_statusBar.refresh();
        };

        // Refresh chain visuals and status bar when session starts/stops
        m_transportBar.onSessionStateChanged = [this] {
            m_chainPanel.repaint();
            m_statusBar.refresh(); // sample rate may be decided by the selected device
        };

        // Rebuild chain display after an effect is added from the browser
        m_browserPanel.onEffectAdded = [this] {
            m_chainPanel.refreshChain();
        };

        // Clicking an empty slot expands the browser
        m_chainPanel.onEmptySlotClicked = [this](int) {
            m_browserPanel.expandForSlot();
        };

        // Animate the browser height between collapsed and expanded.
        m_browserPanel.onExpandedStateChanged = [this](bool expanded) {
            const int targetH = expanded ? expandedBrowserHeight() : EffectBrowserPanel::k_collapsedHeight;
            const int statusBarH = StatusBar::k_height;

            juce::Desktop::getInstance().getAnimator().animateComponent(
                &m_browserPanel,
                juce::Rectangle<int>(0, getHeight() - statusBarH - targetH, getWidth(), targetH),
                1.0f, 250, false, 1.0, 0.0);
        };

        // Set keyboard focus so that Alt/Ctrl shortcuts work even before the user has clicked any control.
        setWantsKeyboardFocus(true);
    }

    int expandedBrowserHeight() const noexcept {
        return juce::jmax(200, getHeight() * 60 / 100);
    }

    /** Handles application-wide keyboard shortcuts: Alt+letter opens a top-level
        menu or focuses a transport control, Ctrl+letter triggers a File action
        or opens the effect browser.

        @note Matching is done with KeyPress::operator==() rather than by reading
        getTextCharacter() directly because on Windows, holding Ctrl makes the OS translate
        the keystroke to a C0 control character (e.g. Ctrl+N becomes 0x0E), which
        JUCE then discards, leaving getTextCharacter() == 0 for every Ctrl shortcut.
        operator==() treats a zero textCharacter on either side as a wildcard and
        compares keyCode case-insensitively instead, so it works for both Alt and Ctrl combinations. */
    bool keyPressed(const juce::KeyPress &key) override {
        using juce::KeyPress;
        using juce::ModifierKeys;

        if (key == KeyPress('f', ModifierKeys::altModifier, 0)) { m_menuBar.openMenu(0); return true; } // File
        if (key == KeyPress('a', ModifierKeys::altModifier, 0)) { m_menuBar.openMenu(1); return true; } // Audio
        if (key == KeyPress('h', ModifierKeys::altModifier, 0)) { m_menuBar.openMenu(2); return true; } // Help

        if (key == KeyPress('i', ModifierKeys::altModifier, 0)) { m_transportBar.focusInputDevice(); return true; }
        if (key == KeyPress('o', ModifierKeys::altModifier, 0)) { m_transportBar.focusOutputDevice(); return true; }
        if (key == KeyPress('s', ModifierKeys::altModifier, 0)) { m_transportBar.triggerStartStop(); return true; }

        if (key == KeyPress('n', ModifierKeys::commandModifier, 0)) { m_menuBar.triggerNewChain(); return true; }
        if (key == KeyPress('s', ModifierKeys::commandModifier, 0)) { m_menuBar.triggerSavePreset(); return true; }
        if (key == KeyPress('o', ModifierKeys::commandModifier, 0)) { m_menuBar.triggerLoadPreset(); return true; }
        if (key == KeyPress('e', ModifierKeys::commandModifier, 0)) { m_browserPanel.expandForSlot(); return true; }

        // Fallback effect-browser navigation: the catalogue handles these
        // itself once it has actual keyboard focus, but this also makes them
        // work right after Ctrl+E, before that focus transfer has settled.
        if (m_browserPanel.isExpanded()) {
            if (key.isKeyCode(KeyPress::leftKey))  { m_browserPanel.moveCatalogueHighlight(0, -1); return true; }
            if (key.isKeyCode(KeyPress::rightKey)) { m_browserPanel.moveCatalogueHighlight(0, 1);  return true; }
            if (key.isKeyCode(KeyPress::upKey))    { m_browserPanel.moveCatalogueHighlight(-1, 0); return true; }
            if (key.isKeyCode(KeyPress::downKey))  { m_browserPanel.moveCatalogueHighlight(1, 0);  return true; }

            if (key.isKeyCode(KeyPress::returnKey) || key.isKeyCode(KeyPress::spaceKey)) {
                m_browserPanel.addHighlightedEffect();
                return true;
            }
        }

        return false;
    }

    void resized() override {
        const int w = getWidth();
        const int h = getHeight();
        constexpr int menuH = AppMenuBar::k_height;
        constexpr int transportH = TransportBar::k_height;
        constexpr int browserH = EffectBrowserPanel::k_collapsedHeight;
        constexpr int statusBarH = StatusBar::k_height;

        m_menuBar.setBounds(0, 0, w, menuH);
        m_transportBar.setBounds(0, menuH, w, transportH);
        m_statusBar.setBounds(0, h - statusBarH, w, statusBarH);
        const int currentBrowserH = m_browserPanel.isExpanded() ? expandedBrowserHeight() : browserH;
        m_browserPanel.setBounds(0, h - statusBarH - currentBrowserH, w, currentBrowserH);

        // Chain panel fills the space between the transport bar and the browser header
        m_chainPanel.setBounds(0, menuH + transportH, w, h - menuH - transportH - browserH - statusBarH);

        // Notification popup: centred horizontally in the chain panel area
        constexpr int popupW = 420;
        constexpr int popupH = 54;
        const int chainTop = menuH + transportH;
        const int chainH = h - menuH - transportH - browserH - statusBarH;
        m_errorPopup.setBounds((w - popupW) / 2, chainTop + (chainH - popupH) / 2, popupW, popupH);
    }

private:
    AppMenuBar m_menuBar;
    TransportBar m_transportBar;
    EffectChainPanel m_chainPanel;
    EffectBrowserPanel m_browserPanel;
    StatusBar m_statusBar;
    NotificationPopup m_errorPopup;
    juce::TooltipWindow m_tooltipWindow{this, 500};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
};


MainWindow::MainWindow(const juce::String &name,
                       ApplicationController &controller,
                       UIRegistry &uiRegistry)
    : DocumentWindow(name,
                     juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(backgroundColourId),
                     allButtons),
      m_controller(controller),
      m_uiRegistry(uiRegistry) {
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(k_minWidth, k_minHeight, 0x3fffffff, 0x3fffffff);

    auto *content = new MainContentComponent(controller, uiRegistry);
    content->setSize(1050, 720);
    setContentOwned(content, true);

    centreWithSize(getWidth(), getHeight());
    setVisible(true);

    content->grabKeyboardFocus();
}

void MainWindow::closeButtonPressed() {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
