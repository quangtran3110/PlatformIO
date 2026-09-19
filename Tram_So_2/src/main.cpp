/* V0-chọn người vận hành
   V1-date/time
   V2-Rua loc
   V3-key_noti
   V4-on/off bao ve ampe
   V5-keyterminal
   V10-Áp suất
   V11-Chọn máy để cài bảo vệ
   V12-Chọn giá trị bảo vệ Min
   V13-Chọn giá trị bảo vệ Max
   V14-
   V15 - on Bom 30kw
   V16 - off bom 30kw
   V17 -
   V18 -
   V19 -
   V20 - ONG3
   ...
   V32- I7
   V33-
   V34-con lai
   V35-volume
   V36- I8
   V37-dosau
   V38-statusPRE
   V39-statusRL
   V40-I0
   ...
   V46-I6
   V47- btn nén khí 1
   V48- btn nén khí 2
   V49-
   V50-volume_terminal
   V51-unused
   V52-quality: 2 good, 1 usable, 0 poor/offline
   V53-
   V56-unused
   V57-
   V58-G2 LL1m3
   V59-G2 LL24h
   V60-G3 LL1m3
   V61-G1 LL24H
   V62-G1 LL1m3
   V63-G3 LL24h
   V64-LLG1_RL
   V65-LLG2_RL
   V66-LLG3_RL

   V70 - TIMERUN_G1
   V71 - TIMERUN_G1_24H
   ...
   V83 - TIMERUN_B4_24H
*/
/*
#define BLYNK_TEMPLATE_ID "TMPLK0N90h0w"
#define BLYNK_TEMPLATE_NAME "Trạm Số 2"
#define BLYNK_AUTH_TOKEN "ESzia3fpA-29cs8gt85pGnrPq_rICcqf"
*/
#define BLYNK_TEMPLATE_ID "TMPL6iEcXJQ6i"
#define BLYNK_TEMPLATE_NAME "TRẠM SỐ 2"
#define BLYNK_AUTH_TOKEN "BDm1LNQi_LhtaKAQU8RWUaGbiOyKIcd3"
/*#define BLYNK_AUTH_TOKEN "JA2vIDxR4JMV732MUdsZ50i3r1xEAaxc"*/ // test
#define VOLUME_TOKEN_G1 "L_2oEOyv4bmrdsesIoasyKiEEOFZVgBO"
#define VOLUME_TOKEN_G2 "Hc5DgCBzl4Oi5hW_JOaNZ6oBKoGy5kFI"
#define VOLUME_TOKEN_G3 "JTnEpJjGVVJ8DM1aJx7zZT4cyNYJrhr_"

#define BLYNK_FIRMWARE_VERSION "260917.2"
#define BLYNK_PRINT Serial
#define APP_DEBUG

//-----------------------------
#define BLYNK_MSG_LIMIT 0 // Rate limited by serviceNetwork(), never busy-wait.
#define BLYNK_HEARTBEAT 10
#define BLYNK_TIMEOUT_MS 3000UL
#include "network_io.h"
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <UrlEncode.h>
#include <Wire.h>
const char *ssid = "Hiddennet";
const char *password = "Password";
/*const char *ssid = "tram bom so 4";
const char *password = "0943950555";*/
//-----------------------------
#include "EmonLib.h"
EnergyMonitor emon0, emon1, emon2, emon3, emon4, emon5, emon6, emon7, emon8;
int xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0, xSetAmpe3 = 0, xSetAmpe4 = 0, xSetAmpe5 = 0, xSetAmpe6 = 0, xSetAmpe7 = 0, xSetAmpe8 = 0;
unsigned long int yIrms0 = 0, yIrms1 = 0, yIrms2 = 0, yIrms3 = 0, yIrms4 = 0, yIrms5 = 0, yIrms6 = 0, yIrms7 = 0, yIrms8 = 0, dem1 = 0, dem2 = 0, dem3 = 0;
unsigned long int xIrms0 = 0, xIrms1 = 0, xIrms2 = 0, xIrms3 = 0, xIrms4 = 0, xIrms5 = 0, xIrms6 = 0, xIrms7 = 0, xIrms8 = 0;
float Irms0, Irms1, Irms2, Irms3, Irms4, Irms5, Irms6, Irms7, Irms8, pre;
bool trip0 = false, trip1 = false, trip2 = false, trip3 = false, trip4 = false, trip5 = false, trip6 = false, trip7 = false, trip8 = false;
//-----------------------------
#include "RTClib.h"
#include <WidgetRTC.h>
RTC_DS3231 rtc_module;
char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
//-----------------------------
#include <Eeprom24C32_64.h>
#include "station_store.h"
#define MEMORY_SIZE 4096
#define PAGE_SIZE 32
I2C_eeprom ee(0x57, MEMORY_SIZE);
//-----------------------------

#include "pump_schedule.h"
#include <math.h>
const int pin_on_G1 = 7;
const int pin_off_G1 = 6;
const int pin_on_G2 = 5;
const int pin_off_G2 = 4;
const int pin_on_G3 = 3;
const int pin_off_G3 = 2;
const int pin_NK1 = 1;
const int pin_NK2 = 0;
const int pin_on_Bom1 = 8; // 18
const int pin_off_Bom1 = 9;
const int pin_on_Bom2 = 10; // 30
const int pin_off_Bom2 = 11;
const int pin_on_Bom3 = 12; // 7.5
const int pin_off_Bom3 = 13;
const int pin_on_Bom4 = 14; // 11
const int pin_off_Bom4 = 15;
const int pin_WATCHDOG = 15; // Chọn chân GPIO tùy chỉnh để điều khiển thiết bị khác
const int Pin8 = 8;

//-----------------------------
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <SimpleKalmanFilter.h>
// Bạn cần "tune" 3 giá trị này để có kết quả tốt nhất. Hãy bắt đầu với các giá trị này.
SimpleKalmanFilter levelKalmanFilter(2, 2, 0.01);
String Tram2_Rualoc = "f_mIttU4MH80_pakaBYWjXq1cOWpqqYg";
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/refs/heads/main/Tram_So_2/.pio/build/nodemcuv2/firmware.bin"
//-----------------------------
const int S0 = 3;
const int S1 = 2;
const int S2 = 1;
const int S3 = 0;
//----------------------------------
const int dai = 2000;
const int rong = 1000;
const int dosau = 515; // Chiều cao tối đa của bể (cm)
const float LEVEL_SENSOR_BOTTOM_OFFSET_CM = 100.0f; // Cảm biến đặt cao hơn đáy bể 1 m
int volume, volume1, dungtich;
float smoothDistance; // Thay int bằng float để có độ chính xác cao hơn

// --- BỘ LỌC KẾT HỢP: MEDIAN + KALMAN ---
const int MEDIAN_WINDOW_SIZE = 5;
int median_buffer[MEDIAN_WINDOW_SIZE];
int median_buffer_index = 0;
float kalman_filtered_adc_value = 0; // Biến lưu giá trị ADC đã lọc, dùng cho hiệu chuẩn

// Hàm sắp xếp và lấy trung vị
int getMedian(int arr[], int size) {
  for (int i = 1; i < size; i++) {
    int key = arr[i];
    int j = i - 1;
    while (j >= 0 && arr[j] > key) {
      arr[j + 1] = arr[j];
      j = j - 1;
    }
    arr[j + 1] = key;
  }
  return arr[size / 2];
}
//----------------------------------
bool keySwitchQ = false, keySwitchD = false, keySwitchP = false, keySet = false, data12 = true, data13 = true, keyPRE2 = true, keyPRE4 = true, noti = true;
bool event30p = true;
bool blynk_first_connect = false;
int timer_rtc, timer_I, timer_tank;
int z, n, m;
byte status_g1, status_g2, status_g3, status_b1, status_b2, status_b3, status_b4;
int LLG2_1m3, LLG1_1m3, LLG3_1m3;
const unsigned long WATCHDOG_TOGGLE_INTERVAL_MS = 5000UL;
uint8_t watchdogOutputLevel = LOW;
unsigned long watchdogLastToggleMs = 0;
bool otaWatchdogReady = false;
const uint32_t OTA_RTC_OFFSET_WORDS = 32;
const uint32_t OTA_RTC_MAGIC = 0x4F544132UL; // "OTA2"
const unsigned long OTA_WIFI_TIMEOUT_MS = 45000UL;
const int32_t OTA_ERROR_WIFI_TIMEOUT = -1001;
const int32_t OTA_ERROR_WATCHDOG_INIT = -1002;
const int32_t OTA_ERROR_NO_UPDATE = -1003;
const int OTA_NETWORK_TIMEOUT_MS = 8000;
int8_t lastOtaProgressBucket = -1;
String lastOtaStatus = "none";

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

void serviceExternalWatchdog();
bool rearmExternalWatchdog();

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
// Khi Auto dang thu khoi dong bom, state machine se tu xu ly truong hop
// chua co dong. Bao ve qua/thieu dong sau khi bom da len dong van giu nguyen.
bool auto_start_guard_b2 = false;
bool auto_start_guard_b4 = false;
//----------------------------------
#define DATA_VERSION 4

#define MAX_CALIB_POINTS 5
struct CalibPoint {
  uint16_t adc;   // Giá trị ADC (0-1023)
  uint16_t value; // Giá trị quy đổi (cm)
};

struct Data {
  uint8_t version;
  byte SetAmpemax, SetAmpemin;
  byte SetAmpe1max, SetAmpe1min;
  byte SetAmpe2max, SetAmpe2min;
  byte SetAmpe3max, SetAmpe3min;
  byte SetAmpe4max, SetAmpe4min;
  byte SetAmpe5max, SetAmpe5min;
  byte SetAmpe6max, SetAmpe6min;
  byte SetAmpe7max, SetAmpe7min;
  byte SetAmpe8max, SetAmpe8min;
  byte man;
  int save_num;
  byte time_run_nk1, time_stop_nk1, time_run_nk2, time_stop_nk2;
  byte status_rualoc;
  uint32_t runtime_date, legacy_ll_g2; // Same EEPROM offsets; runtime_date = YYYYMMDD.
  uint8_t reserved_legacy[sizeof(float) + sizeof(int)]; // Unused EEPROM bytes.
  int LLG2_RL, LLG1_RL, LLG3_RL;
  byte protect;
  byte reset_day;
  uint32_t timerun_G1, timerun_G2, timerun_G3, timerun_B1, timerun_B2, timerun_B3, timerun_B4; // Seconds in version 4.
  byte key_noti;
  // -- BIẾN CHẠY LUÂN PHIÊN BƠM 2 VÀ BƠM 4 --
  bool en_auto_b2_b4;                 // Cờ Bật/Tắt chế độ Auto luân phiên
  uint8_t auto_start_h, auto_start_m; // Giờ bắt đầu (Bơm 4 chạy, Bơm 2 tắt)
  uint8_t auto_stop_h, auto_stop_m;   // Giờ kết thúc (Bơm 4 tắt, Bơm 2 chạy)
  // Thêm các trường hiệu chuẩn cho cảm biến mực nước
  CalibPoint level_points[MAX_CALIB_POINTS];
  uint8_t num_level_points;
} data, dataCheck;
const struct Data dataDefault = {};
StationStore<Data, offsetof(Data, reserved_legacy)> cs;

// Boot guards. EEPROM version 4 preserves the existing 120-byte layout.
bool relayReady = false, rtcReady = false, configLoaded = false;
bool controlReady = false, currentScanValid = false;
bool auto_resync_required = false;
uint8_t goodCurrentScans = 0;
unsigned long currentSampleMs = 0;

uint32_t relayPulseUntil[16] = {};
uint16_t relayWord = 0xFFFF, auxWord = 0x0100;
uint8_t channelGood[9] = {};
uint16_t channelValid = 0;
uint32_t channelSampleMs[9] = {};
uint8_t motorRunning = 0, motorStopping = 0;
uint32_t motorSince[7] = {};
uint16_t motorRemainder[7] = {}; // Subsecond part is RAM only.
uint32_t accessUntil[3] = {};
bool storageDirty = false, storageUrgent = false, storageFailed = false;
bool saveReport = false;
uint32_t lastStorageWrite = 0, lastStorageAttempt = 0;
enum RestartRequest : uint8_t { RESTART_NONE, RESTART_NORMAL, RESTART_OTA };
RestartRequest restartRequest = RESTART_NONE;
uint32_t restartNotBefore = 0;
uint32_t noticeSince[7] = {};
uint8_t noticeActor[7] = {};
uint8_t noticePending = 0, noticeOn = 0;
uint8_t draftMin = 0, draftMax = 0, draftMotor = 0;
uint8_t levelSamples = 0;
uint32_t levelSampleMs = 0;
uint32_t lastRuntimePublish = 0;
bool runtimePublished = false;
uint16_t helpPosition = UINT16_MAX;
uint32_t helpNext = 0;

bool validStoredData(const Data &value);
bool channelFresh(uint8_t channel);
bool canProtect(uint8_t channel, byte minimum, byte maximum);
bool writePcfWord(uint8_t address, uint16_t word);
bool writeAuxPin(uint8_t pin, uint8_t value);
void recordCurrentSample(uint8_t channel);
void noteMotorSample(uint8_t channel);
void collectMotorRuntimes();
void handleManualMotor(uint8_t motor, bool on, const BlynkParam &param);
void serviceMotorNotices();
void serviceOperatorAccess();
void grantOperatorAccess(uint8_t actor, uint32_t duration);
void clearOperatorAccess();
void serviceStorage();
void serviceRestartRequest();
bool pulsesPending();
void prepareCurrentDraft();
void setCurrentDraft(bool minimum, const BlynkParam &param);
void serviceTerminalHelp();

bool validCurrentLimits(byte minimum, byte maximum) {
  return maximum > 3 && maximum > minimum;
}

bool currentInRange(float current, byte minimum, byte maximum, bool exclusiveMaximum = false) {
  return isfinite(current) && current > 3 &&
         validCurrentLimits(minimum, maximum) &&
         current >= minimum && (exclusiveMaximum ? current < maximum : current <= maximum);
}

bool selectAnalogChannel(uint8_t channel) {
  // S0=P3, S1=P2, S2=P1, S3=P0: one write, preserve watchdog/P8.
  uint16_t bits = ((channel & 1) << 3) | ((channel & 2) << 1) |
                  ((channel & 4) >> 1) | ((channel & 8) >> 3);
  auxWord = (auxWord & 0xFFF0) | bits;
  if (!writePcfWord(0x20, auxWord)) {
    currentScanValid = false;
    controlReady = false;
    if (channel < 9) { channelGood[channel] = 0; channelValid &= ~(uint16_t(1) << channel); }
    else levelSamples = 0;
    return false;
  }
  delayMicroseconds(200);
  return true;
}

