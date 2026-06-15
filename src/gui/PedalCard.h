#pragma once

#include <functional>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../effects/IAudioEffect.h"
#include "EffectUIDescriptor.h"

/**
 * PedalCard - GUI representation of a single effect in the chain.
 *
 * Interactions:
 *   - Click on LED button (left of header) -> toggle bypass
 *   - Click on x -> fires onRemoveRequested
 *   - Drag from header title or body -> initiates DragAndDropContainer drag
 *
 * Knobs are auto-generated from the effect's APVTS parameter list and bound via SliderAttachment
 * (changes propagate to the audio thread automatically).
 */
class PedalCard : public juce::Component,
                  public juce::SettableTooltipClient {
public:
    /**
     * Creates the card for the given effect, generates knobs from its APVTS parameters,
     * and applies visual styling from the descriptor.
     * @param effect The effect this card controls; must outlive the card.
     * @param descriptor Visual metadata (colours, label, tooltips) for this effect type.
     * @param index Position of this effect in the chain; used as the drag-and-drop payload.
     */
    PedalCard(IAudioEffect &effect,
              const EffectUIDescriptor &descriptor,
              int index);

    /** Renders the card body, header divider, LED bypass indicator, and effect name. */
    void paint(juce::Graphics &g) override;

    /** Places the remove button in the top-right corner and arranges the knob grid below the header. */
    void resized() override;

    /** Marks a toggle as pending if the click lands on the header area (outside the remove button). */
    void mouseDown(const juce::MouseEvent &e) override;

    /** Commits the pending bypass toggle and repaints the card. */
    void mouseUp(const juce::MouseEvent &e) override;

    /**
     * Cancels any pending toggle and starts a drag-and-drop operation after 8 pixels of movement.
     * The drag payload is the effect's chain index, used by EffectChainPanel to perform the reorder.
     */
    void mouseDrag(const juce::MouseEvent &e) override;

    /** Returns the chain index this card was constructed with. */
    [[nodiscard]] int getEffectIndex() const noexcept { return m_index; }

    /** Returns the preferred card height in pixels, computed from the number of knob rows. */
    [[nodiscard]] int getPreferredHeight() const noexcept;

    /** Fired when the user clicks the remove (x) button. */
    std::function<void()> onRemoveRequested;

    static constexpr int k_headerH = 32;

private:
    /** Iterates the effect's APVTS parameters and creates one ParameterKnob plus a label per parameter. */
    void buildKnobs();

    IAudioEffect &m_effect;
    const EffectUIDescriptor &m_descriptor;
    int m_index;

    bool m_pendingToggle{false};
    juce::TextButton m_removeButton{"x"};

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::vector<std::unique_ptr<juce::Slider> > m_knobs;
    std::vector<std::unique_ptr<juce::Label> > m_knobLabels;
    std::vector<std::unique_ptr<SliderAttachment> > m_attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PedalCard)
};
