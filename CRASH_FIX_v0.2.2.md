# v0.2.2 crash fix

v0.2.0/v0.2.1 added dynamically-created EQ band selector buttons. The editor
enabled JUCE host resizing before those buttons had been created, while the new
`resized()` implementation dereferenced all four button pointers unconditionally.

Hosts are allowed to trigger `resized()` during `setResizable()` /
`setResizeLimits()`. In REAPER this could therefore dereference a null button
pointer while the plug-in editor was being opened.

v0.2.2:
- creates all dynamic controls before enabling host resizing;
- guards EQ band button access in `resized()` and `selectEqBand()`;
- avoids forcing an early layout before the editor has a valid size.

DSP is unchanged from v0.2.1.
