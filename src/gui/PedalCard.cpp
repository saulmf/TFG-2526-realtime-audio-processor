#include "PedalCard.h"

class ParameterKnob : public juce::Slider {
public:
    ParameterKnob() : Slider(RotaryVerticalDrag, TextBoxBelow) {
        setTextBoxStyle(TextBoxBelow, false, 50, k_textBoxH);

        setColour(textBoxTextColourId, juce::Colours::white.withAlpha(0.75f));
        setColour(textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(textBoxHighlightColourId, juce::Colours::white.withAlpha(0.2f));
    }

    void setParameter(juce::RangedAudioParameter *param) noexcept {
        m_param = param;
    }

    static constexpr int k_textBoxH = 14;

    void mouseDown(const juce::MouseEvent &e) override {
        if (e.mods.isRightButtonDown()) {
            showContextMenu();
            return;
        }
        Slider::mouseDown(e);
    }

private:
    juce::RangedAudioParameter *m_param{nullptr};

    void showContextMenu() {
        juce::PopupMenu menu;
        menu.addItem(1, TRANS("Set value..."));
        menu.addItem(2, TRANS("Reset to default"));
        menu.addSeparator();
        menu.addItem(3, TRANS("Copy value"));

        menu.showMenuAsync(
            juce::PopupMenu::Options().withTargetComponent(this),
            [this](int result) {
                switch (result) {
                    case 1: showTextBox();
                        break;
                    case 2: resetToDefault();
                        break;
                    case 3: copyValue();
                        break;
                    default: break;
                }
            });
    }

    void resetToDefault() {
        if (m_param != nullptr)
            setValue(m_param->convertFrom0to1(m_param->getDefaultValue()),
                     juce::sendNotificationAsync);
    }

    void copyValue() {
        juce::SystemClipboard::copyTextToClipboard(getTextFromValue(getValue()));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterKnob)
};

PedalCard::PedalCard(IAudioEffect &effect,
                     const EffectUIDescriptor &descriptor,
                     int index)
    : m_effect(effect),
      m_descriptor(descriptor),
      m_index(index) {
    m_removeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    m_removeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.7f));
    m_removeButton.onClick = [this] { if (onRemoveRequested) onRemoveRequested(); };
    addAndMakeVisible(m_removeButton);

    setTooltip(juce::translate(descriptor.tooltip));

    buildKnobs();
}

void PedalCard::buildKnobs() {
    auto &apvts = m_effect.getAPVTS();
    auto &params = apvts.processor.getParameters();

    for (auto *p: params) {
        auto *rp = dynamic_cast<juce::RangedAudioParameter *>(p);
        if (rp == nullptr) continue;

        // Knob (ParameterKnob stored as unique_ptr<Slider> via implicit upcast)
        auto knob = std::make_unique<ParameterKnob>();
        knob->setParameter(rp);
        knob->setColour(juce::Slider::rotarySliderFillColourId, m_descriptor.accentColour);
        knob->setColour(juce::Slider::rotarySliderOutlineColourId, m_descriptor.bodyColour.darker(0.5f));

        const auto tipIt = m_descriptor.paramTooltips.find(rp->paramID);
        if (tipIt != m_descriptor.paramTooltips.end())
            knob->setTooltip(juce::translate(tipIt->second));

        // Parameter name label (below the knob's text box)
        auto label = std::make_unique<juce::Label>("", rp->getName(16));
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::FontOptions(9.0f));
        label->setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.55f));

        auto attachment = std::make_unique<SliderAttachment>(apvts, rp->paramID, *knob);

        addAndMakeVisible(*knob);
        addAndMakeVisible(*label);

        m_knobs.push_back(std::move(knob));
        m_knobLabels.push_back(std::move(label));
        m_attachments.push_back(std::move(attachment));
    }
}