bool writeRelay(uint8_t pin, uint8_t value) {
  if (pin > 15) return false;
  const bool on = pin >= 2 && ((uint16_t(1) << pin) & 0x55A8);
  static const uint8_t channels[] = {5, 1, 3, 0, 6, 4, 2};
  uint8_t channel = pin < 2 ? (pin == 1 ? 7 : 8) : channels[(pin - 2) / 2];
  if (value == LOW) {
    if (!relayReady || !configLoaded) return false;
    if (on) {
      if (!controlReady || restartRequest != RESTART_NONE ||
          uint32_t(millis() - currentSampleMs) > 5000) return false;
    } else if (!channelFresh(channel)) return false; // Local STOP uses its own channel.
  }
  const uint32_t now = millis();
  // No expired command may reappear in another pin's full-word write.
  for (uint8_t k = 2; k < 16; ++k)
    if (relayPulseUntil[k] && int32_t(now - relayPulseUntil[k]) >= 0)
      relayWord |= uint16_t(1) << k;
  const uint16_t bit = uint16_t(1) << pin;
  if (value == HIGH) relayWord |= bit;
  else relayWord &= ~bit;
  bool ok = writePcfWord(0x21, relayWord);
  if (!ok) {
    relayReady = false; controlReady = false;
    if (value == LOW && on) {
      // Delivery is uncertain. Never replay this ON, but always attempt release.
      relayWord |= bit;
      relayPulseUntil[pin] = now ? now : 1;
    }
    Serial.println(F("Relay I2C error; starts blocked, pending releases retained"));
  }
  return ok;
}

bool readStationTime(DateTime &now) {
  if (!rtcReady)
    return false;
  Wire.beginTransmission(0x68);
  Wire.write(0);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(0x68, 16) != 16)
    return false;
  uint8_t raw[16];
  for (uint8_t k = 0; k < 16; ++k)
    raw[k] = Wire.read();
  // Do not use a stopped RTC, 12-hour register or malformed BCD for control.
  if ((raw[15] & 0x80) || (raw[2] & 0x40) || (raw[5] & 0x80))
    return false;
  const uint8_t indices[] = {0, 1, 2, 4, 5, 6};
  for (uint8_t k : indices) {
    if ((raw[k] & 0x0F) > 9 || (raw[k] >> 4) > 9)
      return false;
  }
  auto decimal = [](uint8_t bcd) -> uint8_t { return (bcd >> 4) * 10 + (bcd & 15); };
  now = DateTime(2000 + decimal(raw[6]), decimal(raw[5]), decimal(raw[4]),
                 decimal(raw[2]), decimal(raw[1]), decimal(raw[0]));
  return now.isValid() && now.year() >= 2026 && now.year() <= 2099;
}

bool validLevelCalibration() {
  if (data.num_level_points < 2 || data.num_level_points > MAX_CALIB_POINTS)
    return false;
  for (uint8_t k = 0; k < data.num_level_points; ++k) {
    if (data.level_points[k].adc > 1023 ||
        (k > 0 && data.level_points[k].adc <= data.level_points[k - 1].adc))
      return false;
  }
  return true;
}

// Fixed, small state: no command queue and no dynamic telemetry backlog.
StationHttp remoteHttp;
uint32_t remoteLastCommand[16] = {};
uint16_t remoteCommandSeen = 0;
uint16_t remoteCommandHigh = 0;
bool currentScanRequested = false;
uint32_t lastTelemetryAttempt = 0, lastTelemetryAck = 0, lastProbeAttempt = 0;
uint32_t lastStatusPublish = 0, previousLoopTick = 0;
int8_t publishedNetworkLevel = -1;
uint16_t telemetryProbe = 0;
bool dailySending = false;
uint8_t pendingDailyDay = 0;
uint32_t pendingDailyDate = 0;
uint32_t dailySnapshot[7] = {};



bool startRelayPulse(uint8_t pin, uint32_t duration);
void serviceRelayPulses();
void serviceNetwork();
void serviceCurrentScan();
bool acceptRemoteControl(uint8_t pin, const BlynkParam &param);


WidgetTerminal keyterminal(V5);
WidgetTerminal volume_terminal(V50);
WidgetRTC rtc_widget;
BlynkTimer timer, timer1;

String pending_auto_notice;
bool pending_auto_notice_is_error = false;

void logAutoPumpStep(const String &message) {
  Serial.println(String("[AUTO B2/B4] ") + message);
}

void sendAutoPumpNotice(const String &message, bool is_error) {
  logAutoPumpStep(message);

  if (!data.key_noti) {
    pending_auto_notice = "";
    return;
  }

  if (!Blynk.connected()) {
    pending_auto_notice = message;
    pending_auto_notice_is_error = is_error;
    return;
  }

  // Chi gui cac ket qua chot len Notifications. Khong ghi vao Terminal V5
  // vi Terminal co the bi clear va khong phu hop de luu lich su su kien.
  Blynk.logEvent(is_error ? "error" : "info", message);

  pending_auto_notice = "";
}

BLYNK_CONNECTED() {
  rtc_widget.begin();
  blynk_first_connect = true;
  runtimePublished = false;
  clearOperatorAccess();
  stationNetwork.health.service(millis(), false);
  publishedNetworkLevel = -1;
  lastTelemetryAttempt = millis() - 5000;
  lastProbeAttempt = millis() - 5000;
  telemetryProbe = 0;

  // EEPROM la nguon trang thai dieu khien. Xuat lai de V17/V3 tren app
  // khong chi hien gia tri cu dang luu tren Blynk Cloud.
  Blynk.virtualWrite(V17, data.en_auto_b2_b4);
  Blynk.virtualWrite(V3, data.key_noti);

  if (pending_auto_notice.length() > 0)
    sendAutoPumpNotice(pending_auto_notice, pending_auto_notice_is_error);
}
//----------------------------------
void connectionstatus() {
  if (WiFi.status() != WL_CONNECTED)
    WiFi.reconnect(); // ISP problems do not require WiFi.disconnect()/ESP.restart().
}

void update_started() {
  lastOtaProgressBucket = -1;
  rearmExternalWatchdog();
  Serial.println("CALLBACK:  HTTP update process started");
  Serial.printf("OTA heap: free=%u max_block=%u fragmentation=%u%%\n",
                ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(),
                ESP.getHeapFragmentation());
}
void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
  writeOtaRtcState(OTA_RTC_SUCCESS);
}
void update_progress(int cur, int total) {
  ESP.wdtFeed();
  serviceExternalWatchdog();
  if (total <= 0)
    return;

  int8_t bucket = static_cast<int8_t>((static_cast<uint32_t>(cur) * 10UL) /
                                      static_cast<uint32_t>(total));
  if (bucket != lastOtaProgressBucket) {
    lastOtaProgressBucket = bucket;
    Serial.printf("OTA progress: %d%% (%d/%d bytes)\n", bucket * 10, cur, total);
  }
}
void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
  writeOtaRtcState(OTA_RTC_FAILED, err);
}
void update_fw() {
  WiFiClientSecure client_;
  client_.setInsecure();
  client_.setTimeout(OTA_NETWORK_TIMEOUT_MS);
  ESPhttpUpdate.setClientTimeout(OTA_NETWORK_TIMEOUT_MS);
  ESPhttpUpdate.closeConnectionsOnUpdate(true);
  Serial.println("OTA: bat dau tai firmware");
  rearmExternalWatchdog();
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client_, URL_fw_Bin);
  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n",
                  ESPhttpUpdate.getLastError(),
                  ESPhttpUpdate.getLastErrorString().c_str());
    writeOtaRtcState(OTA_RTC_FAILED, ESPhttpUpdate.getLastError());
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    writeOtaRtcState(OTA_RTC_FAILED, OTA_ERROR_NO_UPDATE);
    break;
  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
} //-------------------------

void updata() {
  if (!Blynk.connected() || !stationNetwork.socket.ready()) return;
  Blynk.beginGroup();
  const float currents[] = {Irms0, Irms1, Irms2, Irms3, Irms4, Irms5, Irms6, Irms7, Irms8};
  const uint8_t pins[] = {40, 41, 42, 43, 44, 45, 46, 32, 36};
  for (uint8_t k = 0; k < 9; ++k)
    if (channelFresh(k)) Blynk.virtualWrite(pins[k], String(currents[k], 1));
  if (levelSamples >= 5 && uint32_t(millis() - levelSampleMs) <= 5000 &&
      isfinite(smoothDistance)) {
    Blynk.virtualWrite(V34, String(smoothDistance, 1));
    Blynk.virtualWrite(V35, volume1);
  }
  if (!runtimePublished || uint32_t(millis() - lastRuntimePublish) >= 30000) {
    collectMotorRuntimes();
    const uint32_t values[] = {data.timerun_G1, data.timerun_G2, data.timerun_G3,
      data.timerun_B1, data.timerun_B2, data.timerun_B3, data.timerun_B4};
    for (uint8_t k = 0; k < 7; ++k)
      Blynk.virtualWrite(70 + k * 2, String(values[k] / 3600.0f, 2));
    runtimePublished = true; lastRuntimePublish = millis();
  }
  Blynk.endGroup();
}

bool initializeCleanOtaWatchdog() {
  // Preserve the normal states used on PCF8575 0x20. This does not touch
  // PCF8575 0x21, which controls all pump start/stop pulse outputs.
  auxWord = 0x0100;
  otaWatchdogReady = writePcfWord(0x20, auxWord);
  if (!otaWatchdogReady) {
    Serial.println("OTA: khong tim thay PCF8575 watchdog tai 0x20");
    return false;
  }
  return rearmExternalWatchdog();
}

void runCleanOtaMode() {
  Serial.printf("OTA clean boot: firmware=%s reset=%s\n", BLYNK_FIRMWARE_VERSION,
                ESP.getResetReason().c_str());
  if (!initializeCleanOtaWatchdog()) {
    writeOtaRtcState(OTA_RTC_FAILED, OTA_ERROR_WATCHDOG_INIT);
    delay(500);
    ESP.restart();
    return;
  }

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, password);

  unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         static_cast<unsigned long>(millis() - startedAt) < OTA_WIFI_TIMEOUT_MS) {
    ESP.wdtFeed();
    serviceExternalWatchdog();
    delay(50);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("OTA clean boot: WiFi timeout");
    writeOtaRtcState(OTA_RTC_FAILED, OTA_ERROR_WIFI_TIMEOUT);
    delay(500);
    ESP.restart();
    return;
  }

  Serial.printf("OTA WiFi connected: RSSI=%d heap=%u max_block=%u\n", WiFi.RSSI(),
                ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
  update_fw();

  // Success restarts inside ESPhttpUpdate. Reaching here means no image was
  // installed, so return to normal operation without entering a reboot loop.
  serviceExternalWatchdog();
  delay(500);
  ESP.restart();
}

