cmake_minimum_required(VERSION 3.22)
project(DreamDAW VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG        8.0.6
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(DreamDAW
  COMPANY_NAME             "Dreamdaw"
  PLUGIN_MANUFACTURER_CODE Drmd
  PLUGIN_CODE              Ddw1
  FORMATS                  VST3
  PRODUCT_NAME             "DREAMDAW"
  VST3_CATEGORIES          Instrument Synth
  IS_SYNTH                 TRUE
  NEEDS_MIDI_INPUT         TRUE
  NEEDS_MIDI_OUTPUT        FALSE
  IS_MIDI_EFFECT           FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  NEEDS_CURL               FALSE
  COPY_PLUGIN_AFTER_BUILD  FALSE)

target_sources(DreamDAW PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_compile_definitions(DreamDAW PUBLIC
  JUCE_WEB_BROWSER=1
  JUCE_USE_WIN_WEBVIEW2=1
  JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0
  JUCE_STRICT_REFCOUNTEDPOINTER=1)

if(MSVC)
  target_compile_options(DreamDAW PRIVATE /utf-8)
endif()

target_link_libraries(DreamDAW
  PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_extra
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
