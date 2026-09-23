# JSFX -> C++ port notes

The C++ DSP in `Source/DSP/Live32ChannelDSP.*` is a direct behavioural port of the current Live32 channel strip rather than a redesign.

## Deliberately retained details

- 24 dB/oct LOW CUT: two cascaded second-order high-pass sections, Q = 0.5411961001 and 1.3065629649.
- Gate detector uses the larger absolute value of L/R.
- Gate envelope and gate-gain transition use the same attack/release coefficient style as the JSFX.
- Gate hold is sample-count based.
- VEQ is the same broad-bell approximation: `max(0.45, Q * 0.55)`.
- Compressor is stereo-linked using the larger L/R detector value.
- Compressor knee control 0..5 is converted to 0..15 dB using `knee * 3`.
- External sidechain is represented by an optional VST3 sidechain bus.
- Key filter is the same first-order high-pass detector filter.
- Compressor gain change uses attack when gain is reducing and release when recovering.

## Known semantic oddity retained for parity

The present JSFX applies the compressor makeup gain even when the compressor is switched off. The C++ prototype currently retains this so measurements against Live32 are meaningful. We can decide whether to change both implementations together later.
