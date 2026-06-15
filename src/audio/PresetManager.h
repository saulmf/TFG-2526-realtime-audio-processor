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
     * @param chain Chain whose state is to be serialised.
     * @param file Destination file; created or overwritten if it already exists.
     * @return true on success, false if the file could not be written.
     */
    bool savePreset(const EffectChain &chain, const juce::File &file) const;

    /**
     * Loads a preset from an XML file and fully reconstructs the chain.
     * The existing chain contents are replaced.
     * @param chain Chain to reconstruct; its current contents are cleared before loading.
     * @param factory Used to instantiate each effect by the type ID stored in the file.
     * @param file Source XML preset file.
     * @param outSkippedTypeIds Populated with any type IDs present in the file but not recognised by the factory.
     * @return true on success, false if the file is missing or malformed.
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
