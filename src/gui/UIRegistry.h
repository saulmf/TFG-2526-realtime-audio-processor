#pragma once

#include <map>

#include <juce_core/juce_core.h>

#include "EffectUIDescriptor.h"

/**
 * UIRegistry - maps effect type IDs to their visual descriptors.
 *
 * Mirrors EffectFactory in structure: effects are registered by TYPE_ID string at startup (in main.cpp)
 * and looked up by the GUI when building pedal cards.
 *
 * @note Owned by MainApplication and passed by reference to MainWindow, kept in the GUI layer.
 * Audio classes never see this registry class.
 */
class UIRegistry {
public:
    void registerDescriptor(const juce::String &typeId, EffectUIDescriptor descriptor);

    // Returns nullptr if typeId has no registered descriptor
    [[nodiscard]] const EffectUIDescriptor *getDescriptor(const juce::String &typeId) const;

    // Returns all registered type IDs, used for building the effect browser
    [[nodiscard]] juce::StringArray getRegisteredTypeIds() const;

private:
    std::map<juce::String, EffectUIDescriptor> m_registry;
};