bool handleOtaBootState() {
  OtaRtcState otaState = {};
  if (!readOtaRtcState(otaState))
    return false;

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
//----------------------------------

// --- CÁC HÀM HỖ TRỢ HIỆU CHUẨN ĐA ĐIỂM ---
void sortCalibPoints(CalibPoint points[], uint8_t num_points) {
  for (uint8_t i = 1; i < num_points; i++) {
    CalibPoint key = points[i];
    int8_t j = i - 1;
    while (j >= 0 && points[j].adc > key.adc) {
      points[j + 1] = points[j];
      j--;
    }
    points[j + 1] = key;
  }
}

void addOrUpdateCalibPoint(CalibPoint new_point, CalibPoint points[], uint8_t &num_points) {
  if (num_points > MAX_CALIB_POINTS)
    num_points = 0; // Reset nếu dữ liệu bị lỗi
  if (num_points < MAX_CALIB_POINTS) {
    points[num_points] = new_point;
    num_points++;
  } else {
    int8_t closest_idx = -1;
    uint16_t min_diff = 65535;
    for (uint8_t i = 0; i < num_points; i++) {
      uint16_t diff = abs((int)points[i].value - (int)new_point.value);
      if (closest_idx == -1 || diff < min_diff) {
        min_diff = diff;
        closest_idx = i;
      }
    }
    if (closest_idx != -1)
      points[closest_idx] = new_point;
  }
  sortCalibPoints(points, num_points);
}

bool isNonNegativeNumber(const String &text, bool allow_decimal) {
  if (text.length() == 0)
    return false;

  bool has_digit = false;
  bool has_decimal = false;
  for (uint16_t i = 0; i < text.length(); i++) {
    char ch = text.charAt(i);
    if (ch >= '0' && ch <= '9') {
      has_digit = true;
    } else if (allow_decimal && ch == '.' && !has_decimal) {
      has_decimal = true;
    } else {
      return false;
    }
  }
  return has_digit;
}

bool replaceCalibPoint(uint8_t point_number, CalibPoint new_point,
                       CalibPoint points[], uint8_t num_points) {
  if (point_number < 1 || point_number > num_points)
    return false;

  points[point_number - 1] = new_point;
  sortCalibPoints(points, num_points);
  return true;
}

float interpolate(float current_adc, const CalibPoint points[], uint8_t num_points) {
  if (num_points < 2)
    return (num_points == 1) ? (float)points[0].value : 0.0f;
  const CalibPoint *p1, *p2;
  if (current_adc <= points[0].adc) {
    p1 = &points[0];
    p2 = &points[1];
  } else if (current_adc >= points[num_points - 1].adc) {
    p1 = &points[num_points - 2];
    p2 = &points[num_points - 1];
  } else {
    uint8_t i = 0;
    while (i < num_points - 1 && current_adc > points[i + 1].adc)
      i++;
    p1 = &points[i];
    p2 = &points[i + 1];
  }
  float x = current_adc, x1 = p1->adc, y1 = p1->value, x2 = p2->adc, y2 = p2->value;
  if (x2 == x1)
    return y1;
  return y1 + (x - x1) * (y2 - y1) / (x2 - x1);
}

void bridge_Tram2C(String token, int virtual_pin, float value_to_send) {
  // One attempt only. An uncertain command is not replayed after recovery.
  if (!stationNetwork.health.allowControl(millis()) ||
      !remoteHttp.send("batch/update?token=" + token + "&V" +
        String(virtual_pin) + "=" + String(value_to_send)))
    Blynk.virtualWrite(V5, "Khong gui duoc lenh van: mang yeu/ban. Khong tu gui lai.\n");
}
//----------------------------------
void event_30p() {
  if (event30p) {
    event30p = false;
    timer1.setTimeout(300000L, []() { event30p = true; });
    if (data.man == 1) {
      if (data.key_noti)
        Blynk.logEvent("D", String("Đã ngưng Giếng 30p.\nBể chứa còn: ") + volume1 + String(" m3 (") + smoothDistance + String(" cm)"));
    } else if (data.man == 2) {
      if (data.key_noti)
        Blynk.logEvent("G", String("Đã ngưng Giếng 30p.\nBể chứa còn: ") + volume1 + String(" m3 (") + smoothDistance + String(" cm)"));
    } else if (data.man == 3) {
      if (data.key_noti)
        Blynk.logEvent("Q", String("Đã ngưng Giếng 30p.\nBể chứa còn: ") + volume1 + String(" m3 (") + smoothDistance + String(" cm)"));
    }
  }
}
void event_pressure() {
  if (data.man == 1) {
    if (data.key_noti)
      Blynk.logEvent("1-al", String("Áp lực hiện tại là ") + pre + String(" bar"));
  } else if (data.man == 2) {
    if (data.key_noti)
      Blynk.logEvent("2-al", String("Áp lực hiện tại là ") + pre + String(" bar"));
  } else if (data.man == 3) {
    if (data.key_noti)
      Blynk.logEvent("q-al", String("Áp lực hiện tại là ") + pre + String(" bar"));
  }
}
void savedata() {
  if (!configLoaded) return;
  collectMotorRuntimes();
  storageDirty = memcmp(&data, &dataCheck, sizeof(data)) != 0;
  if (storageDirty) storageUrgent = true;
}
//----------------------------------
bool onG1() {
  if (!trip3 && data.SetAmpe3max > 3 && validCurrentLimits(data.SetAmpe3min, data.SetAmpe3max)) {
    if (!startRelayPulse(pin_on_G1, 2500))
      return false;
    status_g1 = HIGH;
    motorStopping &= ~(uint8_t(1) << 0);
    return true;
  } else {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Giếng 1 lỗi!\nHãy 'reset' trước khi chạy!");
  }
  return false;
}
bool offG1() {
  if (!startRelayPulse(pin_off_G1, 1000))
    return false;
  status_g1 = LOW;
  motorStopping |= uint8_t(1) << 0;
  return true;
}
bool onG2() {
  if (!trip1 && data.SetAmpe1max > 3 && validCurrentLimits(data.SetAmpe1min, data.SetAmpe1max)) {
    if (!startRelayPulse(pin_on_G2, 2500))
      return false;
    status_g2 = HIGH;
    motorStopping &= ~(uint8_t(1) << 1);
    return true;
  } else {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Giếng 2 lỗi!\nHãy 'reset' trước khi chạy!");
  }
  return false;
}
bool offG2() {
  if (!startRelayPulse(pin_off_G2, 1000))
    return false;
  status_g2 = LOW;
  motorStopping |= uint8_t(1) << 1;
  return true;
}
bool onG3() {
  if (!trip5 && data.SetAmpe5max > 3 && validCurrentLimits(data.SetAmpe5min, data.SetAmpe5max)) {
    if (!startRelayPulse(pin_on_G3, 2500))
      return false;
    status_g3 = HIGH;
    motorStopping &= ~(uint8_t(1) << 2);
    return true;
  } else {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Giếng 3 lỗi!\nHãy 'reset' trước khi chạy!");
  }
  return false;
}
bool offG3() {
  if (!startRelayPulse(pin_off_G3, 1000))
    return false;
  status_g3 = LOW;
  motorStopping |= uint8_t(1) << 2;
  return true;
}
//----------------------------------
bool on_Bom1() { // 18.5Kw
  if (!trip0 && data.SetAmpemax > 3 && validCurrentLimits(data.SetAmpemin, data.SetAmpemax)) {
    if (!startRelayPulse(pin_on_Bom1, 200))
      return false;
    status_b1 = HIGH;
    motorStopping &= ~(uint8_t(1) << 3);
    return true;
  } else {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Bơm 1 lỗi!\nHãy 'reset' trước khi chạy!");
  }
  return false;
}
bool off_Bom1() {
  if (!startRelayPulse(pin_off_Bom1, 200))
    return false;
  status_b1 = LOW;
  motorStopping |= uint8_t(1) << 3;
  return true;
}
bool on_Bom2() {
  if (!trip6 && data.SetAmpe6max > 3 && validCurrentLimits(data.SetAmpe6min, data.SetAmpe6max)) {
    if (!startRelayPulse(pin_on_Bom2, 200))
      return false;
    status_b2 = HIGH;
    motorStopping &= ~(uint8_t(1) << 4);
    return true;
  } else {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Bơm 2 lỗi!\nHãy 'reset' trước khi chạy!");
  }
  return false;
}
bool off_Bom2() {
  if (!startRelayPulse(pin_off_Bom2, 200))
    return false;
  status_b2 = LOW;
  motorStopping |= uint8_t(1) << 4;
  return true;
}
bool on_Bom3() {
  if (!trip4 && data.SetAmpe4max > 3 && validCurrentLimits(data.SetAmpe4min, data.SetAmpe4max)) {
    if (!startRelayPulse(pin_on_Bom3, 200))
      return false;
    status_b3 = HIGH;
    motorStopping &= ~(uint8_t(1) << 5);
    return true;
  } else {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Bơm 3 lỗi!\nHãy 'reset' trước khi chạy!");
  }
  return false;
}
bool off_Bom3() {
  if (!startRelayPulse(pin_off_Bom3, 200))
    return false;
  status_b3 = LOW;
  motorStopping |= uint8_t(1) << 5;
  return true;
}
bool on_Bom4() {
  if (!trip2 && data.SetAmpe2max > 3 && validCurrentLimits(data.SetAmpe2min, data.SetAmpe2max)) {
    if (!startRelayPulse(pin_on_Bom4, 200))
      return false;
    status_b4 = HIGH;
    motorStopping &= ~(uint8_t(1) << 6);
    return true;
  } else {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Bơm 4 lỗi!\nHãy 'reset' trước khi chạy!");
  }
  return false;
}
bool off_Bom4() {
  if (!startRelayPulse(pin_off_Bom4, 200))
    return false;
  status_b4 = LOW;
  motorStopping |= uint8_t(1) << 6;
  return true;
}
//----------------------------------
void on_NK1() {
  if (!trip7) {
    writeRelay(pin_NK1, HIGH);
  }
}
void off_NK1() {
  writeRelay(pin_NK1, LOW);
}
void on_NK2() {
  if (!trip8) {
    writeRelay(pin_NK2, HIGH);
  }
}
void off_NK2() {
  writeRelay(pin_NK2, LOW);
}
//----------------------------------
void hidden() {
  const uint8_t pins[] = {11, 12, 13, 18};
  for (uint8_t pin : pins) Blynk.setProperty(pin, "isHidden", true);
}
void visible() {
  const uint8_t pins[] = {11, 12, 13, 18};
  for (uint8_t pin : pins) Blynk.setProperty(pin, "isHidden", false);
}
void i2c_scaner() {
  byte error, address;
  int nDevices;
  String stringOne;

  Blynk.virtualWrite(V5, "Firmware: ", BLYNK_FIRMWARE_VERSION,
                     "\nReset: ", ESP.getResetReason(),
                     "\nLast OTA: ", lastOtaStatus,
                     "\nHeap: ", ESP.getFreeHeap(),
                     " max block: ", ESP.getMaxFreeBlockSize(), "\n");
  Blynk.virtualWrite(V5, "Network: ", stationNetwork.health.level,
                     " RTT: ", stationNetwork.health.rtt, "ms",
                     " loop max: ", stationNetwork.health.maxLoopGap, "ms\n");

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      stringOne = String(address, HEX);
      if (address < 16)
        Blynk.virtualWrite(V5, "I2C device found at address 0x0", stringOne, " !\n");
      Blynk.virtualWrite(V5, "I2C device found at address 0x", stringOne, " !\n");
      nDevices++;
    } else if (error == 4) {
      stringOne = String(address, HEX);

      if (address < 16)
        Blynk.virtualWrite(V5, "Unknown error at address 0x0", stringOne, " !\n");
      Blynk.virtualWrite(V5, "I2C device found at address 0x", stringOne, " !\n");
    }
  }
  if (nDevices == 0)
    Blynk.virtualWrite(V5, "No I2C devices found\n");
}
//------------------------+----------
BLYNK_WRITE(V0) // Nguoi truc
{
  if (!stationNetwork.health.allowControl(millis())) return;
  if (keySwitchP || keySwitchD || keySwitchQ) {
    switch (param.asInt()) {
    case 0: { // Duc
      data.man = 1;
      break;
    }
    case 1: { // Phong
      data.man = 2;
      break;
    }
    case 2: { // Quang
      data.man = 3;
      break;
    }
    }
    savedata();
  } else
    Blynk.virtualWrite(V0, data.man - 1);
}
BLYNK_WRITE(V2) // Chế độ Rửa Lọc Độc Lập
{
  if (!stationNetwork.health.allowControl(millis())) return;
  if (keySwitchP || keySwitchD || keySwitchQ) {
    int mode = param.asInt();
    if (mode < 0 || mode > 3 || mode == data.status_rualoc) return;

    // BƯỚC 1: Chốt số lưu lượng cho bất kỳ giếng nào đang rửa trước khi đổi trạng thái
    if ((data.LLG1_RL != 0) || (data.LLG2_RL != 0) || (data.LLG3_RL != 0)) {
      if (data.LLG1_RL != 0) {
        Blynk.virtualWrite(V64, LLG1_1m3 - data.LLG1_RL);
        data.LLG1_RL = 0;
      }
      if (data.LLG2_RL != 0) {
        Blynk.virtualWrite(V65, LLG2_1m3 - data.LLG2_RL);
        data.LLG2_RL = 0;
      }
      if (data.LLG3_RL != 0) {
        Blynk.virtualWrite(V66, LLG3_1m3 - data.LLG3_RL);
        data.LLG3_RL = 0;
      }
    }

    // BƯỚC 2: Thiết lập trạng thái Rửa lọc mới
    data.status_rualoc = mode;
    switch (mode) {
    case 0:
      // Tắt rửa lọc (đã xử lý chốt số ở Bước 1)
      break;
    case 1:
      // Bật rửa lọc Giếng 1
      data.LLG1_RL = LLG1_1m3;
      break;
    case 2:
      // Bật rửa lọc Giếng 2
      data.LLG2_RL = LLG2_1m3;
      break;
    case 3:
      // Bật rửa lọc Giếng 3
      data.LLG3_RL = LLG3_1m3;
      break;
    }

    savedata(); // Lưu trạng thái vào EEPROM

    // Gửi lệnh đồng bộ sang mạch van điều khiển (Tram2C)
    bridge_Tram2C(Tram2_Rualoc, 0, data.status_rualoc);
  } else {
    // Từ chối lệnh nếu không có quyền, trả lại trạng thái cũ trên App
    Blynk.virtualWrite(V2, data.status_rualoc);
  }
}
BLYNK_WRITE(V3) // Thông báo
{
  if (keySet) {
    if (param.asInt() == LOW)
      data.key_noti = false;
    else
      data.key_noti = true;
    savedata();
  } else
    Blynk.virtualWrite(V3, data.key_noti);
}

BLYNK_WRITE(V4) // PROTECT
{
  if (!stationNetwork.health.allowControl(millis())) return;
  if (keySet) {
    if (param.asInt() != 0 && param.asInt() != 1) return;
    if (param.asInt() == LOW) {
      data.protect = false;
    } else {
      data.protect = true;
    }
  } else {
    Blynk.virtualWrite(V4, data.protect);
  }
}
BLYNK_WRITE(V5) // data string
{
  if (!stationNetwork.health.allowControl(millis())) return;
  String dataS = param.asStr();
  dataS.trim(); // Xóa khoảng trắng và ký tự xuống dòng thừa
  if (dataS == "help") {
    keyterminal.clear(); helpPosition = 0; helpNext = millis(); return;
  }
  if (restartRequest != RESTART_NONE && dataS != "reset") return;
  if ((dataS == "t2")) {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Người vận hành: 'T.Phong'\nKích hoạt trong 15s\n");
    grantOperatorAccess(1, 15000);
  } else if (dataS == "M") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Người vận hành: 'M.Quang'\nKích hoạt trong 10s\n");
    grantOperatorAccess(2, 10000);
  } else if ((dataS == "đ") || (dataS == "Đ")) {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Người vận hành: 'C.Đức'\nKích hoạt trong 15s\n");
    grantOperatorAccess(0, 15000);
  } else if (dataS == "active") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Kích hoạt chế độ sửa lỗi!\nKHÔNG sử dụng phần mềm cho đến khi thông báo nào mất!");
    keySwitchQ = true;
    accessUntil[2] = 0;
    keySet = true;
    visible();
  } else if (dataS == "deactive") {
    keyterminal.clear();
    keySwitchQ = false;
    accessUntil[2] = 0;
    keySet = false;
    hidden();
    Blynk.virtualWrite(V5, "Hãy nhập mã...!\n");
  } else if (dataS == "save") {
    keyterminal.clear();
    savedata();
    saveReport = storageDirty;
    Blynk.virtualWrite(V5, !configLoaded ? "EEPROM chua hop le; khong luu.\n" :
      (storageDirty ? "Da nhan yeu cau luu.\n" : "Khong co thay doi can luu.\n"));
  } else if (dataS == "reset") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Đã reset!");
    trip0 = false;
    trip1 = false;
    trip2 = false;
    trip3 = false;
    trip4 = false;
    trip5 = false;
    trip6 = false;
    trip7 = false;
    trip8 = false;
    writeRelay(pin_off_G3, HIGH);   // G3
    writeRelay(pin_off_G2, HIGH);   // G2
    writeRelay(pin_off_G1, HIGH);   // G1
    writeRelay(pin_off_Bom4, HIGH); // 11kw
    writeRelay(pin_off_Bom3, HIGH); // 7.5kw
    writeRelay(pin_off_Bom1, HIGH); // 18.5kw
    writeRelay(pin_off_Bom2, HIGH); // 30kw
    writeRelay(pin_NK1, HIGH);      // NK1
    writeRelay(pin_NK2, HIGH);      // NK2
  } else if (dataS == "update") {
    keyterminal.clear();
    savedata();
    Blynk.virtualWrite(V5, "Da nhan lenh OTA; ESP se khoi dong vao che do cap nhat sach\n");
    restartRequest = RESTART_OTA;
    restartNotBefore = millis() + 250;
    auto_resync_required = true;
  } else if (dataS == "save_num") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Số lần ghi EEPROM: ", data.save_num);
  } else if (dataS == "rst") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "ESP Khởi động lại sau 3s");
    savedata();
    restartRequest = RESTART_NORMAL;
    restartNotBefore = millis() + 3000;
    auto_resync_required = true;
  } else if (dataS == "calib") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "--- THÔNG TIN HIỆU CHUẨN ---\n");
    Blynk.virtualWrite(V5, "[CẢM BIẾN MỰC NƯỚC]\n");
    char buff[100];
    snprintf(buff, sizeof(buff), " - Số điểm: %d/%d\n", data.num_level_points, MAX_CALIB_POINTS);
    Blynk.virtualWrite(V5, buff);
    for (uint8_t i = 0; i < data.num_level_points; i++) {
      snprintf(buff, sizeof(buff), " #%d: ADC=%d -> %d cm\n", i + 1, data.level_points[i].adc, data.level_points[i].value);
      Blynk.virtualWrite(V5, buff);
    }
    snprintf(buff, sizeof(buff), " - ADC hiện tại: %.2f\n", kalman_filtered_adc_value);
    Blynk.virtualWrite(V5, buff);
    snprintf(buff, sizeof(buff), " => Mực nước (đã bù +100 cm): %.1f cm\n", smoothDistance);
    Blynk.virtualWrite(V5, buff);
  } else if (dataS == "level_clear") {
    data.num_level_points = 0;
    for (int i = 0; i < MAX_CALIB_POINTS; i++) {
      data.level_points[i].adc = 0;
      data.level_points[i].value = 0;
    }
    savedata();
    Blynk.virtualWrite(V5, "Đã xóa calib mực nước.\n");
  } else if (dataS.startsWith("level_") && (levelSamples < 5 ||
             uint32_t(millis() - levelSampleMs) > 5000)) {
    Blynk.virtualWrite(V5, "Chua co ADC muc nuoc moi/hop le; khong hieu chuan.\n");
  } else if (dataS.startsWith("level_") && dataS.indexOf('_', 6) >= 0) {
    int separator = dataS.indexOf('_', 6);
    String point_text = dataS.substring(6, separator);
    String value_text = dataS.substring(separator + 1);

    if (!isNonNegativeNumber(point_text, false) || !isNonNegativeNumber(value_text, false)) {
      Blynk.virtualWrite(V5, "Sai định dạng. Dùng level_N_YYY (ví dụ level_5_500).\n");
    } else {
      int point_number = point_text.toInt();
      unsigned long level_known = value_text.toInt();
      if (point_number < 1 || point_number > data.num_level_points) {
        Blynk.virtualWrite(V5, "Điểm mực nước không tồn tại. Xem danh sách bằng calib.\n");
      } else if (level_known > 65535UL) {
        Blynk.virtualWrite(V5, "Giá trị mực nước vượt giới hạn lưu trữ.\n");
      } else {
        CalibPoint pt;
        pt.adc = (uint16_t)round(kalman_filtered_adc_value);
        pt.value = (uint16_t)level_known;
        for (uint8_t k=0;k<data.num_level_points;++k)
          if (k != point_number-1 && data.level_points[k].adc==pt.adc) {
            Blynk.virtualWrite(V5,"ADC trung voi diem khac; khong ap dung.\n");return;
          }
        replaceCalibPoint((uint8_t)point_number, pt, data.level_points, data.num_level_points);
        savedata();
        char buff[80];
        snprintf(buff, sizeof(buff), "Đã thay điểm #%d: ADC=%d -> %d cm\n", point_number, pt.adc, pt.value);
        Blynk.virtualWrite(V5, buff);
      }
    }
  } else if (dataS.startsWith("level_")) { // Lệnh thêm điểm mực nước đã biết, ví dụ: level_500
    String value_text = dataS.substring(6);
    if (!isNonNegativeNumber(value_text, false)) {
      Blynk.virtualWrite(V5, "Sai định dạng. Dùng level_YYY (ví dụ level_500).\n");
    } else {
      unsigned long level_known = value_text.toInt();
      if (level_known > 65535UL) {
        Blynk.virtualWrite(V5, "Giá trị mực nước vượt giới hạn lưu trữ.\n");
      } else {
        CalibPoint pt;
        pt.adc = (uint16_t)round(kalman_filtered_adc_value);
        pt.value = (uint16_t)level_known;
        for (uint8_t k=0;k<data.num_level_points;++k)
          if (data.level_points[k].adc==pt.adc) {
            Blynk.virtualWrite(V5,"ADC da co; dung level_N_YYY de thay diem.\n");return;
          }
        addOrUpdateCalibPoint(pt, data.level_points, data.num_level_points);
        savedata();
        char buff[64];
        snprintf(buff, sizeof(buff), "Đã lưu điểm: ADC=%d -> %d cm\n", pt.adc, pt.value);
        Blynk.virtualWrite(V5, buff);
      }
    }
  } else if (dataS == "i2c") {
    i2c_scaner();
  } else {
    Blynk.virtualWrite(V5, "Mã không hợp lệ!\nVui lòng nhập lại.\n");
  }
}

