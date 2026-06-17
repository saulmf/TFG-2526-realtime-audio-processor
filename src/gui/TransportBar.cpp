#include "TransportBar.h"

class TransportBar::MnemonicLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    void drawLabel(juce::Graphics &g, juce::Label &label) override {
        LookAndFeel_V4::drawLabel(g, label);

        const auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());
        drawMnemonicUnderline(g, label.getText(), getLabelFont(label), textArea,
                               label.getJustificationType(),
                               label.findColour(juce::Label::textColourId)
                                   .withMultipliedAlpha(label.isEnabled() ? 1.0f : 0.5f));
    }

    void drawButtonText(juce::Graphics &g, juce::TextButton &button,
                         bool isMouseOverButton, bool isButtonDown) override {
        LookAndFeel_V4::drawButtonText(g, button, isMouseOverButton, isButtonDown);

        drawMnemonicUnderline(g, button.getButtonText(), getTextButtonFont(button, button.getHeight()),
                               button.getLocalBounds(), juce::Justification::centred,
                               button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
                                                                          : juce::TextButton::textColourOffId)
                                   .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));
    }

private:
    static void drawMnemonicUnderline(juce::Graphics &g, const juce::String &text, const juce::Font &font,
                                       juce::Rectangle<int> area, juce::Justification justification,
                                       juce::Colour colour) {
        if (text.isEmpty() || area.getWidth() <= 0)
            return;

        const float fullTextWidth = juce::GlyphArrangement::getStringWidth(font, text);
        if (fullTextWidth <= 0.0f)
            return;

        const float scale = juce::jmin(1.0f, static_cast<float>(area.getWidth()) / fullTextWidth);
        const float textWidth = fullTextWidth * scale;

        float startX;
        if (justification.testFlags(juce::Justification::horizontallyCentred))
            startX = static_cast<float>(area.getX()) + (static_cast<float>(area.getWidth()) - textWidth) * 0.5f;
        else if (justification.testFlags(juce::Justification::right))
            startX = static_cast<float>(area.getRight()) - textWidth;
        else
            startX = static_cast<float>(area.getX());

        const float charWidth = juce::GlyphArrangement::getStringWidth(font, text.substring(0, 1)) * scale;
        const float textTop = static_cast<float>(area.getY()) + (static_cast<float>(area.getHeight()) - font.getHeight()) * 0.5f;
        const float underlineY = textTop + font.getAscent() + 2.0f;

        g.setColour(colour);
        g.drawLine(startX, underlineY, startX + charWidth, underlineY, 1.0f);
    }
};

