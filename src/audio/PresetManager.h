#pragma once

#include <juce_core/juce_core.h>

class EffectChain;
class EffectFactory;

/**
 * PresetManager is a stateless utility that serializes and deserializes
 * the full effect chain state to and from XML preset files.
 *
 * It has no members of its own - all state lives in EffectChain.
 * An instance is owned by ApplicationController, which provides the chain and factory references at call time.
 */
class PresetManager {
public:
    PresetManager() = default;

    ~PresetManager() = default;

    /**
     * Saves the current chain state (effect order, enabled flags, and all parameter values) to an XML file.
     * Returns true on success, false if the file could not be written.
     */
    bool savePreset(const EffectChain &chain, const juce::File &file) const;

    /**
     * Loads a preset from an XML file and fully reconstructs the chain.
     * The existing chain contents are replaced.
     * Returns true on success, false if the file is missing or malformed.
     * On success, outSkippedTypeIds is populated with any type IDs that were
     * not recognised by the factory and therefore skipped.
     */
    bool loadPreset(EffectChain &chain,
                    const EffectFactory &factory,
                    const juce::File &file,
                    juce::StringArray &outSkippedTypeIds) const;

private:
    static constexpr const char *k_presetTag = "Preset";
    static constexpr const char *k_versionAttr = "version";
    static constexpr int k_version = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
