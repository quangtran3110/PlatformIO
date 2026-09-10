#define BLYNK_TEMPLATE_ID "TMPL6xNbwEQiD"
#define BLYNK_TEMPLATE_NAME "TRAM2.G3   TRAM4"
#define BLYNK_AUTH_TOKEN "XQjby78lmTxxCG7JiC_-fyN7qEA-YrGE"

#define BLYNK_FIRMWARE_VERSION "260910.1"
#define BLYNK_PRINT Serial
#define APP_DEBUG

#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <I2C_eeprom.h>
#include <RTClib.h>
#include <SPI.h>
#include <TimeLib.h>
#include <WiFiClientSecure.h>
#include <UrlEncode.h>
#include <Wire.h>

const char *ssid = "Hiddennet";
const char *password = "Password";

const char *MAIN_TOKEN = "BDm1LNQi_LhtaKAQU8RWUaGbiOyKIcd3";
const char *BLYNK_API_BASE = "https://sgp1.blynk.cloud/external/api/";
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/VOLUME/TRAM2_G3/.pio/build/nodemcuv2/firmware.bin"

constexpr uint8_t EEPROM_I2C_ADDRESS = 0x57;
constexpr uint16_t EEPROM_SIZE = 4096;
constexpr uint8_t EEPROM_PAGE_SIZE = 32;

constexpr uint8_t STATE_SLOT_COUNT = 64;
constexpr uint8_t DAILY_QUEUE_CAPACITY = 40;
constexpr uint16_t DAILY_QUEUE_START = STATE_SLOT_COUNT * EEPROM_PAGE_SIZE;

constexpr uint32_t STATE_MAGIC = 0x47334451UL; // "G3DQ"
constexpr uint16_t STATE_SCHEMA_VERSION = 1;
constexpr uint16_t STATE_FLAG_RTC_TRUSTED = 0x0001;
constexpr uint8_t DAILY_RECORD_FLAG_VALID = 0x01;

constexpr int32_t LOCAL_UTC_OFFSET_SECONDS = 7L * 60L * 60L;
constexpr uint32_t MIN_VALID_UNIX = 1704067200UL; // 2024-01-01 00:00:00 UTC
constexpr uint32_t MAX_VALID_UNIX = 4102444799UL; // 2099-12-31 23:59:59 UTC

constexpr uint32_t LIVE_UPLOAD_INTERVAL_MS = 90UL * 1000UL;
constexpr uint32_t DAILY_RETRY_MIN_MS = 15000UL;
constexpr uint32_t DAILY_RETRY_MAX_MS = 300000UL;
constexpr uint32_t PULSE_PERSIST_MIN_INTERVAL_MS = 250UL;
// 100 pulse/hour gives 36 seconds minimum; keep 6 seconds of flow tolerance.
constexpr uint32_t MIN_VALID_PULSE_INTERVAL_US = 30000000UL;
// The meter output is configured for a 4.2-second pulse.
constexpr uint32_t MIN_VALID_PULSE_WIDTH_US = 3200000UL;
constexpr uint32_t MAX_VALID_PULSE_WIDTH_US = 5200000UL;
constexpr uint16_t HTTP_TIMEOUT_MS = 5000;
constexpr uint8_t FLOW_PULSE_PIN = D6;
constexpr uint8_t PULSE_ACTIVE_LEVEL = HIGH;

// Keep this block above byte 128 because eboot uses the first 128 bytes of RTC
// user memory while applying an OTA image.
constexpr uint32_t OTA_RTC_OFFSET_WORDS = 32;
constexpr uint32_t OTA_RTC_MAGIC = 0x4F544131UL; // "OTA1"
constexpr uint32_t OTA_WIFI_TIMEOUT_MS = 45000UL;
constexpr int32_t OTA_ERROR_WIFI_TIMEOUT = -1001;

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

struct __attribute__((packed)) PersistedState {
  uint32_t magic;
  uint16_t schemaVersion;
  uint16_t flags;
  uint32_t generation;
  uint32_t activePulse;
  uint32_t activeDayKey;
  uint32_t lastRtcUnix;
  uint16_t nextSequence;
  uint8_t queueHead;
  uint8_t queueCount;
  uint16_t reserved;
  uint16_t crc;
};

struct __attribute__((packed)) DailyRecord {
  uint32_t timestampUtc;
  uint32_t pulse;
  uint16_t sequence;
  uint8_t flags;
  uint8_t reserved[3];
  uint16_t crc;
};

