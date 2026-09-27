# macOS VST3 builds

The GitHub workflow `.github/workflows/build-macos-vst3.yml` builds:

1. Apple Silicon (`arm64`) on `macos-15`
2. Intel (`x86_64`) on `macos-15-intel`
3. Universal (`arm64 + x86_64`) on `macos-15`

The Universal configuration follows JUCE's documented CMake approach using:

`-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64`

The workflow verifies the resulting Mach-O architecture with `lipo -archs`
before creating the downloadable artifact.
