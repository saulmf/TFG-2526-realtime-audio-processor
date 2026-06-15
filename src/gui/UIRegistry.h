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
    /**
     * Registers the visual descriptor for a given effect type ID.
     * @param typeId Must match the TYPE_ID of the corresponding effect class.
     * @param descriptor Visual metadata to associate with this type.
     * @note Asserts in debug builds if typeId is empty or already registered.
     */
    void registerDescriptor(const juce::String &typeId, EffectUIDescriptor descriptor);

    /**
     * Returns a pointer to the descriptor for the given type ID.
     * @param typeId Type ID to look up.
     * @return Pointer to the descriptor, or nullptr if the type ID has not been registered.
     */
    [[nodiscard]] const EffectUIDescriptor *getDescriptor(const juce::String &typeId) const;

    /** Returns the type IDs of all registered descriptors, used to build the effect browser catalogue.
        @return StringArray of registered type IDs, in insertion order. */
    [[nodiscard]] juce::StringArray getRegisteredTypeIds() const;

private:
    std::map<juce::String, EffectUIDescriptor> m_registry;
};
