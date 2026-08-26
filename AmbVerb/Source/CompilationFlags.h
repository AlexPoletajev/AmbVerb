#pragma once

#include <cstddef>

inline constexpr int Qmin = 200;
inline constexpr int Qmax = 400;
inline constexpr int MinRoomsize = 1000;
inline constexpr int MaxRoomsize = 2000;
inline constexpr float WindowStartsAt_xRoomsize = 2.5f;
inline constexpr float WindowEndsAt_xRoomsize = 0.3f;
inline constexpr float maxDelayTimeAt_xRoomsize = 2.5f;

// The current DSP implementation and bundled matrices are fixed to third-order
// Ambisonics (ACN channel count: (order + 1)^2 = 16).
inline constexpr int AmbisonicsOrder = 3;
inline constexpr int NumAmbisonicsChannels = (AmbisonicsOrder + 1) * (AmbisonicsOrder + 1);

inline constexpr int earlyref_Log2N = 14;
inline constexpr std::size_t earlyref_Buffersize = std::size_t { 1 } << earlyref_Log2N;
inline constexpr float Qx = 1.9f;
inline constexpr float Qy = 1.3f;
inline constexpr int Trunc = 8;

inline constexpr int fdn_Log2N = 14;
inline constexpr std::size_t fdn_Buffersize = std::size_t { 1 } << fdn_Log2N;
inline constexpr int NumDelaylines = 16;

// Accelerate/vDSP handles the shared 16,384-point transforms more efficiently
// than the partition bookkeeping on Apple Silicon. Portable JUCE FFT backends
// benefit substantially from the block-adaptive partitioned runtime path.
#if JUCE_MAC
inline constexpr bool UsePartitionedRuntimeConvolution = false;
#else
inline constexpr bool UsePartitionedRuntimeConvolution = true;
#endif
