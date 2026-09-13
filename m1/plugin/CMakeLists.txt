cmake_minimum_required(VERSION 3.22)

project(OpenChordMCore VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Plugin product name is M Core. Hardware box is OpenChord M1 (separate).
juce_add_plugin(OpenChordMCore
    COMPANY_NAME "OpenChord"
    COMPANY_WEBSITE "https://github.com/openchord"
    COMPANY_EMAIL ""
    BUNDLE_ID com.openchord.mcore
    PLUGIN_MANUFACTURER_CODE OpCh
    PLUGIN_CODE OcMC
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "OpenChord M Core"
    DESCRIPTION "MIDI chord brain — Type mode (same engine as OpenChord M1 hardware)"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    AU_MAIN_TYPE kAudioUnitType_MIDIProcessor
    VST3_CATEGORIES "Instrument|Generator"
    VERSION ${PROJECT_VERSION}
)

juce_generate_juce_header(OpenChordMCore)

target_sources(OpenChordMCore
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/MidiMap.cpp
        ../engine/chord_engine.cpp
        ../engine/session.cpp
)

target_include_directories(OpenChordMCore
    PRIVATE
        Source
        ../engine
)

target_compile_definitions(OpenChordMCore
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(OpenChordMCore
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
