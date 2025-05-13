cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "Runtime Library")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "Build for 10.13")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_STANDALONE "Build Standalone plugin format" ON) # Allow overriding from the command line

project(GATE12 VERSION 0.4.1)

add_compile_definitions(PROJECT_VERSION="${PROJECT_VERSION}")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_XCODE_GENERATE_SCHEME OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(libs/JUCE)

set(plugin_formats
    VST3
    LV2
    AU
)

if(BUILD_STANDALONE)
    list(APPEND plugin_formats Standalone)
endif()

juce_add_plugin(${PROJECT_NAME}
        COMPANY_NAME Tilr
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        JUCE_VST3_CAN_REPLACE_VST2 FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE Tilr
        PLUGIN_CODE gt12
        FORMATS ${plugin_formats}
        PRODUCT_NAME "GATE-12"
        BUNDLE_ID "com.tilr.gate12"
        LV2URI "https://github.com/tiagolr/gate12"
)

file(GLOB_RECURSE src
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

source_group(Source
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PluginProcessor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PluginProcessor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PluginEditor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/PluginEditor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Globals.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Presets.h
)

source_group(Source\\ui
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/Rotary.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/Rotary.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/CustomLookAndFeel.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/CustomLookAndFeel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/GridSelector.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/GridSelector.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/MultiSelect.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/MultiSelect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/View.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/View.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/ui/About.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/About.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/SettingsButton.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/SettingsButton.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/AudioDisplay.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/AudioDisplay.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/PaintTool.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/PaintTool.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/PaintToolWidget.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/PaintToolWidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/TextDial.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/TextDial.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/Sequencer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/Sequencer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/SequencerWidget.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/SequencerWidget.cpp
)

source_group(Source\\dsp
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/Pattern.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/Pattern.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/Filter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/Filter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/Transient.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/Transient.cpp
)

# Make the SourceFiles buildable
target_sources(${PROJECT_NAME} PRIVATE ${src})

# These are some toggleable options from the JUCE CMake API
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# JUCE libraries to bring into our project
juce_generate_juce_header(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_core
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

file(GLOB_RECURSE res "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.*")
juce_add_binary_data(${PROJECT_NAME}_res SOURCES ${res})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_res)

if(APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC JUCE_AU=1)
endif()
