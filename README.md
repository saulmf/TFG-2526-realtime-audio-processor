# Digital Guitar Pedalboard

A real-time multi-effects audio processor that simulates a virtual guitar pedalboard on desktop. Built as a Final Degree Project (TFG) for the Software Engineering degree at the [School of Computer Science Engineering](https://ingenieriainformatica.uniovi.es/), University of Oviedo - 2025-26.

> **Live docs:** [saulmf.github.io/TFG-2526-realtime-audio-processor](https://saulmf.github.io/TFG-2526-realtime-audio-processor/)  
> **API reference:** [Doxygen documentation](https://saulmf.github.io/TFG-2526-realtime-audio-processor/html/index.html)

---

## Overview

The application receives a mono audio signal from an external input device (e.g. a guitar connected via an audio interface), routes it through a configurable chain of up to 8 effects, and outputs the processed signal in real time. Each effect can be individually enabled, reordered, and tuned - replicating the behaviour of a physical pedalboard.

---

## Effects

| Category | Effect | Parameters |
|---|---|---|
| Nonlinear | Overdrive | Drive, Level, Tone |
| Nonlinear | Hard Clipping | Gain, Level, Tone |
| Nonlinear | Fuzz | Fuzz, Level |
| Filtering | Parametric EQ | Freq / Gain / Q × 3 bands |
| Filtering | High-Pass Filter | Cutoff (Hz), Order |
| Filtering | Low-Pass Filter | Cutoff (Hz), Order |
| Time-based | Digital Delay | Delay Time (ms), Feedback, Mix |
| Time-based | Algorithmic Reverb | Room Size, Damping, Width, Mix |

---

## Features

- Real-time audio processing with low latency (WASAPI via JUCE)
- Up to 8 effects active simultaneously in a reorderable chain
- Drag-and-drop effect reordering
- Per-effect enable/bypass toggle
- Input and output level meters
- Preset save and load (XML)
- Keyboard navigation support throughout the UI
- Accessible colour contrast (WCAG AA compliant)

---

## Technology Stack

| | |
|---|---|
| Language | C++17 |
| Framework | [JUCE 8](https://juce.com/) |
| Build | CMake 3.22+ |
| Platform | Windows 10 / 11 (x86-64) |
| Audio API | WASAPI (abstracted by `juce::AudioDeviceManager`) |

---

## Building from Source

### Prerequisites

- CMake 3.22 or later
- A C++17-capable compiler (MSVC 2022 recommended)
- Windows 10 or 11

### Steps

```bash
git clone --recurse-submodules https://github.com/saulmf/TFG-2526-realtime-audio-processor.git
cd TFG-2526-realtime-audio-processor
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The standalone executable will be placed in `build/TFG-2526-realtime-audio-processor_artefacts/Release/Standalone/`.

> **Note:** JUCE is included as a git submodule. The `--recurse-submodules` flag is required when cloning.

---

## Project Structure

```
TFG-2526-realtime-audio-processor/
├── src/
│   ├── audio/          # AudioEngine, EffectChain, EffectFactory, PresetManager
│   ├── effects/
│   │   ├── nonlinear/  # Overdrive, HardClipping, Fuzz
│   │   ├── filtering/  # ParametricEQ, HighPass, LowPass
│   │   └── timebased/  # DigitalDelay, AlgorithmicReverb
│   └── gui/            # MainWindow and all UI components
├── tests/              # Unit and performance tests
├── docs/               # Generated documentation (Doxygen)
├── resources/          # Icons and assets
├── JUCE/               # JUCE submodule
├── Doxyfile
└── CMakeLists.txt
```

---

## License

This project is licensed under the **MIT License** - see the [LICENSE](./LICENSE) file for details.

---

## Author

**Saúl Martín Fernández** - UO294936@uniovi.es
University of Oviedo · School of Computer Science Engineering · 2025-2026
