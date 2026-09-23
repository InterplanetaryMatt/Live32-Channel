# Live32 Channel v0.1.0 — VST3 prototype

**Live32 Channel** is the first stand-alone plug-in spin-off from Live32: a digital live-console channel strip based on the same custom DSP used by the REAPER/JSFX project.

This is a development prototype, not a finished release.

## Included DSP

Signal path:

`TRIM / PHASE -> 24 dB/oct LOW CUT -> GATE -> 4-BAND EQ -> COMPRESSOR -> OUTPUT`

The C++ port currently mirrors the Live32 channel-strip JSFX design, including:

- trim: -18 to +18 dB
- polarity inversion
- dedicated 20–400 Hz 4th-order Butterworth high-pass filter (24 dB/oct)
- gate threshold, range, attack, hold and release
- 4-band EQ
  - LOW: LCUT / low shelf / parametric
  - LO MID: broad VEQ-style bell / parametric
  - HI MID: broad VEQ-style bell / parametric
  - HIGH: parametric / high shelf / HCUT
- compressor threshold, ratio, attack, release, makeup and knee
- key-filter high-pass
- optional external VST3 sidechain
- input, output, compressor GR and gate-state metering

## GUI

The interface uses independent **LIVE32 CHANNEL** branding but deliberately follows the familiar physical language of a digital live desk:

- dark console surface
- silver/black encoders
- segmented amber LED rings
- PREAMP, GATE, DYNAMICS and EQUALISER sections
- live EQ response display

It does not use Midas logos or proprietary artwork.

## Build on Windows

### Requirements

- Windows 10/11
- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.22+
- Git (CMake downloads the pinned JUCE source automatically)

Then double-click:

`build_windows.bat`

or run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target Live32Channel_VST3
```

The VST3 bundle should appear under:

`build/Live32Channel_artefacts/Release/VST3/`

Copy the resulting `Live32 Channel.vst3` bundle to your normal VST3 folder, typically:

`C:\Program Files\Common Files\VST3\`

Then rescan plug-ins in REAPER.

## Easiest build: GitHub Actions

The repository includes `.github/workflows/build-windows-vst3.yml`.

If you put this project in a GitHub repository, open **Actions -> Build Windows VST3 -> Run workflow**. GitHub will compile the Windows VST3 and provide it as a downloadable build artifact.

## DSP verification

The core DSP has no JUCE dependency. A smoke test is included and was compiled/run during creation of this package.

At 48 kHz the dedicated 80 Hz high-pass test measured approximately:

- 20 Hz: -48.2 dB
- 40 Hz: -24.1 dB
- 80 Hz: -3.01 dB
- 160 Hz: -0.017 dB

This is the expected response of the two-stage 4th-order Butterworth topology used in Live32.

On Linux/macOS with a compiler available, run:

```bash
./build_dsp_test.sh
```

## Current prototype limitations

- This package has been DSP-tested in the build environment, but the JUCE VST3 wrapper has **not yet been compiled inside REAPER in this environment**.
- Parameter changes currently update coefficients once per audio block; smoothing/zipper-noise work is a later polish step.
- EQ display uses a 48 kHz visual reference. DSP itself uses the actual host sample rate.
- The compressor behaviour intentionally follows the current Live32 JSFX, including its existing makeup-gain behaviour.
- The GUI is the first pass, not the final artwork.

## Next targets

1. Compile Windows VST3 through GitHub Actions.
2. Load in REAPER and verify automation/state restore.
3. Null/measurement comparison against `Live32_Channel.jsfx`.
4. Refine GUI proportions from M32 workflow reference photographs without copying trademarks/artwork.
5. Add oversampling only if measurements demonstrate a real need.
6. Eventually compare against a physical M32 and tune behaviour where useful.

## Framework/licensing

This first prototype uses JUCE 9.0.2. Please read `LICENSE.md` before distributing binaries.

Live32 is independent and is not affiliated with or endorsed by Midas or Music Tribe.