void PedalCard::paint(juce::Graphics &g) {
    const bool enabled = m_effect.isEnabled();
    const auto body = enabled ? m_descriptor.bodyColour : m_descriptor.bodyColour.darker(0.5f).withAlpha(0.6f);

    g.setColour(body);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    g.setColour(enabled ? m_descriptor.accentColour.withAlpha(0.6f) : juce::Colours::grey.withAlpha(0.25f));
    g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 8.0f, 1.5f);

    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.drawHorizontalLine(k_headerH, 6.0f, static_cast<float>(getWidth()) - 6.0f);

    constexpr int ledSize = 10;
    // Green if enabled, Red if not
    const auto ledColour = enabled ? juce::Colour(0xFF22C55E) : juce::Colour(0xFFEF4444);
    const auto led = juce::Rectangle<float>(8.0f,
                                            (k_headerH - ledSize) / 2.0f,
                                            ledSize, ledSize);
    g.setColour(ledColour.withAlpha(0.25f));
    g.fillEllipse(led);
    g.setColour(ledColour);
    g.fillEllipse(led.reduced(2.0f));
    g.setColour(juce::Colours::white.withAlpha(0.45f));
    g.fillEllipse(led.getX() + led.getWidth() * 0.18f,
                  led.getY() + led.getHeight() * 0.14f,
                  led.getWidth() * 0.38f,
                  led.getHeight() * 0.32f);

    g.setColour(enabled ? juce::Colours::white : juce::Colours::grey);
    g.setFont(juce::FontOptions(11.5f).withStyle("Bold"));
    g.drawText(m_descriptor.displayName,
               22, 0, getWidth() - 22 - k_headerH, k_headerH,
               juce::Justification::centredLeft);
}

void PedalCard::resized() {
    m_removeButton.setBounds(getWidth() - k_headerH, 0, k_headerH, k_headerH);

    const int n = static_cast<int>(m_knobs.size());
    if (n == 0) return;

    int cols;
    if (n <= 3) cols = n;
    else if (n == 4) cols = 2;
    else if (n <= 8) cols = 4;
    else cols = 3;
    const int rows = (n + cols - 1) / cols;

    constexpr int labelH = 12;
    constexpr int textBoxH = ParameterKnob::k_textBoxH;
    constexpr int padX = 6;
    constexpr int padY = 4;

    const int areaX = padX;
    const int areaY = k_headerH + padY;
    const int areaW = getWidth() - 2 * padX;
    const int areaH = getHeight() - areaY - padY;

    const int cellW = areaW / cols;
    const int cellH = areaH / rows;
    const int knobSz = juce::jmin(cellW - 8, cellH - textBoxH - labelH - 4);

    for (int i = 0; i < n; ++i) {
        const int col = i % cols;
        const int row = i / cols;
        const int cellX = areaX + col * cellW;
        const int cellY = areaY + row * cellH;

        const int knobX = cellX + (cellW - knobSz) / 2;
        const int knobY = cellY + (cellH - knobSz - textBoxH - labelH) / 2;

        m_knobs[i]->setBounds(knobX, knobY, knobSz, knobSz + textBoxH);
        m_knobLabels[i]->setBounds(cellX, knobY + knobSz + textBoxH, cellW, labelH);
    }
}

int PedalCard::getPreferredHeight() const noexcept {
    const int n = static_cast<int>(m_knobs.size());
    if (n == 0) return k_headerH + 40;

    int cols;
    if (n <= 3) cols = n;
    else if (n == 4) cols = 2;
    else if (n <= 8) cols = 4;
    else cols = 3;
    const int rows = (n + cols - 1) / cols;

    constexpr int padY = 4;
    constexpr int knobSz = 56;
    constexpr int labelH = 12;

    return k_headerH + padY + rows * (knobSz + ParameterKnob::k_textBoxH + labelH + padY);
}

void PedalCard::mouseDown(const juce::MouseEvent &e) {
    // Toggle only for clicks on the header area (outside the remove button) for enabling/disabling the effect
    if (e.y <= k_headerH && e.x < getWidth() - k_headerH)
        m_pendingToggle = true;
}

void PedalCard::mouseUp(const juce::MouseEvent &e) {
    juce::ignoreUnused(e);
    if (m_pendingToggle) {
        m_pendingToggle = false;
        m_effect.setEnabled(!m_effect.isEnabled());
        repaint();
    }
}

void PedalCard::mouseDrag(const juce::MouseEvent &e) {
    if (e.getDistanceFromDragStart() > 8) {
        m_pendingToggle = false; // dragging the effect - cancel the pending toggle
        if (auto *dc = juce::DragAndDropContainer::findParentDragContainerFor(this))
            dc->startDragging(m_index, this, juce::ScaledImage(), true);
    }
}
