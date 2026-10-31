cmake_minimum_required(VERSION 3.22)

project(OrbitDAW VERSION 0.3.255 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
endif()

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
    BUILD_VERSION "276"
    COMPANY_NAME "Touch The Universe"
    BUNDLE_ID "${ORBIT_APP_BUNDLE_ID}"
    PLUGINHOST_AU ${ORBIT_HOST_AU})

# Pinned MIT Open303 DSP only; no VST2 SDK or upstream plugin shell.
file(GLOB ORBIT_ACID_DSP CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Synth/ThirdParty/Open303/*.cpp")
add_library(orbit_acid_dsp STATIC Source/Synth/AcidBassVoice.cpp ${ORBIT_ACID_DSP})
target_compile_features(orbit_acid_dsp PRIVATE cxx_std_20)
target_link_libraries(Orbit PRIVATE orbit_acid_dsp)

# Geonkick DSP audition, with exclusively owned voices (no upstream UI/worker).
set(ORBIT_GEONKICK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/Synth/ThirdParty/Geonkick")
add_library(orbit_geonkick_dsp STATIC Source/Synth/GeonkickVoice.c
    "${ORBIT_GEONKICK_DIR}/oscillator.c" "${ORBIT_GEONKICK_DIR}/envelope.c"
    "${ORBIT_GEONKICK_DIR}/filter.c" "${ORBIT_GEONKICK_DIR}/distortion.c"
    "${ORBIT_GEONKICK_DIR}/gkick_buffer.c")
set_target_properties(orbit_geonkick_dsp PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
target_include_directories(orbit_geonkick_dsp PRIVATE "${ORBIT_GEONKICK_DIR}/quamplex_dsp_tools")
target_compile_definitions(orbit_geonkick_dsp PRIVATE ORBIT_GEONKICK_SINGLE_OWNER=1
    GEONKICK_VERSION_HEX=0x030900 GEONKICK_VERSION_STR="3.9.0")
if(UNIX AND NOT APPLE)
    target_link_libraries(orbit_geonkick_dsp PRIVATE m)
endif()
target_link_libraries(Orbit PRIVATE orbit_geonkick_dsp)

# Modified 9W9 port. GPL-3.0 and additional terms in the pinned NOTICE.
set(ORBIT_NINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/Synth/ThirdParty/9W9")
add_library(orbit_nine_dsp STATIC Source/Synth/NineKitVoice.c "${ORBIT_NINE_DIR}/9w9_engine.c")
set_target_properties(orbit_nine_dsp PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(UNIX AND NOT APPLE)
    target_link_libraries(orbit_nine_dsp PRIVATE m)
endif()
target_link_libraries(Orbit PRIVATE orbit_nine_dsp)

# Carry source notices with standalone app outputs on every platform.
set(ORBIT_OPEN303_NOTICE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/Synth/ThirdParty/Open303")
if(APPLE)
    set(ORBIT_NOTICE_OUTPUT "$<TARGET_BUNDLE_CONTENT_DIR:Orbit>/Resources/Third-party notices/Open303")
else()
    set(ORBIT_NOTICE_OUTPUT "$<TARGET_FILE_DIR:Orbit>/Third-party notices/Open303")
endif()
add_custom_command(TARGET Orbit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ORBIT_NOTICE_OUTPUT}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ORBIT_OPEN303_NOTICE_DIR}/License.txt"
        "${ORBIT_OPEN303_NOTICE_DIR}/OOURA-LICENSE.txt"
        "${ORBIT_OPEN303_NOTICE_DIR}/UPSTREAM.md" "${ORBIT_NOTICE_OUTPUT}"
    VERBATIM)
add_custom_command(TARGET Orbit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ORBIT_NOTICE_OUTPUT}/../Geonkick"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ORBIT_GEONKICK_DIR}/LICENSE" "${ORBIT_GEONKICK_DIR}/UPSTREAM.md"
        "${ORBIT_NOTICE_OUTPUT}/../Geonkick"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ORBIT_GEONKICK_DIR}/quamplex_dsp_tools/LICENSE"
        "${ORBIT_NOTICE_OUTPUT}/../Geonkick/QUAMPLEX-LICENSE"
    VERBATIM)
add_custom_command(TARGET Orbit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ORBIT_NOTICE_OUTPUT}/../9W9"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ORBIT_NINE_DIR}/LICENSE" "${ORBIT_NINE_DIR}/NOTICE" "${ORBIT_NINE_DIR}/UPSTREAM.md"
        "${ORBIT_NOTICE_OUTPUT}/../9W9"
    VERBATIM)

juce_generate_juce_header(Orbit)

if(APPLE)
    target_sources(Orbit PRIVATE Source/Generator/NativeViewClip.mm)
    target_link_libraries(Orbit PRIVATE "-framework QuartzCore")
endif()

target_sources(Orbit PRIVATE
    Source/Generator/GeneratorProcessor.h
    Source/Workspace/WorkspaceWindow.h
    Source/Automation/LfoTool.h
    Source/Generator/GeneratorControlWindow.h
    Source/Mixing/LookaheadLimiter.h
    Source/Mixing/LookaheadLimiterPlugin.h
    Source/Mixing/PeakCompressor.h
    Source/Mixing/PeakCompressorPlugin.h
    Source/Mixing/DistortionCore.h
    Source/Mixing/DistortionPlugin.h
    Source/Generator/GeneratorControlHostMethods.h
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
    Source/Automation/AutomationTouchListener.h
    Source/Automation/ParameterEventSlider.h
    Source/Automation/AutomationProjection.h
    Source/Automation/SongEventEditor.h
    Source/Mixing/EqSaturationPlugin.h
    Source/Mixing/EqSaturationHostMethods.h
    Source/Mixing/OutputGatePlugin.h
    Source/Mixing/EffectHostMethods.h
    Source/Mixing/MasterMuteHostMethods.h
    Source/Mixing/SidechainHostMethods.h
    Source/Mixing/AudioRoutingHostMethods.h
    Source/Mixing/MixerHostMethods.h
    Source/Automation/SongEventHostMethods.h
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
    Source/Project/PluginCatalog.h
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
    MACOSX_BUNDLE_BUNDLE_VERSION "276"
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
