#include "EffectChainPanel.h"

EffectChainPanel::EffectChainPanel(ApplicationController &controller, UIRegistry &uiRegistry)
    : m_controller(controller), m_uiRegistry(uiRegistry) {
    refreshChain();
}

// Layout helpers

int EffectChainPanel::getRowHeight(int row) const {
    constexpr int k_minH = 120;

    int pref[k_rows]{};
    for (int r = 0; r < k_rows; ++r) {
        pref[r] = k_minH;
        for (int c = 0; c < k_cols; ++c) {
            const int si = r * k_cols + c;
            if (si < static_cast<int>(m_cards.size()))
                pref[r] = juce::jmax(pref[r], m_cards[si]->getPreferredHeight());
        }
    }

    const int totalPref = pref[0] + pref[1];
    const int available = getHeight() - 2 * k_margin - (k_rows - 1) * k_cardGapY;

    // Fill available height proportionally (never leave empty space below cards).
    return juce::roundToInt(static_cast<float>(pref[row]) / static_cast<float>(totalPref) * static_cast<float>(available));
}

juce::Rectangle<int> EffectChainPanel::getSlotBounds(int slotIndex) const {
    const int col = slotIndex % k_cols;
    const int row = slotIndex / k_cols;

    const int totalW = getWidth() - 2 * k_margin - (k_cols - 1) * k_cardGapX;
    const int cardW = totalW / k_cols;

    int yOffset = k_margin;
    for (int r = 0; r < row; ++r)
        yOffset += getRowHeight(r) + k_cardGapY;

    const int x = k_margin + col * (cardW + k_cardGapX);
    return {x, yOffset, cardW, getRowHeight(row)};
}

int EffectChainPanel::getSlotAt(juce::Point<int> pos) const {
    for (int i = 0; i < k_totalSlots; ++i)
        if (getSlotBounds(i).contains(pos))
            return i;
    return -1;
}

// Chain management

void EffectChainPanel::refreshChain() {

    for (auto &card: m_cards)
        removeChildComponent(card.get());
    m_cards.clear();

    const int numEffects = m_controller.getNumEffects();

    for (int i = 0; i < numEffects; ++i) {
        IAudioEffect *fx = m_controller.getEffect(i);
        if (fx == nullptr) continue;

        const EffectUIDescriptor *desc = m_uiRegistry.getDescriptor(fx->getTypeId());
        if (desc == nullptr) continue;

        auto card = std::make_unique<PedalCard>(*fx, *desc, i);

        const int capturedIndex = i;
        card->onRemoveRequested = [this, capturedIndex] {
            m_controller.removeEffect(capturedIndex);
            refreshChain();
        };

        addAndMakeVisible(*card);
        m_cards.push_back(std::move(card));
    }

    resized();
    repaint();
}

void EffectChainPanel::resized() {
    for (int i = 0; i < static_cast<int>(m_cards.size()); ++i)
        m_cards[i]->setBounds(getSlotBounds(i));
}

void EffectChainPanel::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xFF1C1C2E));

    const int numEffects = m_controller.getNumEffects();
    for (int i = numEffects; i < k_totalSlots; ++i)
        paintEmptySlot(g, i);

    // Drag-over highlight
    if (m_dragOverSlot >= 0 && m_dragOverSlot < k_totalSlots) {
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.fillRoundedRectangle(getSlotBounds(m_dragOverSlot).toFloat(), 8.0f);
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawRoundedRectangle(getSlotBounds(m_dragOverSlot).reduced(1).toFloat(), 8.0f, 1.5f);
    }

    paintArrows(g);
}

void EffectChainPanel::paintEmptySlot(juce::Graphics &g, int slotIndex) const {
    const auto bounds = getSlotBounds(slotIndex).toFloat();

    juce::Path border;
    border.addRoundedRectangle(bounds.reduced(1.0f), 8.0f);

    const float dashes[]{6.0f, 4.0f};
    g.setColour(juce::Colours::grey.withAlpha(0.25f));
    g.strokePath(border,
                 juce::PathStrokeType(1.0f),
                 juce::AffineTransform());

    g.setColour(juce::Colours::grey.withAlpha(0.2f));
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("+", bounds, juce::Justification::centred);

    juce::ignoreUnused(dashes);
}