static_assert(sizeof(PersistedState) == EEPROM_PAGE_SIZE,
              "PersistedState must occupy exactly one EEPROM page");
static_assert(sizeof(DailyRecord) <= EEPROM_PAGE_SIZE,
              "DailyRecord must fit in one EEPROM page");
static_assert(DAILY_QUEUE_START + DAILY_QUEUE_CAPACITY * EEPROM_PAGE_SIZE <= EEPROM_SIZE,
              "EEPROM layout exceeds AT24C32 capacity");

I2C_eeprom ee(EEPROM_I2C_ADDRESS, EEPROM_SIZE);
RTC_DS3231 rtcModule;
BearSSL::WiFiClientSecure apiClient;
BlynkTimer timer;

PersistedState state = {};
volatile uint32_t pulseCount = 0;
volatile uint32_t rejectedPulseCount = 0;
volatile uint32_t lastAcceptedPulseMicros = 0;
volatile uint32_t lastPulseEdgeMicros = 0;
volatile bool pulseFilterArmed = false;
volatile bool pulseEdgeTracking = false;

bool storageReady = false;
bool rtcPresent = false;
bool rtcTrusted = false;
bool keyI2cScan = false;
bool clockRollbackReported = false;
bool queueFullReported = false;

int16_t currentStateSlot = -1;
uint32_t lastPersistedPulse = 0;
uint32_t nextPulsePersistMs = 0;
uint32_t nextLiveUploadMs = 5000UL;
uint32_t lastLiveSentPulse = UINT32_MAX;
uint32_t nextDailyUploadMs = 5000UL;
uint32_t dailyRetryMs = DAILY_RETRY_MIN_MS;

String terminalText;
String lastOtaStatus = "none";
int8_t lastOtaProgressBucket = -1;

uint32_t otaRtcCheck(const OtaRtcState &otaState) {
  return otaState.magic ^ otaState.phase ^ static_cast<uint32_t>(otaState.error) ^
         0xA55A3CC3UL;
}

bool writeOtaRtcState(OtaRtcPhase phase, int32_t error = 0) {
  OtaRtcState otaState = {OTA_RTC_MAGIC, static_cast<uint32_t>(phase), error, 0};
  otaState.check = otaRtcCheck(otaState);
  return ESP.rtcUserMemoryWrite(OTA_RTC_OFFSET_WORDS,
                                reinterpret_cast<uint32_t *>(&otaState),
                                sizeof(otaState));
}

bool readOtaRtcState(OtaRtcState &otaState) {
  if (!ESP.rtcUserMemoryRead(OTA_RTC_OFFSET_WORDS,
                             reinterpret_cast<uint32_t *>(&otaState),
                             sizeof(otaState))) {
    return false;
  }
  return otaState.magic == OTA_RTC_MAGIC && otaState.check == otaRtcCheck(otaState);
}

void clearOtaRtcState() {
  OtaRtcState emptyState = {};
  ESP.rtcUserMemoryWrite(OTA_RTC_OFFSET_WORDS,
                         reinterpret_cast<uint32_t *>(&emptyState),
                         sizeof(emptyState));
}

