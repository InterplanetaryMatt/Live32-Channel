# Changelog

## 0.3.1
- Replaced JUCE slider text boxes with dedicated Live32 value labels, eliminating long floating-point readouts on logarithmic controls.
- Frequencies above 1 kHz now use compact kHz formatting.
- Re-spaced the EQ encoder stack so WIDTH / FREQUENCY / GAIN labels and values cannot overlap.
- Moved the EQ on/off button to the left-hand control area beneath MODE.
- Changed the phase-invert legend to Φ.
- Compressor display now always shows the configured transfer curve, even while COMP is bypassed, so ratio and knee remain visible.
- Added a square threshold point and visible knee-boundary markers.
- No DSP/audio changes.

## 0.3.0
- Reworked the editor into a much more desk-like physical layout.
- Moved EQ and dynamics graphs into a shared right-hand CHANNEL DISPLAY area.
- Moved EQ MODE beside the physical EQ controls instead of floating under the graph.
- Fixed slider readouts so APVTS attachment no longer exposes long raw floating-point values.
- Rebuilt the dynamics display around threshold / ratio / knee geometry.
- Added threshold marker, soft-knee region, GR meter, and THRESH / RATIO / KNEE / MAKEUP readouts.
- Compressor transfer graph intentionally excludes makeup gain so its shape clearly represents compressor behaviour.
- DSP/audio processing itself is unchanged.

## 0.2.2
- Fixed a REAPER editor-instantiation crash introduced by the v0.2 desk-style GUI.
- Dynamic EQ band buttons are now created before host resizing is enabled.
- `resized()` and EQ-band selection now guard against early host callbacks and uninitialised button pointers.
- No DSP changes; this is a GUI-lifecycle safety fix.

## 0.2.1
- Fixed the Windows/MSVC build failure in the new compressor transfer-curve display.
- Explicitly capture the graph dB range constants in the coordinate-mapping lambdas used by `DynamicsCurve::paint()`.
- No DSP or UI-design changes from 0.2.0; this is a compile-fix release.

## 0.2.0
- Reworked the plug-in layout to follow the physical M32 channel-strip geography much more closely: CONFIG/PREAMP over GATE, EQUALISER over DYNAMICS.
- Replaced the four simultaneous EQ control columns with one hardware-style WIDTH / FREQUENCY / GAIN control stack and LOW / LO MID / HI MID / HIGH band-selection keys.
- Added a compressor transfer-curve display with threshold indication and live gain-reduction readout.
- Fixed the EQ response display so the dedicated 24 dB/oct LOW CUT is included in the plotted response even when the four-band EQ is bypassed.
- Cleaned numeric readouts so controls no longer show long floating-point values.

## 0.1.2
- Fixed first-open UI layout in REAPER: rotary controls, toggles and EQ mode selectors now receive their bounds after creation.
- The previous build could show only the panel backgrounds, EQ graph and meter because `setSize()` triggered `resized()` before the dynamically-created controls existed.
- Updated the Windows GitHub Actions workflow to the Windows 2022 runner, `actions/checkout@v5`, and `actions/upload-artifact@v7`.

## v0.1.1

- Fixed JUCE 9 CMake configuration by enabling both C and C++ languages (`LANGUAGES C CXX`).

# Changelog

## 0.1.0-prototype

- First C++ port of the Live32 channel-strip DSP.
- Gain/phase and dedicated 24 dB/oct LOW CUT.
- Gate with threshold/range/attack/hold/release.
- Four-band channel EQ with Live32 band modes.
- Compressor with threshold/ratio/attack/release/makeup/knee.
- Key filter and optional external sidechain.
- First M32-inspired LIVE32-branded GUI.
- Segmented amber encoder LED rings.
- EQ response display and input/output/GR/gate meters.
- CMake/JUCE VST3 + Standalone targets.
- GitHub Actions Windows VST3 build workflow.
- Framework-independent DSP smoke test.
