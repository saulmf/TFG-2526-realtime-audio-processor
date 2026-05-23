#include "PresetManager.h"
#include "EffectChain.h"
#include "EffectFactory.h"


bool PresetManager::savePreset(const EffectChain &chain, const juce::File &file) const {
    juce::ValueTree preset(k_presetTag);
    preset.setProperty(k_versionAttr, k_version, nullptr);
    preset.appendChild(chain.getState(), nullptr);

    if (const auto xml = preset.createXml())
        return xml->writeTo(file);

    DBG("PresetManager::savePreset - failed to serialise state");
    return false;
}

bool PresetManager::loadPreset(EffectChain &chain,
                               const EffectFactory &factory,
                               const juce::File &file) const {
    if (!file.existsAsFile()) {
        DBG("PresetManager::loadPreset - file not found: " + file.getFullPathName());
        return false;
    }

    const auto xml = juce::parseXML(file);

    if (xml == nullptr) {
        DBG("PresetManager::loadPreset - failed to parse XML: " + file.getFullPathName());
        return false;
    }

    const auto state = juce::ValueTree::fromXml(*xml);

    if (!state.hasType(k_presetTag)) {
        DBG("PresetManager::loadPreset - not a valid preset file");
        return false;
    }

    const auto chainState = state.getChildWithName("EffectChain");

    if (!chainState.isValid()) {
        DBG("PresetManager::loadPreset - missing EffectChain node");
        return false;
    }

    chain.setState(chainState, factory);
    return true;
}
