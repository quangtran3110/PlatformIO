#define BLYNK_TEMPLATE_ID "TMPL6_lALgEzj"
#define BLYNK_TEMPLATE_NAME "RESETK12"
#define BLYNK_AUTH_TOKEN "0WCBRojUNdIx0yKeuJVnM96EAoqX_QmL"

#define BLYNK_FIRMWARE_VERSION "260911.2"
#define BLYNK_PRINT Serial
#define APP_DEBUG

#include <BlynkSimpleEsp8266.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>

const char *ssid = "net";
const char *password = "Password";
const char *firmwareUrl =
    "https://raw.githubusercontent.com/quangtran3110/PlatformIO/refs/heads/"
    "main/ResetK12/.pio/build/nodemcuv2/firmware.bin";

const uint32_t OTA_RTC_OFFSET_WORDS = 32;
const uint32_t OTA_RTC_MAGIC = 0x4B31324FUL; // "K12O"
const unsigned long OTA_WIFI_TIMEOUT_MS = 45000UL;
const int OTA_NETWORK_TIMEOUT_MS = 8000;
const int32_t OTA_ERROR_WIFI_TIMEOUT = -1001;
const int32_t OTA_ERROR_NO_UPDATE = -1002;
int8_t lastOtaProgressBucket = -1;

enum OtaRtcPhase : uint32_t {
  OTA_RTC_NONE = 0,
  OTA_RTC_REQUESTED = 1,
  OTA_RTC_RUNNING = 2,
  OTA_RTC_SUCCESS = 3,
  OTA_RTC_FAILED = 4,
};

struct OtaRtcState {
  uint32_t magic;
  uint32_t phase;
  int32_t error;
  uint32_t check;
};

uint32_t otaRtcCheck(const OtaRtcState &state) {
  return state.magic ^ state.phase ^ static_cast<uint32_t>(state.error) ^
         0x5AA5C33CUL;
}

bool writeOtaRtcState(OtaRtcPhase phase, int32_t error = 0) {
  OtaRtcState state = {OTA_RTC_MAGIC, static_cast<uint32_t>(phase), error, 0};
  state.check = otaRtcCheck(state);
  return ESP.rtcUserMemoryWrite(OTA_RTC_OFFSET_WORDS,
                                reinterpret_cast<uint32_t *>(&state),
                                sizeof(state));
}

bool readOtaRtcState(OtaRtcState &state) {
  if (!ESP.rtcUserMemoryRead(OTA_RTC_OFFSET_WORDS,
                             reinterpret_cast<uint32_t *>(&state),
                             sizeof(state))) {
    return false;
  }
  return state.magic == OTA_RTC_MAGIC && state.check == otaRtcCheck(state);
}

void clearOtaRtcState() {
  OtaRtcState state = {};
  ESP.rtcUserMemoryWrite(OTA_RTC_OFFSET_WORDS,
                         reinterpret_cast<uint32_t *>(&state), sizeof(state));
}

void setResetRelaySafe() {
  pinMode(D3, OUTPUT);
  digitalWrite(D3, HIGH);
}

void otaStarted() {
  lastOtaProgressBucket = -1;
  setResetRelaySafe();
  Serial.printf("OTA started: heap=%u max_block=%u fragmentation=%u%%\n",
                ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),
                ESP.getHeapFragmentation());
}

void otaFinished() {
  Serial.println("OTA finished");
  writeOtaRtcState(OTA_RTC_SUCCESS);
}

void otaProgress(int current, int total) {
  ESP.wdtFeed();
  setResetRelaySafe();
  if (total <= 0) {
    return;
  }

  int8_t bucket = static_cast<int8_t>(
      (static_cast<uint32_t>(current) * 10UL) / static_cast<uint32_t>(total));
  if (bucket != lastOtaProgressBucket) {
    lastOtaProgressBucket = bucket;
    Serial.printf("OTA progress: %d%% (%d/%d bytes)\n", bucket * 10, current,
                  total);
  }
}

