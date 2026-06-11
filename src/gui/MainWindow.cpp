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

        // Clicking an empty slot expands the browser and targets that slot position
        m_chainPanel.onEmptySlotClicked = [this](int slotIndex) {
            m_browserPanel.expandForSlot(slotIndex);
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
    }

    int expandedBrowserHeight() const noexcept {
        return juce::jmax(200, getHeight() * 60 / 100);
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

    auto *content = new MainContentComponent(controller, uiRegistry);
    content->setSize(1050, 720);
    setContentOwned(content, true);

    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

void MainWindow::closeButtonPressed() {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
