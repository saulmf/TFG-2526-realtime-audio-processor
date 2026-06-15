#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <AppTranslations.h>

#include "audio/AudioEngine.h"
#include "ApplicationController.h"
#include "gui/MainWindow.h"
#include "gui/UIRegistry.h"

#include "effects/nonlinear/OverdriveEffect.h"
#include "effects/nonlinear/HardClippingEffect.h"
#include "effects/nonlinear/FuzzEffect.h"
#include "effects/filtering/ParametricEQEffect.h"
#include "effects/filtering/HighPassFilterEffect.h"
#include "effects/filtering/LowPassFilterEffect.h"
#include "effects/timebased/DigitalDelayEffect.h"
#include "effects/timebased/AlgorithmicReverbEffect.h"

/**
 * MainApplication is the JUCE application entry point.
 *
 * Owns all top-level objects (AudioEngine, ApplicationController, UIRegistry, MainWindow)
 * and is responsible for creating them in the right order during startup and destroying them on quit.
 *
 * Startup sequence:
 *   1. Load Spanish translations if the system language is Spanish.
 *   2. Create AudioEngine and register all effect types with EffectFactory.
 *   3. Create ApplicationController (facade between GUI and audio).
 *   4. Populate UIRegistry with visual descriptors for each effect type.
 *   5. Create MainWindow (kicks off the GUI).
 */
class MainApplication : public juce::JUCEApplication {
public:
    /** Returns the application name displayed in the title bar. */
    const juce::String getApplicationName() override { return "Realtime Audio Processor"; }

    /** Returns the current application version string. */
    const juce::String getApplicationVersion() override { return "1.0.0"; }