TransportBar::TransportBar(ApplicationController &controller)
    : m_controller(controller) {
    m_mnemonicLookAndFeel = std::make_unique<MnemonicLookAndFeel>();
    m_inputLabel.setLookAndFeel(m_mnemonicLookAndFeel.get());
    m_outputLabel.setLookAndFeel(m_mnemonicLookAndFeel.get());
    m_startStopButton.setLookAndFeel(m_mnemonicLookAndFeel.get());

    // Input device combo box
    addAndMakeVisible(m_inputLabel);
    addAndMakeVisible(m_inputBox);
    m_inputBox.onChange = [this] {
        const auto name = m_inputBox.getText();
        if (name.isNotEmpty()) {
            m_controller.setInputDevice(name);
            DBG("[TransportBar] Input device set to: " + name);
        }
    };

    // Output device combo box
    addAndMakeVisible(m_outputLabel);
    addAndMakeVisible(m_outputBox);
    m_outputBox.onChange = [this] {
        const auto name = m_outputBox.getText();
        if (name.isNotEmpty()) {
            m_controller.setOutputDevice(name);
            DBG("[TransportBar] Output device set to: " + name);
        }
    };

    // Master volume slider
    m_volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 20);
    m_volumeSlider.setTextValueSuffix("%");
    m_volumeSlider.setRange(0.0, 100.0, 1.0);
    m_volumeSlider.setValue(m_controller.getMasterVolume() * 100.0,
                            juce::dontSendNotification);
    m_volumeSlider.onValueChange = [this] {
        m_controller.setMasterVolume(
            static_cast<float>(m_volumeSlider.getValue()) / 100.0f);
    };
    addAndMakeVisible(m_volLabel);
    addAndMakeVisible(m_volumeSlider);

    // Start / Stop button
    addAndMakeVisible(m_startStopButton);
    m_startStopButton.onClick = [this] {
        if (!m_controller.isRunning()) {
            const juce::String error = m_controller.start();

            if (error.isEmpty()) {
                DBG("[TransportBar] Session started.");
                m_startStopButton.setButtonText(TRANS("Stop session"));
                m_inputLevelMeter.setActive(true);
                m_outputLevelMeter.setActive(true);
            } else {
                DBG("[TransportBar] Start failed: " + error);
                if (onErrorMessage)
                    onErrorMessage(TRANS(error.toRawUTF8()));
            }
        } else {
            m_controller.stop();
            DBG("[TransportBar] Session stopped.");
            m_startStopButton.setButtonText(TRANS("Start session"));
            m_inputLevelMeter.setActive(false);
            m_outputLevelMeter.setActive(false);
        }

        refreshStatus();

        if (onSessionStateChanged)
            onSessionStateChanged();
    };

    // Status label
    m_statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(m_statusLabel);

    //Input / Output level meters
    auto setupMeterLabel = [](juce::Label &label) {
        label.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        label.setColour(juce::Label::textColourId, juce::Colours::grey);
        label.setJustificationType(juce::Justification::centredRight);
    };
    setupMeterLabel(m_inputMeterLabel);
    setupMeterLabel(m_outputMeterLabel);
    addAndMakeVisible(m_inputMeterLabel);
    addAndMakeVisible(m_outputMeterLabel);

    m_inputLevelMeter.setLevelSource([&controller]() { return controller.getInputLevel(); });
    m_outputLevelMeter.setLevelSource([&controller]() { return controller.getOutputLevel(); });
    addAndMakeVisible(m_inputLevelMeter);
    addAndMakeVisible(m_outputLevelMeter);

    m_inputBox.setTooltip(TRANS("Select the audio input device (e.g. your USB audio interface)."));
    m_outputBox.setTooltip(TRANS("Select the audio output device for the processed signal."));
    m_startStopButton.setTooltip(TRANS("Start or stop the real-time audio processing session."));
    m_volumeSlider.setTooltip(TRANS("Adjusts the overall output level sent to the audio device."));
    m_inputLevelMeter.setTooltip(TRANS("Shows the input signal level from the selected audio device."));
    m_outputLevelMeter.setTooltip(TRANS("Shows the output signal level after all effects and master volume."));

    populateDeviceLists();
    refreshStatus();
}

TransportBar::~TransportBar() {
    m_inputLabel.setLookAndFeel(nullptr);
    m_outputLabel.setLookAndFeel(nullptr);
    m_startStopButton.setLookAndFeel(nullptr);
}

void TransportBar::focusInputDevice() {
    m_inputBox.showPopup();
}

void TransportBar::focusOutputDevice() {
    m_outputBox.showPopup();
}

void TransportBar::triggerStartStop() {
    m_startStopButton.triggerClick();
}

void TransportBar::populateDeviceLists() {
    m_inputBox.clear(juce::dontSendNotification);
    const auto inputs = m_controller.getAvailableInputDevices();
    for (int i = 0; i < inputs.size(); ++i)
        m_inputBox.addItem(inputs[i], i + 1);

    m_outputBox.clear(juce::dontSendNotification);
    const auto outputs = m_controller.getAvailableOutputDevices();
    for (int i = 0; i < outputs.size(); ++i)
        m_outputBox.addItem(outputs[i], i + 1);

    DBG("[TransportBar] Found " + juce::String(inputs.size()) + " input device(s), "
        + juce::String(outputs.size()) + " output device(s).");
}