uint16_t crc16Ccitt(const uint8_t *buffer, size_t length) {
  uint16_t crc = 0xFFFF;
  while (length--) {
    crc ^= static_cast<uint16_t>(*buffer++) << 8;
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

uint32_t readPulseCount() {
  noInterrupts();
  uint32_t value = pulseCount;
  interrupts();
  return value;
}

uint32_t readRejectedPulseCount() {
  noInterrupts();
  uint32_t value = rejectedPulseCount;
  interrupts();
  return value;
}

void releasePulseFiltersAfterQuietPeriod() {
  noInterrupts();
  uint32_t nowMicros = micros();
  if (pulseFilterArmed &&
      static_cast<uint32_t>(nowMicros - lastAcceptedPulseMicros) >=
          MIN_VALID_PULSE_INTERVAL_US) {
    pulseFilterArmed = false;
  }
  if (pulseEdgeTracking &&
      static_cast<uint32_t>(nowMicros - lastPulseEdgeMicros) >
          MAX_VALID_PULSE_WIDTH_US) {
    pulseEdgeTracking = false;
  }
  interrupts();
}

bool timeReached(uint32_t nowMs, uint32_t targetMs) {
  return static_cast<int32_t>(nowMs - targetMs) >= 0;
}

bool isValidUnix(uint32_t value) {
  return value >= MIN_VALID_UNIX && value <= MAX_VALID_UNIX;
}

uint32_t makeDayKey(const DateTime &localTime) {
  return static_cast<uint32_t>(localTime.year()) * 10000UL +
         static_cast<uint32_t>(localTime.month()) * 100UL + localTime.day();
}

uint32_t currentLocalDayKey() {
  // Blynk's rtc sync already supplies the device's local time.
  return makeDayKey(rtcModule.now());
}

uint32_t closeTimestampUtc(uint32_t dayKey) {
  uint16_t yearValue = dayKey / 10000UL;
  uint8_t monthValue = (dayKey / 100UL) % 100UL;
  uint8_t dayValue = dayKey % 100UL;
  if (yearValue < 2024 || yearValue > 2099 || monthValue < 1 || monthValue > 12 ||
      dayValue < 1 || dayValue > 31) {
    return 0;
  }

  DateTime localEnd(yearValue, monthValue, dayValue, 23, 59, 59);
  uint32_t localEpoch = localEnd.unixtime();
  if (localEpoch <= static_cast<uint32_t>(LOCAL_UTC_OFFSET_SECONDS)) {
    return 0;
  }
  return localEpoch - static_cast<uint32_t>(LOCAL_UTC_OFFSET_SECONDS);
}

bool isNewerGeneration(uint32_t candidate, uint32_t reference) {
  return static_cast<int32_t>(candidate - reference) > 0;
}

bool validState(const PersistedState &candidate) {
  if (candidate.magic != STATE_MAGIC || candidate.schemaVersion != STATE_SCHEMA_VERSION ||
      candidate.queueHead >= DAILY_QUEUE_CAPACITY ||
      candidate.queueCount > DAILY_QUEUE_CAPACITY || candidate.nextSequence == 0) {
    return false;
  }
  return candidate.crc ==
         crc16Ccitt(reinterpret_cast<const uint8_t *>(&candidate),
                    offsetof(PersistedState, crc));
}

bool validDailyRecord(const DailyRecord &record) {
  if ((record.flags & DAILY_RECORD_FLAG_VALID) == 0 || record.sequence == 0 ||
      !isValidUnix(record.timestampUtc)) {
    return false;
  }
  return record.crc ==
         crc16Ccitt(reinterpret_cast<const uint8_t *>(&record),
                    offsetof(DailyRecord, crc));
}

uint16_t stateSlotAddress(uint8_t slot) {
  return static_cast<uint16_t>(slot) * EEPROM_PAGE_SIZE;
}

uint16_t dailyRecordAddress(uint8_t slot) {
  return DAILY_QUEUE_START + static_cast<uint16_t>(slot) * EEPROM_PAGE_SIZE;
}

bool readDailyRecord(uint8_t slot, DailyRecord &record) {
  if (!storageReady || slot >= DAILY_QUEUE_CAPACITY) {
    return false;
  }
  if (ee.readBlock(dailyRecordAddress(slot), reinterpret_cast<uint8_t *>(&record),
                   sizeof(record)) != sizeof(record)) {
    return false;
  }
  return validDailyRecord(record);
}

bool writeDailyRecord(uint8_t slot, DailyRecord &record) {
  if (!storageReady || slot >= DAILY_QUEUE_CAPACITY) {
    return false;
  }
  record.crc = crc16Ccitt(reinterpret_cast<const uint8_t *>(&record),
                          offsetof(DailyRecord, crc));
  return ee.writeBlockVerify(dailyRecordAddress(slot),
                             reinterpret_cast<const uint8_t *>(&record), sizeof(record));
}

bool persistState() {
  if (!storageReady) {
    return false;
  }

  PersistedState candidate = state;
  candidate.magic = STATE_MAGIC;
  candidate.schemaVersion = STATE_SCHEMA_VERSION;
  candidate.activePulse = readPulseCount();
  candidate.generation = state.generation + 1UL;
  candidate.reserved = 0;
  candidate.crc = crc16Ccitt(reinterpret_cast<const uint8_t *>(&candidate),
                             offsetof(PersistedState, crc));

  uint8_t nextSlot = currentStateSlot < 0
                         ? 0
                         : static_cast<uint8_t>((currentStateSlot + 1) % STATE_SLOT_COUNT);
  if (!ee.writeBlockVerify(stateSlotAddress(nextSlot),
                           reinterpret_cast<const uint8_t *>(&candidate),
                           sizeof(candidate))) {
    Serial.println(F("EEPROM: khong ghi duoc trang thai"));
    return false;
  }

  state = candidate;
  currentStateSlot = nextSlot;
  lastPersistedPulse = candidate.activePulse;
  return true;
}

bool loadState() {
  bool found = false;
  PersistedState newest = {};
  int16_t newestSlot = -1;

  for (uint8_t slot = 0; slot < STATE_SLOT_COUNT; slot++) {
    PersistedState candidate = {};
    if (ee.readBlock(stateSlotAddress(slot), reinterpret_cast<uint8_t *>(&candidate),
                     sizeof(candidate)) != sizeof(candidate)) {
      continue;
    }
    if (!validState(candidate)) {
      continue;
    }
    if (!found || isNewerGeneration(candidate.generation, newest.generation)) {
      newest = candidate;
      newestSlot = slot;
      found = true;
    }
  }

  if (!found) {
    memset(&state, 0, sizeof(state));
    state.magic = STATE_MAGIC;
    state.schemaVersion = STATE_SCHEMA_VERSION;
    state.nextSequence = 1;
    currentStateSlot = -1;
    pulseCount = 0;
    lastPersistedPulse = 0;
    return persistState();
  }

  state = newest;
  currentStateSlot = newestSlot;
  noInterrupts();
  pulseCount = state.activePulse;
  interrupts();
  lastPersistedPulse = state.activePulse;
  return true;
}

void validateQueuedRecords() {
  if (!storageReady || state.queueCount == 0) {
    return;
  }

  uint8_t validCount = 0;
  for (uint8_t index = 0; index < state.queueCount; index++) {
    uint8_t slot = (state.queueHead + index) % DAILY_QUEUE_CAPACITY;
    DailyRecord record = {};
    if (!readDailyRecord(slot, record)) {
      break;
    }
    validCount++;
  }

  if (validCount != state.queueCount) {
    Serial.printf("EEPROM: cat hang doi tu %u con %u ban ghi hop le\n", state.queueCount,
                  validCount);
    state.queueCount = validCount;
    persistState();
  }
}

bool initializeStorage() {
  if (!ee.begin() || !ee.isConnected()) {
    Serial.printf("EEPROM: khong tim thay tai 0x%02X\n", EEPROM_I2C_ADDRESS);
    return false;
  }

  storageReady = true;
  if (!loadState()) {
    storageReady = false;
    Serial.println(F("EEPROM: khoi tao trang thai that bai"));
    return false;
  }
  validateQueuedRecords();
  Serial.printf("EEPROM: 0x%02X san sang, pending=%u, pulse=%lu\n",
                EEPROM_I2C_ADDRESS, state.queueCount,
                static_cast<unsigned long>(readPulseCount()));
  return true;
}

int apiGet(const String &url, String *response = nullptr) {
  if (WiFi.status() != WL_CONNECTED) {
    return -1;
  }
  HTTPClient http;
  if (!http.begin(apiClient, url)) {
    return -2;
  }
  http.setTimeout(HTTP_TIMEOUT_MS);
  int statusCode = http.GET();
  if (response != nullptr && statusCode > 0) {
    *response = http.getString();
  }
  http.end();
  return statusCode;
}

int apiPostJson(const String &url, const String &body, String &response) {
  if (WiFi.status() != WL_CONNECTED) {
    return -1;
  }
  HTTPClient http;
  if (!http.begin(apiClient, url)) {
    return -2;
  }
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.addHeader(F("Content-Type"), F("application/json"));
  int statusCode = http.POST(body);
  if (statusCode > 0) {
    response = http.getString();
  }
  http.end();
  return statusCode;
}

bool sendLivePulse(uint32_t pulse) {
  String url = String(BLYNK_API_BASE) + "batch/update?token=" + MAIN_TOKEN +
               "&V60=" + String(pulse);
  String response;
  int statusCode = apiGet(url, &response);
  if (statusCode == HTTP_CODE_OK) {
    return true;
  }
  Serial.printf("V60: HTTP %d\n", statusCode);
  return false;
}

bool buildDailyUploadBody(String &body) {
  if (state.queueCount == 0) {
    return false;
  }

  body = "[";
  body.reserve(static_cast<unsigned int>(state.queueCount) * 32U + 2U);
  for (uint8_t index = 0; index < state.queueCount; index++) {
    uint8_t slot = (state.queueHead + index) % DAILY_QUEUE_CAPACITY;
    DailyRecord record = {};
    if (!readDailyRecord(slot, record)) {
      Serial.printf("V63: ban ghi EEPROM loi tai slot %u\n", slot);
      return false;
    }

    if (index != 0) {
      body += ',';
    }
    char entry[48];
    unsigned long long timestampMs =
        static_cast<unsigned long long>(record.timestampUtc) * 1000ULL;
    snprintf(entry, sizeof(entry), "[%llu,%lu]", timestampMs,
             static_cast<unsigned long>(record.pulse));
    body += entry;
  }
  body += ']';
  return true;
}

bool uploadQueuedDailyRecords() {
  String body;
  if (!buildDailyUploadBody(body)) {
    return false;
  }

  String url = String(BLYNK_API_BASE) + "batch/update?token=" + MAIN_TOKEN +
               "&pin=V63";
  String response;
  int statusCode = apiPostJson(url, body, response);
  response.trim();
  if (statusCode == HTTP_CODE_OK && response == "OK") {
    Serial.printf("V63: da gui %u ban ghi timestamped\n", state.queueCount);
    return true;
  }

  Serial.printf("V63: HTTP %d, response=%s\n", statusCode, response.c_str());
  return false;
}

bool enqueueClosedDay(uint32_t newDayKey) {
  if (!storageReady) {
    return false;
  }
  if (state.queueCount >= DAILY_QUEUE_CAPACITY) {
    if (!queueFullReported) {
      queueFullReported = true;
      Serial.println(F("V63: hang doi day, chua the chot ngay"));
    }
    return false;
  }
  queueFullReported = false;

  uint32_t timestampUtc = closeTimestampUtc(state.activeDayKey);
  if (!isValidUnix(timestampUtc)) {
    Serial.println(F("RTC: ngay dang hoat dong khong hop le"));
    return false;
  }

  noInterrupts();
  uint32_t closedPulse = pulseCount;
  pulseCount = 0;
  interrupts();

  DailyRecord record = {};
  record.timestampUtc = timestampUtc;
  record.pulse = closedPulse;
  record.sequence = state.nextSequence;
  record.flags = DAILY_RECORD_FLAG_VALID;

  uint8_t tail = (state.queueHead + state.queueCount) % DAILY_QUEUE_CAPACITY;
  if (!writeDailyRecord(tail, record)) {
    noInterrupts();
    pulseCount += closedPulse;
    interrupts();
    Serial.println(F("EEPROM: khong ghi duoc du lieu chot ngay"));
    return false;
  }

  state.queueCount++;
  state.nextSequence++;
  if (state.nextSequence == 0) {
    state.nextSequence = 1;
  }
  state.activeDayKey = newDayKey;

  if (!persistState()) {
    Serial.println(F("EEPROM: ban ghi ngay da luu, dang cho luu header"));
  }

  Serial.printf("Chot ngay: ts=%lu, pulse=%lu, pending=%u\n",
                static_cast<unsigned long>(timestampUtc),
                static_cast<unsigned long>(closedPulse), state.queueCount);
  nextDailyUploadMs = millis();
  return true;
}

void serviceClockAndRollover() {
  if (!rtcPresent || !rtcTrusted) {
    return;
  }

  uint32_t rtcUnix = rtcModule.now().unixtime();
  if (!isValidUnix(rtcUnix)) {
    rtcTrusted = false;
    state.flags &= ~STATE_FLAG_RTC_TRUSTED;
    persistState();
    Serial.println(F("RTC: thoi gian khong hop le"));
    return;
  }

  uint32_t todayKey = currentLocalDayKey();
  if (state.activeDayKey == 0) {
    state.activeDayKey = todayKey;
    persistState();
    Serial.printf("RTC: bat dau ngay %lu\n", static_cast<unsigned long>(todayKey));
    return;
  }

  if (todayKey > state.activeDayKey) {
    clockRollbackReported = false;
    enqueueClosedDay(todayKey);
  } else if (todayKey < state.activeDayKey && !clockRollbackReported) {
    clockRollbackReported = true;
    Serial.printf("RTC: tu choi lui ngay tu %lu ve %lu\n",
                  static_cast<unsigned long>(state.activeDayKey),
                  static_cast<unsigned long>(todayKey));
  }
}

void applyCloudTime(uint32_t cloudUnix) {
  if (!isValidUnix(cloudUnix)) {
    Serial.printf("RTC: bo qua cloud time %lu\n", static_cast<unsigned long>(cloudUnix));
    return;
  }

  setTime(cloudUnix);
  if (!rtcPresent) {
    Serial.println(F("RTC: DS3231 khong san sang"));
    return;
  }

  rtcModule.adjust(DateTime(cloudUnix));
  rtcTrusted = true;
  state.flags |= STATE_FLAG_RTC_TRUSTED;
  state.lastRtcUnix = cloudUnix;
  persistState();
  Serial.printf("RTC: dong bo gio dia phuong %lu\n", static_cast<unsigned long>(cloudUnix));
  serviceClockAndRollover();
}

void servicePulsePersistence() {
  releasePulseFiltersAfterQuietPeriod();
  if (!storageReady) {
    return;
  }

  uint32_t currentPulse = readPulseCount();
  if (currentPulse == lastPersistedPulse || !timeReached(millis(), nextPulsePersistMs)) {
    return;
  }

  if (persistState()) {
    nextPulsePersistMs = millis() + PULSE_PERSIST_MIN_INTERVAL_MS;
  } else {
    nextPulsePersistMs = millis() + 5000UL;
  }
}

void serviceLiveUpload() {
  uint32_t nowMs = millis();
  if (!timeReached(nowMs, nextLiveUploadMs)) {
    return;
  }
  nextLiveUploadMs = nowMs + LIVE_UPLOAD_INTERVAL_MS;

  uint32_t currentPulse = readPulseCount();
  if (currentPulse == lastLiveSentPulse) {
    return;
  }
  if (sendLivePulse(currentPulse)) {
    lastLiveSentPulse = currentPulse;
  }
}

void serviceDailyUpload() {
  if (!storageReady || state.queueCount == 0) {
    return;
  }

  uint32_t nowMs = millis();
  if (!timeReached(nowMs, nextDailyUploadMs)) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    nextDailyUploadMs = nowMs + dailyRetryMs;
    return;
  }

  if (!uploadQueuedDailyRecords()) {
    nextDailyUploadMs = nowMs + dailyRetryMs;
    dailyRetryMs = dailyRetryMs >= DAILY_RETRY_MAX_MS / 2U
                       ? DAILY_RETRY_MAX_MS
                       : dailyRetryMs * 2U;
    return;
  }

  PersistedState beforeAck = state;
  uint8_t sentCount = state.queueCount;
  state.queueHead = (state.queueHead + sentCount) % DAILY_QUEUE_CAPACITY;
  state.queueCount = 0;
  queueFullReported = false;

  if (!persistState()) {
    state = beforeAck;
    nextDailyUploadMs = nowMs + dailyRetryMs;
    Serial.println(F("EEPROM: chua luu duoc xac nhan V63, se gui lai"));
    return;
  }

  dailyRetryMs = DAILY_RETRY_MIN_MS;
  nextDailyUploadMs = nowMs + DAILY_RETRY_MIN_MS;
}

void connectionStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi: dang ket noi lai"));
    WiFi.reconnect();
    return;
  }

  if (!Blynk.connected()) {
    Serial.println(F("Blynk: dang ket noi lai, khong restart ESP"));
    Blynk.connect(1000);
  }
}

