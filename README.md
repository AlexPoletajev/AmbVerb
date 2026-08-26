# AmbVerb

AmbVerb is a third-order Ambisonics reverb plug-in built with JUCE. It combines
synthesised early reflections with a 16-line feedback delay network (FDN).

The native implementation is fixed to ACN third-order Ambisonics: 16 input
channels and 16 output channels. An optional compatibility build exposes a
stereo bus and encodes/decodes it around the unchanged 16-channel DSP core.

## Build

The DSP uses JUCE's cross-platform FFT and vector primitives. JUCE selects its
native vDSP backend on Apple systems and portable backends on Windows and Linux.
The CMake project downloads the pinned JUCE 8.0.15 dependency during its first
configure step.

Requirements:

- A supported macOS, Windows or Linux C++17 toolchain
- CMake 3.22 or newer
- Git

```sh
cmake -S . -B Build -DBUILD_TESTING=ON
cmake --build Build --config Release --parallel 3
ctest --test-dir Build -C Release --output-on-failure
```

Generated formats are AU/VST3/Standalone on macOS, VST3/Standalone on Windows,
and LV2/VST3/Standalone on Linux. To build the stereo compatibility variant in
a separate directory, add `-DAMBVERB_STEREO_COMPATIBILITY=ON` when configuring.

Generated plug-in and application artifacts are placed below `Build/`.

## Project structure

- `AmbVerb/Source/PluginProcessor.*`: JUCE processor, parameters and state
- `AmbVerb/Source/EarlyReflections.*`: early-reflection matrix convolution
- `AmbVerb/Source/FeedbackDelayNetwork.*`: late-reverb FDN
- `AmbVerb/Source/PortableDsp.hpp`: platform-neutral FFT and spectrum operations
- `AmbVerb/RotationMatrices/`: source matrices; order-3 data is embedded in the binary
- `Tests/`: build-time validation for required matrix data
- `Old Project Files/`: historical reference only; not part of the build

## Current limits

- Supported sample rates: 8 kHz through 96 kHz
- Maximum host block size depends on the delay-buffer headroom and is validated
  in `prepareToPlay`
- Plug-in formats: AU, LV2, VST3 and Standalone, depending on platform
- Platforms: macOS, Windows and Linux

Structural parameters are calculated away from the audio callback. Completed
rotation matrices and FDN impulse responses are activated with non-blocking
buffer swaps at the start of an audio block.

## Audio regression baseline

`AmbVerbAudioRegression` renders deterministic 16-channel test signals directly
through the processor. Its four cases isolate the direct, early-reflection and
late-reverb paths and include different sample rates, block sizes, room sizes,
T60 values and input channels.

The committed WAVs were captured from the stabilized Accelerate/vDSP version on
macOS. They can be regenerated deliberately with:

```sh
cmake --build Build --config Release --target AmbVerbCaptureAudioReference
```

The files are 16-channel, 32-bit floating-point WAVs. The regular CTest run
compares every output sample with absolute and relative tolerances and saves
mismatching renders below `Build/AudioRegression/current` for inspection.

## DSP benchmark

`AmbVerbDspBenchmark` measures construction and preparation time plus the mean,
p50, p95, p99 and maximum callback time for host block sizes from 64 through
1024 samples. It also reports callback time as a fraction of the available
real-time budget. Run the Release benchmark with:

```sh
cmake --build Build --config Release --target AmbVerbRunDspBenchmark
```

The machine-readable result is written below `Build/DspBenchmark/`. Performance
results are retained as CI artifacts but are not used as hard pass/fail limits,
because shared runners and platform FFT backends have different timing noise.

## Next DSP work

Replace the remaining fixed 16,384-sample runtime transforms with block-adaptive
partitioned convolution. The runtime delay paths already use ring buffers, the
early-reflection matrix shares input/output transforms, and the FDN feedback
uses a fast Walsh-Hadamard transform.
