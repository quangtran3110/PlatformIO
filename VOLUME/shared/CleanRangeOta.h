#pragma once

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Updater.h>

namespace CleanRangeOta {

constexpr uint32_t RTC_OFFSET_WORDS = 32;
constexpr uint32_t RTC_MAGIC = 0x4F544134UL; // "OTA4"
constexpr unsigned long WIFI_TIMEOUT_MS = 45000UL;
constexpr int NETWORK_TIMEOUT_MS = 15000;
constexpr unsigned long TOTAL_TIMEOUT_MS = 5UL * 60UL * 1000UL;
constexpr uint16_t TLS_RX_BUFFER = 6144;
constexpr uint32_t RANGE_SIZE = 4096UL;
constexpr uint8_t RANGE_RETRIES = 3;

constexpr uint32_t MIN_HEAP_BEFORE_UPDATE = 8000;
constexpr uint32_t MIN_BLOCK_BEFORE_UPDATE = 7500;
constexpr uint32_t MIN_HEAP_AFTER_UPDATE = 2500;
constexpr uint32_t MIN_BLOCK_AFTER_UPDATE = 2200;
constexpr uint32_t MIN_HEAP_DURING_WRITE = 1800;
constexpr uint32_t MIN_BLOCK_DURING_WRITE = 1600;

constexpr int32_t ERROR_WIFI_TIMEOUT = -1001;
constexpr int32_t ERROR_HTTP = -1004;
constexpr int32_t ERROR_LENGTH = -1005;
constexpr int32_t ERROR_BEGIN = -1006;
constexpr int32_t ERROR_STREAM_TIMEOUT = -1007;
constexpr int32_t ERROR_WRITE = -1008;
constexpr int32_t ERROR_END = -1009;
constexpr int32_t ERROR_TOTAL_TIMEOUT = -1010;
constexpr int32_t ERROR_RANGE = -1011;
constexpr int32_t ERROR_VERSION = -1012;
constexpr int32_t ERROR_LOW_HEAP_BEFORE = -1013;
constexpr int32_t ERROR_LOW_HEAP_AFTER = -1014;
constexpr int32_t ERROR_LOW_HEAP_DURING = -1015;

enum Phase : uint32_t {
  NONE = 0,
  REQUESTED = 1,
  RUNNING = 2,
  SUCCESS = 3,
  FAILED = 4,
};

struct State {
  uint32_t magic;
  uint32_t phase;
  int32_t error;
  uint32_t check;
};

static uint8_t writeBuffer[1024];
static int8_t progressBucket = -1;
static uint32_t minimumHeap = UINT32_MAX;
static uint32_t previousMinimumHeap = 0;
static String previousStatus = "none";
static bool restartScheduled = false;
static unsigned long restartAt = 0;

inline uint32_t checksum(const State &state) {
  return state.magic ^ state.phase ^ static_cast<uint32_t>(state.error) ^
         0xA55A3CC3UL;
}

inline bool writeState(Phase phase, int32_t error = 0) {
  State state = {RTC_MAGIC, static_cast<uint32_t>(phase), error, 0};
  state.check = checksum(state);
  return ESP.rtcUserMemoryWrite(RTC_OFFSET_WORDS,
                                reinterpret_cast<uint32_t *>(&state),
                                sizeof(state));
}

inline bool readState(State &state) {
  if (!ESP.rtcUserMemoryRead(RTC_OFFSET_WORDS,
                             reinterpret_cast<uint32_t *>(&state),
                             sizeof(state))) {
    return false;
  }
  return state.magic == RTC_MAGIC && state.check == checksum(state);
}

inline void clearState() {
  State empty = {};
  ESP.rtcUserMemoryWrite(RTC_OFFSET_WORDS,
                         reinterpret_cast<uint32_t *>(&empty), sizeof(empty));
}

inline void sampleHeap() {
  minimumHeap = min(minimumHeap, ESP.getFreeHeap());
}

inline bool memoryBelow(uint32_t minHeap, uint32_t minBlock,
                        const char *stage) {
  const uint32_t freeHeap = ESP.getFreeHeap();
  const uint32_t maxBlock = ESP.getMaxFreeBlockSize();
  sampleHeap();
  if (freeHeap >= minHeap && maxBlock >= minBlock) return false;
  Serial.printf("OTA low memory at %s: heap=%u/%u max_block=%u/%u\n", stage,
                freeHeap, minHeap, maxBlock, minBlock);
  return true;
}

inline void progress(size_t current, size_t total) {
  ESP.wdtFeed();
  sampleHeap();
  if (!total) return;
  const int8_t bucket = static_cast<int8_t>((current * 10UL) / total);
  if (bucket == progressBucket) return;
  progressBucket = bucket;
  Serial.printf("OTA progress: %d%% (%u/%u bytes) heap=%u max_block=%u\n",
                bucket * 10, static_cast<unsigned>(current),
                static_cast<unsigned>(total), ESP.getFreeHeap(),
                ESP.getMaxFreeBlockSize());
}

inline void printTlsError(WiFiClientSecure &client) {
  char detail[96] = {};
  const int code = client.getLastSSLError(detail, sizeof(detail));
  Serial.printf("OTA TLS: code=%d detail=%s\n", code,
                detail[0] ? detail : "none");
}

inline bool download(const char *firmwareUrl, const char *currentVersion) {
  String url = String(firmwareUrl) + "?from=" + currentVersion +
               "&boot=" + String(ESP.getCycleCount(), HEX);
  Serial.println(F("OTA: HTTPS range download on one connection"));
  const unsigned long startedAt = millis();
  minimumHeap = UINT32_MAX;
  progressBucket = -1;
  sampleHeap();

  WiFiClientSecure client;
  client.setBufferSizes(TLS_RX_BUFFER, 512);
  client.setInsecure();
  client.setTimeout(NETWORK_TIMEOUT_MS);

  HTTPClient http;
  http.setTimeout(NETWORK_TIMEOUT_MS);
  http.setReuse(true);
  const char *headers[] = {"x-MD5", "x-firmware-version", "Content-Range"};
  http.collectHeaders(headers, 3);
  if (!http.begin(client, url)) {
    writeState(FAILED, ERROR_HTTP);
    return false;
  }

  int total = 0;
  int written = 0;
  uint8_t retries = 0;
  bool updateBegun = false;
  String expectedMd5;
  String expectedVersion;
  int32_t failure = ERROR_STREAM_TIMEOUT;

  while (total == 0 || written < total) {
    ESP.wdtFeed();
    if (static_cast<unsigned long>(millis() - startedAt) >= TOTAL_TIMEOUT_MS) {
      Serial.printf("OTA total timeout at %d/%d\n", written, total);
      failure = ERROR_TOTAL_TIMEOUT;
      break;
    }

    uint32_t rangeEnd = static_cast<uint32_t>(written) + RANGE_SIZE - 1;
    if (total > 0 && rangeEnd >= static_cast<uint32_t>(total)) {
      rangeEnd = total - 1;
    }
    char rangeHeader[48];
    snprintf(rangeHeader, sizeof(rangeHeader), "bytes=%d-%lu", written,
             static_cast<unsigned long>(rangeEnd));
    http.addHeader("Range", rangeHeader);

    const int httpCode = http.GET();
    if (httpCode != HTTP_CODE_PARTIAL_CONTENT) {
      Serial.printf("OTA range HTTP error: %d at %d\n", httpCode, written);
      printTlsError(client);
      client.stop();
      if (++retries < RANGE_RETRIES) {
        delay(20);
        continue;
      }
      failure = ERROR_HTTP;
      break;
    }

    int responseStart = -1;
    int responseEnd = -1;
    int responseTotal = -1;
    const String contentRange = http.header("Content-Range");
    if (sscanf(contentRange.c_str(), "bytes %d-%d/%d", &responseStart,
               &responseEnd, &responseTotal) != 3 || responseStart != written ||
        responseEnd < responseStart || responseTotal <= 0) {
      Serial.printf("OTA Content-Range invalid: %s\n", contentRange.c_str());
      failure = ERROR_RANGE;
      break;
    }

    const String responseMd5 = http.header("x-MD5");
    const String responseVersion = http.header("x-firmware-version");
    if (!updateBegun) {
      total = responseTotal;
      expectedMd5 = responseMd5;
      expectedVersion = responseVersion;
      if (expectedMd5.length() != 32 || expectedVersion.length() == 0 ||
          expectedVersion == currentVersion) {
        Serial.printf("OTA header invalid: md5=%u version=%s\n",
                      expectedMd5.length(), expectedVersion.c_str());
        failure = expectedVersion == currentVersion ? ERROR_VERSION : ERROR_LENGTH;
        break;
      }

      Serial.printf("OTA target=%s size=%d heap=%u max_block=%u\n",
                    expectedVersion.c_str(), total, ESP.getFreeHeap(),
                    ESP.getMaxFreeBlockSize());
      if (memoryBelow(MIN_HEAP_BEFORE_UPDATE, MIN_BLOCK_BEFORE_UPDATE,
                      "before Update.begin")) {
        failure = ERROR_LOW_HEAP_BEFORE;
        break;
      }
      Update.onProgress(progress);
      if (!Update.begin(static_cast<size_t>(total))) {
        Serial.printf("OTA Update.begin error: %u\n", Update.getError());
        failure = ERROR_BEGIN;
        break;
      }
      updateBegun = true;
      if (memoryBelow(MIN_HEAP_AFTER_UPDATE, MIN_BLOCK_AFTER_UPDATE,
                      "after Update.begin")) {
        failure = ERROR_LOW_HEAP_AFTER;
        break;
      }
      if (!Update.setMD5(expectedMd5.c_str())) {
        Serial.println(F("OTA: invalid MD5 header"));
        failure = ERROR_LENGTH;
        break;
      }
    } else if (responseTotal != total || responseMd5 != expectedMd5 ||
               responseVersion != expectedVersion) {
      Serial.println(F("OTA: firmware changed during download"));
      failure = ERROR_LENGTH;
      break;
    }

    const int expected = responseEnd - responseStart + 1;
    int receivedInRange = 0;
    unsigned long lastDataAt = millis();
    WiFiClient *stream = http.getStreamPtr();
    while (receivedInRange < expected) {
      ESP.wdtFeed();
      const int available = stream->available();
      if (available > 0) {
        size_t wanted = min(static_cast<size_t>(available), sizeof(writeBuffer));
        wanted = min(wanted, static_cast<size_t>(expected - receivedInRange));
        const int received = stream->read(writeBuffer, wanted);
        if (received > 0) {
          const size_t flashed = Update.write(writeBuffer,
                                              static_cast<size_t>(received));
          if (flashed != static_cast<size_t>(received)) {
            Serial.printf("OTA flash write error: %u\n", Update.getError());
            failure = ERROR_WRITE;
            break;
          }
          written += received;
          receivedInRange += received;
          lastDataAt = millis();
          if (memoryBelow(MIN_HEAP_DURING_WRITE, MIN_BLOCK_DURING_WRITE,
                          "during write")) {
            failure = ERROR_LOW_HEAP_DURING;
            break;
          }
          progress(static_cast<size_t>(written), static_cast<size_t>(total));
          continue;
        }
      }
      if (!http.connected() ||
          static_cast<unsigned long>(millis() - lastDataAt) >=
              static_cast<unsigned long>(NETWORK_TIMEOUT_MS)) {
        break;
      }
      delay(1);
    }

    if (failure == ERROR_WRITE || failure == ERROR_LOW_HEAP_DURING) break;
    if (receivedInRange == expected) {
      retries = 0;
      failure = 0;
      continue;
    }

    Serial.printf("OTA range interrupted at %d/%d\n", written, total);
    client.stop();
    failure = ERROR_STREAM_TIMEOUT;
    if (receivedInRange > 0) retries = 0;
    if (++retries >= RANGE_RETRIES) break;
  }

  http.end();
  if (failure != 0) {
    if (updateBegun) Update.end(false);
    writeState(FAILED, failure);
    return false;
  }

  if (!(Update.end() && Update.isFinished())) {
    Serial.printf("OTA Update.end error: %u\n", Update.getError());
    writeState(FAILED, ERROR_END);
    return false;
  }

  const int32_t minHeap = minimumHeap == UINT32_MAX
                              ? 0
                              : static_cast<int32_t>(minimumHeap);
  writeState(SUCCESS, minHeap);
  Serial.println(F("OTA: download and MD5 verification completed"));
  delay(200);
  ESP.restart();
  return true;
}

inline void run(const char *ssid, const char *password, const char *firmwareUrl,
                const char *currentVersion) {
  Serial.printf("OTA clean boot: firmware=%s reset=%s\n", currentVersion,
                ESP.getResetReason().c_str());
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, password);

  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         static_cast<unsigned long>(millis() - startedAt) < WIFI_TIMEOUT_MS) {
    ESP.wdtFeed();
    delay(50);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("OTA clean boot: WiFi timeout"));
    writeState(FAILED, ERROR_WIFI_TIMEOUT);
    delay(500);
    ESP.restart();
    return;
  }

  Serial.printf("OTA WiFi connected: RSSI=%d heap=%u max_block=%u\n",
                WiFi.RSSI(), ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
  download(firmwareUrl, currentVersion);

  // A failed or rejected image never becomes active. Restart into the existing
  // firmware and report the RTC failure code on the normal boot.
  delay(500);
  ESP.restart();
}

