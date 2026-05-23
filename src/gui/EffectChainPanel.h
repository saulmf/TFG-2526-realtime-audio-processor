#pragma once

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../ApplicationController.h"
#include "PedalCard.h"
#include "UIRegistry.h"

/**
 * EffectChainPanel - 2x4 grid of pedal cards representing the effect chain.
 *
 * Filled slots show a PedalCard; empty slots show a placeholder.
 * Signal-flow arrows are painted between adjacent filled slots.
 *
 * Drag-to-reorder: PedalCard initiates the drag; this panel is the DragAndDropContainer + DragAndDropTarget.
 * Dropping calls moveEffect() and rebuilds the cards.
 *
 * Call refreshChain() after any external change to the effect chain (effect added from browser, preset loaded, etc.).
 */
class EffectChainPanel : public juce::Component,
                         public juce::DragAndDropContainer,
                         public juce::DragAndDropTarget {
public:
    EffectChainPanel(ApplicationController &controller, UIRegistry &uiRegistry);

    void paint(juce::Graphics &g) override;

    void resized() override;

    void mouseDown(const juce::MouseEvent &e) override;

    void refreshChain();

    // Called when the user clicks an empty slot - slot index is passed
    std::function<void(int slotIndex)> onEmptySlotClicked;

    // DragAndDropTarget
    bool isInterestedInDragSource(const SourceDetails &details) override;

    void itemDragMove(const SourceDetails &details) override;

    void itemDragExit(const SourceDetails &details) override;

    void itemDropped(const SourceDetails &details) override;

private:
    [[nodiscard]] juce::Rectangle<int> getSlotBounds(int slotIndex) const;

    [[nodiscard]] int getSlotAt(juce::Point<int> pos) const;

    [[nodiscard]] int getRowHeight(int row) const;

    void paintEmptySlot(juce::Graphics &g, int slotIndex) const;

    void paintArrows(juce::Graphics &g) const;

    ApplicationController &m_controller;
    UIRegistry &m_uiRegistry;

    std::vector<std::unique_ptr<PedalCard> > m_cards;
    int m_dragOverSlot{-1};

    static constexpr int k_cols = 4;
    static constexpr int k_rows = 2;
    static constexpr int k_totalSlots = k_cols * k_rows;
    static constexpr int k_margin = 14;
    static constexpr int k_cardGapX = 10;
    static constexpr int k_cardGapY = 20;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectChainPanel)
};
