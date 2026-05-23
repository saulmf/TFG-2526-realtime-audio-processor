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
 * (changes propagate to the audio thread automaticall)y.
 */
class PedalCard : public juce::Component,
                  public juce::SettableTooltipClient {
public:
    PedalCard(IAudioEffect &effect,
              const EffectUIDescriptor &descriptor,
              int index);

    void paint(juce::Graphics &g) override;

    void resized() override;

    void mouseDown(const juce::MouseEvent &e) override;

    void mouseUp(const juce::MouseEvent &e) override;

    void mouseDrag(const juce::MouseEvent &e) override;

    [[nodiscard]] int getEffectIndex() const noexcept { return m_index; }

    [[nodiscard]] int getPreferredHeight() const noexcept;

    std::function<void()> onRemoveRequested;

    static constexpr int k_headerH = 32;

private:
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
