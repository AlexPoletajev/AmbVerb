# AmbVerb

AmbVerb is a third-order Ambisonics reverb plug-in built with JUCE. It combines
synthesised early reflections with a 16-line feedback delay network (FDN).

The active implementation is currently fixed to ACN third-order Ambisonics:
16 input channels and 16 output channels. Stereo and other Ambisonics orders
are rejected deliberately so that a host cannot select a layout that the DSP
buffers do not support.

## Build

The DSP currently uses Apple Accelerate/vDSP, so this revision builds on macOS
only. The CMake project downloads the pinned JUCE 8.0.15 dependency during its
first configure step.

Requirements:

- macOS with Xcode command-line tools
- CMake 3.22 or newer
- Git

```sh
cmake -S . -B Build -DBUILD_TESTING=ON
cmake --build Build --config Release \
  --target AmbVerb_Standalone AmbVerb_VST3 AmbVerb_AU
ctest --test-dir Build -C Release --output-on-failure
```

Generated plug-in and application artifacts are placed below `Build/`.

## Project structure

- `AmbVerb/Source/PluginProcessor.*`: JUCE processor, parameters and state
- `AmbVerb/Source/EarlyReflections.*`: early-reflection matrix convolution
- `AmbVerb/Source/FeedbackDelayNetwork.*`: late-reverb FDN
- `AmbVerb/RotationMatrices/`: source matrices; order-3 data is embedded in the binary
- `Tests/`: build-time validation for required matrix data
- `Old Project Files/`: historical reference only; not part of the build

## Current limits

- Supported sample rates: 8 kHz through 96 kHz
- Maximum host block size depends on the delay-buffer headroom and is validated
  in `prepareToPlay`
- Plug-in formats: AU, VST3 and Standalone
- Platform: macOS

Structural parameters are calculated away from the audio callback. Completed
rotation matrices and FDN impulse responses are activated with non-blocking
buffer swaps at the start of an audio block.

## Audio regression baseline

`AmbVerbAudioRegression` renders deterministic 16-channel test signals directly
through the processor. Its four cases isolate the direct, early-reflection and
late-reverb paths and include different sample rates, block sizes, room sizes,
T60 values and input channels.

Before the Accelerate/vDSP portability refactor, capture the reference WAVs on
macOS:

```sh
cmake --build Build --config Release --target AmbVerbCaptureAudioReference
```

The files are 16-channel, 32-bit floating-point WAVs. After `manifest.json` and
the WAV files have been committed and CMake has been configured again, the
regular CTest run compares every output sample with absolute and relative
tolerances and saves mismatching renders below `Build/AudioRegression/current`
for inspection. Until then, macOS CI publishes a baseline candidate as a
downloadable workflow artifact.

## Next DSP work

Capture and commit the audio baseline first. The portability pass can then
replace Accelerate calls behind JUCE DSP primitives while continuously checking
signal-path parity. Ring buffers and further FFT caching should follow with the
same regression suite in place.