BLYNK_WRITE(V11) // Chon máy cài đặt bảo vệ
{
  if (param.asInt() < 0 || param.asInt() > 9) return;
  switch (param.asInt()) {
  case 0: {
    z = 0;
    Blynk.virtualWrite(V12, 0);
    Blynk.virtualWrite(V13, 0);
    break;
  }
  case 1: { // 18.5kw
    z = 1;
    Blynk.virtualWrite(V12, data.SetAmpemin);
    Blynk.virtualWrite(V13, data.SetAmpemax);
    break;
  }
  case 2: { // 30k
    z = 2;
    Blynk.virtualWrite(V12, data.SetAmpe6min);
    Blynk.virtualWrite(V13, data.SetAmpe6max);
    break;
  }
  case 3: { // 7.5kw
    z = 3;
    Blynk.virtualWrite(V12, data.SetAmpe4min);
    Blynk.virtualWrite(V13, data.SetAmpe4max);
    break;
  }
  case 4: { // 11kw
    z = 4;
    Blynk.virtualWrite(V12, data.SetAmpe2min);
    Blynk.virtualWrite(V13, data.SetAmpe2max);
    break;
  }
  case 5: { // G1
    z = 5;
    Blynk.virtualWrite(V12, data.SetAmpe3min);
    Blynk.virtualWrite(V13, data.SetAmpe3max);
    break;
  }
  case 6: { // G2
    z = 6;
    Blynk.virtualWrite(V12, data.SetAmpe1min);
    Blynk.virtualWrite(V13, data.SetAmpe1max);
    break;
  }
  case 7: { // G3
    z = 7;
    Blynk.virtualWrite(V12, data.SetAmpe5min);
    Blynk.virtualWrite(V13, data.SetAmpe5max);
    break;
  }
  case 8: { // NK1
    z = 8;
    Blynk.virtualWrite(V12, data.SetAmpe7min);
    Blynk.virtualWrite(V13, data.SetAmpe7max);
    break;
  }
  case 9: { // NK2
    z = 9;
    Blynk.virtualWrite(V12, data.SetAmpe8min);
    Blynk.virtualWrite(V13, data.SetAmpe8max);
    break;
  }
  }
  prepareCurrentDraft();
}
BLYNK_WRITE(V12) { setCurrentDraft(true, param); }
BLYNK_WRITE(V13) { setCurrentDraft(false, param); }
BLYNK_WRITE(V15) { handleManualMotor(4, true, param); }
BLYNK_WRITE(V16) { handleManualMotor(4, false, param); }
BLYNK_WRITE(V17) // Bật/Tắt chế độ Auto luân phiên Bơm 2 và Bơm 4
{
  if (!stationNetwork.health.allowControl(millis())) return;
  // Cho phép thay đổi nếu đang trong chế độ active (keySet)
  // HOẶC đã nhập mã người vận hành (Q, P, D)
  if (keySet || keySwitchQ || keySwitchP || keySwitchD) {
    if (param.asInt() != 0 && param.asInt() != 1) return;
    data.en_auto_b2_b4 = param.asInt();
    auto_resync_required = true;
    savedata(); // Lưu trạng thái mới vào EEPROM

    // (Tùy chọn) Ghi log để biết ai là người vừa thay đổi cài đặt
    if (data.key_noti) {
      String who = keySet ? "Admin" : (keySwitchQ ? "Quang" : (keySwitchP ? "Phong" : "Đức"));
      Blynk.logEvent("info", who + String(" đã ") + (data.en_auto_b2_b4 ? "BẬT" : "TẮT") + String(" Auto luân phiên."));
    }
  } else {
    // NẾU CHƯA NHẬP MÃ: Trả lại trạng thái hiện tại trên app Blynk
    Blynk.virtualWrite(V17, data.en_auto_b2_b4);

    // Xóa terminal và gửi thông báo nhắc nhở
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Lỗi: Chưa cấp quyền!\nHãy nhập mã người vận hành.\n");
  }
}
BLYNK_WRITE(V18) // Time Input - Khung giờ Bơm 4
{
  if (!stationNetwork.health.allowControl(millis())) return;
  // Kiểm tra quyền: Chỉ cho phép đổi nếu đang active hoặc đã nhập pass (Q, P, D)
  if (keySet || keySwitchQ || keySwitchP || keySwitchD) {

    TimeInputParam t(param);
    auto_resync_required = true;

    if (t.hasStartTime()) {
      data.auto_start_h = t.getStartHour();
      data.auto_start_m = t.getStartMinute();
    }
    if (t.hasStopTime()) {
      data.auto_stop_h = t.getStopHour();
      data.auto_stop_m = t.getStopMinute();
    }

    savedata(); // Lưu thời gian mới vào EEPROM

    // (Tùy chọn) Ghi log sự kiện để biết ai vừa đổi khung giờ
    if (data.key_noti) {
      String who = keySet ? "Admin" : (keySwitchQ ? "Quang" : (keySwitchP ? "Phong" : "Đức"));
      Blynk.logEvent("info", who + String(" đã thay đổi khung giờ luân phiên."));
    }
  } else {
    // NẾU CHƯA NHẬP MÃ: Báo lỗi ra màn hình Terminal
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Lỗi: Chưa cấp quyền!\nHãy nhập mã người vận hành.\n");
  }
}
BLYNK_WRITE(V20) { handleManualMotor(2, true, param); }
BLYNK_WRITE(V21) { handleManualMotor(1, false, param); }
BLYNK_WRITE(V22) { handleManualMotor(6, false, param); }
BLYNK_WRITE(V23) { handleManualMotor(0, false, param); }
BLYNK_WRITE(V24) { handleManualMotor(5, false, param); }
BLYNK_WRITE(V25) { handleManualMotor(3, false, param); }
BLYNK_WRITE(V26) { handleManualMotor(2, false, param); }
BLYNK_WRITE(V27) { handleManualMotor(3, true, param); }
BLYNK_WRITE(V28) { handleManualMotor(5, true, param); }
BLYNK_WRITE(V29) { handleManualMotor(0, true, param); }
BLYNK_WRITE(V30) { handleManualMotor(6, true, param); }
BLYNK_WRITE(V31) { handleManualMotor(1, true, param); }
BLYNK_WRITE(V47) // On-Off Nen khi 1
{
  if (!acceptRemoteControl(pin_NK1, param)) return;
  int data47 = param.asInt();
  if (keySwitchQ || keySwitchP || keySwitchD) {
    if (data47 == 1) {
      writeRelay(pin_NK1, HIGH);
    } else {
      writeRelay(pin_NK1, LOW);
    }
  }
}
BLYNK_WRITE(V48) // On-Off Nen khi 2
{
  if (!acceptRemoteControl(pin_NK2, param)) return;
  int data48 = param.asInt();
  if (keySwitchQ || keySwitchP || keySwitchD) {
    if (data48 == 1) {
      writeRelay(pin_NK2, HIGH);
    } else {
      writeRelay(pin_NK2, LOW);
    }
  }
}
BLYNK_WRITE(V50) {
  if (!stationNetwork.health.allowControl(millis())) return;
  String dataS = param.asStr();
  if ((dataS == "rst_G1") || (dataS == "update_G1") || (dataS == "rst_vl_G1") || (dataS == "i2c_G1")) {
    volume_terminal.clear();
    if (!remoteHttp.send("batch/update?token=" + String(VOLUME_TOKEN_G1) +
                         "&V0=" + urlEncode(dataS)))
      Blynk.virtualWrite(V50, "Mang ban/yeu; lenh khong duoc xep hang hay tu gui lai.\n");
  } else if ((dataS == "rst_G2") || (dataS == "update_G2") || (dataS == "rst_vl_G2") || (dataS == "i2c_G2")) {
    volume_terminal.clear();
    if (!remoteHttp.send("batch/update?token=" + String(VOLUME_TOKEN_G2) +
                         "&V0=" + urlEncode(dataS)))
      Blynk.virtualWrite(V50, "Mang ban/yeu; lenh khong duoc xep hang hay tu gui lai.\n");
  } else if ((dataS == "rst_G3") || (dataS == "update_G3") || (dataS == "rst_vl_G3") || (dataS == "i2c_G3")) {
    volume_terminal.clear();
    if (!remoteHttp.send("batch/update?token=" + String(VOLUME_TOKEN_G3) +
                         "&V0=" + urlEncode(dataS)))
      Blynk.virtualWrite(V50, "Mang ban/yeu; lenh khong duoc xep hang hay tu gui lai.\n");
  }
}
BLYNK_WRITE(V58) // Lưu lượng 1m3 G2
{
  LLG2_1m3 = param.asInt();
}
BLYNK_WRITE(V60) // Lưu lượng 1m3 G3
{
  LLG3_1m3 = param.asInt();
}
BLYNK_WRITE(V62) // Lưu lượng 1m3 G1
{
  LLG1_1m3 = param.asInt();
}
//----------------------------------
BLYNK_WRITE(V10) // Ap luc
{
  if (!stationNetwork.health.allowControl(millis()) || !isfinite(param.asFloat())) return;
  pre = param.asFloat();

  // --- 1. KIỂM TRA THỜI GIAN THỰC TẾ ---
  DateTime now;
  if (!readStationTime(now)) return;
  int current_time_mins = now.hour() * 60 + now.minute();

  // Lấy thời gian hẹn giờ (Giả sử dùng chung biến khung giờ Bơm của V18)
  int start_mins = data.auto_start_h * 60 + data.auto_start_m;
  int stop_mins = data.auto_stop_h * 60 + data.auto_stop_m;

  bool is_in_window = false;
  if (start_mins <= stop_mins) {
    is_in_window = (current_time_mins >= start_mins && current_time_mins < stop_mins);
  } else {
    // Trường hợp mốc giờ kéo dài qua đêm
    is_in_window = (current_time_mins >= start_mins || current_time_mins < stop_mins);
  }

  // --- LOGIC TRÌ HOÃN 5 PHÚT KHI HẾT GIỜ (BƠM TẮT) ---
  static bool last_window_state_v10 = false;
  static bool is_first_run_v10 = true;
  static unsigned long time_window_ended = 0;
  static bool is_in_grace_period = false;

  // Khởi tạo trạng thái lần đầu tiên mạch có điện
  if (is_first_run_v10) {
    last_window_state_v10 = is_in_window;
    is_first_run_v10 = false;
  }

  // Bắt sự kiện vừa thoát khỏi khung giờ (từ đang chạy sang tắt)
  if (last_window_state_v10 == true && is_in_window == false) {
    time_window_ended = millis();
    is_in_grace_period = true;
  }
  last_window_state_v10 = is_in_window;

  // Nếu đang trong thời gian chờ, kiểm tra xem đã đủ 5 phút (300000ms) chưa
  if (is_in_grace_period && (millis() - time_window_ended >= 300000)) {
    is_in_grace_period = false;
  }

  // --- 2. XÁC ĐỊNH NGƯỠNG BÁO LỖI ---
  // Trong giờ hẹn -> ngưỡng 1 kg (1 bar)
  // Ngoài giờ hẹn -> ngưỡng 2 kg (2 bar)
  float min_threshold = is_in_window ? 1.0 : 2.0;

  // NẾU ĐANG TRONG 5 PHÚT VỪA TẮT BƠM -> Đặt ngưỡng xuống âm để tạm thời BỎ QUA kiểm tra áp thấp
  if (is_in_grace_period) {
    min_threshold = -1.0;
  }

  // --- 3. LOGIC XỬ LÝ CẢNH BÁO ---
  if (pre < min_threshold) {
    n++;
    if ((n >= 5) && (keyPRE2)) {
      if (noti) {
        event_pressure();
      }
      keyPRE2 = false;
      timer1.setTimeout(600000L, []() { // 10p (hoặc 15p) sau cho phép báo lại
        keyPRE2 = true;
      });
    }
  } else if (pre >= 4.4) {
    // Nếu áp lớn hơn 4.4 bar (Giữ nguyên logic cũ của bạn)
    m++;
    if ((m >= 3) && (keyPRE4)) {
      if (noti) {
        event_pressure();
      }
      keyPRE4 = false;
      timer1.setTimeout(180000L, []() { // 3p báo lại
        keyPRE4 = true;
      });
    }
  } else {
    // Áp suất ổn định (nằm giữa ngưỡng thấp và ngưỡng cao) -> Reset bộ đếm lỗi
    n = 0;
    m = 0;
  }
}
void readcurrent() // C0 - 18.5 KW
{
  if (!selectAnalogChannel(0))
    return;
  float rms0 = emon0.calcIrms(740);
  if (!isfinite(rms0)) {
    currentScanValid = false;
    controlReady = false;
    channelGood[0] = 0;
    channelValid &= ~(uint16_t(1) << 0);
    return;
  }
  recordCurrentSample(0);
  if (rms0 <= 3) {
    Irms0 = 0;
    yIrms0 = 0;
    if (status_b1 == HIGH) {
      if (xIrms0 < 4) ++xIrms0;
      if ((xIrms0 > 3) && (canProtect(0, data.SetAmpemin, data.SetAmpemax))) {
        xIrms0 = 0;
        off_Bom1();
        trip0 = true;
        if (data.key_noti)
          Blynk.logEvent("error", String("Bơm 18.5Kw lỗi\nKhông đo được DÒNG ĐIỆN"));
      }
    }

  } else if (rms0 > 3) {
    Irms0 = rms0;
    if (yIrms0 < 4) ++yIrms0;
    xIrms0 = 0;
    if (yIrms0 > 2) {

      if ((Irms0 > data.SetAmpemax) || (Irms0 < data.SetAmpemin)) {
        if (xSetAmpe < 4) ++xSetAmpe;
        if ((xSetAmpe >= 3) && (canProtect(0, data.SetAmpemin, data.SetAmpemax))) {
          if (data.key_noti)
            Blynk.logEvent("error", String("Máy 18.5KW lỗi: ") + Irms0 + String(" A"));
          status_b1 = LOW;
          startRelayPulse(pin_off_Bom1, 3000);
          trip0 = true;
          xSetAmpe = 0;
        }
      } else
        xSetAmpe = 0;
    }
  }
  noteMotorSample(0);
}
void readcurrent1() // C1 - Gieng 2
{
  // Blynk.run();
  if (!selectAnalogChannel(1))
    return;
  float rms1 = emon1.calcIrms(740);
  if (!isfinite(rms1)) {
    currentScanValid = false;
    controlReady = false;
    channelGood[1] = 0;
    channelValid &= ~(uint16_t(1) << 1);
    return;
  }
  recordCurrentSample(1);
  if (rms1 <= 3) {
    Irms1 = 0;
    yIrms1 = 0;
    if (status_g2 == HIGH) {
      if (xIrms1 < 4) ++xIrms1;
      if ((xIrms1 > 3) && (canProtect(1, data.SetAmpe1min, data.SetAmpe1max))) {
        xIrms1 = 0;
        offG2();
        trip1 = true;
        if (data.key_noti)
          Blynk.logEvent("error", String("Giếng 2 lỗi\nKhông đo được DÒNG ĐIỆN"));
      }
    }

    if ((unsigned long)(millis() - dem2) > 1800000) {
      dem2 = millis();
      event_30p();
    }
  } else if (rms1 > 3) {
    Irms1 = rms1;
    if (yIrms1 < 4) ++yIrms1;
    xIrms1 = 0;
    if (yIrms1 > 3) {
      dem2 = millis();
    }
    if (yIrms1 > 2) {

      if ((Irms1 > data.SetAmpe1max) || (Irms1 < data.SetAmpe1min)) {
        if (xSetAmpe1 < 4) ++xSetAmpe1;
        if ((xSetAmpe1 >= 3) && (canProtect(1, data.SetAmpe1min, data.SetAmpe1max))) {
          if (data.key_noti)
            Blynk.logEvent("error", String("Giếng II lỗi: ") + Irms1 + String(" A"));
          trip1 = true;
          xSetAmpe1 = 0;
          startRelayPulse(pin_off_G2, 3000);
        }
      } else
        xSetAmpe1 = 0;
    }
  }
  noteMotorSample(1);
}
void readcurrent2() // C2 - 11 KW
{
  // Blynk.run();
  if (!selectAnalogChannel(2))
    return;
  float rms2 = emon2.calcIrms(740);
  if (!isfinite(rms2)) {
    currentScanValid = false;
    controlReady = false;
    channelGood[2] = 0;
    channelValid &= ~(uint16_t(1) << 2);
    return;
  }
  recordCurrentSample(2);
  if (rms2 <= 3) {
    Irms2 = 0;
    yIrms2 = 0;
    if (status_b4 == HIGH) {
      if (auto_start_guard_b4) {
        // Trong luc Auto dang xac nhan khoi dong, de state machine quyet dinh
        // thu lai hay dung han. Khong vo hieu hoa bao ve qua/thieu dong.
        xIrms2 = 0;
      } else {
        if (xIrms2 < 4) ++xIrms2;
        if ((xIrms2 > 3) && (canProtect(2, data.SetAmpe2min, data.SetAmpe2max))) {
          xIrms2 = 0;
          off_Bom4();
          trip2 = true;
          if (data.key_noti)
            Blynk.logEvent("error", String("Bơm 11Kw lỗi\nKhông đo được DÒNG ĐIỆN"));
        }
      }
    }

  } else if (rms2 > 3) {
    Irms2 = rms2;
    if (yIrms2 < 4) ++yIrms2;
    xIrms2 = 0;
    if (yIrms2 > 2) {

      if ((Irms2 > data.SetAmpe2max) || (Irms2 < data.SetAmpe2min)) {
        if (xSetAmpe2 < 4) ++xSetAmpe2;
        if ((xSetAmpe2 >= 3) && (canProtect(2, data.SetAmpe2min, data.SetAmpe2max))) {
          if (data.key_noti)
            Blynk.logEvent("error", String("Máy 11KW lỗi: ") + Irms2 + String(" A"));
          trip2 = true;
          xSetAmpe2 = 0;
          status_b4 = LOW;
          startRelayPulse(pin_off_Bom4, 3000);
        }
      } else
        xSetAmpe2 = 0;
    }
  }
  noteMotorSample(2);
}
void readcurrent3() // C3 - Gieng 1
{
  // Blynk.run();
  if (!selectAnalogChannel(3))
    return;
  float rms3 = emon3.calcIrms(740);
  if (!isfinite(rms3)) {
    currentScanValid = false;
    controlReady = false;
    channelGood[3] = 0;
    channelValid &= ~(uint16_t(1) << 3);
    return;
  }
  recordCurrentSample(3);
  if (rms3 <= 3) {
    Irms3 = 0;
    yIrms3 = 0;
    if (status_g1 == HIGH) {
      if (xIrms3 < 4) ++xIrms3;
      if ((xIrms3 > 3) && (canProtect(3, data.SetAmpe3min, data.SetAmpe3max))) {
        xIrms3 = 0;
        offG1();
        trip3 = true;
        if (data.key_noti)
          Blynk.logEvent("error", String("Giếng 1 lỗi\nKhông đo được DÒNG ĐIỆN"));
      }
    }

    if ((unsigned long)(millis() - dem1) > 1800000) {
      dem1 = millis();
      event_30p();
    }
  } else if (rms3 > 3) {
    Irms3 = rms3;
    if (yIrms3 < 4) ++yIrms3;
    xIrms3 = 0;
    if (yIrms3 > 3) {
      dem1 = millis();
    }
    if (yIrms3 > 2) {

      if ((Irms3 > data.SetAmpe3max) || (Irms3 < data.SetAmpe3min)) {
        if (xSetAmpe3 < 4) ++xSetAmpe3;
        if ((xSetAmpe3 >= 3) && (canProtect(3, data.SetAmpe3min, data.SetAmpe3max))) {
          if (data.key_noti)
            Blynk.logEvent("error", String("Giếng I lỗi: ") + Irms3 + String(" A"));
          trip3 = true;
          xSetAmpe3 = 0;
          startRelayPulse(pin_off_G1, 3000);
        }
      } else
        xSetAmpe3 = 0;
    }
  }
  noteMotorSample(3);
}
void readcurrent4() // C4 - 7.5 KW
{
  // Blynk.run();
  if (!selectAnalogChannel(4))
    return;
  float rms4 = emon4.calcIrms(740);
  if (!isfinite(rms4)) {
    currentScanValid = false;
    controlReady = false;
    channelGood[4] = 0;
    channelValid &= ~(uint16_t(1) << 4);
    return;
  }
  recordCurrentSample(4);
  if (rms4 <= 3) {
    Irms4 = 0;
    yIrms4 = 0;
    if (status_b3 == HIGH) {
      if (xIrms4 < 4) ++xIrms4;
      if ((xIrms4 > 3) && (canProtect(4, data.SetAmpe4min, data.SetAmpe4max))) {
        xIrms4 = 0;
        off_Bom3();
        trip4 = true;
        if (data.key_noti)
          Blynk.logEvent("error", String("Bơm 7.5Kw lỗi\nKhông đo được DÒNG ĐIỆN"));
      }
    }

  } else if (rms4 > 3) {
    Irms4 = rms4;
    if (yIrms4 < 4) ++yIrms4;
    xIrms4 = 0;
    if (yIrms4 > 2) {


      if ((Irms4 > data.SetAmpe4max) || (Irms4 < data.SetAmpe4min)) {
        if (xSetAmpe4 < 4) ++xSetAmpe4;
        if ((xSetAmpe4 >= 3) && (canProtect(4, data.SetAmpe4min, data.SetAmpe4max))) {
          if (data.key_noti)
            Blynk.logEvent("error", String("Máy 7.5KW lỗi: ") + Irms4 + String(" A"));
          trip4 = true;
          xSetAmpe4 = 0;
          status_b3 = LOW;
          startRelayPulse(pin_off_Bom3, 3000);
        }
      } else
        xSetAmpe4 = 0;
    }
  }
  noteMotorSample(4);
}
void readcurrent5() // C5 - Gieng 3
{
  // Blynk.run();
  if (!selectAnalogChannel(5))
    return;
  float rms5 = emon5.calcIrms(740);
  if (!isfinite(rms5)) {
    currentScanValid = false;
    controlReady = false;
    channelGood[5] = 0;
    channelValid &= ~(uint16_t(1) << 5);
    return;
  }
  recordCurrentSample(5);
  if (rms5 <= 3) {
    Irms5 = 0;
    yIrms5 = 0;
    if (status_g3 == HIGH) {
      if (xIrms5 < 4) ++xIrms5;
      if ((xIrms5 > 3) && (canProtect(5, data.SetAmpe5min, data.SetAmpe5max))) {
        xIrms5 = 0;
        offG3();
        trip5 = true;
        if (data.key_noti)
          Blynk.logEvent("error", String("Giếng 3 lỗi\nKhông đo được DÒNG ĐIỆN"));
      }
    }

    if ((unsigned long)(millis() - dem3) > 1800000) {
      dem3 = millis();
      // event_30p();
    }
  } else if (rms5 >= 3) {
    Irms5 = rms5;
    if (yIrms5 < 4) ++yIrms5;
    xIrms5 = 0;
    if (yIrms5 > 3) {
      dem3 = millis();
    }
    if (yIrms5 > 2) {

      if ((Irms5 > data.SetAmpe5max) || (Irms5 < data.SetAmpe5min)) {
        if (xSetAmpe5 < 4) ++xSetAmpe5;
        if ((xSetAmpe5 >= 3) && (canProtect(5, data.SetAmpe5min, data.SetAmpe5max))) {
          if (data.key_noti)
            Blynk.logEvent("error", String("Giếng III lỗi: ") + Irms5 + String(" A"));
          trip5 = true;
          xSetAmpe5 = 0;
          startRelayPulse(pin_off_G3, 3000);
        }
      } else
        xSetAmpe5 = 0;
    }
  }
  noteMotorSample(5);
}
void readcurrent6() // C6 - 30kw
{
  // Blynk.run();
  if (!selectAnalogChannel(6))
    return;
  float rms6 = emon6.calcIrms(740);
  if (!isfinite(rms6)) {
    currentScanValid = false;
    controlReady = false;
    channelGood[6] = 0;
    channelValid &= ~(uint16_t(1) << 6);
    return;
  }
  recordCurrentSample(6);
  if (rms6 <= 3) {
    Irms6 = 0;
    yIrms6 = 0;
    if (status_b2 == HIGH) {
      if (auto_start_guard_b2) {
        xIrms6 = 0;
      } else {
        if (xIrms6 < 4) ++xIrms6;
        if ((xIrms6 > 3) && (canProtect(6, data.SetAmpe6min, data.SetAmpe6max))) {
          xIrms6 = 0;
          off_Bom2();
          trip6 = true;
          if (data.key_noti)
            Blynk.logEvent("error", String("Bơm 30Kw lỗi\nKhông đo được DÒNG ĐIỆN"));
        }
      }
    }

  } else if (rms6 >= 3) {
    Irms6 = rms6;
    if (yIrms6 < 4) ++yIrms6;
    xIrms6 = 0;
    if (yIrms6 > 2) {

      if ((Irms6 >= data.SetAmpe6max) || (Irms6 < data.SetAmpe6min)) {
        if (xSetAmpe6 < 4) ++xSetAmpe6;
        if ((xSetAmpe6 >= 3) && (canProtect(6, data.SetAmpe6min, data.SetAmpe6max))) {
          if (data.key_noti)
            Blynk.logEvent("error", String("Máy 30KW lỗi: ") + Irms6 + String(" A"));
          trip6 = true;
          xSetAmpe6 = 0;
          status_b2 = LOW;
          startRelayPulse(pin_off_Bom2, 3000);
        }
      } else
        xSetAmpe6 = 0;
    }
  }
  noteMotorSample(6);
}
void readcurrent7() // C7 - NK1
{
  // Blynk.run();
  if (!selectAnalogChannel(7))
    return;
  float rms7 = emon7.calcIrms(740);
  if (!isfinite(rms7)) {
    currentScanValid = false;
    controlReady = false;
    channelGood[7] = 0;
    channelValid &= ~(uint16_t(1) << 7);
    return;
  }
  recordCurrentSample(7);
  if (rms7 < 3) {
    Irms7 = 0;
    yIrms7 = 0;
  } else if (rms7 >= 3) {
    Irms7 = rms7;
    if (yIrms7 < 4) ++yIrms7;
    if ((yIrms7 > 2) && ((Irms7 >= data.SetAmpe7max) || (Irms7 < data.SetAmpe7min))) {
      if (xSetAmpe7 < 4) ++xSetAmpe7;
      if ((xSetAmpe7 >= 3) && (canProtect(7, data.SetAmpe7min, data.SetAmpe7max))) {
        if (data.key_noti)
          Blynk.logEvent("error", String("Máy NÉN KHÍ 1 lỗi: ") + Irms7 + String(" A"));
        trip7 = true;
        xSetAmpe7 = 0;
        writeRelay(pin_NK1, LOW);
      }
    } else {
      xSetAmpe7 = 0;
    }
  }
  // Blynk.virtualWrite(V32, Irms7);  // Irms7 - NK1
  noteMotorSample(7);
}
void readcurrent8() // Z C8 - NK2
{
  // Blynk.run();
  if (!selectAnalogChannel(8))
    return;
  float rms8 = emon8.calcIrms(740);
  if (!isfinite(rms8)) {
    currentScanValid = false;
    controlReady = false;
    channelGood[8] = 0;
    channelValid &= ~(uint16_t(1) << 8);
    return;
  }
  recordCurrentSample(8);
  if (rms8 < 2) {
    Irms8 = 0;
    yIrms8 = 0;
  } else if (rms8 >= 2) {
    Irms8 = rms8;
    if (yIrms8 < 4) ++yIrms8;
    if ((yIrms8 > 2) && ((Irms8 >= data.SetAmpe8max) || (Irms8 < data.SetAmpe8min))) {
      if (xSetAmpe8 < 4) ++xSetAmpe8;
      if ((xSetAmpe8 >= 3) && (canProtect(8, data.SetAmpe8min, data.SetAmpe8max))) {
        if (data.key_noti)
          Blynk.logEvent("error", String("Máy NÉN KHÍ 2 lỗi: ") + Irms8 + String(" A"));
        trip8 = true;
        xSetAmpe8 = 0;
        writeRelay(pin_NK2, LOW);
      }
    } else {
      xSetAmpe8 = 0;
    }
  }
  // Blynk.virtualWrite(V36, Irms8);  // Irms6 - 30kw
  noteMotorSample(8);
}