inline bool handleBoot(const char *ssid, const char *password,
                       const char *firmwareUrl, const char *currentVersion) {
  State state = {};
  if (!readState(state)) return false;

  if (state.phase == REQUESTED) {
    if (!writeState(RUNNING)) {
      clearState();
      return false;
    }
    run(ssid, password, firmwareUrl, currentVersion);
    return true;
  }

  if (state.phase == SUCCESS) {
    previousStatus = "success";
    if (state.error > 0) previousMinimumHeap = static_cast<uint32_t>(state.error);
  } else if (state.phase == FAILED) {
    previousStatus = "failed (" + String(state.error) + ")";
  } else if (state.phase == RUNNING) {
    previousStatus = "interrupted/reset";
  }
  Serial.println("Last OTA: " + previousStatus);
  // Keep the last result until the next OTA request overwrites it. Some field
  // controllers can reset again before Blynk reconnects; clearing here would
  // lose the only success/failure evidence and the measured minimum heap.
  return false;
}

inline bool scheduleRequest(unsigned long delayMs = 1000UL) {
  if (restartScheduled || !writeState(REQUESTED)) return false;
  restartScheduled = true;
  restartAt = millis() + delayMs;
  return true;
}

inline void service() {
  if (!restartScheduled || static_cast<int32_t>(millis() - restartAt) < 0) {
    return;
  }
  delay(20);
  ESP.restart();
}

inline bool pending() { return restartScheduled; }
inline const String &lastStatus() { return previousStatus; }
inline uint32_t lastMinimumHeap() { return previousMinimumHeap; }

} // namespace CleanRangeOta