    /** Called by JUCE on startup. Performs the full initialisation sequence described above. */
    void initialise(const juce::String &) override {
        // 0. Load Spanish translations when the system language is Spanish
        //    English strings are used as keys and serve as the natural fallback, so no English translation file is needed
        if (juce::SystemStats::getUserLanguage().startsWithIgnoreCase("es")) {
            juce::LocalisedStrings::setCurrentMappings(
                new juce::LocalisedStrings(
                    juce::String::fromUTF8(AppTranslations::es_txt,
                                           AppTranslations::es_txtSize),
                    false));
            DBG("[init] Loaded Spanish translations.");
        }

        // 1. Audio subsystem
        m_audioEngine = std::make_unique<AudioEngine>();
        DBG("[init] AudioEngine created.");

        // 2. Effect registration
        EffectFactory &factory = m_audioEngine->getEffectFactory();
        factory.registerEffect(OverdriveEffect::TYPE_ID,
                               []() { return std::make_unique<OverdriveEffect>(); });
        factory.registerEffect(HardClippingEffect::TYPE_ID,
                               []() { return std::make_unique<HardClippingEffect>(); });
        factory.registerEffect(FuzzEffect::TYPE_ID,
                               []() { return std::make_unique<FuzzEffect>(); });
        factory.registerEffect(ParametricEQEffect::TYPE_ID,
                               []() { return std::make_unique<ParametricEQEffect>(); });
        factory.registerEffect(HighPassFilterEffect::TYPE_ID,
                               []() { return std::make_unique<HighPassFilterEffect>(); });
        factory.registerEffect(LowPassFilterEffect::TYPE_ID,
                               []() { return std::make_unique<LowPassFilterEffect>(); });
        factory.registerEffect(DigitalDelayEffect::TYPE_ID,
                               []() { return std::make_unique<DigitalDelayEffect>(); });
        factory.registerEffect(AlgorithmicReverbEffect::TYPE_ID,
                               []() { return std::make_unique<AlgorithmicReverbEffect>(); });

        DBG("[init] Effects registered: "
            + factory.getAvailableTypeIds().joinIntoString(", "));

        // 3. Application controller (facade between GUI and audio subsystem)
        m_controller = std::make_unique<ApplicationController>(*m_audioEngine);
        DBG("[init] ApplicationController created.");

        // 4. UI registry - visual descriptors for each effect type, keyed by TYPE_ID
        m_uiRegistry = std::make_unique<UIRegistry>();
        m_uiRegistry->registerDescriptor("overdrive", {
                                             TRANS("Overdrive"), "Nonlinear", "OD",
                                             juce::Colour(0xFFB45309), juce::Colours::white,
                                             TRANS(
                                                 "Valve-style soft saturation. Adds warm harmonic distortion and increased sustain."),
                                             {
                                                 {"drive", "How hard the signal is pushed into saturation."},
                                                 {"level", "Output volume of the effect."},
                                                 {"tone", "Adjusts the brightness of the tone."}
                                             }
                                         });
        m_uiRegistry->registerDescriptor("hardclip", {
                                             TRANS("Hard Clipping"), "Nonlinear", "CLIP",
                                             juce::Colour(0xFFB91C1C), juce::Colours::white,
                                             TRANS(
                                                 "Hard digital clipping for aggressive distortion. Produces a sharp, heavily saturated sound."),
                                             {
                                                 {"gain", "How hard the signal is pushed into the hard clipper."},
                                                 {"level", "Output volume of the effect."},
                                                 {"tone", "Adjusts the brightness of the distorted tone."}
                                             }
                                         });
        m_uiRegistry->registerDescriptor("fuzz", {
                                             TRANS("Fuzz"), "Nonlinear", "FUZZ",
                                             juce::Colour(0xFF7C3AED), juce::Colours::yellow,
                                             TRANS(
                                                 "Classic fuzz circuit emulation. Creates a thick, sustaining and harmonically rich tone."),
                                             {
                                                 {"fuzz", "Intensity of the fuzz effect."},
                                                 {"level", "Output volume of the effect."}
                                             }
                                         });
        m_uiRegistry->registerDescriptor("parametriceq", {
                                             TRANS("Parametric EQ"), "Filtering", "EQ",
                                             juce::Colour(0xFF0369A1), juce::Colours::white,
                                             TRANS(
                                                 "Three-band parametric equaliser. Shape tone by adjusting frequency, gain and bandwidth (Q) per band."),
                                             {
                                                 {"freq1", "Center frequency of this EQ band."},
                                                 {"gain1", "Boosts or cuts the level at this frequency."},
                                                 {"q1", "Bandwidth: higher values affect a narrower frequency range."},
                                                 {"freq2", "Center frequency of this EQ band."},
                                                 {"gain2", "Boosts or cuts the level at this frequency."},
                                                 {"q2", "Bandwidth: higher values affect a narrower frequency range."},
                                                 {"freq3", "Center frequency of this EQ band."},
                                                 {"gain3", "Boosts or cuts the level at this frequency."},
                                                 {"q3", "Bandwidth: higher values affect a narrower frequency range."}
                                             }
                                         });
        m_uiRegistry->registerDescriptor("highpass", {
                                             TRANS("High-Pass Filter"), "Filtering", "HPF",
                                             juce::Colour(0xFF0891B2), juce::Colours::white,
                                             TRANS(
                                                 "Passes frequencies above the cutoff, removing low-end rumble or bass build-up."),
                                             {
                                                 {"cutoff", "Frequencies below this point are removed."},
                                                 {"order", "Steepness of the filter: higher = sharper rolloff."}
                                             }
                                         });
        m_uiRegistry->registerDescriptor("lowpass", {
                                             TRANS("Low-Pass Filter"), "Filtering", "LPF",
                                             juce::Colour(0xFF1D4ED8), juce::Colours::white,
                                             TRANS(
                                                 "Passes frequencies below the cutoff, removing harshness or high-frequency content."),
                                             {
                                                 {"cutoff", "Frequencies above this point are removed."},
                                                 {"order", "Steepness of the filter: higher = sharper rolloff."}
                                             }
                                         });
        m_uiRegistry->registerDescriptor("digitaldelay", {
                                             TRANS("Digital Delay"), "Time-Based", "DLY",
                                             juce::Colour(0xFF065F46), juce::Colours::lightgreen,
                                             TRANS(
                                                 "Echo effect with adjustable time, feedback and mix. Creates rhythmic repetitions of the signal."),
                                             {
                                                 {
                                                     "delayTime",
                                                     "Time between the original signal and its echo, in milliseconds."
                                                 },
                                                 {"feedback", "How many times the echo repeats before fading."},
                                                 {"mix", "Balance between the original (dry) and delayed (wet) signal."}
                                             }
                                         });
        m_uiRegistry->registerDescriptor("reverb", {
                                             TRANS("Reverb"), "Time-Based", "VERB",
                                             juce::Colour(0xFF1E3A5F), juce::Colour(0xFFAAD4F5),
                                             TRANS(
                                                 "Simulates room reverberation. Controls room size, damping, stereo width and wet/dry mix."),
                                             {
                                                 {
                                                     "roomSize",
                                                     "Size of the simulated room: larger = longer reverb tail."
                                                 },
                                                 {
                                                     "damping",
                                                     "High-frequency absorption: higher = darker, shorter decay."
                                                 },
                                                 {"width", "Stereo spread of the reverb effect."},
                                                 {"mix", "Balance between the original (dry) and reverb (wet) signal."}
                                             }
                                         });
        DBG("[init] UIRegistry populated with "
            + juce::String(m_uiRegistry->getRegisteredTypeIds().size()) + " descriptor(s).");

        // 5. GUI (receives controller and UI registry)
        m_mainWindow = std::make_unique<MainWindow>(getApplicationName(),
                                                    *m_controller,
                                                    *m_uiRegistry);
        DBG("[init] MainWindow created. Application ready.");
    }

    /** Called by JUCE on quit. Destroys all subsystems in reverse creation order. */
    void shutdown() override {
        m_mainWindow = nullptr;
        m_uiRegistry = nullptr;
        m_controller = nullptr;
        m_audioEngine = nullptr;
    }

private:
    std::unique_ptr<AudioEngine> m_audioEngine;
    std::unique_ptr<ApplicationController> m_controller;
    std::unique_ptr<UIRegistry> m_uiRegistry;
    std::unique_ptr<MainWindow> m_mainWindow;
};

START_JUCE_APPLICATION(MainApplication)