void updateStarted() {
  lastOtaProgressBucket = -1;
  Serial.println(F("CALLBACK: HTTP update process started"));
  Serial.printf("OTA heap: free=%u max_block=%u fragmentation=%u%%\n",
                ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),
                ESP.getHeapFragmentation());
}

void updateFinished() {
  Serial.println(F("CALLBACK: HTTP update process finished"));
  writeOtaRtcState(OTA_RTC_SUCCESS);
}

void updateProgress(int current, int total) {
  ESP.wdtFeed();
  if (total <= 0) {
    return;
  }
  int8_t bucket = static_cast<int8_t>((static_cast<uint32_t>(current) * 10UL) /
                                      static_cast<uint32_t>(total));
  if (bucket != lastOtaProgressBucket) {
    lastOtaProgressBucket = bucket;
    Serial.printf("OTA progress: %d%% (%d/%d bytes)\n", bucket * 10, current, total);
  }
}

void updateError(int error) {
  Serial.printf("CALLBACK: HTTP update fatal error code %d\n", error);
  writeOtaRtcState(OTA_RTC_FAILED, error);
}

void updateFirmware() {
  BearSSL::WiFiClientSecure updateClient;
  updateClient.setInsecure();
  ESPhttpUpdate.onStart(updateStarted);
  ESPhttpUpdate.onEnd(updateFinished);
  ESPhttpUpdate.onProgress(updateProgress);
  ESPhttpUpdate.onError(updateError);

  t_httpUpdate_return result = ESPhttpUpdate.update(updateClient, URL_fw_Bin);
  switch (result) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(),
                  ESPhttpUpdate.getLastErrorString().c_str());
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println(F("HTTP_UPDATE_NO_UPDATES"));
    break;
  case HTTP_UPDATE_OK:
    Serial.println(F("HTTP_UPDATE_OK"));
    break;
  }
}

