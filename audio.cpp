#include "audio.h"

#include <SPIFFS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <stdlib.h>
#include <string.h>

namespace {

constexpr uint8_t kAudioPin = 26;
constexpr uint8_t kSilence = 128;

// Arduino's loop task normally runs on core 1 on this ESP32 target. Pinning the
// timing-sensitive player to core 0 at priority 1 leaves core 1 available for
// the UI, sensors, and alert/LED state machine. Higher-priority Wi-Fi/system
// work can still preempt playback, so dacWrite() playback is not jitter-free.
constexpr BaseType_t kAudioCore = 0;
constexpr UBaseType_t kAudioTaskPriority = 1;
constexpr uint32_t kAudioTaskStackBytes = 3072;

struct Playback {
  uint8_t *samples;
  size_t sampleCount;
  uint32_t sampleRate;
};

struct WavInfo {
  uint32_t sampleRate;
  uint32_t dataOffset;
  uint32_t dataSize;
};

SemaphoreHandle_t gControlMutex = nullptr;
SemaphoreHandle_t gPlaybackStart = nullptr;
SemaphoreHandle_t gPlaybackStopped = nullptr;
portMUX_TYPE gStateMux = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t gPlaybackTask = nullptr;
bool gInitialized = false;
bool gPlaying = false;
bool gStopRequested = false;
uint8_t gVolumePercent = 100;

uint16_t readLe16(const uint8_t *p) {
  return static_cast<uint16_t>(p[0]) |
         (static_cast<uint16_t>(p[1]) << 8);
}

uint32_t readLe32(const uint8_t *p) {
  return static_cast<uint32_t>(p[0]) |
         (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

bool readExact(File &file, void *destination, size_t length) {
  uint8_t *out = static_cast<uint8_t *>(destination);
  size_t total = 0;

  while (total < length) {
    const size_t bytesRead = file.read(out + total, length - total);
    if (bytesRead == 0) {
      return false;
    }
    total += bytesRead;
  }
  return true;
}

bool parseWav(File &file, WavInfo &info) {
  const size_t fileSize = file.size();
  if (fileSize < 12 || fileSize > UINT32_MAX) {
    return false;
  }

  uint8_t riffHeader[12];
  if (!file.seek(0) || !readExact(file, riffHeader, sizeof(riffHeader)) ||
      memcmp(riffHeader, "RIFF", 4) != 0 ||
      memcmp(riffHeader + 8, "WAVE", 4) != 0) {
    return false;
  }

  const uint32_t riffPayloadSize = readLe32(riffHeader + 4);
  const uint64_t riffEnd64 = static_cast<uint64_t>(riffPayloadSize) + 8ULL;
  if (riffEnd64 < 12 || riffEnd64 > fileSize) {
    return false;
  }
  const uint32_t riffEnd = static_cast<uint32_t>(riffEnd64);

  bool foundFormat = false;
  bool foundData = false;
  uint32_t position = 12;

  while (position + 8U <= riffEnd) {
    uint8_t chunkHeader[8];
    if (!file.seek(position) || !readExact(file, chunkHeader, sizeof(chunkHeader))) {
      return false;
    }

    const uint32_t chunkSize = readLe32(chunkHeader + 4);
    const uint64_t chunkDataEnd = static_cast<uint64_t>(position) + 8ULL + chunkSize;
    const uint64_t nextChunk = chunkDataEnd + (chunkSize & 1U);
    if (chunkDataEnd > riffEnd || nextChunk > riffEnd) {
      return false;
    }

    if (memcmp(chunkHeader, "fmt ", 4) == 0) {
      if (foundFormat || chunkSize < 16) {
        return false;
      }

      uint8_t format[16];
      if (!file.seek(position + 8U) || !readExact(file, format, sizeof(format))) {
        return false;
      }

      const uint16_t encoding = readLe16(format);
      const uint16_t channels = readLe16(format + 2);
      const uint32_t sampleRate = readLe32(format + 4);
      const uint32_t byteRate = readLe32(format + 8);
      const uint16_t blockAlign = readLe16(format + 12);
      const uint16_t bitsPerSample = readLe16(format + 14);

      if (encoding != 1 || channels != 1 || bitsPerSample != 8 ||
          blockAlign != 1 || sampleRate == 0 || byteRate != sampleRate) {
        return false;
      }

      info.sampleRate = sampleRate;
      foundFormat = true;
    } else if (memcmp(chunkHeader, "data", 4) == 0 && !foundData) {
      if (chunkSize == 0) {
        return false;
      }
      info.dataOffset = position + 8U;
      info.dataSize = chunkSize;
      foundData = true;
    }

    position = static_cast<uint32_t>(nextChunk);
  }

  return foundFormat && foundData;
}

bool stopRequested() {
  bool requested;
  portENTER_CRITICAL(&gStateMux);
  requested = gStopRequested;
  portEXIT_CRITICAL(&gStateMux);
  return requested;
}

uint8_t applyVolume(uint8_t sample) {
  uint8_t volume;
  portENTER_CRITICAL(&gStateMux);
  volume = gVolumePercent;
  portEXIT_CRITICAL(&gStateMux);

  const int32_t centered = static_cast<int32_t>(sample) - kSilence;
  int32_t output = kSilence + ((centered * volume) / 100);
  if (output < 0) output = 0;
  if (output > 255) output = 255;
  return static_cast<uint8_t>(output);
}

void playbackTask(void *parameter) {
  Playback *playback = static_cast<Playback *>(parameter);

  // Do not let a very short clip finish before audioPlayWav() has published
  // the task handle. This closes the create/finish ownership race.
  xSemaphoreTake(gPlaybackStart, portMAX_DELAY);
  const uint32_t start = micros();

  for (size_t i = 0; i < playback->sampleCount; ++i) {
    if (stopRequested()) {
      break;
    }

    dacWrite(kAudioPin, applyVolume(playback->samples[i]));

    // Hardware-proven absolute deadline. Basing every deadline on start avoids
    // accumulating dacWrite()/loop overhead from sample to sample.
    const uint32_t target = start + static_cast<uint32_t>(
        ((static_cast<uint64_t>(i + 1) * 1000000ULL) / playback->sampleRate));
    while (static_cast<int32_t>(micros() - target) < 0) {
      if (stopRequested()) {
        break;
      }
    }
  }

  dacWrite(kAudioPin, kSilence);
  free(playback->samples);
  free(playback);

  portENTER_CRITICAL(&gStateMux);
  gPlaying = false;
  gStopRequested = false;
  gPlaybackTask = nullptr;
  portEXIT_CRITICAL(&gStateMux);

  xSemaphoreGive(gPlaybackStopped);
  vTaskDelete(nullptr);
}

void stopPlaybackLocked() {
  TaskHandle_t task;

  portENTER_CRITICAL(&gStateMux);
  task = gPlaybackTask;
  if (task != nullptr) {
    gStopRequested = true;
  }
  portEXIT_CRITICAL(&gStateMux);

  if (task != nullptr && task != xTaskGetCurrentTaskHandle()) {
    // The playback task owns and frees its buffer. Joining it here prevents a
    // replacement playback from reusing state while the old task is exiting.
    xSemaphoreTake(gPlaybackStopped, portMAX_DELAY);
  }

  dacWrite(kAudioPin, kSilence);
}

}  // namespace

bool audioInit() {
  if (gInitialized) {
    return true;
  }

  gControlMutex = xSemaphoreCreateMutex();
  gPlaybackStart = xSemaphoreCreateBinary();
  gPlaybackStopped = xSemaphoreCreateBinary();
  if (gControlMutex == nullptr || gPlaybackStart == nullptr ||
      gPlaybackStopped == nullptr) {
    if (gControlMutex != nullptr) vSemaphoreDelete(gControlMutex);
    if (gPlaybackStart != nullptr) vSemaphoreDelete(gPlaybackStart);
    if (gPlaybackStopped != nullptr) vSemaphoreDelete(gPlaybackStopped);
    gControlMutex = nullptr;
    gPlaybackStart = nullptr;
    gPlaybackStopped = nullptr;
    return false;
  }

  // Never format a device merely because mounting failed.
  if (!SPIFFS.begin(false)) {
    vSemaphoreDelete(gControlMutex);
    vSemaphoreDelete(gPlaybackStart);
    vSemaphoreDelete(gPlaybackStopped);
    gControlMutex = nullptr;
    gPlaybackStart = nullptr;
    gPlaybackStopped = nullptr;
    return false;
  }

  dacWrite(kAudioPin, kSilence);
  gInitialized = true;
  return true;
}

bool audioPlayWav(const char *filename) {
  if (!gInitialized || filename == nullptr || filename[0] != '/' ||
      gControlMutex == nullptr) {
    return false;
  }

  xSemaphoreTake(gControlMutex, portMAX_DELAY);
  stopPlaybackLocked();

  File file = SPIFFS.open(filename, FILE_READ);
  if (!file) {
    xSemaphoreGive(gControlMutex);
    return false;
  }

  WavInfo info{};
  if (!parseWav(file, info)) {
    file.close();
    xSemaphoreGive(gControlMutex);
    return false;
  }

  uint8_t *samples = static_cast<uint8_t *>(malloc(info.dataSize));
  if (samples == nullptr) {
    file.close();
    xSemaphoreGive(gControlMutex);
    return false;
  }

  if (!file.seek(info.dataOffset) || !readExact(file, samples, info.dataSize)) {
    free(samples);
    file.close();
    xSemaphoreGive(gControlMutex);
    return false;
  }
  file.close();

  Playback *playback = static_cast<Playback *>(malloc(sizeof(Playback)));
  if (playback == nullptr) {
    free(samples);
    xSemaphoreGive(gControlMutex);
    return false;
  }
  playback->samples = samples;
  playback->sampleCount = info.dataSize;
  playback->sampleRate = info.sampleRate;

  // Remove any completion token from the preceding playback before launch.
  xSemaphoreTake(gPlaybackStart, 0);
  xSemaphoreTake(gPlaybackStopped, 0);

  portENTER_CRITICAL(&gStateMux);
  gStopRequested = false;
  gPlaying = true;
  portEXIT_CRITICAL(&gStateMux);

  TaskHandle_t task = nullptr;
  const BaseType_t created = xTaskCreatePinnedToCore(
      playbackTask, "wavPlayback", kAudioTaskStackBytes, playback,
      kAudioTaskPriority, &task, kAudioCore);

  if (created != pdPASS) {
    portENTER_CRITICAL(&gStateMux);
    gPlaying = false;
    portEXIT_CRITICAL(&gStateMux);
    free(samples);
    free(playback);
    dacWrite(kAudioPin, kSilence);
    xSemaphoreGive(gControlMutex);
    return false;
  }

  portENTER_CRITICAL(&gStateMux);
  gPlaybackTask = task;
  portEXIT_CRITICAL(&gStateMux);
  xSemaphoreGive(gPlaybackStart);

  xSemaphoreGive(gControlMutex);
  return true;
}

bool audioIsPlaying() {
  bool playing;
  portENTER_CRITICAL(&gStateMux);
  playing = gPlaying;
  portEXIT_CRITICAL(&gStateMux);
  return playing;
}

void audioStop() {
  if (!gInitialized || gControlMutex == nullptr) {
    return;
  }

  xSemaphoreTake(gControlMutex, portMAX_DELAY);
  stopPlaybackLocked();
  xSemaphoreGive(gControlMutex);
}

void audioSetVolume(uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }

  portENTER_CRITICAL(&gStateMux);
  gVolumePercent = percent;
  portEXIT_CRITICAL(&gStateMux);
}
