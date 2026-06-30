juce_add_console_app(AudioProcessorTests
    PRODUCT_NAME "AudioProcessorTests"
)

target_sources(AudioProcessorTests PRIVATE
    TestMain.cpp
    EffectsSubsystemTests.cpp
    AudioSubsystemTests.cpp
    PerformanceTests.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ApplicationController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/audio/AudioEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/audio/EffectChain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/audio/EffectFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/audio/PresetManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/effects/AudioEffectBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/effects/nonlinear/OverdriveEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/effects/nonlinear/HardClippingEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/effects/nonlinear/FuzzEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/effects/filtering/ParametricEQEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/effects/filtering/HighPassFilterEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/effects/filtering/LowPassFilterEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/effects/timebased/DigitalDelayEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/effects/timebased/AlgorithmicReverbEffect.cpp
)

target_include_directories(AudioProcessorTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_compile_definitions(AudioProcessorTests PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_APPLICATION_NAME_STRING="AudioProcessorTests"
    JUCE_APPLICATION_VERSION_STRING="1.0.0"
)

target_link_libraries(AudioProcessorTests PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
