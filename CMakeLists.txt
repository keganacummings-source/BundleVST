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

# WebView2 SDK (Windows) — required for JUCE_USE_WIN_WEBVIEW2
# Prefer pre-extracted third_party/webview2; otherwise download NuGet at configure time.
set(WEBVIEW2_ROOT "${CMAKE_SOURCE_DIR}/third_party/webview2/build/native")
if(WIN32)
  if(NOT EXISTS "${WEBVIEW2_ROOT}/include/WebView2.h")
    message(STATUS "WebView2 headers missing — downloading Microsoft.Web.WebView2 from NuGet…")
    set(_wv2_ver "1.0.2849.39")
    set(_wv2_zip "${CMAKE_BINARY_DIR}/Microsoft.Web.WebView2.nupkg")
    set(_wv2_url "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${_wv2_ver}")
    file(DOWNLOAD "${_wv2_url}" "${_wv2_zip}" SHOW_PROGRESS TLS_VERIFY ON STATUS _wv2_st)
    list(GET _wv2_st 0 _wv2_code)
    if(NOT _wv2_code EQUAL 0)
      message(FATAL_ERROR "Failed to download WebView2 SDK: ${_wv2_st}")
    endif()
    set(_wv2_extract "${CMAKE_BINARY_DIR}/webview2_extract")
    file(REMOVE_RECURSE "${_wv2_extract}")
    file(MAKE_DIRECTORY "${_wv2_extract}")
    execute_process(
      COMMAND "${CMAKE_COMMAND}" -E tar xf "${_wv2_zip}"
      WORKING_DIRECTORY "${_wv2_extract}"
      RESULT_VARIABLE _wv2_untar)
    if(NOT _wv2_untar EQUAL 0)
      message(FATAL_ERROR "Failed to extract WebView2 nupkg")
    endif()
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/third_party/webview2")
    if(EXISTS "${_wv2_extract}/build/native/include/WebView2.h")
      file(COPY "${_wv2_extract}/build" DESTINATION "${CMAKE_SOURCE_DIR}/third_party/webview2")
    else()
      message(FATAL_ERROR "Unexpected WebView2 nupkg layout under ${_wv2_extract}")
    endif()
  endif()
  if(NOT EXISTS "${WEBVIEW2_ROOT}/include/WebView2.h")
    message(FATAL_ERROR "WebView2.h not found at ${WEBVIEW2_ROOT}/include/WebView2.h")
  endif()
  message(STATUS "WebView2 SDK: ${WEBVIEW2_ROOT}")
endif()

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

# Generates JuceHeader.h into the build tree and adds its include path
juce_generate_juce_header(DreamDAW)

target_sources(DreamDAW PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_compile_definitions(DreamDAW
  PUBLIC
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

if(WIN32)
  # Global so amalgamated juce_gui_extra.cpp sees WebView2.h
  include_directories("${WEBVIEW2_ROOT}/include")
  target_include_directories(DreamDAW PRIVATE "${WEBVIEW2_ROOT}/include")
  if(EXISTS "${WEBVIEW2_ROOT}/x64/WebView2LoaderStatic.lib")
    target_link_libraries(DreamDAW PRIVATE "${WEBVIEW2_ROOT}/x64/WebView2LoaderStatic.lib")
  elseif(EXISTS "${WEBVIEW2_ROOT}/lib/x64/WebView2LoaderStatic.lib")
    target_link_libraries(DreamDAW PRIVATE "${WEBVIEW2_ROOT}/lib/x64/WebView2LoaderStatic.lib")
  endif()
  target_link_libraries(DreamDAW PRIVATE shlwapi version)
endif()

target_link_libraries(DreamDAW
  PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_extra
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
