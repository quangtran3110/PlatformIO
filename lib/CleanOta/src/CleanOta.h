#pragma once

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Updater.h>

namespace CleanOta {

constexpr uint32_t RTC_OFFSET_WORDS = 32;
constexpr uint32_t RTC_MAGIC = 0x4F544131UL; // "OTA1"
constexpr uint32_t WIFI_TIMEOUT_MS = 45000UL;
constexpr uint32_t NETWORK_TIMEOUT_MS = 30000UL;
constexpr uint32_t RANGE_SIZE = 32768UL;
constexpr uint16_t TLS_RX_BUFFER = 4096;
constexpr uint8_t RANGE_RETRIES = 3;
constexpr int32_t ERROR_WIFI_TIMEOUT = -1001;
constexpr int32_t ERROR_HTTP = -1004;
constexpr int32_t ERROR_LENGTH = -1005;
constexpr int32_t ERROR_BEGIN = -1006;
constexpr int32_t ERROR_STREAM_TIMEOUT = -1007;
constexpr int32_t ERROR_WRITE = -1008;
constexpr int32_t ERROR_END = -1009;

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
static uint8_t writeBuffer[1024];

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
  progressBucket = -1;
  String url = String(firmwareUrl) + "?boot=" + String(ESP.getCycleCount(), HEX);
  int total = 0;
  int written = 0;
  uint8_t retries = 0;
  bool updateBegun = false;

  while (total == 0 || written < total) {
    ESP.wdtFeed();
    uint32_t rangeEnd = static_cast<uint32_t>(written) + RANGE_SIZE - 1;
    if (total > 0 && rangeEnd >= static_cast<uint32_t>(total)) {
      rangeEnd = total - 1;
    }

    BearSSL::WiFiClientSecure updateClient;
    updateClient.setBufferSizes(TLS_RX_BUFFER, 512);
    updateClient.setInsecure();
    updateClient.setTimeout(NETWORK_TIMEOUT_MS);

    HTTPClient http;
    http.setTimeout(NETWORK_TIMEOUT_MS);
    http.setReuse(false);
    const char *headers[] = {"x-MD5", "Content-Range"};
    http.collectHeaders(headers, 2);
    if (!http.begin(updateClient, url)) {
      writeState(FAILED, ERROR_HTTP);
      break;
    }
    http.addHeader("Range", "bytes=" + String(written) + "-" + String(rangeEnd));

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_PARTIAL_CONTENT) {
      Serial.printf("OTA: HTTP %d at byte %d\n", httpCode, written);
      http.end();
      if (++retries < RANGE_RETRIES) {
        continue;
      }
      if (updateBegun) {
        Update.end(false);
      }
      writeState(FAILED, ERROR_HTTP);
      break;
    }

    int responseStart = -1;
    int responseEnd = -1;
    int responseTotal = -1;
    String contentRange = http.header("Content-Range");
    if (sscanf(contentRange.c_str(), "bytes %d-%d/%d", &responseStart,
               &responseEnd, &responseTotal) != 3 || responseStart != written ||
        responseEnd < responseStart || responseTotal <= 0) {
      Serial.printf("OTA: invalid Content-Range: %s\n", contentRange.c_str());
      http.end();
      if (updateBegun) {
        Update.end(false);
      }
      writeState(FAILED, ERROR_LENGTH);
      break;
    }

    if (!updateBegun) {
      total = responseTotal;
      String md5 = http.header("x-MD5");
      if (!Update.begin(static_cast<size_t>(total))) {
        Serial.printf("OTA: Update.begin failed %u\n", Update.getError());
        http.end();
        writeState(FAILED, ERROR_BEGIN);
        break;
      }
      updateBegun = true;
      if (md5.length() != 32 || !Update.setMD5(md5.c_str())) {
        Serial.println(F("OTA: invalid MD5 header"));
        http.end();
        Update.end(false);
        writeState(FAILED, ERROR_LENGTH);
        break;
      }
      Serial.println(F("OTA: download started"));
    } else if (responseTotal != total) {
      Serial.println(F("OTA: firmware size changed during download"));
      http.end();
      Update.end(false);
      writeState(FAILED, ERROR_LENGTH);
      break;
    }

    WiFiClient *stream = http.getStreamPtr();
    int expected = responseEnd - responseStart + 1;
    int receivedInRange = 0;
    uint32_t lastDataAt = millis();
    while (receivedInRange < expected) {
      ESP.wdtFeed();
      int available = stream->available();
      if (available > 0) {
        size_t wanted = min(static_cast<size_t>(available), sizeof(writeBuffer));
        wanted = min(wanted, static_cast<size_t>(expected - receivedInRange));
        int received = stream->read(writeBuffer, wanted);
        if (received > 0) {
          size_t flashed = Update.write(writeBuffer, static_cast<size_t>(received));
          if (flashed != static_cast<size_t>(received)) {
            Serial.printf("OTA: flash write failed %u\n", Update.getError());
            Update.end(false);
            http.end();
            writeState(FAILED, ERROR_WRITE);
            delay(500);
            ESP.restart();
            return;
          }
          written += received;
          receivedInRange += received;
          lastDataAt = millis();
          progress(written, total);
          continue;
        }
      }
      if (!http.connected() || millis() - lastDataAt >= NETWORK_TIMEOUT_MS) {
        break;
      }
      delay(1);
    }
    http.end();

    if (receivedInRange == expected) {
      retries = 0;
      Serial.printf("OTA: range OK %d/%d\n", written, total);
      continue;
    }
    Serial.printf("OTA: range interrupted at %d/%d\n", written, total);
    if (receivedInRange > 0) {
      retries = 0;
    }
    if (++retries >= RANGE_RETRIES) {
      Update.end(false);
      writeState(FAILED, ERROR_STREAM_TIMEOUT);
      break;
    }
  }

  if (total > 0 && written == total) {
    bool complete = Update.end() && Update.isFinished();
    if (complete) {
      Serial.println(F("OTA: download finished"));
      writeState(SUCCESS);
      delay(250);
      ESP.restart();
      return;
    }
    Serial.printf("OTA: Update.end failed %u\n", Update.getError());
    writeState(FAILED, ERROR_END);
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
