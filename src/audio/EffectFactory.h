#pragma once

#include <functional>
#include <map>

#include <juce_core/juce_core.h>

#include "../effects/IAudioEffect.h"

/**
 * EffectFactory implements the Registry pattern for effect creation.
 *
 * Each effect type is registered once at application startup (main.cpp) via registerEffect().
 * After that, any part of the application can instantiate an effect by its stable string type ID
 * without knowing the concrete class (decoupling the creation site from the usage site).
 *
 * Adding a new effect requires exactly one registerEffect() call in main.cpp - no other class needs to change.
 *
 * Thread safety: all methods must be called from the GUI thread only.
 */
class EffectFactory {
public:
    /**
     * Signature for effect creator functions.
     * Each registered creator is a callable that returns a fully constructed, heap-allocated effect instance.
     * Effects are self-contained: they create and own any processor state they need internally (e.g. for APVTS ownership).
     */
    using CreatorFn = std::function<std::unique_ptr<IAudioEffect>()>;

    EffectFactory() = default;

    ~EffectFactory() = default;

    /**
     * Registers a creator function for a given type ID.
     * Must be called once per effect type before any create() call.
     * (Called from main.cpp during application initialization.)
     *
     * @param typeId Stable string identifier for the effect type (e.g. "overdrive").
     * @param creator Callable that constructs and returns a new instance of the effect.
     * @note Asserts in debug builds if typeId is empty or already registered.
     */
    void registerEffect(const juce::String &typeId, CreatorFn creator);

    /**
     * Instantiates and returns the effect with the given type ID.
     * The caller takes ownership of the returned object.
     * @param typeId Type ID previously registered via registerEffect().
     * @return New effect instance, or nullptr if the type ID has not been registered.
     */
    [[nodiscard]] std::unique_ptr<IAudioEffect> create(const juce::String &typeId) const;

    /**
     * Returns the type IDs of all registered effects, sorted alphabetically.
     * @return StringArray of type IDs; used by the GUI to populate the add-effect dropdown.
     */
    [[nodiscard]] juce::StringArray getAvailableTypeIds() const;

private:
    std::map<juce::String, CreatorFn> m_registry;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectFactory)
};
