cmake_minimum_required(VERSION 3.22)

project(OrbitDAW VERSION 0.3.217 LANGUAGES C CXX)

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

# The default remains the canonical product identity. A distinct identity lets
# local validation run beside an existing Orbit without targeting its windows.
set(ORBIT_APP_BUNDLE_ID "com.touchtheuniverse.orbit" CACHE STRING "Orbit application bundle identifier")
set(ORBIT_HOST_AU FALSE)
if(APPLE)
    set(ORBIT_HOST_AU TRUE)
endif()

juce_add_gui_app(Orbit
    PRODUCT_NAME "Orbit"
    VERSION "${PROJECT_VERSION}"
    BUILD_VERSION "238"
    COMPANY_NAME "Touch The Universe"
    BUNDLE_ID "${ORBIT_APP_BUNDLE_ID}"
    PLUGINHOST_AU ${ORBIT_HOST_AU})

juce_generate_juce_header(Orbit)

target_sources(Orbit PRIVATE
    Source/Generator/GeneratorProcessor.h
    Source/Generator/GeneratorMidiPlugin.h
    Source/Generator/ArpeggiatorPanel.h
    Source/Generator/GeneratorHostMethods.h
    Source/Recording/MidiTake.h
    Source/Recording/RecordingHostMethods.h
    Source/Options/OptionsPanel.h
    Source/Options/Appearance.h
    Source/Options/MidiInbox.h
    Source/Options/VirtualKeyboard.h
    Source/Options/WallpaperLibrary.h
    Source/Main.cpp
    Source/Editing/PatternTools.h
    Source/Editing/EditToolPanel.h
    Source/Import/OrionSfsReader.cpp
    Source/Import/OrionSfsReader.h
    Source/Import/WaveMediaInfo.cpp
    Source/Import/WaveMediaInfo.h
    Source/Import/OrionSfsReport.cpp
    Source/Import/OrionSfsReport.h
    Source/Import/OrionSfsImportDialog.cpp
    Source/Import/OrionSfsImportDialog.h
    Source/Import/OrionSfsConversion.cpp
    Source/Import/OrionSfsConversion.h
    Source/Import/ProjectImportPlan.h
    Source/Import/ProjectImportPlan.cpp
    Source/Import/ProjectImportAdapter.cpp
    Source/Import/ProjectImportAdapter.h
    Source/Import/ProjectImportSessionAdapter.cpp
    Source/Import/ProjectImportSessionAdapter.h
    Source/Import/OrionSfsImportModel.h
    Source/Project/ProjectDocument.cpp
    Source/Project/ProjectDocument.h)

target_compile_features(Orbit PRIVATE cxx_std_20)

target_compile_definitions(Orbit PRIVATE
    JUCE_PLUGINHOST_AU=$<BOOL:${ORBIT_HOST_AU}>
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
    MACOSX_BUNDLE_BUNDLE_VERSION "238"
    CXX_VISIBILITY_PRESET hidden
    C_VISIBILITY_PRESET hidden)

# Explicit opt-in developer utility; never built as part of an Orbit product
# build, and never packaged as a product binary by the source archive tool.
option(ORBIT_BUILD_SFS_TOOL "Enable standalone experimental SFS preview utility" OFF)
if (ORBIT_BUILD_SFS_TOOL)
    juce_add_console_app(OrbitSfsPreview PRODUCT_NAME "Orbit SFS preview utility")
    set_target_properties(OrbitSfsPreview PROPERTIES EXCLUDE_FROM_ALL TRUE)
    juce_generate_juce_header(OrbitSfsPreview)
    target_sources(OrbitSfsPreview PRIVATE
        Tools/preview-orion-sfs.cpp
        Source/Import/OrionSfsReader.cpp
        Source/Import/WaveMediaInfo.cpp
        Source/Import/OrionSfsReport.cpp
        Source/Import/OrionSfsConversion.cpp
        Source/Import/ProjectImportAdapter.cpp
        Source/Import/ProjectImportPlan.cpp
        Source/Project/ProjectDocument.cpp)
    target_compile_definitions(OrbitSfsPreview PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
    target_link_libraries(OrbitSfsPreview PRIVATE
        juce::juce_data_structures juce::juce_graphics
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