void processAutoPumps() {
  enum AutoPumpState : uint8_t {
    AUTO_IDLE,
    AUTO_STOP_FOR_B4,
    AUTO_WAIT_STOP_FOR_B4,
    AUTO_VERIFY_B4,
    AUTO_RETRY_WAIT_B4,
    AUTO_STOP_B4_FOR_B2,
    AUTO_WAIT_STOP_FOR_B2,
    AUTO_VERIFY_B2,
    AUTO_RETRY_WAIT_B2
  };

  static AutoPumpState state = AUTO_IDLE;
  static PumpSchedule schedule;
  static unsigned long phase_started_ms = 0;
  static unsigned long stopped_since_ms = 0;
  static unsigned long current_since_ms = 0;
  static uint8_t start_attempt = 0;

  const unsigned long STOP_SETTLE_MS = 2000UL;
  const unsigned long RUN_CONFIRM_MS = 10000UL;
  const unsigned long STOP_TIMEOUT_MS = 30000UL;
  const unsigned long START_ATTEMPT_TIMEOUT_MS = 20000UL;
  const unsigned long RETRY_COOLDOWN_MS = 5000UL;
  const uint8_t MAX_START_ATTEMPTS = 2;

  DateTime now;
  bool valid = restartRequest == RESTART_NONE && data.en_auto_b2_b4 && controlReady && currentScanValid &&
               data.auto_start_h < 24 && data.auto_stop_h < 24 &&
               data.auto_start_m < 60 && data.auto_stop_m < 60 &&
               validCurrentLimits(data.SetAmpe2min, data.SetAmpe2max) &&
               validCurrentLimits(data.SetAmpe6min, data.SetAmpe6max) &&
               data.SetAmpe2max > 3 && data.SetAmpe6max > 3 &&
               isfinite(Irms0) && isfinite(Irms2) && isfinite(Irms4) && isfinite(Irms6) &&
               readStationTime(now);
  uint16_t start_mins = data.auto_start_h * 60 + data.auto_start_m;
  uint16_t stop_mins = data.auto_stop_h * 60 + data.auto_stop_m;
  if (!valid || start_mins == stop_mins || auto_resync_required) {
    state = AUTO_IDLE;
    schedule.initialized = false;
    stopped_since_ms = current_since_ms = 0;
    start_attempt = 0;
    auto_start_guard_b2 = auto_start_guard_b4 = false;
    auto_resync_required = false;
    return;
  }

  ScheduleEdge edge = observePumpSchedule(schedule, now.unixtime(), millis(),
                                         now.hour() * 60 + now.minute(),
                                         start_mins, stop_mins);
  if (edge == SCHEDULE_RESYNC) {
    state = AUTO_IDLE;
    stopped_since_ms = current_since_ms = 0;
    start_attempt = 0;
    auto_start_guard_b2 = auto_start_guard_b4 = false;
    return; // Boot/time correction/schedule edit: observe, never recover a command.
  }
  if (edge == SCHEDULE_START || edge == SCHEDULE_STOP) {
    phase_started_ms = millis();
    stopped_since_ms = current_since_ms = 0;
    start_attempt = 0;
    auto_start_guard_b2 = auto_start_guard_b4 = false;
    state = edge == SCHEDULE_START ? AUTO_STOP_FOR_B4 : AUTO_STOP_B4_FOR_B2;
    logAutoPumpStep(edge == SCHEDULE_START ? "Đến giờ chuyển từ Bơm 30 kW sang Bơm 11 kW."
                                          : "Đến giờ chuyển từ Bơm 11 kW sang Bơm 30 kW.");
  }

  unsigned long now_ms = millis();

  switch (state) {
  case AUTO_STOP_FOR_B4:
    // Đầu khung giờ: phát lệnh tắt cả ba bơm 1, 2, 3 một lần.
    auto_start_guard_b2 = false;
    auto_start_guard_b4 = false;
    if (trip2) {
      state = AUTO_IDLE;
      sendAutoPumpNotice("Hủy đổi bơm: Bơm 11 kW đang có trip2.", true);
      break;
    }
    off_Bom1();
    off_Bom2();
    off_Bom3();
    phase_started_ms = millis();
    stopped_since_ms = 0;
    state = AUTO_WAIT_STOP_FOR_B4;
    break;

  case AUTO_WAIT_STOP_FOR_B4:
    if (trip2) {
      state = AUTO_IDLE;
      auto_start_guard_b4 = false;
      sendAutoPumpNotice("Không thể chạy Bơm 11 kW: cờ bảo vệ trip2 đang bật.", true);
      break;
    }

    if (Irms0 == 0 && Irms6 == 0 && Irms4 == 0) {
      if (stopped_since_ms == 0)
        stopped_since_ms = now_ms;

      if ((unsigned long)(now_ms - stopped_since_ms) >= STOP_SETTLE_MS) {
        // Nếu Bơm 4 đã chạy thì không phát thêm xung khởi động.
        ++start_attempt;
        auto_start_guard_b4 = true;
        if (Irms2 == 0) {
          on_Bom4();
          logAutoPumpStep(start_attempt == 1 ? "Đã phát lệnh chạy Bơm 11 kW, lần 1/2."
                                             : "Đã phát lệnh chạy Bơm 11 kW, lần 2/2.");
        } else {
          logAutoPumpStep("Bơm 11 kW đã có dòng, bắt đầu xác nhận ổn định.");
        }

        phase_started_ms = millis();
        current_since_ms = (Irms2 != 0) ? phase_started_ms : 0;
        state = AUTO_VERIFY_B4;
      }
    } else {
      stopped_since_ms = 0;
    }

    if (state == AUTO_WAIT_STOP_FOR_B4 &&
        (unsigned long)(now_ms - phase_started_ms) >= STOP_TIMEOUT_MS) {
      state = AUTO_IDLE;
      auto_start_guard_b4 = false;
      sendAutoPumpNotice("Chuyển sang Bơm 11 kW thất bại: Bơm 1, 2 hoặc 3 chưa dừng.", true);
    }
    break;

  case AUTO_VERIFY_B4:
    if (trip2) {
      state = AUTO_IDLE;
      current_since_ms = 0;
      auto_start_guard_b4 = false;
      sendAutoPumpNotice("Bơm 11 kW phát sinh trip2 trong lúc xác nhận khởi động.", true);
      break;
    }

    if (currentInRange(Irms2, data.SetAmpe2min, data.SetAmpe2max)) {
      if (current_since_ms == 0)
        current_since_ms = now_ms;

      if ((unsigned long)(now_ms - current_since_ms) >= RUN_CONFIRM_MS) {
        state = AUTO_IDLE; // Xác nhận xong, Auto không can thiệp nữa.
        auto_start_guard_b4 = false;
        sendAutoPumpNotice("Đổi từ Bơm 30 kW sang Bơm 11 kW thành công.", false);
      }
    } else {
      current_since_ms = 0;
    }

    if (state == AUTO_VERIFY_B4 &&
        (unsigned long)(now_ms - phase_started_ms) >= START_ATTEMPT_TIMEOUT_MS) {
      off_Bom4();
      auto_start_guard_b4 = false;
      current_since_ms = 0;

      if (start_attempt < MAX_START_ATTEMPTS) {
        phase_started_ms = millis();
        state = AUTO_RETRY_WAIT_B4;
        logAutoPumpStep("Bơm 11 kW chưa lên dòng ổn định; chờ 5 giây rồi thử lại lần cuối.");
      } else {
        if (data.protect && controlReady)
          trip2 = true;
        state = AUTO_IDLE;
        sendAutoPumpNotice("Đổi sang Bơm 11 kW thất bại sau 2 lần thử; đã dừng Auto.", true);
      }
    }
    break;

  case AUTO_RETRY_WAIT_B4:
    if (trip2) {
      state = AUTO_IDLE;
      auto_start_guard_b4 = false;
      sendAutoPumpNotice("Hủy lần thử lại Bơm 11 kW vì cờ bảo vệ trip2 đang bật.", true);
      break;
    }

    if ((unsigned long)(now_ms - phase_started_ms) >= RETRY_COOLDOWN_MS) {
      stopped_since_ms = 0;
      phase_started_ms = millis();
      state = AUTO_WAIT_STOP_FOR_B4; // Recheck stopped pumps before retry ON.
      logAutoPumpStep("Kiểm tra lại các bơm đã dừng trước lần thử 2/2 Bơm 11 kW.");
    }
    break;

  case AUTO_STOP_B4_FOR_B2:
    // Cuối khung giờ: dừng Bơm 4 trước khi chạy Bơm 2.
    auto_start_guard_b4 = false;
    auto_start_guard_b2 = false;
    if (trip6) {
      state = AUTO_IDLE;
      sendAutoPumpNotice("Hủy đổi bơm: Bơm 30 kW đang có trip6.", true);
      break;
    }
    off_Bom4();
    phase_started_ms = millis();
    stopped_since_ms = 0;
    state = AUTO_WAIT_STOP_FOR_B2;
    break;

  case AUTO_WAIT_STOP_FOR_B2:
    if (trip6) {
      state = AUTO_IDLE;
      auto_start_guard_b2 = false;
      sendAutoPumpNotice("Không thể chạy Bơm 30 kW: cờ bảo vệ trip6 đang bật.", true);
      break;
    }

    if (Irms2 == 0) {
      if (stopped_since_ms == 0)
        stopped_since_ms = now_ms;

      if ((unsigned long)(now_ms - stopped_since_ms) >= STOP_SETTLE_MS) {
        // Nếu Bơm 2 đã chạy thì chỉ chuyển sang bước xác nhận.
        ++start_attempt;
        auto_start_guard_b2 = true;
        if (Irms6 == 0) {
          on_Bom2();
          logAutoPumpStep(start_attempt == 1 ? "Đã phát lệnh chạy Bơm 30 kW, lần 1/2."
                                             : "Đã phát lệnh chạy Bơm 30 kW, lần 2/2.");
        } else {
          logAutoPumpStep("Bơm 30 kW đã có dòng, bắt đầu xác nhận ổn định.");
        }

        phase_started_ms = millis();
        current_since_ms = (Irms6 != 0) ? phase_started_ms : 0;
        state = AUTO_VERIFY_B2;
      }
    } else {
      stopped_since_ms = 0;
    }

    if (state == AUTO_WAIT_STOP_FOR_B2 &&
        (unsigned long)(now_ms - phase_started_ms) >= STOP_TIMEOUT_MS) {
      state = AUTO_IDLE;
      auto_start_guard_b2 = false;
      sendAutoPumpNotice("Chuyển sang Bơm 30 kW thất bại: Bơm 11 kW chưa dừng.", true);
    }
    break;

  case AUTO_VERIFY_B2:
    if (trip6) {
      state = AUTO_IDLE;
      current_since_ms = 0;
      auto_start_guard_b2 = false;
      sendAutoPumpNotice("Bơm 30 kW phát sinh trip6 trong lúc xác nhận khởi động.", true);
      break;
    }

    if (currentInRange(Irms6, data.SetAmpe6min, data.SetAmpe6max, true)) {
      if (current_since_ms == 0)
        current_since_ms = now_ms;

      if ((unsigned long)(now_ms - current_since_ms) >= RUN_CONFIRM_MS) {
        state = AUTO_IDLE; // Xác nhận xong, người vận hành được quyền đổi bơm.
        auto_start_guard_b2 = false;
        sendAutoPumpNotice("Đổi từ Bơm 11 kW sang Bơm 30 kW thành công.", false);
      }
    } else {
      current_since_ms = 0;
    }

    if (state == AUTO_VERIFY_B2 &&
        (unsigned long)(now_ms - phase_started_ms) >= START_ATTEMPT_TIMEOUT_MS) {
      off_Bom2();
      auto_start_guard_b2 = false;
      current_since_ms = 0;

      if (start_attempt < MAX_START_ATTEMPTS) {
        phase_started_ms = millis();
        state = AUTO_RETRY_WAIT_B2;
        logAutoPumpStep("Bơm 30 kW chưa lên dòng ổn định; chờ 5 giây rồi thử lại lần cuối.");
      } else {
        if (data.protect && controlReady)
          trip6 = true;
        state = AUTO_IDLE;
        sendAutoPumpNotice("Đổi sang Bơm 30 kW thất bại sau 2 lần thử; đã dừng Auto.", true);
      }
    }
    break;

  case AUTO_RETRY_WAIT_B2:
    if (trip6) {
      state = AUTO_IDLE;
      auto_start_guard_b2 = false;
      sendAutoPumpNotice("Hủy lần thử lại Bơm 30 kW vì cờ bảo vệ trip6 đang bật.", true);
      break;
    }

    if ((unsigned long)(now_ms - phase_started_ms) >= RETRY_COOLDOWN_MS) {
      stopped_since_ms = 0;
      phase_started_ms = millis();
      state = AUTO_WAIT_STOP_FOR_B2; // Recheck stopped pumps before retry ON.
      logAutoPumpStep("Kiểm tra lại Bơm 11 kW đã dừng trước lần thử 2/2 Bơm 30 kW.");
    }
    break;

  case AUTO_IDLE:
  default:
    break;
  }
}
void rtctime() // Irms0 :18.5Kw    Imrs2:11Kw    Imrs4:7.5Kw
{
  //---------------------------------
  DateTime now;
  bool clockValid = readStationTime(now);
  if (rtcReady && blynk_first_connect && timeStatus() == timeSet &&
      year() >= 2026 && year() <= 2099) {
    DateTime cloudTime(year(), month(), day(), hour(), minute(), second());
    if (cloudTime.isValid() && (!clockValid ||
        abs(static_cast<int32_t>(now.unixtime() - cloudTime.unixtime())) > 120)) {
      rtc_module.adjust(cloudTime);
      auto_resync_required = true;
      clockValid = readStationTime(now);
    }
  }
  if (!clockValid) {
    auto_resync_required = true;
    return;
  }
  char display[64];
  snprintf(display, sizeof(display), "%s, %u/%u/%u - %u:%u:%u",
    daysOfTheWeek[now.dayOfTheWeek()], now.day(), now.month(), now.year(),
    now.hour(), now.minute(), now.second());
  Blynk.virtualWrite(V1, display);
  
}
void up_timerun_motor() {
  if (dailySending || remoteHttp.busy || remoteHttp.result || !Blynk.connected() ||
      !stationNetwork.health.allowControl(millis())) return;
  DateTime now;
  if (!readStationTime(now)) return;
  pendingDailyDay = now.day();
  pendingDailyDate = uint32_t(now.year()) * 10000 + now.month() * 100 + now.day();
  collectMotorRuntimes();
  const uint32_t values[] = {data.timerun_G1, data.timerun_G2, data.timerun_G3,
    data.timerun_B1, data.timerun_B2, data.timerun_B3, data.timerun_B4};
  memcpy(dailySnapshot, values, sizeof(dailySnapshot));
  String path; path.reserve(320);
  path = String("batch/update?token=") + BLYNK_AUTH_TOKEN;
  for (uint8_t k = 0; k < 7; ++k)
    path += "&V" + String(71 + k * 2) + "=" + String(dailySnapshot[k] / 3600.0f, 2);
  dailySending = remoteHttp.send(path);
}
void time_run_motor() {
  collectMotorRuntimes();
  DateTime now;
  if (!readStationTime(now)) return;
  uint32_t date = uint32_t(now.year()) * 10000 + now.month() * 100 + now.day();
  if (data.runtime_date != date) up_timerun_motor();
}

