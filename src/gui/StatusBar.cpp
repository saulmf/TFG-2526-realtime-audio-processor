#include "StatusBar.h"

StatusBar::StatusBar(ApplicationController &controller)
    : m_controller(controller) {
    refresh();
}

void StatusBar::refresh() {
    const double sr = m_controller.getCurrentSampleRate();
    const int buf = m_controller.getCurrentBufferSize();

    m_text = juce::String(static_cast<int>(sr)) + " Hz  |  " + juce::String(buf) + " " + TRANS("samples");
    repaint();
}

void StatusBar::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xFF0D0D1A));

    g.setColour(juce::Colours::grey.darker(0.4f));
    g.drawLine(0.0f, 0.0f, static_cast<float>(getWidth()), 0.0f, 1.0f);

    g.setColour(juce::Colours::grey.withAlpha(0.6f));
    g.setFont(juce::FontOptions(10.5f));
    g.drawText(m_text, getLocalBounds().reduced(8, 0),
               juce::Justification::centredRight);
}
