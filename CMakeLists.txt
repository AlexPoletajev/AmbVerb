cmake_minimum_required(VERSION 3.22)

project(AmbVerb VERSION 1.0.0 LANGUAGES C CXX)

if (APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

set(AMBVERB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/AmbVerb/Source")
set(AMBVERB_MATRIX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/AmbVerb/RotationMatrices")

option(AMBVERB_STEREO_COMPATIBILITY
    "Expose a stereo bus and encode/decode it around the 16-channel DSP core"
    OFF)
set(AMBVERB_PRODUCT_NAME "AmbVerb" CACHE STRING "Displayed plug-in product name")
set(AMBVERB_BUNDLE_ID "com.alexanderpoletajev.ambverb"
    CACHE STRING "Plug-in bundle identifier")
set(AMBVERB_PLUGIN_CODE "S0i0" CACHE STRING "Four-character plug-in code")

set(AMBVERB_FORMATS VST3 Standalone)

if (APPLE)
    list(APPEND AMBVERB_FORMATS AU)
elseif (UNIX)
    list(APPEND AMBVERB_FORMATS LV2)
endif()

juce_add_binary_data(AmbVerbData
    NAMESPACE BinaryData
    SOURCES
        "${AMBVERB_MATRIX_DIR}/Rxz3_1"
        "${AMBVERB_MATRIX_DIR}/Rxz3_2"
        "${AMBVERB_MATRIX_DIR}/Ryz3_1"
        "${AMBVERB_MATRIX_DIR}/Ryz3_2"
)

juce_add_plugin(AmbVerb
    COMPANY_NAME "Alexander Poletajev"
    BUNDLE_ID "${AMBVERB_BUNDLE_ID}"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE "${AMBVERB_PLUGIN_CODE}"
    LV2URI "https://github.com/AlexPoletajev/AmbVerb"
    FORMATS ${AMBVERB_FORMATS}
    PRODUCT_NAME "${AMBVERB_PRODUCT_NAME}"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Reverb
)

juce_generate_juce_header(AmbVerb)

target_sources(AmbVerb
    PRIVATE
        "${AMBVERB_SOURCE_DIR}/CompilationFlags.h"
        "${AMBVERB_SOURCE_DIR}/BufferStorage.hpp"
        "${AMBVERB_SOURCE_DIR}/EarlyReflections.cpp"
        "${AMBVERB_SOURCE_DIR}/EarlyReflections.hpp"
        "${AMBVERB_SOURCE_DIR}/FeedbackDelayNetwork.cpp"
        "${AMBVERB_SOURCE_DIR}/FeedbackDelayNetwork.hpp"
        "${AMBVERB_SOURCE_DIR}/PluginEditor.cpp"
        "${AMBVERB_SOURCE_DIR}/PluginEditor.h"
        "${AMBVERB_SOURCE_DIR}/PluginProcessor.cpp"
        "${AMBVERB_SOURCE_DIR}/PluginProcessor.h"
        "${AMBVERB_SOURCE_DIR}/PartitionedConvolution.hpp"
        "${AMBVERB_SOURCE_DIR}/PortableDsp.hpp"
)

target_compile_features(AmbVerb PRIVATE cxx_std_17)

target_compile_definitions(AmbVerb
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

if (AMBVERB_STEREO_COMPATIBILITY)
    target_compile_definitions(AmbVerb PUBLIC AMBVERB_STEREO_COMPATIBILITY=1)
endif()

target_link_libraries(AmbVerb
    PRIVATE
        AmbVerbData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if (AMBVERB_STEREO_COMPATIBILITY)
    juce_add_console_app(AmbVerbStereoSmoke
        PRODUCT_NAME "AmbVerb Stereo Smoke Test"
    )

    juce_generate_juce_header(AmbVerbStereoSmoke)

    target_sources(AmbVerbStereoSmoke
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/Tests/StereoCompatibilityMain.cpp"
    )

    target_include_directories(AmbVerbStereoSmoke
        PRIVATE
            "${AMBVERB_SOURCE_DIR}"
    )

    target_compile_features(AmbVerbStereoSmoke PRIVATE cxx_std_17)

    target_link_libraries(AmbVerbStereoSmoke
        PRIVATE
            AmbVerb
            AmbVerbData
            juce::juce_audio_utils
    )
endif()

include(CTest)

if (BUILD_TESTING)
    add_test(
        NAME AmbVerbRotationMatrixData
        COMMAND "${CMAKE_COMMAND}"
            "-DMATRIX_DIR=${AMBVERB_MATRIX_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/Tests/ValidateRotationMatrices.cmake"
    )

    juce_add_console_app(AmbVerbAudioRegression
        PRODUCT_NAME "AmbVerb Audio Regression"
    )

    juce_generate_juce_header(AmbVerbAudioRegression)

    target_sources(AmbVerbAudioRegression
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioRegressionMain.cpp"
    )

    target_include_directories(AmbVerbAudioRegression
        PRIVATE
            "${AMBVERB_SOURCE_DIR}"
    )

    target_compile_features(AmbVerbAudioRegression PRIVATE cxx_std_17)

    target_compile_definitions(AmbVerbAudioRegression
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
    )

    target_link_libraries(AmbVerbAudioRegression
        PRIVATE
            AmbVerb
            AmbVerbData
            juce::juce_audio_formats
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    juce_add_console_app(AmbVerbPartitionedConvolutionTest
        PRODUCT_NAME "AmbVerb Partitioned Convolution Test"
    )

    target_sources(AmbVerbPartitionedConvolutionTest
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/Tests/PartitionedConvolutionMain.cpp"
    )

    target_include_directories(AmbVerbPartitionedConvolutionTest
        PRIVATE
            "${AMBVERB_SOURCE_DIR}"
    )

    target_compile_features(AmbVerbPartitionedConvolutionTest PRIVATE cxx_std_17)

    target_compile_definitions(AmbVerbPartitionedConvolutionTest
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
    )

    target_link_libraries(AmbVerbPartitionedConvolutionTest
        PRIVATE
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    add_test(
        NAME AmbVerbPartitionedConvolution
        COMMAND AmbVerbPartitionedConvolutionTest
    )

    juce_add_console_app(AmbVerbDspBenchmark
        PRODUCT_NAME "AmbVerb DSP Benchmark"
    )

    juce_generate_juce_header(AmbVerbDspBenchmark)

    target_sources(AmbVerbDspBenchmark
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/Tests/DspBenchmarkMain.cpp"
    )

    target_include_directories(AmbVerbDspBenchmark
        PRIVATE
            "${AMBVERB_SOURCE_DIR}"
    )

    target_compile_features(AmbVerbDspBenchmark PRIVATE cxx_std_17)

    target_compile_definitions(AmbVerbDspBenchmark
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
    )

    target_link_libraries(AmbVerbDspBenchmark
        PRIVATE
            AmbVerb
            AmbVerbData
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    set(AMBVERB_BENCHMARK_ARTIFACT_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/DspBenchmark")

    add_custom_target(AmbVerbRunDspBenchmark
        COMMAND "$<TARGET_FILE:AmbVerbDspBenchmark>"
            --json "${AMBVERB_BENCHMARK_ARTIFACT_DIR}/benchmark-$<CONFIG>.json"
        DEPENDS AmbVerbDspBenchmark
        USES_TERMINAL
        COMMENT "Running AmbVerb DSP benchmark"
    )

    set(AMBVERB_AUDIO_REFERENCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/Tests/ReferenceAudio")
    set(AMBVERB_AUDIO_ARTIFACT_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/AudioRegression")

    add_custom_target(AmbVerbCaptureAudioReference
        COMMAND "$<TARGET_FILE:AmbVerbAudioRegression>"
            --capture "${AMBVERB_AUDIO_REFERENCE_DIR}"
        DEPENDS AmbVerbAudioRegression
        USES_TERMINAL
        COMMENT "Capturing AmbVerb audio regression reference WAV files"
    )

    if (EXISTS "${AMBVERB_AUDIO_REFERENCE_DIR}/manifest.json")
        add_test(
            NAME AmbVerbAudioRegressionTest
            COMMAND AmbVerbAudioRegression
                --compare "${AMBVERB_AUDIO_REFERENCE_DIR}"
                --artifacts "${AMBVERB_AUDIO_ARTIFACT_DIR}/current"
        )
    else()
        add_test(
            NAME AmbVerbAudioBaselineCandidate
            COMMAND AmbVerbAudioRegression
                --capture "${AMBVERB_AUDIO_ARTIFACT_DIR}/baseline-candidate"
        )
    endif()
endif()