void MeasureAndProcessWaterLevel() // C15
{
  // 1. Chọn kênh analog cho cảm biến mực nước
  if (!selectAnalogChannel(15))
    return;

  // 2. Đọc giá trị thô từ ADC
  int raw_value = analogRead(A0);
  levelSampleMs = millis();
  if (levelSamples < 5) ++levelSamples;

  // 3. Áp dụng Median Filter để loại bỏ nhiễu đột biến
  median_buffer[median_buffer_index] = raw_value;
  median_buffer_index = (median_buffer_index + 1) % MEDIAN_WINDOW_SIZE;

  int sorted_buffer[MEDIAN_WINDOW_SIZE];
  memcpy(sorted_buffer, median_buffer, sizeof(median_buffer));
  int median_value = getMedian(sorted_buffer, MEDIAN_WINDOW_SIZE);

  // 4. Đưa giá trị đã qua bộ lọc trung vị vào bộ lọc Kalman
  kalman_filtered_adc_value = levelKalmanFilter.updateEstimate(median_value);

  // 5. Chuyển đổi giá trị ADC đã làm mịn sang đơn vị đo thực tế (cm)
  smoothDistance = interpolate(kalman_filtered_adc_value, data.level_points, data.num_level_points) + LEVEL_SENSOR_BOTTOM_OFFSET_CM;

  // Giới hạn giá trị trong khoảng hợp lý
  smoothDistance = constrain(smoothDistance, 0.0, dosau * 1.4); // Cho phép vượt 40%

  // 6. Tính toán thể tích
  volume1 = (dai * smoothDistance * rong) / 1000000; // m3

  // 7. Logic điều khiển khi bể đầy
  if (smoothDistance >= 500) // Nếu mực nước >= 500cm thì tắt Cấp 1
  {
    if (data.protect && configLoaded && relayReady && levelSamples >= 5 && validLevelCalibration() &&
        isfinite(smoothDistance) && uint32_t(millis() - levelSampleMs) < 5000) {
      if (Irms1 != 0)
        offG2();
      if (Irms3 != 0)
        offG1();
      if (Irms5 != 0)
        offG3();
    }
  }
}

