# MesoParasites

MesoParasites is the hardware firmware counterpart to **MesoTyphoon** for VCV Rack. It is a fork of Patrick Dowling’s [Superparasites firmware](https://github.com/patrickdowling/superparasites), which ports Clouds, [Parasites](https://github.com/mqtthiqs/parasites), and [Kammerl Beat Repeat](https://github.com/jkammerl/eurorack) to the Grayscale [Supercell](https://grayscale.info/supercell/) and [Microcell](https://grayscale.info/microcell/) platforms.

Note that this version has platform-specific drivers and a custom linker script and is **not** compatible with other hardware versions of Clouds.

## Changes in this fork

- **ADSR Envelope** replaces Spectral Clouds in firmware slot 8. It applies a gate-controlled envelope to stereo audio. The Spectral Clouds source remains in the repository.
- **Shoegaze** replaces Beat Repeat in slot 7. It processes stereo audio through a pre-reverb, fuzz with a tone control, and a post-reverb. Each reverb has its own mix and decay controls. The Beat Repeat source remains in the repository.
- **Chaotic Pendulum** adds a monophonic synth voice in firmware slot 9, with V/oct pitch, an ADSR envelope, Chaos Amount, Shape, Fold, and bipolar Animate detuning. The voice shares its DSP implementation with Rack.
- **Typhoon and MesoTyphoon for VCV Rack 2** build directly from this repository’s processor source. Typhoon offers the original eight modes. MesoTyphoon offers this fork’s modes and preserves the slot numbering of existing Rack patches. See the [Rack build and usage guide](rack/README.md).
- **Python 3 WAV encoding** uses the `python3` branch of this fork’s [audio bootloader tools](https://github.com/mdwelsh/stm-audio-bootloader/tree/python3) to generate firmware update WAVs.

## Firmware modes

- Granular
- Pitch shifter/time stretch
- Looping delay
- Spectral madness
- Oliverb
- Resonestor
- Shoegaze
- ADSR envelope
- Chaotic Pendulum

**Shoegaze** is the seventh mode (●●○●). Patch audio into **In L/R**. The
**Position** and **Density** sliders set pre- and post-reverb decay, respectively.
**Size** and **Texture** set fuzz amount and tone, from dark to bright. The
**Mix** and **Pan** knobs set pre- and post-reverb mix. **Feedback** feeds the
post-reverb tail back into the fuzz. **Reverb** sets post-reverb modulation depth.
Save any recording you want to keep before switching to Shoegaze, which
uses the recording RAM for its two reverb tails.

In the ADSR mode, patch stereo audio into the audio inputs and a gate into
**Trig**, or hold the manual trigger to audition.
Set attack, decay, sustain, and release with **Position**, **Density**, **Size**,
and **Texture**, respectively. The gate opens the envelope and holds sustain
until it goes low. Attack, decay, and release range from 1 ms to 10 s with a
cubic slider taper. Sustain sets the held level from silence to unity gain.
ADSR occupies the former Spectral Clouds slot (eighth mode, LED pattern ●●●○).

**Chaotic Pendulum** is the ninth mode (●●●●). Patch pitch to **V/oct** and a
note gate to **Trig**, or hold the manual trigger to audition. The four
sliders still set attack, decay, sustain, and release. **Mix** is Chaos Amount,
**Pan** is Shape, **Feedback** is Fold, and **Reverb** is Animate. Animate spans
one semitone down to one semitone up, with zero detune at noon. The voice
generates its own signal and sends it to both audio outputs.

## Hardware builds

Use the Microcell variant for Typhoon or Microcell, and the Supercell variant
for Grayscale Supercell. These variants account for differences in pot polarity.

| Hardware | Build command | Update WAV |
| --- | --- | --- |
| Typhoon / Microcell | `make -f supercell/makefile VARIANT=MICROCELL wav` | `build/microcell/MesoParasites-microcell.wav` |
| Supercell | `make -f supercell/makefile VARIANT=SUPERCELL wav` | `build/supercell/MesoParasites-supercell.wav` |

Initialize the Git submodules before building. Set `TOOLCHAIN_PATH` to the
ARM GNU toolchain directory if it differs from the makefile default. The WAV
encoder uses Python 3 with NumPy installed. Set `PYTHON` when the default
`python` points to a different interpreter.

This doesn't change anything for the bootloader (1), which should be common to both(famous last words...)

At the risk of repeating myself: This code is not compatible with other hardware versions of Clouds!

## Notes
- (1) The bootloader is the least tested part of this project.
- Upstream releases were compiled using `gcc-arm-none-eabi-5_4-2016q3`.
- The stmlib submodule still uses mqtthiqs/stmlib which has diverged somewhat from pichenettes'. This allowed setting of an external linker script out-of-the-box.
- However, that does mean that the scripts for programming appear to require openocd 0.9.0 so the command line calls may be different to a current version of pichenettes' repo.
- The custom linker script adds sections for relevant C++ features.

## Auditioning DSP changes in VCV Rack

The [Rack plugin](rack/README.md) packages Typhoon and MesoTyphoon together.
MesoTyphoon and MesoParasites share DSP. Shoegaze is slot 7 in both. Hardware
uses slot 8 for ADSR and slot 9 for Chaotic Pendulum. Rack keeps those two in
the opposite order so existing patches load with their saved mode.

## License
Code (STM32F projects): MIT license.