void runCleanOtaMode() {
  Serial.printf("OTA clean boot: firmware=%s reset=%s\n", BLYNK_FIRMWARE_VERSION,
                ESP.getResetReason().c_str());
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, password);

  uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         static_cast<uint32_t>(millis() - startedAt) < OTA_WIFI_TIMEOUT_MS) {
    ESP.wdtFeed();
    delay(50);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("OTA clean boot: WiFi timeout"));
    writeOtaRtcState(OTA_RTC_FAILED, OTA_ERROR_WIFI_TIMEOUT);
    delay(500);
    ESP.restart();
    return;
  }

  Serial.printf("OTA WiFi connected: RSSI=%d heap=%u max_block=%u\n", WiFi.RSSI(),
                ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
  updateFirmware();

  // A successful update normally restarts inside ESPhttpUpdate. Reaching this
  // point means no new image was installed, so return to the normal firmware.
  delay(500);
  ESP.restart();
}

bool handleOtaBootState() {
  OtaRtcState otaState = {};
  if (!readOtaRtcState(otaState)) {
    return false;
  }

  if (otaState.phase == OTA_RTC_REQUESTED) {
    writeOtaRtcState(OTA_RTC_RUNNING);
    runCleanOtaMode();
    return true;
  }

  if (otaState.phase == OTA_RTC_SUCCESS) {
    lastOtaStatus = "success";
  } else if (otaState.phase == OTA_RTC_FAILED) {
    lastOtaStatus = "failed (" + String(otaState.error) + ")";
  } else if (otaState.phase == OTA_RTC_RUNNING) {
    lastOtaStatus = "interrupted/reset";
  }
  Serial.println("Last OTA: " + lastOtaStatus);
  clearOtaRtcState();
  return false;
}

