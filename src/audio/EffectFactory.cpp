#include "EffectFactory.h"

void EffectFactory::registerEffect(const juce::String &typeId, CreatorFn creator) {
    jassert(typeId.isNotEmpty());
    jassert(creator != nullptr);
    jassert(m_registry.count(typeId) == 0); // duplicate registration is considered a programming error

    m_registry[typeId] = std::move(creator);
}

std::unique_ptr<IAudioEffect> EffectFactory::create(const juce::String &typeId) const {
    const auto it = m_registry.find(typeId);

    if (it == m_registry.end())
        return nullptr;

    return it->second();
}

juce::StringArray EffectFactory::getAvailableTypeIds() const {
    juce::StringArray ids;

    for (const auto &entry: m_registry)
        ids.add(entry.first);

    return ids; // Result is in alphabetical order
}
