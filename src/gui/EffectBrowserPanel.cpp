#include "EffectBrowserPanel.h"

class EffectBrowserPanel::EffectCatalogueComponent : public juce::Component,
                                                     public juce::TooltipClient {
public:
    EffectCatalogueComponent(ApplicationController &controller, UIRegistry &uiRegistry)
        : m_controller(controller), m_uiRegistry(uiRegistry) {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        setWantsKeyboardFocus(true);
        buildLayout();
    }

    void paint(juce::Graphics &g) override {
        g.fillAll(juce::Colour(0xFF1A1A2E));

        for (const auto &group: m_groups) {
            // Category label.
            g.setColour(juce::Colours::lightgrey);
            g.setFont(juce::Font(13.0f, juce::Font::bold));
            g.drawText(juce::translate(group.category),
                       k_margin, group.labelY, getWidth() - 2 * k_margin, k_labelH,
                       juce::Justification::centredLeft);

            // Divider line after category label.
            g.setColour(juce::Colours::grey.withAlpha(0.4f));
            g.drawLine(k_margin, static_cast<float>(group.labelY + k_labelH - 2),
                       static_cast<float>(getWidth() - k_margin),
                       static_cast<float>(group.labelY + k_labelH - 2));

            // Effect cards.
            for (int i = 0; i < static_cast<int>(group.cards.size()); ++i) {
                const auto &card = group.cards[i];
                const bool hovered = (m_hoveredGroup == &group - m_groups.data()
                                      && m_hoveredCard == i);

                drawCard(g, card, hovered);
            }
        }
    }

    juce::String getTooltip() override {
        if (m_hoveredGroup >= 0 && m_hoveredCard >= 0
            && m_hoveredGroup < static_cast<int>(m_groups.size())
            && m_hoveredCard < static_cast<int>(m_groups[m_hoveredGroup].cards.size())) {
            const auto *desc = m_groups[m_hoveredGroup].cards[m_hoveredCard].descriptor;
            if (desc != nullptr)
                return juce::translate(desc->tooltip);
        }
        return {};
    }

    void mouseMove(const juce::MouseEvent &e) override { updateHover(e.getPosition()); }
    void mouseExit(const juce::MouseEvent &) override { clearHover(); }

    void mouseDown(const juce::MouseEvent &e) override {
        for (const auto &group: m_groups) {
            for (const auto &card: group.cards) {
                if (card.bounds.contains(e.getPosition())) {
                    addEffect(card);
                    return;
                }
            }
        }
    }

    bool keyPressed(const juce::KeyPress &key) override {
        if (key.isKeyCode(juce::KeyPress::leftKey))  { moveHighlight(0, -1); return true; }
        if (key.isKeyCode(juce::KeyPress::rightKey)) { moveHighlight(0, 1);  return true; }
        if (key.isKeyCode(juce::KeyPress::upKey))    { moveHighlight(-1, 0); return true; }
        if (key.isKeyCode(juce::KeyPress::downKey))  { moveHighlight(1, 0);  return true; }

        if (key.isKeyCode(juce::KeyPress::returnKey) || key.isKeyCode(juce::KeyPress::spaceKey)) {
            addHighlightedEffect();
            return true;
        }

        return false;
    }

    void focusGained(juce::Component::FocusChangeType) override {
        // Highlight the first card so Enter/Space works immediately, without
        // requiring an arrow key press first.
        if (m_hoveredGroup == -1 && !m_groups.empty() && !m_groups[0].cards.empty()) {
            m_hoveredGroup = 0;
            m_hoveredCard = 0;
            repaint();
        }
    }

    /** Moves the highlighted card by one row (dGroup) and/or column (dCard), clamped to the grid.
        If nothing is highlighted yet, lands on the first card regardless of direction. */
    void moveHighlight(int dGroup, int dCard) {
        if (m_groups.empty())
            return;

        int group = m_hoveredGroup;
        int card = m_hoveredCard;

        if (group < 0) {
            group = 0;
            card = 0;
        } else {
            group = juce::jlimit(0, static_cast<int>(m_groups.size()) - 1, group + dGroup);
            card += dCard;
        }

        card = juce::jlimit(0, static_cast<int>(m_groups[group].cards.size()) - 1, card);

        if (group != m_hoveredGroup || card != m_hoveredCard) {
            m_hoveredGroup = group;
            m_hoveredCard = card;
            repaint();
        }
    }

    /** Adds the currently highlighted card's effect to the chain, if any card is highlighted. */
    void addHighlightedEffect() {
        if (m_hoveredGroup < 0 || m_hoveredGroup >= static_cast<int>(m_groups.size()))
            return;

        const auto &group = m_groups[m_hoveredGroup];
        if (m_hoveredCard < 0 || m_hoveredCard >= static_cast<int>(group.cards.size()))
            return;

        addEffect(group.cards[m_hoveredCard]);
    }

private:
    // Layout constants
    static constexpr int k_margin = 12;
    static constexpr int k_cardW = 110;
    static constexpr int k_cardH = 72;
    static constexpr int k_cardGap = 8;
    static constexpr int k_labelH = 24;
    static constexpr int k_catGap = 14;

    struct CardInfo {
        juce::String typeId;
        juce::Rectangle<int> bounds;
        const EffectUIDescriptor *descriptor{nullptr};
    };

    struct CategoryGroup {
        juce::String category;
        int labelY{0};
        std::vector<CardInfo> cards;
    };

    void buildLayout() {
        const juce::StringArray categoryOrder{"Nonlinear", "Filtering", "Time-Based"};

        int y = k_margin;

        for (const auto &cat: categoryOrder) {
            CategoryGroup group;
            group.category = cat;
            group.labelY = y;

            int x = k_margin;
            int cardY = y + k_labelH;

            for (const auto &typeId: m_uiRegistry.getRegisteredTypeIds()) {
                const auto *desc = m_uiRegistry.getDescriptor(typeId);
                if (desc == nullptr || desc->category != cat)
                    continue;

                group.cards.push_back({typeId, {x, cardY, k_cardW, k_cardH}, desc});
                x += k_cardW + k_cardGap;
            }

            if (group.cards.empty())
                continue;

            m_groups.push_back(std::move(group));
            y += k_labelH + k_cardH + k_catGap;
        }

        setSize(800, y + k_margin);
    }

    void drawCard(juce::Graphics &g, const CardInfo &card, bool hovered) const {
        if (card.descriptor == nullptr)
            return;

        const auto bounds = card.bounds.toFloat();
        const auto body = hovered ? card.descriptor->bodyColour.brighter(0.25f) : card.descriptor->bodyColour;

        g.setColour(body);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(card.descriptor->accentColour.withAlpha(0.6f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.5f);

        g.setColour(card.descriptor->accentColour);
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText(card.descriptor->labelText,
                   card.bounds.reduced(4, 0).withTrimmedBottom(18),
                   juce::Justification::centred);

        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.setFont(juce::Font(10.5f));
        g.drawText(card.descriptor->displayName,
                   card.bounds.reduced(4, 0).withTop(card.bounds.getBottom() - 18),
                   juce::Justification::centred);
    }

    void updateHover(juce::Point<int> pos) {
        for (int gi = 0; gi < static_cast<int>(m_groups.size()); ++gi) {
            for (int ci = 0; ci < static_cast<int>(m_groups[gi].cards.size()); ++ci) {
                if (m_groups[gi].cards[ci].bounds.contains(pos)) {
                    if (m_hoveredGroup != gi || m_hoveredCard != ci) {
                        m_hoveredGroup = gi;
                        m_hoveredCard = ci;
                        repaint();
                    }
                    return;
                }
            }
        }
        clearHover();
    }

    void clearHover() {
        if (m_hoveredGroup != -1) {
            m_hoveredGroup = -1;
            m_hoveredCard = -1;
            repaint();
        }
    }

    void addEffect(const CardInfo &card) {
        if (m_controller.isRunning())
            return;

        if (m_controller.getNumEffects() >= ApplicationController::k_maxEffects) {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                TRANS("Effect chain full"),
                TRANS("The effect chain is full (8 effects maximum). Remove an effect before adding a new one."));
            return;
        }

        m_controller.addEffect(card.typeId);
        DBG("[EffectBrowserPanel] Added " + card.typeId);
        if (onEffectAdded) onEffectAdded();
    }

    ApplicationController &m_controller;
    UIRegistry &m_uiRegistry;
    std::vector<CategoryGroup> m_groups;
    int m_hoveredGroup{-1};
    int m_hoveredCard{-1};

public:
    std::function<void()> onEffectAdded;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectCatalogueComponent)
};

