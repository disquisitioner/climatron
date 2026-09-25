# Climatron WAV audio

`audio.cpp` plays unsigned 8-bit, mono, uncompressed PCM WAV files from SPIFFS
through the ESP32 DAC on GPIO 26. It scans RIFF chunks (including odd-byte
padding), so metadata chunks and extended `fmt ` chunks are allowed; a fixed
44-byte header is not assumed.

## Alert mapping

The accessible main-sketch snapshot has three sound-producing event paths:

| Event | SPIFFS filename referenced by the sketch | Supplied status |
| --- | --- | --- |
| CO2 rising rapidly | `/co2_rising.wav` | Required; not supplied |
| No samples available | `/no_samples.wav` | Required; not supplied |
| Device reboot/fatal startup failure | `/reboot.wav` | Required; not supplied |
| Hardware-proven playback sample | `/no_sh.wav` | Confirmed supplied in the earlier hardware test; test-only, not mapped to an alert |

The known `/no_sh.wav` file is 11,025 Hz, contains 12,972 PCM bytes, and takes
about 1.177 seconds at its declared sample rate. No replacement WAV assets were
fabricated as part of this change.

## CPU and timing limits

Playback is a dedicated priority-1 task pinned to core 0. The normal Arduino
application loop remains on core 1, so screen, sensor, and LED alert handling can
continue. The player uses the hardware-proven absolute `micros()` deadline and
busy-waits between samples. This is intentionally not continuous DAC/DMA.

Consequences of that design:

- one core is busy for the duration of each clip;
- higher-priority Wi-Fi/system work can preempt the player and create audible
  timing jitter;
- very high sample rates or long clips are a poor fit for this path;
- the entire PCM data chunk plus small bookkeeping allocations must fit in heap;
- deadline comparison assumes a clip is far shorter than the signed 32-bit
  `micros()` comparison window (about 35.8 minutes).

The proven 11,025 Hz, 12,972-byte clip is within the intended operating range.
Keep production alerts short and at 11,025 Hz unless hardware testing proves a
different rate reliable under normal Wi-Fi/display load.

## Integration

Call `audioInit()` once from `setup()` after basic GPIO initialization. It mounts
SPIFFS without formatting on failure and sets the DAC to 128. Do not attach
GPIO 26 to LEDC anywhere else.

`audioPlayWav()` stops and joins prior playback, parses and loads the replacement
file fully, then transfers buffer ownership to the playback task. Parse, open,
allocation, read, seek, or task-creation failures return `false` without leaking
the sample buffer. `audioStop()` cancels and joins playback. Every completion and
stop path writes DAC silence with `dacWrite(26, 128)`.
