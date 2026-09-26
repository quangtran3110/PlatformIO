#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>

namespace CleanOta {

constexpr uint32_t RTC_OFFSET_WORDS = 32;
constexpr uint32_t RTC_MAGIC = 0x4F544131UL; // "OTA1"
constexpr uint32_t WIFI_TIMEOUT_MS = 45000UL;
constexpr int32_t ERROR_WIFI_TIMEOUT = -1001;

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

static int progressBucket = -1;

inline uint32_t checksum(const State &state) {
  return state.magic ^ state.phase ^ static_cast<uint32_t>(state.error) ^
         0x5A5AA5A5UL;
}

inline bool readState(State &state) {
  if (!ESP.rtcUserMemoryRead(RTC_OFFSET_WORDS,
                             reinterpret_cast<uint32_t *>(&state),
                             sizeof(state))) {
    return false;
  }
  return state.magic == RTC_MAGIC && state.check == checksum(state);
}

inline bool writeState(uint32_t phase, int32_t error = 0) {
  State state = {};
  state.magic = RTC_MAGIC;
  state.phase = phase;
  state.error = error;
  state.check = checksum(state);
  return ESP.rtcUserMemoryWrite(RTC_OFFSET_WORDS,
                                reinterpret_cast<uint32_t *>(&state),
                                sizeof(state));
}

inline void clearState() {
  State state = {};
  ESP.rtcUserMemoryWrite(RTC_OFFSET_WORDS,
                         reinterpret_cast<uint32_t *>(&state), sizeof(state));
}

inline void progress(int current, int total) {
  ESP.wdtFeed();
  if (total <= 0) {
    return;
  }
  int bucket = ((current * 100) / total) / 10;
  if (bucket != progressBucket) {
    progressBucket = bucket;
    Serial.printf("OTA: %d%% (%d/%d bytes)\n", bucket * 10, current, total);
  }
}

inline void run(const char *ssid, const char *password, const char *firmwareUrl) {
  Serial.println(F("--- CLEAN OTA MODE ---"));
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, password);

  uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < WIFI_TIMEOUT_MS) {
    ESP.wdtFeed();
    delay(100);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("OTA: WiFi timeout after 45 seconds"));
    writeState(FAILED, ERROR_WIFI_TIMEOUT);
    delay(500);
    ESP.restart();
    return;
  }

  Serial.printf("OTA: WiFi ready, IP=%s\n", WiFi.localIP().toString().c_str());
  BearSSL::WiFiClientSecure updateClient;
  updateClient.setInsecure();

  progressBucket = -1;
  ESPhttpUpdate.onStart([]() {
    progressBucket = -1;
    Serial.println(F("OTA: download started"));
  });
  ESPhttpUpdate.onEnd([]() {
    Serial.println(F("OTA: download finished"));
    writeState(SUCCESS);
  });
  ESPhttpUpdate.onProgress(progress);
  ESPhttpUpdate.onError([](int error) {
    Serial.printf("OTA: fatal error %d\n", error);
  });

  t_httpUpdate_return result = ESPhttpUpdate.update(updateClient, firmwareUrl);
  switch (result) {
  case HTTP_UPDATE_FAILED: {
    int error = ESPhttpUpdate.getLastError();
    Serial.printf("OTA: failed (%d): %s\n", error,
                  ESPhttpUpdate.getLastErrorString().c_str());
    writeState(FAILED, error);
    break;
  }
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println(F("OTA: no update"));
    writeState(FAILED, -2);
    break;
  case HTTP_UPDATE_OK:
    Serial.println(F("OTA: success"));
    writeState(SUCCESS);
    break;
  }

  delay(500);
  ESP.restart();
}

inline bool handleBoot(const char *ssid, const char *password,
                       const char *firmwareUrl) {
  State state = {};
  if (!readState(state)) {
    return false;
  }

  if (state.phase == REQUESTED) {
    if (!writeState(RUNNING)) {
      Serial.println(F("OTA: cannot mark running; returning to normal boot"));
      clearState();
      return false;
    }
    run(ssid, password, firmwareUrl);
    return true;
  }

  if (state.phase == SUCCESS) {
    Serial.println(F("Last OTA: success"));
  } else if (state.phase == FAILED) {
    Serial.printf("Last OTA: failed (%d)\n", state.error);
  } else if (state.phase == RUNNING) {
    Serial.println(F("Last OTA: interrupted/reset"));
  }
  clearState();
  return false;
}

inline bool requestAndRestart() {
  if (!writeState(REQUESTED)) {
    return false;
  }
  delay(250);
  ESP.restart();
  return true;
}

} // namespace CleanOta
