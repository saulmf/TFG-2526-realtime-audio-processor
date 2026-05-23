#include "UIRegistry.h"

void UIRegistry::registerDescriptor(const juce::String &typeId, EffectUIDescriptor descriptor) {
    jassert(typeId.isNotEmpty());
    jassert(m_registry.find(typeId) == m_registry.end()); // no duplicates

    m_registry.emplace(typeId, std::move(descriptor));
}

const EffectUIDescriptor *UIRegistry::getDescriptor(const juce::String &typeId) const {
    const auto it = m_registry.find(typeId);
    return it != m_registry.end() ? &it->second : nullptr;
}

juce::StringArray UIRegistry::getRegisteredTypeIds() const {
    juce::StringArray ids;
    for (const auto &[typeId, _]: m_registry)
        ids.add(typeId);
    return ids;
}