void otaError(int error) {
  Serial.printf("OTA callback error: %d\n", error);
  writeOtaRtcState(OTA_RTC_FAILED, error);
}

void runCleanOtaMode() {
  setResetRelaySafe();
  Serial.printf("OTA clean boot: firmware=%s reset=%s\n",
                BLYNK_FIRMWARE_VERSION, ESP.getResetReason().c_str());

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, password);

  unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         static_cast<unsigned long>(millis() - startedAt) <
             OTA_WIFI_TIMEOUT_MS) {
    ESP.wdtFeed();
    setResetRelaySafe();
    delay(50);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("OTA WiFi timeout");
    writeOtaRtcState(OTA_RTC_FAILED, OTA_ERROR_WIFI_TIMEOUT);
    delay(500);
    ESP.restart();
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(OTA_NETWORK_TIMEOUT_MS);
  ESPhttpUpdate.setClientTimeout(OTA_NETWORK_TIMEOUT_MS);
  ESPhttpUpdate.closeConnectionsOnUpdate(true);
  ESPhttpUpdate.onStart(otaStarted);
  ESPhttpUpdate.onEnd(otaFinished);
  ESPhttpUpdate.onProgress(otaProgress);
  ESPhttpUpdate.onError(otaError);

  Serial.printf("OTA WiFi connected: RSSI=%d heap=%u max_block=%u\n",
                WiFi.RSSI(), ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
  setResetRelaySafe();
  t_httpUpdate_return result = ESPhttpUpdate.update(client, firmwareUrl);
  if (result == HTTP_UPDATE_FAILED) {
    int error = ESPhttpUpdate.getLastError();
    Serial.printf("OTA failed: %d - %s\n", error,
                  ESPhttpUpdate.getLastErrorString().c_str());
    writeOtaRtcState(OTA_RTC_FAILED, error);
  } else if (result == HTTP_UPDATE_NO_UPDATES) {
    Serial.println("OTA server returned no update");
    writeOtaRtcState(OTA_RTC_FAILED, OTA_ERROR_NO_UPDATE);
  }

  setResetRelaySafe();
  delay(500);
  ESP.restart();
}

void handleOtaBootState() {
  OtaRtcState state = {};
  if (!readOtaRtcState(state)) {
    return;
  }

  if (state.phase == OTA_RTC_REQUESTED) {
    writeOtaRtcState(OTA_RTC_RUNNING);
    runCleanOtaMode();
    return;
  }

  if (state.phase == OTA_RTC_SUCCESS) {
    Serial.println("Last OTA: success");
  } else if (state.phase == OTA_RTC_FAILED) {
    Serial.printf("Last OTA: failed (%d)\n", state.error);
  } else if (state.phase == OTA_RTC_RUNNING) {
    Serial.println("Last OTA: interrupted/reset");
  }
  clearOtaRtcState();
}

BLYNK_CONNECTED() {
}
BLYNK_WRITE(V0) {
  int pinValue = param.asInt();
  if (pinValue == 1) {
    digitalWrite(D3, LOW);
  } else if (pinValue == 0) {
    digitalWrite(D3, HIGH);
  } else if (pinValue == 3) {
    setResetRelaySafe();
    Blynk.virtualWrite(V0, 0);
    if (!writeOtaRtcState(OTA_RTC_REQUESTED)) {
      Serial.println("OTA request could not be saved to RTC memory");
      return;
    }
    Serial.println("OTA requested from V0=3; restarting into clean mode");
    delay(250);
    ESP.restart();
  }
}
//-------------------------
void setup() {
  setResetRelaySafe();
  Serial.begin(9600);
  delay(20);
  Serial.printf("\nBOOT firmware=%s reset=%s\n", BLYNK_FIRMWARE_VERSION,
                ESP.getResetReason().c_str());
  handleOtaBootState();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(5000);
}
void loop() {
  Blynk.run();
}
