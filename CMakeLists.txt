cmake_minimum_required(VERSION 3.22)
project(MasteringCompressor VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Statically embed MinGW runtimes into every binary so the VST3/exe have
# zero external MinGW DLL dependencies (required for FL Studio and other
# DAWs that don't add the plugin directory to the Windows DLL search path).
if(WIN32)
    add_link_options(
        -static-libgcc
        -static-libstdc++
    )
endif()

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MasteringCompressor
    COMPANY_NAME                "Jtekkk"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    Jtkk
    PLUGIN_CODE                 Mcmp
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "Mastering Compressor"
)

target_sources(MasteringCompressor PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(MasteringCompressor PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(MasteringCompressor PRIVATE Source)

juce_generate_juce_header(MasteringCompressor)

target_link_libraries(MasteringCompressor
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
