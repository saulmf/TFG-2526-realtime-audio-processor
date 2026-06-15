#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../ApplicationController.h"
#include "UIRegistry.h"

/**
 * EffectBrowserPanel - expandable and scrollable catalogue of available effects.
 *
 * Always visible as a header bar at the bottom of the window.
 * Clicking the header (or the arrow button) expands the panel upward to k_expandedHeight, overlapping whatever sits above it.
 *
 * The expanded area contains a juce::Viewport that scrolls a custom EffectCatalogueComponent.
 * Effects grouped by category, rendered as colored pedal thumbnails.
 * Clicking a thumbnail calls ApplicationController::addEffect() with the corresponding type ID.
 *
 * @note The expand/collapse transition is animated by the parent via onExpandedStateChanged: the parent animates
 * the panel's bounds so that this component stays unaware of its own position in the hierarchy.
 */
class EffectBrowserPanel : public juce::Component {
public:
    /** Creates the panel, builds the EffectCatalogueComponent, and wires up the toggle button.
        @param controller Used to call addEffect() when the user clicks an effect thumbnail.
        @param uiRegistry Visual descriptors used to render each effect thumbnail in the catalogue. */
    EffectBrowserPanel(ApplicationController &controller, UIRegistry &uiRegistry);

    ~EffectBrowserPanel() override;

    /** Positions the toggle button at the top and the scrollable catalogue viewport below it. */
    void resized() override;

    /** Draws the panel header bar background. */
    void paint(juce::Graphics &g) override;

    [[nodiscard]] bool isExpanded() const noexcept { return m_isExpanded; }

    /** Expands the browser if it is currently collapsed. Called when the user clicks an empty slot in the chain panel. */
    void expandForSlot();

    // Called by the parent to animate the bounds change on toggle.
    std::function<void(bool expanded)> onExpandedStateChanged;

    // Called after an effect thumbnail is clicked and addEffect() succeeds.
    std::function<void()> onEffectAdded;

    // Fired with an error message when an action is blocked (e.g. session is running).
    std::function<void(const juce::String &)> onErrorMessage;

    static constexpr int k_collapsedHeight = 40;

private:
    /** Flips the expanded state and fires onExpandedStateChanged so the parent can animate the bounds. */
    void toggleExpanded();

    ApplicationController &m_controller;
    UIRegistry &m_uiRegistry;

    bool m_isExpanded{false};

    juce::TextButton m_toggleButton{TRANS("^ Browse Effects")};
    juce::Viewport m_viewport;

    // Forward-declared inner class - defined in the .cpp
    class EffectCatalogueComponent;
    std::unique_ptr<EffectCatalogueComponent> m_catalogue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectBrowserPanel)
};