void printTerminalDevice() {
  Serial.println(terminalText);
  String messageUrl = String(BLYNK_API_BASE) + "batch/update?token=" +
                      BLYNK_AUTH_TOKEN + "&V0=" + urlEncode(terminalText);
  int messageStatus = apiGet(messageUrl);
  if (messageStatus != HTTP_CODE_OK) {
    Serial.printf("V0 API: HTTP %d\n", messageStatus);
  }
}

void scanI2cOnce() {
  if (!keyI2cScan) {
    return;
  }
  keyI2cScan = false;

  String report = "Firmware: " BLYNK_FIRMWARE_VERSION "\n";
  report += "Reset: " + ESP.getResetReason() + "\n";
  report += "Last OTA: " + lastOtaStatus + "\n";
  report += "Heap: " + String(ESP.getFreeHeap()) +
            ", max block: " + String(ESP.getMaxFreeBlockSize()) + "\n";
  report += "I2C scan:\n";
  uint8_t found = 0;
  uint8_t scanErrors = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      char line[24];
      snprintf(line, sizeof(line), "- found 0x%02X\n", address);
      report += line;
      found++;
    } else if (error != 2) {
      scanErrors++;
    }
  }
  if (found == 0) {
    report += "- I2C ERROR: no device found\n";
  }
  if (scanErrors > 0) {
    report += "- I2C ERROR: bus communication failure (" + String(scanErrors) + ")\n";
  }
  report += "Pulse accepted: " + String(readPulseCount()) + "\n";
  report += "Pulse rejected: " + String(readRejectedPulseCount()) + "\n";
  report += "WiFi: " + String(WiFi.RSSI()) + " dBm\n";
  terminalText = report;
  printTerminalDevice();
}

