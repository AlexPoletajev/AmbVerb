# Audio regression references

This directory will contain the 16-channel, 32-bit floating-point WAV files
captured from the stabilized Accelerate/vDSP implementation before the portable
DSP backend is introduced.

On macOS, capture the baseline with the dedicated build target:

```sh
cmake --build Build --config Release --target AmbVerbCaptureAudioReference
```

Commit `manifest.json` and the generated WAV files before replacing the DSP
backend. Once the manifest exists, CMake automatically changes the audio CTest
from baseline capture mode to reference comparison mode on the next configure
run.

Do not update the references merely to make a failing refactor pass. Inspect the
generated `Build/AudioRegression/current` WAV files and `report.json` first, and
only accept a new baseline when the audible or mathematical change is intended.
