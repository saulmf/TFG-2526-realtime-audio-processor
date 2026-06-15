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
    juce::String displayName;  ///< Full name shown in the pedal card header and browser (e.g. "Overdrive").
    juce::String category;     ///< Group name used to organise effects in the browser (e.g. "Nonlinear").
    juce::String labelText;    ///< Short label painted on the pedal body (e.g. "OD", "FUZZ").

    juce::Colour bodyColour;   ///< Background fill colour of the pedal card.
    juce::Colour accentColour; ///< Colour used for knob caps, the active LED, and label text.

    juce::String tooltip;      ///< Tooltip shown when hovering over the whole pedal card.

    /** Per-parameter tooltip strings, keyed by APVTS parameter ID. */
    std::map<juce::String, juce::String> paramTooltips;
};