void EffectChainPanel::paintArrows(juce::Graphics &g) const {
    g.setColour(juce::Colours::grey.withAlpha(0.55f));

    const int numFilled = m_controller.getNumEffects();

    // Horizontal arrows between adjacent filled slots on the same row
    for (int i = 0; i < numFilled - 1; ++i) {
        const int nextSlot = i + 1;

        // Skip the line jump (slot 3 -> slot 4 handled separately)
        if (i % k_cols == k_cols - 1)
            continue;

        const auto from = getSlotBounds(i);
        const auto to = getSlotBounds(nextSlot);

        const float y = static_cast<float>(from.getCentreY());
        const float x1 = static_cast<float>(from.getRight()) + 1.0f;
        const float x2 = static_cast<float>(to.getX()) - 1.0f;

        g.drawArrow({x1, y, x2, y}, 1.5f, 7.0f, 5.0f);
    }

    // Row jump: The horizontal segment is in free space so it is always visible, and the height
    // of both the descent and the arrowhead scale automatically with the dynamic row heights.
    if (numFilled > k_cols) {
        const auto lastRow0 = getSlotBounds(k_cols - 1); // slot 3
        const auto firstRow1 = getSlotBounds(k_cols); // slot 4

        const float row0CY = static_cast<float>(lastRow0.getCentreY());
        const float gapMidY = (static_cast<float>(lastRow0.getBottom())
                               + static_cast<float>(firstRow1.getY())) * 0.5f;
        const float rightX = static_cast<float>(lastRow0.getRight()) + 6.0f;
        const float slot4CX = static_cast<float>(firstRow1.getCentreX());
        const float slot4TY = static_cast<float>(firstRow1.getY());

        juce::Path path;
        path.startNewSubPath(static_cast<float>(lastRow0.getRight()) + 1.0f, row0CY);
        path.lineTo(rightX, row0CY); // small right extension past slot 3
        path.lineTo(rightX, gapMidY); // descend to the middle of the inter-row gap
        path.lineTo(slot4CX, gapMidY); // cross the gap horizontally to above slot 4

        g.strokePath(path, juce::PathStrokeType(1.5f));

        // Downward arrowhead dropping from the gap into slot 4
        g.drawArrow({slot4CX, gapMidY, slot4CX, slot4TY - 1.0f}, 1.5f, 7.0f, 5.0f);
    }
}


// Mouse interaction

void EffectChainPanel::mouseDown(const juce::MouseEvent &e) {
    const int numEffects = m_controller.getNumEffects();
    const int slot = getSlotAt(e.getPosition());

    if (slot >= numEffects && slot < k_totalSlots) {
        if (onEmptySlotClicked)
            onEmptySlotClicked(slot);
    }
}


// Drag and drop

bool EffectChainPanel::isInterestedInDragSource(const SourceDetails &details) {
    return details.description.isInt();
}

void EffectChainPanel::itemDragMove(const SourceDetails &details) {
    const int slot = getSlotAt(details.localPosition);
    if (slot != m_dragOverSlot) {
        m_dragOverSlot = slot;
        repaint();
    }
}

void EffectChainPanel::itemDragExit(const SourceDetails &) {
    m_dragOverSlot = -1;
    repaint();
}

void EffectChainPanel::itemDropped(const SourceDetails &details) {
    const int fromIndex = details.description;
    const int toSlot = getSlotAt(details.localPosition);

    m_dragOverSlot = -1;

    if (toSlot >= 0
        && toSlot < m_controller.getNumEffects()
        && fromIndex != toSlot) {
        m_controller.moveEffect(fromIndex, toSlot);
    }

    refreshChain();
}
