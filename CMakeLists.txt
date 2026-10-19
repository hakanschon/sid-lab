cmake_minimum_required(VERSION 3.25)
project(SIDLab VERSION 1.5.2 LANGUAGES C CXX OBJC OBJCXX)
if(NOT APPLE)
    message(FATAL_ERROR "SID Lab targets macOS arm64. Use Xcode command line tools on macOS.")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "Native Apple Silicon")
set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "Minimum macOS")
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify_dependencies.py"
    RESULT_VARIABLE deps_result OUTPUT_VARIABLE deps_output ERROR_VARIABLE deps_error)
if(NOT deps_result EQUAL 0)
    message(FATAL_ERROR "Dependency verification failed: ${deps_output}${deps_error}")
endif()
# Keep local checkout paths out of diagnostics, __FILE__ and debug information
# embedded in distributable binaries, including third-party targets.
add_compile_options("-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=."
    "-fdebug-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=.")
include(cmake/Libresidfp.cmake)
add_library(sid_engine STATIC source/Parameters.cpp source/SidEngine.cpp source/Randomizer.cpp source/Drums.cpp source/DiscoPatches.cpp source/SynthpopPatches.cpp)
target_include_directories(sid_engine PUBLIC source)
target_link_libraries(sid_engine PUBLIC residfp)
set_target_properties(sid_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_executable(engine_probe tests/EngineProbe.cpp)
target_link_libraries(engine_probe PRIVATE sid_engine)
option(SIDLAB_ENGINE_ONLY "Build just the authentic SID proof renderer" OFF)
if(SIDLAB_ENGINE_ONLY)
    return()
endif()
add_subdirectory(third_party/JUCE EXCLUDE_FROM_ALL)
juce_add_plugin(SIDLab
    COMPANY_NAME "SID Lab" BUNDLE_ID "org.sidlab.instrument"
    PLUGIN_MANUFACTURER_CODE Sdlb PLUGIN_CODE Sidl
    PRODUCT_NAME "SID Lab" DESCRIPTION "Authentic SID music instrument"
    IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE COPY_PLUGIN_AFTER_BUILD FALSE
    AU_SANDBOX_SAFE TRUE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS AU Standalone
    NEEDS_WEB_BROWSER FALSE NEEDS_CURL FALSE
    MICROPHONE_PERMISSION_ENABLED FALSE)
target_sources(SIDLab PRIVATE source/PluginProcessor.cpp source/PluginEditor.cpp source/Presets.cpp source/AudioDisplay.cpp)
target_compile_definitions(SIDLab PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_USE_FLAC=0 JUCE_USE_OGGVORBIS=0
    JUCE_MODAL_LOOPS_PERMITTED=0)
target_link_libraries(SIDLab PRIVATE sid_engine juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags)
# Carry the original copyright/license notices with both binary formats.
set(SIDLAB_LEGAL "${CMAKE_CURRENT_BINARY_DIR}/Legal")
file(MAKE_DIRECTORY "${SIDLAB_LEGAL}")
foreach(notice LICENSE LICENSES.md CREDITS.md THIRD_PARTY_NOTICES.md dependencies.lock)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${notice}" "${SIDLAB_LEGAL}/${notice}" COPYONLY)
endforeach()
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/licenses" DESTINATION "${SIDLAB_LEGAL}")
juce_add_bundle_resources_directory(SIDLab_AU "${SIDLAB_LEGAL}")
juce_add_bundle_resources_directory(SIDLab_Standalone "${SIDLAB_LEGAL}")
include(CTest)
if(BUILD_TESTING)
    add_executable(sid_tests tests/TestMain.cpp tests/AllocationGuard.cpp)
    target_link_libraries(sid_tests PRIVATE SIDLab sid_engine juce::juce_audio_utils juce::juce_dsp)
    target_include_directories(sid_tests PRIVATE source)
    target_compile_definitions(sid_tests PRIVATE SIDLAB_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME sid_lab_verification COMMAND sid_tests "${CMAKE_CURRENT_SOURCE_DIR}/evidence")
    set_tests_properties(sid_lab_verification PROPERTIES TIMEOUT 240)
endif()
