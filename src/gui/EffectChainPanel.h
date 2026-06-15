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
    /** Creates the panel and builds PedalCards for the current chain state.
        @param controller Used to modify the chain in response to user actions (remove, move, enable).
        @param uiRegistry Visual descriptors used to style each PedalCard. */
    EffectChainPanel(ApplicationController &controller, UIRegistry &uiRegistry);

    /** Draws slot backgrounds, signal-flow arrows between filled slots, and the drag-over highlight. */
    void paint(juce::Graphics &g) override;

    /** Distributes PedalCards and empty-slot placeholders across the 2×4 grid. */
    void resized() override;

    /** Fires onEmptySlotClicked when the user clicks on an empty slot. */
    void mouseDown(const juce::MouseEvent &e) override;

    /** Rebuilds all PedalCards to reflect the current effect chain state. Call after any external chain change. */
    void refreshChain();

    /** Called when the user clicks an empty slot; the slot index is passed as argument. */
    std::function<void(int slotIndex)> onEmptySlotClicked;

    /** Fired with an error message when a chain modification is blocked (e.g. the session is running). */
    std::function<void(const juce::String &)> onErrorMessage;

    // DragAndDropTarget

    /** Returns true if the drag source carries an effect index (i.e. it comes from a PedalCard).
        @param details Drag source info; the description must be a var holding an integer effect index.
        @return true if this panel should accept the drag. */
    bool isInterestedInDragSource(const SourceDetails &details) override;

    /** Updates the drop-highlight slot as the dragged card moves over the panel.
        @param details Contains the current drag position used to determine the target slot. */
    void itemDragMove(const SourceDetails &details) override;

    /** Clears the drop-highlight when the drag leaves the panel.
        @param details Unused; present to satisfy the interface. */
    void itemDragExit(const SourceDetails &details) override;

    /** Calls moveEffect() to commit the reorder and rebuilds the PedalCards.
        @param details Contains the source effect index and drop position. */
    void itemDropped(const SourceDetails &details) override;

private:
    /** Returns the bounding rectangle for the given slot index in local coordinates.
        @param slotIndex Zero-based slot position (0–7, row-major order).
        @return Bounds in the panel's local coordinate system. */
    [[nodiscard]] juce::Rectangle<int> getSlotBounds(int slotIndex) const;

    /** Returns the slot index under the given local position.
        @param pos Point in local coordinates.
        @return Slot index, or -1 if the point is outside all slots. */
    [[nodiscard]] int getSlotAt(juce::Point<int> pos) const;

    /** Returns the pixel height allocated to the given row.
        @param row Row index (0 = top, 1 = bottom).
        @return Height in pixels, based on the tallest PedalCard in that row. */
    [[nodiscard]] int getRowHeight(int row) const;

    /** Draws a single empty-slot placeholder at the given slot index.
        @param g Graphics context to draw into.
        @param slotIndex Zero-based slot position. */
    void paintEmptySlot(juce::Graphics &g, int slotIndex) const;

    /** Draws the signal-flow arrows between adjacent filled slots.
        @param g Graphics context to draw into. */
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
