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
    DESCRIPTION "MIDI chord brain — Keys and Scale modes (same engine as OpenChord M1 hardware)"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
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
        ../firmware/input.cpp
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

# iCloud/Documents builds sometimes drop Info.plist from the bundle, which macOS
# reports as "damaged". Copy plists + ad-hoc sign + install ourselves.
set(_oc_art "${CMAKE_CURRENT_BINARY_DIR}/OpenChordMCore_artefacts/$<CONFIG>")
set(_oc_code "${CMAKE_CURRENT_BINARY_DIR}/OpenChordMCore_artefacts/JuceLibraryCode")

foreach(_fmt IN ITEMS VST3 AU)
    if(_fmt STREQUAL "VST3")
        set(_bundle "${_oc_art}/VST3/OpenChord M Core.vst3")
        set(_plist "${_oc_code}/OpenChordMCore_VST3/Info.plist")
        set(_dest "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/OpenChord M Core.vst3")
    else()
        set(_bundle "${_oc_art}/AU/OpenChord M Core.component")
        set(_plist "${_oc_code}/OpenChordMCore_AU/Info.plist")
        set(_dest "$ENV{HOME}/Library/Audio/Plug-Ins/Components/OpenChord M Core.component")
    endif()

    add_custom_command(TARGET OpenChordMCore_${_fmt} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_bundle}/Contents"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_plist}" "${_bundle}/Contents/Info.plist"
        COMMAND xattr -cr "${_bundle}"
        COMMAND codesign --force --deep --sign - "${_bundle}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${_dest}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${_bundle}" "${_dest}"
        COMMAND xattr -cr "${_dest}"
        COMMAND codesign --force --deep --sign - "${_dest}"
        COMMENT "Fix Info.plist, sign, and install OpenChord M Core ${_fmt}"
        VERBATIM
    )
endforeach()