// EffectBrowserPanel

EffectBrowserPanel::~EffectBrowserPanel() = default;

EffectBrowserPanel::EffectBrowserPanel(ApplicationController &controller,
                                       UIRegistry &uiRegistry)
    : m_controller(controller),
      m_uiRegistry(uiRegistry),
      m_catalogue(std::make_unique<EffectCatalogueComponent>(controller, uiRegistry)) {

    // Header toggle button
    m_toggleButton.setClickingTogglesState(false);
    m_toggleButton.onClick = [this] { toggleExpanded(); };
    m_toggleButton.setTooltip(TRANS("Open or close the effect browser to add new effects to the chain."));
    addAndMakeVisible(m_toggleButton);

    // Forward catalogue add events to panel consumers
    m_catalogue->onEffectAdded = [this] { if (onEffectAdded) onEffectAdded(); };

    // Viewport wrapping the scrollable catalogue
    m_viewport.setViewedComponent(m_catalogue.get(), false);
    m_viewport.setScrollBarsShown(true, false);
    m_viewport.setVisible(false);   // hidden until expanded
    addAndMakeVisible(m_viewport);
}

void EffectBrowserPanel::toggleExpanded() {
    if (!m_isExpanded && m_controller.isRunning()) {
        if (onErrorMessage)
            onErrorMessage(TRANS("Stop the audio session before adding effects to the chain."));
        return;
    }

    m_isExpanded = !m_isExpanded;

    m_toggleButton.setButtonText(m_isExpanded ? TRANS("v Browse Effects") : TRANS("^ Browse Effects"));
    m_viewport.setVisible(m_isExpanded);

    if (m_isExpanded)
        m_catalogue->grabKeyboardFocus();
    else
        m_toggleButton.grabKeyboardFocus();

    if (onExpandedStateChanged)
        onExpandedStateChanged(m_isExpanded);
}

