cmake_minimum_required(VERSION 3.22)

project(OrbitDAW VERSION 0.3.79 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/tracktion_engine")
if (NOT EXISTS "${ENGINE_ROOT}/modules/juce/CMakeLists.txt")
    message(FATAL_ERROR
        "Tracktion Engine/JUCE dependencies are missing. See README.md for setup.")
endif()

add_subdirectory("${ENGINE_ROOT}/modules/juce" "${CMAKE_BINARY_DIR}/juce")
add_subdirectory("${ENGINE_ROOT}/modules" "${CMAKE_BINARY_DIR}/tracktion_modules")

juce_add_gui_app(Orbit
    PRODUCT_NAME "Orbit"
    VERSION "${PROJECT_VERSION}"
    BUILD_VERSION "100"
    COMPANY_NAME "Touch The Universe"
    BUNDLE_ID "com.touchtheuniverse.orbit"
    PLUGINHOST_AU TRUE)

juce_generate_juce_header(Orbit)

target_sources(Orbit PRIVATE
    Source/Main.cpp
    Source/Project/ProjectDocument.cpp
    Source/Project/ProjectDocument.h)

target_compile_features(Orbit PRIVATE cxx_std_20)

target_compile_definitions(Orbit PRIVATE
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_MODAL_LOOPS_PERMITTED=1)

target_link_libraries(Orbit PRIVATE
    tracktion::tracktion_core
    tracktion::tracktion_engine
    tracktion::tracktion_graph
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

set_target_properties(Orbit PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "100"
    CXX_VISIBILITY_PRESET hidden
    C_VISIBILITY_PRESET hidden)
