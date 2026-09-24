# Live32 Channel v0.3.1 UI fixes

This revision responds to visual issues found in v0.3.0:

- All knob values are rendered by Live32-owned labels, rather than JUCE's parameter text, so logarithmic values no longer expose raw floating-point precision.
- EQ WIDTH, FREQUENCY and GAIN controls are vertically separated and the EQ button now sits with MODE on the left of the EQ section.
- Phase invert uses the requested Φ legend.
- The dynamics LCD always displays the configured compressor law. The bend shows ratio; a non-zero KNEE produces a smooth transition bounded by two subtle vertical markers; a square marker shows threshold. COMP bypass simply dims the curve instead of replacing it with a unity line.

DSP is unchanged from v0.3.0.
