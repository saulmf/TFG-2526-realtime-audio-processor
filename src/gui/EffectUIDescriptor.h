#pragma once

#include <map>

#include <juce_graphics/juce_graphics.h>

/**
 * EffectUIDescriptor - visual identity of a single effect type.
 *
 * Holds all GUI-layer metadata for one effect type.
 * Completely decoupled from the DSP implementation.
 *
 * Registered in UIRegistry keyed by the same TYPE_ID string used in EffectFactory,
 * so the two registries stay in sync without coupling.
 */
struct EffectUIDescriptor {
    juce::String displayName;
    juce::String category;
    juce::String labelText; // short label on the pedal body (e.g. "OD", "FUZZ")

    juce::Colour bodyColour;
    juce::Colour accentColour; // knob caps, LED active color, label text

    juce::String tooltip;

    // Per-parameter tooltip strings, keyed by APVTS paramID
    std::map<juce::String, juce::String> paramTooltips;
};