BLYNK_CONNECTED() {
  Blynk.sendInternal("rtc", "sync");
}

BLYNK_WRITE(InternalPinRTC) {
  uint32_t cloudUnix = strtoul(param.asStr(), nullptr, 10);
  applyCloudTime(cloudUnix);
}

BLYNK_WRITE(V0) {
  String command = param.asStr();
  if (command == "rst") {
    persistState();
    terminalText = "ESP khoi dong lai sau 3s";
    printTerminalDevice();
    delay(3000);
    ESP.restart();
  } else if (command == "update") {
    persistState();
    terminalText = "Da nhan lenh OTA; ESP se khoi dong vao che do cap nhat sach";
    printTerminalDevice();
    apiClient.stop();
    if (!writeOtaRtcState(OTA_RTC_REQUESTED)) {
      terminalText = "OTA error: khong ghi duoc yeu cau vao RTC memory";
      printTerminalDevice();
      return;
    }
    delay(250);
    ESP.restart();
  } else if (command == "rst_vl") {
    noInterrupts();
    pulseCount = 0;
    interrupts();
    if (rtcTrusted) {
      state.activeDayKey = currentLocalDayKey();
    }
    persistState();
    lastLiveSentPulse = UINT32_MAX;
    terminalText = "Da reset Volume hien tai; giu nguyen du lieu ngay dang cho gui";
    printTerminalDevice();
  } else if (command == "i2c") {
    keyI2cScan = true;
  } else if (command == "savedata") {
    bool saved = persistState();
    terminalText = saved ? "DATA_SAVE... ok!" : "DATA_SAVE... error!";
    printTerminalDevice();
  }
}

