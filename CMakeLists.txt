cmake_minimum_required(VERSION 3.22)
project(RealtimeAudioProcessor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE is added as a subdirectory (git submodule at ./JUCE)
add_subdirectory(JUCE)

juce_add_binary_data(AppTranslations
        HEADER_NAME AppTranslations.h
        NAMESPACE   AppTranslations
        SOURCES     resources/translations/es.txt
)

juce_add_gui_app(RealtimeAudioProcessor
        PRODUCT_NAME "RealtimeAudioProcessor"
        COMPANY_NAME "Saul Martin"
        BUNDLE_ID    "com.saul.realtimeaudioprocessor"
        ICON_BIG     "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/logo.png"
        ICON_SMALL   "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/logo.png"
)

# All source files
target_sources(RealtimeAudioProcessor PRIVATE
        src/main.cpp
        src/ApplicationController.h
        src/ApplicationController.cpp

        src/audio/AudioEngine.h
        src/audio/AudioEngine.cpp
        src/audio/EffectChain.h
        src/audio/EffectChain.cpp
        src/audio/EffectFactory.h
        src/audio/EffectFactory.cpp
        src/audio/PresetManager.h
        src/audio/PresetManager.cpp

        src/effects/IAudioEffect.h
        src/effects/AudioEffectBase.h
        src/effects/AudioEffectBase.cpp

        src/effects/nonlinear/OverdriveEffect.h
        src/effects/nonlinear/OverdriveEffect.cpp
        src/effects/nonlinear/HardClippingEffect.h
        src/effects/nonlinear/HardClippingEffect.cpp
        src/effects/nonlinear/FuzzEffect.h
        src/effects/nonlinear/FuzzEffect.cpp

        src/effects/filtering/ParametricEQEffect.h
        src/effects/filtering/ParametricEQEffect.cpp
        src/effects/filtering/HighPassFilterEffect.h
        src/effects/filtering/HighPassFilterEffect.cpp
        src/effects/filtering/LowPassFilterEffect.h
        src/effects/filtering/LowPassFilterEffect.cpp

        src/effects/timebased/DigitalDelayEffect.h
        src/effects/timebased/DigitalDelayEffect.cpp
        src/effects/timebased/AlgorithmicReverbEffect.h
        src/effects/timebased/AlgorithmicReverbEffect.cpp

        src/gui/MainWindow.h
        src/gui/MainWindow.cpp
        src/gui/AppMenuBar.h
        src/gui/AppMenuBar.cpp
        src/gui/LevelMeter.h
        src/gui/LevelMeter.cpp
        src/gui/StatusBar.h
        src/gui/StatusBar.cpp
        src/gui/EffectUIDescriptor.h
        src/gui/UIRegistry.h
        src/gui/UIRegistry.cpp
        src/gui/TransportBar.h
        src/gui/TransportBar.cpp
        src/gui/EffectBrowserPanel.h
        src/gui/EffectBrowserPanel.cpp
        src/gui/EffectChainPanel.h
        src/gui/EffectChainPanel.cpp
        src/gui/PedalCard.h
        src/gui/PedalCard.cpp
)

# JUCE modules used by this project
target_compile_definitions(RealtimeAudioProcessor PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:RealtimeAudioProcessor,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:RealtimeAudioProcessor,JUCE_VERSION>"
)

target_link_libraries(RealtimeAudioProcessor PRIVATE
        AppTranslations
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

add_subdirectory(tests)