void EffectBrowserPanel::refreshLanguage() {
    m_toggleButton.setButtonText(m_isExpanded ? TRANS("v Browse Effects") : TRANS("^ Browse Effects"));
    m_toggleButton.setTooltip(TRANS("Open or close the effect browser to add new effects to the chain."));
    m_catalogue->repaint();
}

void EffectBrowserPanel::expandForSlot() {
    if (m_controller.isRunning())
        return;

    if (!m_isExpanded)
        toggleExpanded();
}

void EffectBrowserPanel::moveCatalogueHighlight(int dGroup, int dCard) {
    m_catalogue->moveHighlight(dGroup, dCard);
}

void EffectBrowserPanel::addHighlightedEffect() {
    m_catalogue->addHighlightedEffect();
}

void EffectBrowserPanel::paint(juce::Graphics &g) {
    // Header background
    g.fillAll(juce::Colour(0xFF252540));
    g.setColour(juce::Colours::grey.darker(0.3f));
    g.drawLine(0.0f, 0.0f, static_cast<float>(getWidth()), 0.0f, 1.5f);
}

void EffectBrowserPanel::resized() {
    constexpr int headerH = k_collapsedHeight;

    m_toggleButton.setBounds(0, 0, getWidth(), headerH);

    if (getHeight() > headerH) {
        m_viewport.setBounds(0, headerH, getWidth(), getHeight() - headerH);

        // Catalogue height is computed internally
        m_catalogue->setSize(getWidth(), m_catalogue->getHeight());
    }
}
