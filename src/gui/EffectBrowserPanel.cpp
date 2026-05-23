#include "EffectBrowserPanel.h"

class EffectBrowserPanel::EffectCatalogueComponent : public juce::Component,
                                                     public juce::TooltipClient {
public:
    EffectCatalogueComponent(ApplicationController &controller, UIRegistry &uiRegistry)
        : m_controller(controller), m_uiRegistry(uiRegistry) {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
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
                return desc->tooltip;
        }
        return {};
    }

    void mouseMove(const juce::MouseEvent &e) override { updateHover(e.getPosition()); }
    void mouseExit(const juce::MouseEvent &) override { clearHover(); }

    void setTargetSlot(int slot) noexcept { m_targetSlot = slot; }

    void mouseDown(const juce::MouseEvent &e) override {
        for (const auto &group: m_groups) {
            for (const auto &card: group.cards) {
                if (card.bounds.contains(e.getPosition())) {
                    if (m_controller.getNumEffects() >= ApplicationController::k_maxEffects) {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::MessageBoxIconType::WarningIcon,
                            TRANS("Effect chain full"),
                            TRANS("The effect chain is full (8 effects maximum). Remove an effect before adding a new one."));
                        return;
                    }

                    m_controller.addEffect(card.typeId, m_targetSlot);
                    m_targetSlot = -1; // reset so next open-via-button appends
                    DBG("[EffectBrowserPanel] Added " + card.typeId);
                    if (onEffectAdded) onEffectAdded();
                    return;
                }
            }
        }
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

    ApplicationController &m_controller;
    UIRegistry &m_uiRegistry;
    std::vector<CategoryGroup> m_groups;
    int m_hoveredGroup{-1};
    int m_hoveredCard{-1};
    int m_targetSlot{-1}; // insertion position (-1 = append)

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
    m_isExpanded = !m_isExpanded;

    // Opening via the button always appends (no specific target slot)
    m_catalogue->setTargetSlot(-1);

    m_toggleButton.setButtonText(m_isExpanded ? TRANS("v Browse Effects") : TRANS("^ Browse Effects"));
    m_viewport.setVisible(m_isExpanded);

    if (onExpandedStateChanged)
        onExpandedStateChanged(m_isExpanded);
}

void EffectBrowserPanel::expandForSlot(int slotIndex) {
    if (!m_isExpanded)
        toggleExpanded();

    m_catalogue->setTargetSlot(slotIndex);
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
