#include "ApplicationController.h"

ApplicationController::ApplicationController(AudioEngine &audioEngine)
    : m_audioEngine(audioEngine) {
    DBG("[ApplicationController] Initialised. "
        + juce::String(getAvailableEffectTypes().size())
        + " effect type(s) available: "
        + getAvailableEffectTypes().joinIntoString(", "));
}

// Session control

juce::String ApplicationController::start() {
    return m_audioEngine.start();
}

void ApplicationController::stop() {
    m_audioEngine.stop();
}

bool ApplicationController::isRunning() const {
    return m_audioEngine.isRunning();
}


// Device configuration

juce::StringArray ApplicationController::getAvailableInputDevices() const {
    return m_audioEngine.getAvailableInputDevices();
}

juce::StringArray ApplicationController::getAvailableOutputDevices() const {
    return m_audioEngine.getAvailableOutputDevices();
}

juce::String ApplicationController::getCurrentInputDeviceName() const {
    return m_audioEngine.getCurrentInputDeviceName();
}

juce::String ApplicationController::getCurrentOutputDeviceName() const {
    return m_audioEngine.getCurrentOutputDeviceName();
}

bool ApplicationController::setInputDevice(const juce::String &deviceName) {
    return m_audioEngine.setInputDevice(deviceName);
}

bool ApplicationController::setOutputDevice(const juce::String &deviceName) {
    return m_audioEngine.setOutputDevice(deviceName);
}

bool ApplicationController::setSampleRate(double sampleRate) {
    return m_audioEngine.setSampleRate(sampleRate);
}

bool ApplicationController::setBufferSize(int bufferSizeInSamples) {
    return m_audioEngine.setBufferSize(bufferSizeInSamples);
}

double ApplicationController::getCurrentSampleRate() const {
    return m_audioEngine.getCurrentSampleRate();
}

float ApplicationController::getInputLevel() const {
    return m_audioEngine.getInputLevel();
}

float ApplicationController::getOutputLevel() const {
    return m_audioEngine.getOutputLevel();
}

void ApplicationController::setMasterVolume(float v) {
    m_audioEngine.setMasterVolume(v);
}

float ApplicationController::getMasterVolume() const {
    return m_audioEngine.getMasterVolume();
}

int ApplicationController::getCurrentBufferSize() const {
    return m_audioEngine.getCurrentBufferSize();
}

const juce::Array<int> &ApplicationController::getValidBufferSizes() {
    return AudioEngine::k_validBufferSizes;
}

const juce::Array<double> &ApplicationController::getValidSampleRates() {
    return AudioEngine::k_validSampleRates;
}


// Effect chain management

void ApplicationController::addEffect(const juce::String &typeId, int position) {
    if (m_audioEngine.getEffectChain().getNumEffects() >= k_maxEffects) {
        DBG("ApplicationController::addEffect - chain full (max " + juce::String(k_maxEffects) + " effects).");
        return;
    }

    auto effect = m_audioEngine.getEffectFactory().create(typeId);

    if (effect == nullptr) {
        DBG("ApplicationController::addEffect - unknown typeId: " + typeId);
        return;
    }

    m_audioEngine.getEffectChain().addEffect(std::move(effect), position);
}

void ApplicationController::removeEffect(int index) {
    m_audioEngine.getEffectChain().removeEffect(index);
}

void ApplicationController::clearChain() {
    auto &chain = m_audioEngine.getEffectChain();
    while (chain.getNumEffects() > 0)
        chain.removeEffect(0);
}

void ApplicationController::moveEffect(int fromIndex, int toIndex) {
    m_audioEngine.getEffectChain().moveEffect(fromIndex, toIndex);
}

void ApplicationController::setEffectEnabled(int index, bool enabled) {
    m_audioEngine.getEffectChain().setEffectEnabled(index, enabled);
}

int ApplicationController::getNumEffects() const {
    return m_audioEngine.getEffectChain().getNumEffects();
}

IAudioEffect *ApplicationController::getEffect(int index) {
    return m_audioEngine.getEffectChain().getEffect(index);
}

juce::StringArray ApplicationController::getAvailableEffectTypes() const {
    return m_audioEngine.getEffectFactory().getAvailableTypeIds();
}


// Preset management

bool ApplicationController::savePreset(const juce::File &file) {
    return m_presetManager.savePreset(m_audioEngine.getEffectChain(), file);
}

bool ApplicationController::loadPreset(const juce::File &file) {
    return m_presetManager.loadPreset(
        m_audioEngine.getEffectChain(),
        m_audioEngine.getEffectFactory(),
        file);
}