void serviceExternalWatchdog() {
  unsigned long now = millis();
  if ((unsigned long)(now - watchdogLastToggleMs) < WATCHDOG_TOGGLE_INTERVAL_MS)
    return;

  watchdogLastToggleMs = now;
  uint8_t nextLevel = (watchdogOutputLevel == LOW) ? HIGH : LOW;
  if (writeAuxPin(pin_WATCHDOG, nextLevel)) {
    watchdogOutputLevel = nextLevel;
  } else {
    Serial.println("Khong gui duoc heartbeat den watchdog");
  }
}

bool rearmExternalWatchdog() {
  if (!writeAuxPin(pin_WATCHDOG, HIGH)) {
    Serial.println("Khong tao duoc muc HIGH cho watchdog");
    return false;
  }
  delay(100);
  if (!writeAuxPin(pin_WATCHDOG, LOW)) {
    Serial.println("Khong tao duoc canh LOW cho watchdog");
    return false;
  }
  watchdogOutputLevel = LOW;
  watchdogLastToggleMs = millis();
  return true;
}

//----------------------------------------------------
//----------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(20);
  Serial.printf("\nBOOT firmware=%s reset=%s\n", BLYNK_FIRMWARE_VERSION,
                ESP.getResetReason().c_str());
  if (handleOtaBootState())
    return;
  Wire.begin();
  relayWord = 0xFFFF;
  relayReady = writePcfWord(0x21, relayWord); // One idle write, NK default HIGH.
  if (!relayReady)
    Serial.println("Relay PCF8575 unavailable; control blocked");
  auxWord = 0x0100;
  otaWatchdogReady = writePcfWord(0x20, auxWord);
  if (otaWatchdogReady)
    rearmExternalWatchdog();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);

  emon0.current(A0, 110);
  emon1.current(A0, 112);
  emon2.current(A0, 112);
  emon3.current(A0, 110);
  emon4.current(A0, 112);
  emon5.current(A0, 110);
  emon6.current(A0, 109);
  emon7.current(A0, 109);
  emon8.current(A0, 109);

  rtcReady = rtc_module.begin();

  ee.begin();
  ee.setExtraWriteCycleTime(15); // AT24C32 may need up to 20ms; ACK polling ends early.
  configLoaded = cs.begin(ee, PAGE_SIZE, MEMORY_SIZE / PAGE_SIZE,
                         data, validStoredData, serviceExternalWatchdog);
  if (configLoaded && cs.needsMigration()) {
    // Convert exactly once, after fully reading/validating the old structure.
    uint32_t *runtimes[] = {&data.timerun_G1, &data.timerun_G2, &data.timerun_G3,
      &data.timerun_B1, &data.timerun_B2, &data.timerun_B3, &data.timerun_B4};
    for (uint8_t k = 0; k < 7; ++k) {
      motorRemainder[k] = *runtimes[k] % 1000;
      *runtimes[k] /= 1000;
    }
    // Old firmware stored bool as any nonzero byte; canonicalize before version 4 CRC.
    data.en_auto_b2_b4 = reinterpret_cast<const uint8_t *>(&data)
      [offsetof(Data, en_auto_b2_b4)] != 0;
    DateTime clock;
    data.runtime_date = readStationTime(clock) && data.reset_day == clock.day()
      ? uint32_t(clock.year()) * 10000 + clock.month() * 100 + clock.day() : 0;
    data.version = DATA_VERSION;
    configLoaded = cs.write(data);
  }

  if (!configLoaded) {
    Serial.println("EEPROM invalid; control blocked, stored data preserved");
    memset(&data, 0, sizeof(data));
    data.version = DATA_VERSION;
  }
  memcpy(&dataCheck, &data, sizeof(data));

  timer.setTimeout(5000L, []() {
    timer_I = timer.setInterval(1589, []() {
      currentScanRequested = true;

    });
    timer_tank = timer.setInterval(230L, MeasureAndProcessWaterLevel);
    timer.setInterval(15005L, []() {
      rtctime();
      time_run_motor();
    });
    timer.setInterval(30000L, []() {
      connectionstatus();
    }); });
  keyterminal.clear();
  previousLoopTick = millis();
  lastStorageWrite = millis();
}

void loop() {
  uint32_t now = millis();
  uint32_t gap = now - previousLoopTick;
  previousLoopTick = now;
  if (gap > stationNetwork.health.maxLoopGap) stationNetwork.health.maxLoopGap = gap;
  serviceOperatorAccess();
  serviceRelayPulses();
  timer.run();
  timer1.run();
  serviceRelayPulses();
  serviceCurrentScan();
  serviceRelayPulses();
  stationNetwork.service();
  remoteHttp.service();
  serviceNetwork();
  Blynk.run();
  serviceNetwork();
  serviceRelayPulses();
  serviceExternalWatchdog();
  serviceMotorNotices();
  serviceStorage();
  serviceRestartRequest();
}


bool startRelayPulse(uint8_t pin, uint32_t duration) {
  if (pin < 2 || pin > 15) return false;
  uint8_t opposite = pin ^ 1;
  bool isOn = pin < 8 ? (pin & 1) : !(pin & 1);
  uint32_t due = millis() + duration;
  if (!due) due = 1;
  if (relayPulseUntil[pin]) {
    if (!isOn && duration == 3000) relayPulseUntil[pin] = due;
    return false; // Never repeat LOW for an active pulse.
  }
  if (relayPulseUntil[opposite]) {
    if (isOn || !writeRelay(opposite, HIGH)) return false;
    relayPulseUntil[opposite] = 0; // STOP cancels an unfinished ON pulse.
  }
  relayPulseUntil[pin] = due;
  if (!writeRelay(pin, LOW)) return false;
  return true;
}

void serviceRelayPulses() {
  uint32_t now = millis();
  for (uint8_t pin = 2; pin < 16; ++pin) {
    if (relayPulseUntil[pin] && int32_t(now - relayPulseUntil[pin]) >= 0) {
      if (writeRelay(pin, HIGH)) relayPulseUntil[pin] = 0;
      else {
        relayPulseUntil[pin] = now + 100; // Retry release; never repeat LOW.
        if (!relayPulseUntil[pin]) relayPulseUntil[pin] = 1;
      }
    }
  }
}

bool acceptRemoteControl(uint8_t pin, const BlynkParam &param) {
  uint32_t now = millis();
  bool high = param.asInt() == HIGH;
  serviceOperatorAccess();
  if (restartRequest != RESTART_NONE) return false;
  if (pin >= 2 && !high) return false; // Ignore a momentary button's release.
  if (!(keySwitchQ || keySwitchP || keySwitchD) ||
      !stationNetwork.health.allowControl(now) || !stationNetwork.socket.ready() ||
      !controlReady || uint32_t(now - currentSampleMs) > 5000) return false;
  if (high && ((pin == pin_NK1 && trip7) || (pin == pin_NK2 && trip8))) return false;
  // Optional [ON/OFF, Unix time at click]. Plain legacy buttons cannot prove
  // their age on the WAN; they still require a healthy, freshly-probed session.
  auto sent = param[1];
  if (sent.isValid()) {
    DateTime clock;
    if (!readStationTime(clock)) return false;
    int64_t age = int64_t(clock.unixtime()) - sent.asLongLong();
    if (age < -1 || age > 5) return false;
  }
  uint16_t bit = uint16_t(1) << pin;
  if ((remoteCommandSeen & bit) && bool(remoteCommandHigh & bit) == high &&
      uint32_t(now - remoteLastCommand[pin]) < 5000)
    return false;
  remoteCommandSeen |= bit;
  if (high) remoteCommandHigh |= bit;
  else remoteCommandHigh &= ~bit;
  remoteLastCommand[pin] = now;
  return true;
}

void serviceNetwork() {
  uint32_t now = millis();
  auto &network = stationNetwork;
  network.health.service(now, Blynk.connected() && network.socket.ready());
  if (network.health.level == 0) {
    clearOperatorAccess();
    if (remoteHttp.busy) remoteHttp.cancel();
  }
  if (!Blynk.connected() || !network.socket.ready()) {
    clearOperatorAccess();
    telemetryProbe = 0;
    if (remoteHttp.busy) remoteHttp.cancel();
    return;
  }
  if (!remoteHttp.busy && remoteHttp.result) {
    bool accepted = remoteHttp.result >= 200 && remoteHttp.result < 300;
    if (dailySending && accepted) {
      uint32_t *values[] = {&data.timerun_G1, &data.timerun_G2, &data.timerun_G3,
        &data.timerun_B1, &data.timerun_B2, &data.timerun_B3, &data.timerun_B4};
      for (uint8_t k = 0; k < 7; ++k) *values[k] -= dailySnapshot[k];
      data.reset_day = pendingDailyDay;
      data.runtime_date = pendingDailyDate;
      savedata();
    } else if (!dailySending && !accepted)
      Blynk.virtualWrite(V5, "Lenh lien mach chua duoc xac nhan; khong tu gui lai.\n");
    dailySending = false;
    remoteHttp.result = 0;
  }
  if (network.acknowledgedId) {
    if (network.acknowledgedId == telemetryProbe) {
      lastTelemetryAck = now;
      telemetryProbe = 0;
    }
    network.acknowledgedId = 0;
  }
  serviceTerminalHelp();
  if (network.probeId || network.socket.txSize) return;
  if (publishedNetworkLevel != network.health.level || now - lastStatusPublish >= 30000) {
    Blynk.virtualWrite(V52, network.health.level);
    publishedNetworkLevel = network.health.level;
    lastStatusPublish = now;
  }
  uint32_t interval = network.health.level == 2 ? 5000 : 15000;
  bool telemetry = uint32_t(now - lastTelemetryAttempt) >= interval;
  if (!telemetry && uint32_t(now - lastProbeAttempt) < 5000) return;
  if (telemetry) {
    lastTelemetryAttempt = now;
    updata();
  }
  lastProbeAttempt = now;
  uint16_t id = Blynk.getNextMsgId();
  network.probeId = id;
  network.probeStarted = now;
  if (telemetry) telemetryProbe = id;
  Blynk.sendCmd(BLYNK_CMD_PING, id);
}

