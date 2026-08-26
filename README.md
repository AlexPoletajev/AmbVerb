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
- `AmbVerb/Source/PartitionedConvolution.hpp`: shared zero-latency MIMO partitioner
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
An additional synthetic MIMO test compares fixed and deliberately irregular
callback sequences against direct time-domain convolution.

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

## Runtime convolution

On Windows and Linux, the early-reflection matrix and FDN correction filters use
zero-latency, uniform partitioned convolution. For a host maximum block size
`B`, the partition size `P` is the next power of two and the runtime FFT size is
`2P`. Input transforms are shared by all matrix routes, output spectra are
accumulated before a single inverse transform per channel, and complex
multiplication uses JUCE's SIMD vector primitives. The convolver accepts
changing callback lengths up to the prepared maximum without changing timing.

On Apple systems the benchmark selects the shared full-size path instead.
Accelerate/vDSP processes the 16,384-point transforms faster than the additional
partition bookkeeping on the current Apple Silicon runner. The synthetic
partitioner test still runs on macOS so both implementations remain covered.

On the development i9-14900KS Linux environment at 48 kHz, the mean callback
time for the benchmark's 64--1024 sample cases developed as follows:

| Implementation | Mean callback time |
| --- | ---: |
| Original route-by-route 16,384-point FFTs | 24.6--25.4 ms |
| Shared full-size FFTs and ring buffers | 7.0--7.2 ms |
| Shared partitioned MIMO convolution (Linux) | 1.2--1.9 ms |

The fixed-transform part therefore falls from transforms of size 16,384 to
size `2P`. Its approximate FFT-work reduction is
`(16384 * log2(16384)) / (2P * log2(2P))`, before accounting for matrix products
and the FDN itself. Exact results remain platform-dependent, so the committed
benchmark and CI artifacts are the source of truth for comparisons.

The next higher-order bottleneck is the number of non-zero matrix routes and
filter partitions. Further scaling work should target matrix factorisation and
route sparsity rather than adding more FFT sharing.