IRAM_ATTR void buttonPressed() {
  uint32_t nowMicros = micros();

  if (digitalRead(FLOW_PULSE_PIN) == PULSE_ACTIVE_LEVEL) {
    lastPulseEdgeMicros = nowMicros;
    pulseEdgeTracking = true;
    return;
  }

  if (!pulseEdgeTracking) {
    return;
  }

  uint32_t pulseWidth = nowMicros - lastPulseEdgeMicros;
  pulseEdgeTracking = false;
  if (pulseWidth < MIN_VALID_PULSE_WIDTH_US ||
      pulseWidth > MAX_VALID_PULSE_WIDTH_US) {
    rejectedPulseCount++;
    return;
  }

  if (pulseFilterArmed &&
      static_cast<uint32_t>(nowMicros - lastAcceptedPulseMicros) <
          MIN_VALID_PULSE_INTERVAL_US) {
    rejectedPulseCount++;
    return;
  }

  lastAcceptedPulseMicros = nowMicros;
  pulseFilterArmed = true;
  pulseCount++;
}

void setup() {
  Serial.begin(115200);
  delay(20);
  Serial.printf("\nBOOT firmware=%s reset=%s\n", BLYNK_FIRMWARE_VERSION,
                ESP.getResetReason().c_str());
  if (handleOtaBootState()) {
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  apiClient.setInsecure();

  Wire.begin();
  rtcPresent = rtcModule.begin();
  bool rtcLostPower = true;
  if (rtcPresent) {
    rtcLostPower = rtcModule.lostPower();
  } else {
    Serial.println(F("RTC: khong tim thay DS3231 tai 0x68"));
  }

  initializeStorage();

  if (rtcPresent && !rtcLostPower && (state.flags & STATE_FLAG_RTC_TRUSTED) != 0 &&
      isValidUnix(rtcModule.now().unixtime())) {
    rtcTrusted = true;
    Serial.println(F("RTC: dung thoi gian DS3231 da dong bo"));
  } else {
    rtcTrusted = false;
    if ((state.flags & STATE_FLAG_RTC_TRUSTED) != 0) {
      state.flags &= ~STATE_FLAG_RTC_TRUSTED;
      persistState();
    }
    Serial.println(F("RTC: cho dong bo thoi gian tu Blynk"));
  }

  pinMode(FLOW_PULSE_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(FLOW_PULSE_PIN), buttonPressed, CHANGE);
  serviceClockAndRollover();

  timer.setInterval(1000L, serviceClockAndRollover);
  timer.setInterval(250L, servicePulsePersistence);
  timer.setInterval(5000L, serviceLiveUpload);
  timer.setInterval(5000L, serviceDailyUpload);
  timer.setInterval(60000L, connectionStatus);
  timer.setInterval(1000L, scanI2cOnce);
}

void loop() {
  Blynk.run();
  timer.run();
}
