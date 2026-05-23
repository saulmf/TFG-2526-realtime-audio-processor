#include "LevelMeter.h"

LevelMeter::LevelMeter() {
    setInterceptsMouseClicks(false, false);
}

void LevelMeter::setLevelSource(std::function<float()> source) {
    m_levelSource = std::move(source);
}

void LevelMeter::setActive(bool shouldBeActive) {
    m_active = shouldBeActive;

    if (shouldBeActive) {
        startTimerHz(30);
    } else {
        stopTimer();
        m_displayLevel = 0.0f;
        m_peakHold = 0.0f;
        m_peakHoldFrames = 0;
        m_clipHoldFrames = 0;
        repaint();
    }
}

void LevelMeter::timerCallback() {
    if (!m_levelSource)
        return;

    const float newLevel = m_levelSource();

    // Bar level with fast attack, slow decay
    if (newLevel >= m_displayLevel)
        m_displayLevel = newLevel;
    else
        m_displayLevel *= k_decayFactor;

    // Peak hold: snap up instantly, hold then decay
    if (newLevel >= m_peakHold) {
        m_peakHold = newLevel;
        m_peakHoldFrames = k_peakHoldFrames;
    } else if (m_peakHoldFrames > 0) {
        --m_peakHoldFrames;
    } else {
        m_peakHold *= k_decayFactor;
    }

    // Clip indicator
    if (newLevel >= 1.0f)
        m_clipHoldFrames = k_clipHoldFrames;
    else if (m_clipHoldFrames > 0)
        --m_clipHoldFrames;

    repaint();
}


void LevelMeter::paint(juce::Graphics &g) {
    const auto bounds = getLocalBounds();
    constexpr int clipW = 10;
    constexpr int clipGap = 3;
    const int barW = bounds.getWidth() - clipW - clipGap;
    const int barH = bounds.getHeight();

    g.setColour(juce::Colour(0xFF0A0A14));
    g.fillRoundedRectangle(bounds.toFloat(), 3.0f);

    if (!m_active) {
        g.setColour(juce::Colours::grey.withAlpha(0.35f));
        g.setFont(juce::Font(10.0f));
        g.drawText("- - -", juce::Rectangle<int>(0, 0, barW, barH),
                   juce::Justification::centred);
    } else {
        const float db = (m_displayLevel > 0.00001f)
                             ? juce::jmax(k_minDb, 20.0f * std::log10(m_displayLevel))
                             : k_minDb;
        const float normalised = (db - k_minDb) / (-k_minDb);
        const int fillW = juce::roundToInt(normalised * static_cast<float>(barW));

        const int greenEnd = juce::roundToInt(0.80f * static_cast<float>(barW));
        const int yellowEnd = juce::roundToInt(0.95f * static_cast<float>(barW));

        if (fillW > 0) {
            g.setColour(juce::Colour(0xFF22C55E)); // green
            g.fillRect(0, 0, juce::jmin(fillW, greenEnd), barH);
        }
        if (fillW > greenEnd) {
            g.setColour(juce::Colour(0xFFFACC15)); // yellow
            g.fillRect(greenEnd, 0, juce::jmin(fillW, yellowEnd) - greenEnd, barH);
        }
        if (fillW > yellowEnd) {
            g.setColour(juce::Colour(0xFFEF4444)); // red
            g.fillRect(yellowEnd, 0, fillW - yellowEnd, barH);
        }

        if (m_peakHold > 0.00001f) {
            const float peakDb = juce::jmax(k_minDb, 20.0f * std::log10(m_peakHold));
            const float peakNorm = (peakDb - k_minDb) / (-k_minDb);
            const int peakX = juce::roundToInt(peakNorm * static_cast<float>(barW)) - 1;

            g.setColour(juce::Colours::white.withAlpha(0.75f));
            g.fillRect(juce::jmax(0, peakX), 0, 2, barH);
        }

        g.setColour(juce::Colours::white.withAlpha(0.15f));
        for (float tickDb: {-48.0f, -24.0f, -12.0f, -6.0f}) {
            const float n = (tickDb - k_minDb) / (-k_minDb);
            const int x = juce::roundToInt(n * static_cast<float>(barW));
            g.drawVerticalLine(x, 0.0f, static_cast<float>(barH));
        }
    }

    // Clip indicator LED at the end of the bar
    const int clipX = barW + clipGap;
    const bool clipping = m_clipHoldFrames > 0;

    // If there is signal clipping: Red. If not: Black
    g.setColour(clipping ? juce::Colour(0xFFEF4444) : juce::Colour(0xFF1A1A2E));
    g.fillRoundedRectangle(static_cast<float>(clipX), 0.0f,
                           static_cast<float>(clipW), static_cast<float>(barH), 2.0f);
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawRoundedRectangle(static_cast<float>(clipX) + 0.5f, 0.5f,
                           static_cast<float>(clipW) - 1.0f,
                           static_cast<float>(barH) - 1.0f, 2.0f, 0.5f);
}