void serviceCurrentScan() {
  static uint8_t channel = 0;
  if (!currentScanRequested) return;
  if (channel == 0) currentScanValid = true;
  // One existing RMS measurement per loop; keep relay/network service between
  // channels, without changing calibration or the sample count of EmonLib.
  switch (channel++) {
    case 0: readcurrent(); break;
    case 1: readcurrent1(); break;
    case 2: readcurrent2(); break;
    case 3: readcurrent3(); break;
    case 4: readcurrent4(); break;
    case 5: readcurrent5(); break;
    case 6: readcurrent6(); break;
    case 7: readcurrent7(); break;
    case 8: readcurrent8(); break;
  }
  if (channel < 9) return;
  channel = 0;
  currentScanRequested = false;
  if (currentScanValid) {
    currentSampleMs = millis();
    if (goodCurrentScans < 3) ++goodCurrentScans;
  } else goodCurrentScans = 0;
  controlReady = relayReady && configLoaded && goodCurrentScans >= 3;
  processAutoPumps();
}


struct Motor {
  uint8_t channel, onPin, offPin;
  float *current;
  bool *trip;
  uint8_t *status;
  uint32_t *runtime;
  byte *minimum, *maximum;
  bool (*on)(); bool (*off)();
  const char *name;
};
static const Motor motors[] = {
  {3,7,6,&Irms3,&trip3,&status_g1,&data.timerun_G1,&data.SetAmpe3min,&data.SetAmpe3max,onG1,offG1,"Gieng 1"},
  {1,5,4,&Irms1,&trip1,&status_g2,&data.timerun_G2,&data.SetAmpe1min,&data.SetAmpe1max,onG2,offG2,"Gieng 2"},
  {5,3,2,&Irms5,&trip5,&status_g3,&data.timerun_G3,&data.SetAmpe5min,&data.SetAmpe5max,onG3,offG3,"Gieng 3"},
  {0,8,9,&Irms0,&trip0,&status_b1,&data.timerun_B1,&data.SetAmpemin,&data.SetAmpemax,on_Bom1,off_Bom1,"Bom 18.5kW"},
  {6,10,11,&Irms6,&trip6,&status_b2,&data.timerun_B2,&data.SetAmpe6min,&data.SetAmpe6max,on_Bom2,off_Bom2,"Bom 30kW"},
  {4,12,13,&Irms4,&trip4,&status_b3,&data.timerun_B3,&data.SetAmpe4min,&data.SetAmpe4max,on_Bom3,off_Bom3,"Bom 7.5kW"},
  {2,14,15,&Irms2,&trip2,&status_b4,&data.timerun_B4,&data.SetAmpe2min,&data.SetAmpe2max,on_Bom4,off_Bom4,"Bom 11kW"}
};
bool writePcfWord(uint8_t address, uint16_t word) {
  Wire.beginTransmission(address);
  Wire.write(uint8_t(word)); Wire.write(uint8_t(word >> 8));
  return Wire.endTransmission() == 0;
}
bool writeAuxPin(uint8_t pin, uint8_t value) {
  uint16_t bit = uint16_t(1) << pin;
  if (value == HIGH) auxWord |= bit; else auxWord &= ~bit;
  return writePcfWord(0x20, auxWord);
}
bool channelFresh(uint8_t channel) {
  return channel < 9 && channelGood[channel] >= 3 &&
    (channelValid & (uint16_t(1) << channel)) &&
    uint32_t(millis() - channelSampleMs[channel]) <= 5000;
}
void recordCurrentSample(uint8_t channel) {
  channelValid |= uint16_t(1) << channel;
  channelSampleMs[channel] = millis();
  if (channelGood[channel] < 3) ++channelGood[channel];
}
bool canProtect(uint8_t channel, byte minimum, byte maximum) {
  return configLoaded && relayReady && data.protect &&
    channelFresh(channel) && validCurrentLimits(minimum, maximum);
}
bool validStoredData(const Data &v) {
  if (v.version != 3 && v.version != DATA_VERSION) return false;
  const uint8_t *raw = reinterpret_cast<const uint8_t *>(&v);
  if ((v.version != 3 && raw[offsetof(Data,en_auto_b2_b4)] > 1) ||
      v.protect > 1 || v.key_noti > 1 ||
      v.man > 3 || v.status_rualoc > 3 || v.reset_day > 31 || v.save_num < 0 ||
      v.num_level_points > MAX_CALIB_POINTS || v.auto_start_h > 23 ||
      v.auto_stop_h > 23 || v.auto_start_m > 59 || v.auto_stop_m > 59) return false;
  const byte mins[] = {v.SetAmpemin,v.SetAmpe1min,v.SetAmpe2min,v.SetAmpe3min,v.SetAmpe4min,
    v.SetAmpe5min,v.SetAmpe6min,v.SetAmpe7min,v.SetAmpe8min};
  const byte maxs[] = {v.SetAmpemax,v.SetAmpe1max,v.SetAmpe2max,v.SetAmpe3max,v.SetAmpe4max,
    v.SetAmpe5max,v.SetAmpe6max,v.SetAmpe7max,v.SetAmpe8max};
  for (uint8_t k=0;k<9;++k)
    if ((mins[k] || maxs[k]) && !validCurrentLimits(mins[k],maxs[k])) return false;
  for (uint8_t k=0;k<v.num_level_points;++k)
    if (v.level_points[k].adc>1023 || (k && v.level_points[k].adc<=v.level_points[k-1].adc)) return false;
  if (v.version == 3) {
    const uint32_t times[] = {v.timerun_G1,v.timerun_G2,v.timerun_G3,v.timerun_B1,
      v.timerun_B2,v.timerun_B3,v.timerun_B4};
    for (uint32_t t:times) if (t>INT32_MAX) return false;
  }
  return true;
}
void accountMotor(uint8_t k, uint32_t now) {
  if (!(motorRunning & (uint8_t(1) << k))) return;
  uint32_t elapsed = now - motorSince[k];
  motorSince[k] = now;
  uint32_t seconds = elapsed / 1000;
  uint16_t fraction = elapsed % 1000 + motorRemainder[k];
  seconds += fraction / 1000; motorRemainder[k] = fraction % 1000;
  if (seconds) { *motors[k].runtime += seconds; storageDirty = true; }
}
void collectMotorRuntimes() {
  uint32_t now = millis();
  for (uint8_t k=0;k<7;++k) accountMotor(k,now);
}
void noteMotorSample(uint8_t channel) {
  if (!configLoaded || !channelFresh(channel)) return;
  for (uint8_t k=0;k<7;++k) {
    const Motor &m=motors[k];
    if (m.channel!=channel) continue;
    uint8_t bit=uint8_t(1)<<k;
    if (*m.current>3) {
      if (!(motorRunning&bit)) { motorSince[k]=millis(); motorRunning|=bit; }
      if (!*m.trip && !(motorStopping&bit)) *m.status=HIGH;
    } else {
      accountMotor(k,millis());
      if (motorRunning&bit) { motorRunning&=~bit; savedata(); }
      motorStopping&=~bit;
    }
    return;
  }
}
void handleManualMotor(uint8_t k, bool on, const BlynkParam &param) {
  const Motor &m=motors[k];
  if (!acceptRemoteControl(on?m.onPin:m.offPin,param)) return;
  auto_resync_required=true;
  uint8_t actor=keySwitchQ?2:(keySwitchP?1:0);
  bool accepted=on?m.on():m.off();
  if (!accepted) {
    if (data.key_noti) Blynk.logEvent("error",String("Khong gui duoc lenh ")+(on?"bat ":"tat ")+m.name);
    return;
  }
  uint8_t bit=uint8_t(1)<<k;
  noticePending|=bit; if(on)noticeOn|=bit;else noticeOn&=~bit;
  noticeSince[k]=millis();noticeActor[k]=actor;
}
void serviceMotorNotices() {
  for(uint8_t k=0;k<7;++k) {
    uint8_t bit=uint8_t(1)<<k;
    if(!(noticePending&bit) || uint32_t(millis()-noticeSince[k])<3000) continue;
    const Motor &m=motors[k];bool on=noticeOn&bit;
    bool fresh=channelFresh(m.channel) && int32_t(channelSampleMs[m.channel]-noticeSince[k])>=0;
    bool reached=fresh && (on?(*m.current>3 && !*m.trip):(*m.current==0));
    if(!reached && uint32_t(millis()-noticeSince[k])<10000) continue;
    noticePending&=~bit;
    if(!data.key_noti) continue;
    static const char *const names[]={"Duc","Phong","Quang"};
    Blynk.logEvent(reached?"info":"error", String(names[noticeActor[k]])+
      (reached?(on?" da bat ":" da tat "):" khong xac nhan duoc ")+m.name);
  }
}
void grantOperatorAccess(uint8_t actor,uint32_t duration) {
  accessUntil[actor]=millis()+duration;
  if(!accessUntil[actor])accessUntil[actor]=1;
  if(actor==0)keySwitchD=true;else if(actor==1)keySwitchP=true;else keySwitchQ=true;
}
void serviceOperatorAccess() {
  uint32_t now=millis();
  for(uint8_t k=0;k<3;++k) if(accessUntil[k] && int32_t(now-accessUntil[k])>=0) {
    accessUntil[k]=0;
    if(k==0)keySwitchD=false;else if(k==1)keySwitchP=false;else keySwitchQ=false;
    keyterminal.clear();
  }
}
void clearOperatorAccess() {
  keySwitchQ=keySwitchP=keySwitchD=keySet=false;
  memset(accessUntil,0,sizeof(accessUntil));
}
bool pulsesPending() {
  for(uint8_t k=2;k<16;++k) if(relayPulseUntil[k]) return true;
  return false;
}
void serviceStorage() {
  if(!configLoaded || !storageDirty || pulsesPending()) return;
  uint32_t now=millis();
  if(storageFailed && uint32_t(now-lastStorageAttempt)<5000) return;
  if(!storageUrgent && uint32_t(now-lastStorageWrite)<60000) return;
  collectMotorRuntimes();
  Data snapshot; memcpy(&snapshot,&data,sizeof(snapshot));
  if(snapshot.save_num<INT32_MAX) ++snapshot.save_num;
  lastStorageAttempt=now;
  if(!cs.write(snapshot)) { storageFailed=true; return; }
  memcpy(&data,&snapshot,sizeof(data));memcpy(&dataCheck,&data,sizeof(data));
  storageDirty=storageUrgent=storageFailed=false;lastStorageWrite=millis();
  if(saveReport) { Blynk.virtualWrite(V5,"Da luu thanh cong.\n");saveReport=false; }
  Blynk.setProperty(V5,"label",data.save_num);
}
void serviceRestartRequest() {
  if(restartRequest==RESTART_NONE || int32_t(millis()-restartNotBefore)<0 ||
     pulsesPending()) return;
  savedata(); // Include the last measured running interval before restarting.
  if (storageDirty) return;
  // Write a full idle pulse word and require ACK. NK bits are preserved.
  uint16_t idle=relayWord|0xFFFC;
  if(!writePcfWord(0x21,idle)) {
    relayReady=controlReady=false;
    restartRequest=RESTART_NONE;
    Blynk.virtualWrite(V5,"Khong nhan duoc ACK nha relay; huy khoi dong/cap nhat.\n");
    return;
  }
  relayWord=idle;
  if(restartRequest==RESTART_OTA && !writeOtaRtcState(OTA_RTC_REQUESTED)) {
    restartRequest=RESTART_NONE;
    Blynk.virtualWrite(V5,"Khong ghi duoc yeu cau OTA; da huy.\n");return;
  }
  ESP.restart();
}
void prepareCurrentDraft() {
  draftMotor=z;
  byte *mins[]={&data.SetAmpemin,&data.SetAmpe6min,&data.SetAmpe4min,&data.SetAmpe2min,
    &data.SetAmpe3min,&data.SetAmpe1min,&data.SetAmpe5min,&data.SetAmpe7min,&data.SetAmpe8min};
  byte *maxs[]={&data.SetAmpemax,&data.SetAmpe6max,&data.SetAmpe4max,&data.SetAmpe2max,
    &data.SetAmpe3max,&data.SetAmpe1max,&data.SetAmpe5max,&data.SetAmpe7max,&data.SetAmpe8max};
  draftMin=z?*mins[z-1]:0; draftMax=z?*maxs[z-1]:0;
}
void setCurrentDraft(bool minimum,const BlynkParam &param) {
  serviceOperatorAccess();
  if(!stationNetwork.health.allowControl(millis()) || !keySet || z<1 || z>9) return;
  int value=param.asInt();
  if(value<1 || value>255) { Blynk.virtualWrite(V5,"Nguong dong phai tu 1 den 255 A.\n");return; }
  if(draftMotor!=z)prepareCurrentDraft();
  if(minimum)draftMin=value;else draftMax=value;
  if(!validCurrentLimits(draftMin,draftMax)) {
    Blynk.virtualWrite(V5,"Bo min/max chua hop le; chua ap dung.\n");return;
  }
  byte *mins[]={&data.SetAmpemin,&data.SetAmpe6min,&data.SetAmpe4min,&data.SetAmpe2min,
    &data.SetAmpe3min,&data.SetAmpe1min,&data.SetAmpe5min,&data.SetAmpe7min,&data.SetAmpe8min};
  byte *maxs[]={&data.SetAmpemax,&data.SetAmpe6max,&data.SetAmpe4max,&data.SetAmpe2max,
    &data.SetAmpe3max,&data.SetAmpe1max,&data.SetAmpe5max,&data.SetAmpe7max,&data.SetAmpe8max};
  *mins[z-1]=draftMin;*maxs[z-1]=draftMax;savedata();auto_resync_required=true;
}
static const char helpText[] PROGMEM =
  "t2, M, đ: quyen van hanh 10-15s\n"
  "active / deactive: vao / thoat cai dat\n"
  "save: luu EEPROM; reset: xoa loi may\n"
  "rst: khoi dong lai; update: OTA\n"
  "i2c / save_num / calib: xem thong tin\n"
  "level_YYY: them diem muc nuoc\n"
  "level_N_YYY: thay diem thu N\n"
  "level_clear: xoa hieu chuan\n";
void serviceTerminalHelp() {
  if(helpPosition==UINT16_MAX || !stationNetwork.health.allowControl(millis()) ||
     stationNetwork.socket.txSize || int32_t(millis()-helpNext)<0) return;
  char line[112];uint8_t count=0;
  while(count<sizeof(line)-1) {
    char ch=pgm_read_byte(helpText+helpPosition);
    if(!ch) { helpPosition=UINT16_MAX;break; }
    ++helpPosition;line[count++]=ch;if(ch=='\n')break;
  }
  line[count]=0;if(count)Blynk.virtualWrite(V5,line);
  helpNext=millis()+200;
}
