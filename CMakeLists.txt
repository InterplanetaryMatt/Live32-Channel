cmake_minimum_required(VERSION 3.22)

project(Live32Channel VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# JUCE is deliberately pinned so a future framework update cannot silently
# change the build. JUCE is dual-licensed AGPLv3/commercial; see LICENSE.md.
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Live32Channel
    COMPANY_NAME "InterplanetaryMatt"
    BUNDLE_ID "com.interplanetarymatt.live32channel"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE IMat
    PLUGIN_CODE L32C
    FORMATS VST3 Standalone
    PRODUCT_NAME "Live32 Channel"
    VST3_CATEGORIES Fx Dynamics EQ
)

juce_generate_juce_header(Live32Channel)

target_sources(Live32Channel PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/DSP/Live32ChannelDSP.cpp
    Source/DSP/Live32ChannelDSP.h
    Source/UI/Live32LookAndFeel.cpp
    Source/UI/Live32LookAndFeel.h
    Source/UI/EQCurve.cpp
    Source/UI/EQCurve.h
)

target_include_directories(Live32Channel PRIVATE Source)

target_compile_definitions(Live32Channel PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Live32Channel PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Framework-independent DSP smoke test. This can be built without JUCE using
# tests/build_dsp_test.* and is useful for checking the JSFX-to-C++ port.
add_executable(Live32DSPTest
    tests/dsp_smoke.cpp
    Source/DSP/Live32ChannelDSP.cpp
)
target_include_directories(Live32DSPTest PRIVATE Source/DSP)
