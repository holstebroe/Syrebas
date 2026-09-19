# Syrebas

![Syrebas Hero](syrebas_hero.jpg)

**Syrebas** is a lightweight, accurate Roland TB-303 bass synthesizer emulator plugin written in modern C++17 using the [CLAP](https://cleveraudio-plug.info/) (Clever Audio Plug-in) standard.

## Features

- **Coupled Diode-Ladder DSP Engine**:
  - 4x oversampled 2nd-order Runge-Kutta (RK2) diode-ladder filter solver with
    inter-stage $\tanh()$ nonlinearity and feedback saturation.
  - Resonance-dependent bass-drop high-pass network in the feedback path.
  - BA662 VCA model with control-current summing and asymmetric overdrive saturation.
  - Authentic control-voltage (CV) cross-talk, resonance CV bleed, and accent capacitor
    energy accumulation across consecutive notes.
- **Classic 303 Sound Engine**:
  - Sawtooth (14 kHz LPF rounded, quadratic distortion) and Square waveforms (derived from Saw, 46% duty cycle, 150 Hz HPF tilt).
  - Authentic pitch slide / portamento (~60–70ms glide when notes overlap legato).
  - Velocity-triggered accent (filter decay override, envelope frequency chirp, VCA gain boost, and drive saturation).
- **Lightweight Native GUI**:
  - Embedded GUI using pure native drawing primitives without heavy frameworks (e.g. JUCE).
  - Smooth 303-style knob rendering with vertical drag and fine-tuning precision modifiers.
- **CLAP & MIDI Support**:
  - 7 fully automatable parameters exposed to the host DAW.
  - Full MIDI Control Change (CC) parameter control and state load/save persistence.

---

## Compact User Guide

### How to Trigger Slide (Portamento)
- **Legato Note Overlap**: Play or sequence a new note before releasing the current active note (note overlap / legato playing).
- **Glide Behavior**: The pitch smoothly glides to the new note pitch with an authentic ~60–70ms time constant.
- **Envelope State**: Slid notes do not retrigger envelope attacks; the filter envelope continues its natural exponential decay while the VCA remains open.

### How to Trigger Accent
- **MIDI Velocity Threshold**: Accent is triggered on any note played with a MIDI velocity of **0.8 or higher** (velocity $\ge 102$ out of 127).
- **Accent Effects**:
  - **Filter Decay Floor**: Forces the filter envelope decay duration directly to its minimum (~200ms), overriding the Decay knob.
  - **Envelope Sweep Chirp**: Boosts filter modulation depth for a snappy top-end chirp, controlled by the **Accent** knob.
  - **VCA Saturation Boost**: Boosts signal gain into the VCA stage, producing characteristic analog saturation.
  - **Energy Accumulation**: Rapid consecutive accented notes accumulate charge on the accent capacitor, causing baseline cutoff to drift upward over repeated accent hits.

### Front-Panel Controls & GUI Interactions
| Control | Description |
| :--- | :--- |
| **Cutoff** | Base VCF filter cutoff frequency (200 Hz to 2.5 kHz baseline). |
| **Resonance** | VCF feedback intensity, featuring low-end bass drop and feedback high-pass filtering. |
| **Env Mod** | Filter envelope sweep depth (sweeping cutoff up to 7.5 kHz). |
| **Decay** | VCF envelope decay duration (200ms to 2.5s). |
| **Accent** | Accent intensity level for high-velocity notes ($\ge 0.8$). |
| **Waveform** | Switch between **Saw** and **Square** oscillator waveforms. |
| **Volume** | Master output level. |

- **Knob Adjustments**: Click and drag **vertically** (up/down) on any knob to adjust its parameter.
- **Fine Adjustment**: Hold the **Shift** key while dragging a knob for fine-grained value adjustments.

---

## Building from Source

### Prerequisites
- C++17 compliant compiler (GCC, Clang, or MSVC)
- CMake 3.15 or higher
- Linux: `libx11-dev` (for native GUI windowing on X11)

### Build Instructions
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The resulting CLAP plugin (`syrebas.clap`) will be located in the `build/` directory.

### Running Test Executables
```bash
# Run DSP tests and audio verification
./build/syrebas_dsp_test

# Run GUI window rendering & event loop test
./build/syrebas_gui_test
```

## License

MIT License. See [LICENSE](LICENSE) for details.