void TransportBar::refreshStatus() {
    const bool running = m_controller.isRunning();

    m_statusLabel.setText(running ? TRANS("RUNNING") : TRANS("Stopped"),
                          juce::dontSendNotification);

    repaint(); // redraw the LED indicator
}

void TransportBar::paint(juce::Graphics &g) {
    g.fillAll(getLookAndFeel()
        .findColour(juce::ResizableWindow::backgroundColourId)
        .darker(0.15f));

    g.setColour(juce::Colours::grey.darker(0.3f));
    g.drawLine(0.0f, static_cast<float>(getHeight()) - 1.0f,
               static_cast<float>(getWidth()), static_cast<float>(getHeight()) - 1.0f);

    // Session LED indicator
    if (!m_ledBounds.isEmpty()) {
        const bool running = m_controller.isRunning();
        // Green if running, Red if not
        const auto colour = running ? juce::Colour(0xFF22C55E) : juce::Colour(0xFFEF4444);
        const auto led = m_ledBounds.toFloat();
        g.setColour(colour.withAlpha(0.25f));
        g.fillEllipse(led);
        g.setColour(colour);
        g.fillEllipse(led.reduced(2.0f));
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.fillEllipse(led.getX() + led.getWidth() * 0.18f,
                      led.getY() + led.getHeight() * 0.14f,
                      led.getWidth() * 0.38f,
                      led.getHeight() * 0.32f);
    }
}

void TransportBar::resized() {
    constexpr int margin = 8;
    constexpr int rowH = 26;
    constexpr int meterH = 16;
    constexpr int meterGap = 4;
    constexpr int gap = 6;
    constexpr int labelW = 55;
    constexpr int comboW = 250;
    constexpr int buttonW = 110;
    constexpr int mLabelW = 30;
    constexpr int totalH = rowH + gap + meterH + meterGap + meterH;
    const int startY = (getHeight() - totalH) / 2;
    constexpr int statusW = 80;
    constexpr int ledSize = 12;
    const int statusX = getWidth() - margin - statusW;
    const int ledX = statusX - gap - ledSize;
    const int buttonX = ledX - gap - buttonW;

    m_startStopButton.setBounds(buttonX, startY, buttonW, rowH);
    m_statusLabel.setBounds(statusX, startY, statusW, rowH);

    m_ledBounds = {ledX, startY + (rowH - ledSize) / 2, ledSize, ledSize};

    m_inputLabel.setBounds(margin, startY, labelW, rowH);
    m_inputBox.setBounds(margin + labelW, startY, comboW, rowH);

    m_outputLabel.setBounds(margin + labelW + comboW + gap, startY, labelW, rowH);
    m_outputBox.setBounds(margin + labelW + comboW + gap + labelW, startY, comboW, rowH);

    constexpr int volLabelW = 38;
    const int volAreaX = margin + labelW + comboW + gap + labelW + comboW + gap;
    const int volAreaW = buttonX - gap - volAreaX;
    m_volLabel.setBounds(volAreaX, startY, volLabelW, rowH);
    m_volumeSlider.setBounds(volAreaX + volLabelW, startY, volAreaW - volLabelW, rowH);

    const int meter1Y = startY + rowH + gap;
    const int meter2Y = meter1Y + meterH + meterGap;
    const int meterW = getWidth() - margin * 2 - mLabelW - gap;

    m_inputMeterLabel.setBounds(margin, meter1Y, mLabelW, meterH);
    m_inputLevelMeter.setBounds(margin + mLabelW + gap, meter1Y, meterW, meterH);

    m_outputMeterLabel.setBounds(margin, meter2Y, mLabelW, meterH);
    m_outputLevelMeter.setBounds(margin + mLabelW + gap, meter2Y, meterW, meterH);
}
