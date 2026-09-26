/* Blynk virtual pins: V0..V75 keep the existing template mapping.
   V76 reports network quality: 2=good, 1=usable, 0=remote control locked. */
#define BLYNK_TEMPLATE_ID "TMPL61agYO8cM"
#define BLYNK_TEMPLATE_NAME "TRẠM CÁI CÁT"
#define BLYNK_AUTH_TOKEN "vcz0jVXPSGPK6XmFP5Dqi_etQA32VNPL"

#define VOLUME_TOKEN_G1 "jaQFoaOgdcZcKbyI_ME_oi6tThEf4FR5"
#define VOLUME_TOKEN_G2 "HZCB36tVTkXZqdwbjV2i6TsewQJx5LWe"
#define VOLUME_TOKEN_G3 "DEdOyQWTbvQ5_ma_MEP1_Z8gefY_rnfE"

#define BLYNK_PRINT Serial
#define BLYNK_FIRMWARE_VERSION "260926.4"

const char *ssid = "NHA MAY NUOC CAI CAT";
const char *password = "12345678";

#define APP_DEBUG
#include "CleanOta.h"
#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <DallasTemperature.h>
#include <Eeprom24C32_64.h>
#include <EmonLib.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <PCF8575.h>
#include <RTClib.h>
#include <UrlEncode.h>
#include <WidgetRTC.h>
#include <Wire.h>
#include <math.h>
#include <stddef.h>

// I/O expander: pulse outputs are active LOW; P0/P1 are continuous compressor relays.
PCF8575 pcf8575_1(0x20);
const uint8_t pin_on_G1 = P7;
const uint8_t pin_off_G1 = P6;
const uint8_t pin_on_G2 = P5;
const uint8_t pin_off_G2 = P4;
const uint8_t pin_on_G3 = P3;
const uint8_t pin_off_G3 = P2;
const uint8_t pin_NK1 = P1;
const uint8_t pin_NK2 = P0;
const uint8_t pin_on_Bom1 = P8;
const uint8_t pin_off_Bom1 = P9;
const uint8_t pin_on_Bom2 = P10;
const uint8_t pin_off_Bom2 = P11;
const uint8_t pin_on_Bom3 = P12;
const uint8_t pin_off_Bom3 = P13;
const uint8_t pin_on_Bom4 = P14;
const uint8_t pin_off_Bom4 = P15;
const uint16_t COMPRESSOR_MASK = (1U << P0) | (1U << P1);

bool relayReady = false;
uint16_t relayShadow = 0xFFFF;
uint32_t pulseUntil[16] = {};

// Analog multiplexer and sensors.
const uint8_t S0 = 14;
const uint8_t S1 = 12;
const uint8_t S2 = 13;
const uint8_t S3 = 15;
EnergyMonitor emon0, emon1, emon2, emon3, emon4, emon5;
float Irms0 = 0, Irms1 = 0, Irms2 = 0, Irms3 = 0, Irms4 = 0, Irms5 = 0;
uint8_t xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0, xSetAmpe3 = 0, xSetAmpe4 = 0, xSetAmpe5 = 0;
uint8_t yIrms0 = 0, yIrms1 = 0, yIrms2 = 0, yIrms3 = 0, yIrms4 = 0, yIrms5 = 0;
bool trip0 = false, trip1 = false, trip2 = false, trip3 = false, trip4 = false, trip5 = false;

#define ONE_WIRE_BUS 0
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress temp = {0x28, 0xFF, 0x05, 0x2E, 0x32, 0x17, 0x03, 0x0E};
DeviceAddress temp2 = {0x28, 0xA8, 0x45, 0x79, 0xA2, 0x01, 0x03, 0x7C};
DeviceAddress temp3 = {0x28, 0x83, 0xF3, 0x79, 0xA2, 0x00, 0x03, 0x25};
float temp_1 = 0, temp_2 = 0, temp_3 = 0;
bool temperaturePending = false;
uint32_t temperatureReadyAt = 0;

RTC_DS3231 rtc_module;
WidgetRTC rtc_widget;
char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";
bool rtcAvailable = false;
bool rtcValid = false;
uint32_t timestamp = 0;
uint32_t lastRtcUnix = 0;
uint32_t lastRtcMillis = 0;

#define EEPROM_ADDRESS 0x57
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
const uint16_t DATA_SLOT_A = 128;
const uint16_t DATA_SLOT_B = 256;
const uint32_t DATA_MAGIC = 0x43434154UL; // CCAT
const uint16_t DATA_VERSION = 1;

struct Data {
  uint8_t SetAmpemax, SetAmpemin;
  uint8_t SetAmpe1max, SetAmpe1min;
  uint8_t SetAmpe2max, SetAmpe2min;
  uint8_t SetAmpe3max, SetAmpe3min;
  uint8_t SetAmpe4max, SetAmpe4min;
  uint8_t SetAmpe5max, SetAmpe5min;
  uint8_t SetAmpe6max, SetAmpe6min;
  uint8_t SetAmpe7max, SetAmpe7min;
  uint8_t SetAmpe8max, SetAmpe8min;
  uint8_t man;
  uint32_t save_num;
  uint8_t reboot_num;
  int32_t start, stop;
  uint8_t keyp, rualoc;
  uint32_t LLG1_RL, LLG2_RL, LLG3_RL;
  float clo;
  uint32_t time_clo;
  uint8_t reset_day;
  uint32_t timerun_G1, timerun_G2, timerun_G3, timerun_B1, timerun_B2, timerun_B3;
};

struct StoredData {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  uint32_t sequence;
  Data payload;
  uint16_t crc;
};

Data data = {};
Data dataCheck = {};
uint32_t dataSequence = 0;
uint16_t activeDataSlot = DATA_SLOT_B;
bool configValid = false;

const char SERVER_NAME[] = "http://sgp1.blynk.cloud/external/api/";
const char RUALOC_TOKEN[] = "mAEloc4FYavbw8Jh8KPbhJSjUGWyxKqn";
#define URL_fw_Bin "https://tram-cc-private-ota.dieu-hanh-cap-nuoc.workers.dev/tram-cc/sdzs83XGinW2PABJ9OEbBH6QMIKXNS0pL8HLc3g8luw/firmware.bin"

BlynkTimer timer;
WidgetTerminal keyterminal(V5);
WidgetTerminal terminal_volume(V45);

bool key = false;
uint32_t keyExpiresAt = 0;
uint32_t blynkConnectedAt = 0;
uint8_t networkQuality = 0;
uint32_t lastNetworkProbeOk = 0;
uint16_t lastNetworkLatency = 0;
uint8_t lastPublishedNetworkQuality = 255;
uint32_t lastRemoteCommandAt[22] = {};

uint32_t LLG1_1m3 = 0, LLG2_1m3 = 0, LLG3_1m3 = 0;
float clo_cache = 0;
const int dai = 2400;
const int rong = 1230;
float conlai = 0, thetich = 0;
int z = 0;
float Result1 = 0, value1 = 0;
uint8_t currentChannel = 0;
uint8_t completeCurrentScans = 0;
uint32_t lastFullCurrentScanAt = 0;
uint32_t G1_start = 0, G2_start = 0, G3_start = 0, B1_start = 0, B2_start = 0, B3_start = 0;
bool G1_save = false, G2_save = false, G3_save = false, B1_save = false, B2_save = false, B3_save = false;

bool scheduleInitialized = false;
bool scheduleWasInside = false;
enum AutoStage : uint8_t { AUTO_IDLE, AUTO_WAIT_B3 };
AutoStage autoStage = AUTO_IDLE;
uint32_t autoDeadline = 0;

uint16_t crc16(const uint8_t *bytes, size_t length) {
  uint16_t crc = 0xFFFF;
  while (length--) {
    crc ^= *bytes++;
    for (uint8_t i = 0; i < 8; i++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

bool sanitizeData(Data &value) {
  bool changed = false;
  uint8_t *limits[] = {&value.SetAmpemax, &value.SetAmpemin, &value.SetAmpe1max, &value.SetAmpe1min,
                       &value.SetAmpe2max, &value.SetAmpe2min, &value.SetAmpe3max, &value.SetAmpe3min,
                       &value.SetAmpe4max, &value.SetAmpe4min, &value.SetAmpe5max, &value.SetAmpe5min,
                       &value.SetAmpe6max, &value.SetAmpe6min, &value.SetAmpe7max, &value.SetAmpe7min,
                       &value.SetAmpe8max, &value.SetAmpe8min};
  for (uint8_t *limit : limits) {
    if (*limit > 100) { *limit = 0; changed = true; }
  }
  const bool protectionConfigValid =
      value.SetAmpemax > value.SetAmpemin && value.SetAmpe1max > value.SetAmpe1min &&
      value.SetAmpe2max > value.SetAmpe2min && value.SetAmpe3max > value.SetAmpe3min &&
      value.SetAmpe4max > value.SetAmpe4min && value.SetAmpe5max > value.SetAmpe5min;
  if (value.keyp == 1 && !protectionConfigValid) { value.keyp = 0; changed = true; }
  if (value.man > 1) { value.man = 0; changed = true; }
  if (value.keyp > 1) { value.keyp = 0; changed = true; }
  if (value.rualoc > 3) { value.rualoc = 0; changed = true; }
  if (value.start < 0 || value.start >= 86400) { value.start = 0; changed = true; }
  if (value.stop < 0 || value.stop >= 86400) { value.stop = 0; changed = true; }
  if (value.reset_day > 31) { value.reset_day = 0; changed = true; }
  if (!isfinite(value.clo) || value.clo < 0 || value.clo > 100) { value.clo = 0; changed = true; }
  uint32_t *runtimes[] = {&value.timerun_G1, &value.timerun_G2, &value.timerun_G3,
                          &value.timerun_B1, &value.timerun_B2, &value.timerun_B3};
  for (uint32_t *runtime : runtimes) {
    if (*runtime > 172800000UL) { *runtime = 0; changed = true; }
  }
  return changed;
}

bool readStoredData(uint16_t slot, StoredData &record) {
  eeprom.readBytes(slot, sizeof(record), reinterpret_cast<byte *>(&record));
  if (record.magic != DATA_MAGIC || record.version != DATA_VERSION || record.size != sizeof(Data)) return false;
  return record.crc == crc16(reinterpret_cast<const uint8_t *>(&record), offsetof(StoredData, crc));
}

bool writeStoredData(uint16_t slot, const StoredData &record) {
  eeprom.writeBytes(slot, sizeof(record), const_cast<byte *>(reinterpret_cast<const byte *>(&record)));
  StoredData check = {};
  return readStoredData(slot, check) && check.sequence == record.sequence &&
         memcmp(&check.payload, &record.payload, sizeof(Data)) == 0;
}

void savedata(bool force = false) {
  if (!force && memcmp(&data, &dataCheck, sizeof(Data)) == 0) return;
  data.save_num++;
  StoredData record = {};
  record.magic = DATA_MAGIC;
  record.version = DATA_VERSION;
  record.size = sizeof(Data);
  record.sequence = ++dataSequence;
  record.payload = data;
  record.crc = crc16(reinterpret_cast<const uint8_t *>(&record), offsetof(StoredData, crc));
  const uint16_t target = activeDataSlot == DATA_SLOT_A ? DATA_SLOT_B : DATA_SLOT_A;
  if (writeStoredData(target, record)) {
    activeDataSlot = target;
    dataCheck = data;
    if (Blynk.connected()) Blynk.setProperty(V5, "label", data.save_num);
  } else {
    data.save_num--;
    dataSequence--;
  }
}

void loadData() {
  StoredData a = {}, b = {};
  const bool validA = readStoredData(DATA_SLOT_A, a);
  const bool validB = readStoredData(DATA_SLOT_B, b);
  if (validA || validB) {
    const bool useA = validA && (!validB || static_cast<int32_t>(a.sequence - b.sequence) > 0);
    const StoredData &selected = useA ? a : b;
    data = selected.payload;
    dataSequence = selected.sequence;
    activeDataSlot = useA ? DATA_SLOT_A : DATA_SLOT_B;
    const bool changed = sanitizeData(data);
    dataCheck = data;
    if (changed) savedata(true);
  } else {
    // Migrate the exact legacy structure stored at address 0.
    eeprom.readBytes(0, sizeof(Data), reinterpret_cast<byte *>(&data));
    sanitizeData(data);
    memset(&dataCheck, 0xFF, sizeof(dataCheck));
    savedata(true);
  }
  configValid = true;
}

bool readPcfWord(uint16_t &value) {
  Wire.requestFrom(static_cast<uint8_t>(0x20), static_cast<uint8_t>(2));
  if (Wire.available() < 2) return false;
  value = Wire.read();
  value |= static_cast<uint16_t>(Wire.read()) << 8;
  return true;
}

bool initializeRelays() {
  Wire.begin();
  uint16_t observed = 0xFFFF;
  const bool observedOk = readPcfWord(observed);
  // Release every pulse relay. Preserve only the two continuous relays on an ESP-only reset.
  relayShadow = 0xFFFF;
  if (observedOk) relayShadow = (relayShadow & ~COMPRESSOR_MASK) | (observed & COMPRESSOR_MASK);
  for (uint8_t pin = 0; pin < 16; pin++)
    pcf8575_1.pinMode(pin, OUTPUT, (relayShadow & (1U << pin)) ? HIGH : LOW);
  relayReady = pcf8575_1.begin();
  return relayReady;
}

bool writeRelay(uint8_t pin, uint8_t value) {
  if (!relayReady || pin > 15) return false;
  if (!pcf8575_1.digitalWrite(pin, value)) {
    relayReady = false;
    return false;
  }
  if (value == HIGH) relayShadow |= 1U << pin;
  else relayShadow &= ~(1U << pin);
  return true;
}

bool startRelayPulse(uint8_t pin, uint32_t durationMs) {
  if (!writeRelay(pin, LOW)) return false;
  pulseUntil[pin] = millis() + durationMs;
  return true;
}

void releaseRelayPulse(uint8_t pin) {
  pulseUntil[pin] = 0;
  writeRelay(pin, HIGH);
}

void serviceRelayPulses() {
  const uint32_t now = millis();
  for (uint8_t pin = 0; pin < 16; pin++) {
    if (pulseUntil[pin] && static_cast<int32_t>(now - pulseUntil[pin]) >= 0) releaseRelayPulse(pin);
  }
}

bool anyRelayPulseActive() {
  for (uint8_t pin = 0; pin < 16; pin++) if (pulseUntil[pin]) return true;
  return false;
}

void releaseAllRelays() {
  for (uint8_t pin = 0; pin < 16; pin++) {
    pulseUntil[pin] = 0;
    writeRelay(pin, HIGH);
  }
}

void updateNetworkQuality() {
  uint8_t quality = 0;
  const uint32_t now = millis();
  if (WiFi.status() == WL_CONNECTED && Blynk.connected() &&
      static_cast<uint32_t>(now - blynkConnectedAt) >= 5000UL &&
      lastNetworkProbeOk && static_cast<uint32_t>(now - lastNetworkProbeOk) <= 45000UL) {
    const int rssi = WiFi.RSSI();
    if (rssi >= -70 && lastNetworkLatency <= 800) quality = 2;
    else if (rssi >= -85 && lastNetworkLatency <= 1500) quality = 1;
  }
  networkQuality = quality;
  if (Blynk.connected() && networkQuality != lastPublishedNetworkQuality) {
    Blynk.virtualWrite(V76, networkQuality);
    lastPublishedNetworkQuality = networkQuality;
  }
}

bool httpGet(const String &url, bool measureNetwork = true) {
  if (WiFi.status() != WL_CONNECTED || anyRelayPulseActive()) return false;
  WiFiClient httpClient;
  HTTPClient request;
  request.setTimeout(1200);
  const uint32_t started = millis();
  if (!request.begin(httpClient, url)) return false;
  const int code = request.GET();
  request.end();
  const uint32_t elapsed = millis() - started;
  const bool ok = code >= 200 && code < 300;
  if (measureNetwork) {
    if (ok) {
      lastNetworkProbeOk = millis();
      lastNetworkLatency = elapsed > 65535 ? 65535 : elapsed;
    } else {
      lastNetworkProbeOk = 0;
    }
    updateNetworkQuality();
  }
  return ok;
}

void probeNetwork() {
  if (WiFi.status() != WL_CONNECTED || !Blynk.connected() || anyRelayPulseActive()) {
    updateNetworkQuality();
    return;
  }
  char url[180];
  snprintf(url, sizeof(url), "%sget?token=%s&V29", SERVER_NAME, BLYNK_AUTH_TOKEN);
  httpGet(String(url));
}

bool remoteControlReady(uint8_t virtualPin) {
  const uint32_t now = millis();
  const bool ready = key && relayReady && configValid && rtcValid && completeCurrentScans >= 3 &&
                     lastFullCurrentScanAt && now - lastFullCurrentScanAt < 5000UL && networkQuality > 0;
  if (!ready || virtualPin > 21) return false;
  if (lastRemoteCommandAt[virtualPin] && now - lastRemoteCommandAt[virtualPin] < 5000UL) return false;
  lastRemoteCommandAt[virtualPin] = now;
  return true;
}

bool localControlReady() {
  return relayReady && configValid && rtcValid && completeCurrentScans >= 3 &&
         lastFullCurrentScanAt && millis() - lastFullCurrentScanAt < 5000UL;
}

void rejectRemoteCommand(uint8_t virtualPin) {
  if (Blynk.connected()) {
    Blynk.virtualWrite(virtualPin, 0);
    Blynk.virtualWrite(V5, "Lệnh bị khóa: mạng hoặc dữ liệu vận hành chưa đủ tin cậy.");
  }
}

BLYNK_CONNECTED() {
  rtc_widget.begin();
  blynkConnectedAt = millis();
  lastNetworkProbeOk = 0;
  networkQuality = 0;
  lastPublishedNetworkQuality = 255;
  for (uint8_t pin = 10; pin <= 21; pin++) Blynk.virtualWrite(pin, 0);
  Blynk.virtualWrite(V76, 0);
}

void connectionstatus() {
  if (WiFi.status() != WL_CONNECTED) WiFi.begin(ssid, password);
  if (!Blynk.connected()) {
    networkQuality = 0;
    lastNetworkProbeOk = 0;
  }
  updateNetworkQuality();
}

void sendTelemetryStep() {
  if (!Blynk.connected()) return;
  static uint8_t step = 0;
  switch (step) {
  case 0: Blynk.virtualWrite(V29, Result1); break;
  case 1: Blynk.virtualWrite(V30, Irms0); break;
  case 2: Blynk.virtualWrite(V31, Irms1); break;
  case 3: Blynk.virtualWrite(V32, Irms2); break;
  case 4: Blynk.virtualWrite(V33, Irms3); break;
  case 5: Blynk.virtualWrite(V34, Irms4); break;
  case 6: Blynk.virtualWrite(V35, Irms5); break;
  case 7: Blynk.virtualWrite(V36, temp_1); break;
  case 8: Blynk.virtualWrite(V37, temp_2); break;
  case 9: Blynk.virtualWrite(V38, temp_3); break;
  case 10: Blynk.virtualWrite(V60, data.timerun_G1 / 3600000.0f); break;
  case 11: Blynk.virtualWrite(V62, data.timerun_G2 / 3600000.0f); break;
  case 12: Blynk.virtualWrite(V64, data.timerun_G3 / 3600000.0f); break;
  case 13: Blynk.virtualWrite(V66, data.timerun_B1 / 3600000.0f); break;
  case 14: Blynk.virtualWrite(V68, data.timerun_B2 / 3600000.0f); break;
  case 15: Blynk.virtualWrite(V70, data.timerun_B3 / 3600000.0f); break;
  default: Blynk.virtualWrite(V76, networkQuality); break;
  }
  step = (step + 1) % 17;
}

void up_timerun_motor() {
  if (!Blynk.connected()) return;
  Blynk.virtualWrite(V61, data.timerun_G1 / 3600000.0f);
  Blynk.virtualWrite(V63, data.timerun_G2 / 3600000.0f);
  Blynk.virtualWrite(V65, data.timerun_G3 / 3600000.0f);
  Blynk.virtualWrite(V67, data.timerun_B1 / 3600000.0f);
  Blynk.virtualWrite(V69, data.timerun_B2 / 3600000.0f);
  Blynk.virtualWrite(V71, data.timerun_B3 / 3600000.0f);
}

void time_run_motor() {
  const uint8_t rtcDay = rtcValid ? rtc_module.now().day() : 0;
  if (rtcDay && data.reset_day != rtcDay && Blynk.connected()) {
    up_timerun_motor();
    data.timerun_G1 = data.timerun_G2 = data.timerun_G3 = 0;
    data.timerun_B1 = data.timerun_B2 = data.timerun_B3 = 0;
    data.reset_day = rtcDay;
    savedata();
  }
  if (!(G1_save || G2_save || G3_save || B1_save || B2_save || B3_save)) return;
  const uint32_t now = millis();
  if (G1_start) { data.timerun_G1 += now - G1_start; G1_start = now; G1_save = false; }
  if (G2_start) { data.timerun_G2 += now - G2_start; G2_start = now; G2_save = false; }
  if (G3_start) { data.timerun_G3 += now - G3_start; G3_start = now; G3_save = false; }
  if (B1_start) { data.timerun_B1 += now - B1_start; B1_start = now; B1_save = false; }
  if (B2_start) { data.timerun_B2 += now - B2_start; B2_start = now; B2_save = false; }
  if (B3_start) { data.timerun_B3 += now - B3_start; B3_start = now; B3_save = false; }
  savedata();
}

bool send_rualoc(const char *token, int virtualPin, float value) {
  char url[190];
  snprintf(url, sizeof(url), "%sbatch/update?token=%s&V%d=%.3f", SERVER_NAME, token, virtualPin, value);
  return httpGet(String(url));
}

bool sendVolumeCommand(const char *token, const String &command) {
  String url;
  url.reserve(190);
  url = SERVER_NAME;
  url += "batch/update?token=";
  url += token;
  url += "&V0=";
  url += urlEncode(command);
  return httpGet(url);
}

void hidden() { Blynk.setProperty(V6, V7, V8, V9, V3, V4, "isHidden", true); }
void visible() { Blynk.setProperty(V6, V7, V8, V9, V3, V4, "isHidden", false); }

void onG1() { startRelayPulse(pin_on_G1, 2500); }
void offG1() { startRelayPulse(pin_off_G1, 1000); }
void onG2() { startRelayPulse(pin_on_G2, 2500); }
void offG2() { startRelayPulse(pin_off_G2, 1000); }
void onG3() { startRelayPulse(pin_on_G3, 2500); }
void offG3() { startRelayPulse(pin_off_G3, 1000); }
void on_Bom1() { startRelayPulse(pin_on_Bom1, 300); }
void off_Bom1() { startRelayPulse(pin_off_Bom1, 300); }
void on_Bom2() { startRelayPulse(pin_on_Bom2, 300); }
void off_Bom2() { startRelayPulse(pin_off_Bom2, 300); }
void on_Bom3() { startRelayPulse(pin_on_Bom3, 300); }
void off_Bom3() { startRelayPulse(pin_off_Bom3, 300); }
void on_Bom4() { startRelayPulse(pin_on_Bom4, 300); }
void off_Bom4() { startRelayPulse(pin_off_Bom4, 300); }

void reset_RL() { send_rualoc(RUALOC_TOKEN, 1, 1); }
void on_nk1_RL() { send_rualoc(RUALOC_TOKEN, 1, 2); }
void off_nk1_RL() { send_rualoc(RUALOC_TOKEN, 1, 3); }
void on_nk2_RL() { send_rualoc(RUALOC_TOKEN, 1, 4); }
void off_nk2_RL() { send_rualoc(RUALOC_TOKEN, 1, 5); }
void reset_btn_RL() { send_rualoc(RUALOC_TOKEN, 1, 6); }

void logProtection(const String &message) {
  if (Blynk.connected()) Blynk.logEvent("error", message);
}

void processCurrent(float rms, float &current, uint8_t &runningCount, uint8_t &faultCount,
                    uint8_t minValue, uint8_t maxValue, bool &trip, uint8_t stopPin,
                    uint32_t &startedAt, uint32_t &runtime, bool &saveFlag, const char *name) {
  if (rms < 3) {
    current = 0;
    runningCount = 0;
    faultCount = 0;
    if (startedAt) {
      runtime += millis() - startedAt;
      startedAt = 0;
      savedata();
    }
    return;
  }
  current = rms;
  if (runningCount < 255) runningCount++;
  if (runningCount <= 3) return;
  if (!startedAt) startedAt = millis();
  else saveFlag = millis() - startedAt > 60000UL;
  const bool invalid = current > maxValue || current < minValue;
  if (invalid && data.keyp) {
    if (faultCount < 255) faultCount++;
    if (faultCount >= 3 && !trip) {
      trip = true;
      faultCount = 0;
      logProtection(String(name) + " lỗi: " + current + " A");
      startRelayPulse(stopPin, 15000);
    }
  } else {
    faultCount = 0;
  }
  if (trip && !pulseUntil[stopPin]) trip = false;
}

void selectMux(uint8_t channel) {
  digitalWrite(S0, channel & 1);
  digitalWrite(S1, channel & 2);
  digitalWrite(S2, channel & 4);
  digitalWrite(S3, channel & 8);
}

void readcurrent() {
  selectMux(0);
  processCurrent(emon0.calcIrms(740), Irms0, yIrms0, xSetAmpe, data.SetAmpemin, data.SetAmpemax,
                 trip0, pin_off_Bom1, B1_start, data.timerun_B1, B1_save, "Bơm 1");
}
void readcurrent1() {
  selectMux(1);
  processCurrent(emon1.calcIrms(740), Irms1, yIrms1, xSetAmpe1, data.SetAmpe1min, data.SetAmpe1max,
                 trip1, pin_off_Bom2, B2_start, data.timerun_B2, B2_save, "Bơm 2");
}
void readcurrent2() {
  selectMux(2);
  processCurrent(emon2.calcIrms(740), Irms2, yIrms2, xSetAmpe2, data.SetAmpe2min, data.SetAmpe2max,
                 trip2, pin_off_Bom3, B3_start, data.timerun_B3, B3_save, "Bơm 3");
}
void readcurrent3() {
  selectMux(3);
  processCurrent(emon3.calcIrms(740), Irms3, yIrms3, xSetAmpe3, data.SetAmpe3min, data.SetAmpe3max,
                 trip3, pin_off_G1, G1_start, data.timerun_G1, G1_save, "Giếng 1");
}
void readcurrent4() {
  selectMux(4);
  processCurrent(emon4.calcIrms(740), Irms4, yIrms4, xSetAmpe4, data.SetAmpe4min, data.SetAmpe4max,
                 trip4, pin_off_G2, G2_start, data.timerun_G2, G2_save, "Giếng 2");
}
void readcurrent5() {
  selectMux(5);
  processCurrent(emon5.calcIrms(740), Irms5, yIrms5, xSetAmpe5, data.SetAmpe5min, data.SetAmpe5max,
                 trip5, pin_off_G3, G3_start, data.timerun_G3, G3_save, "Giếng 3");
}

void sampleNextCurrent() {
  switch (currentChannel) {
  case 0: readcurrent(); break;
  case 1: readcurrent1(); break;
  case 2: readcurrent2(); break;
  case 3: readcurrent3(); break;
  case 4: readcurrent4(); break;
  default: readcurrent5(); break;
  }
  currentChannel++;
  if (currentChannel >= 6) {
    currentChannel = 0;
    if (completeCurrentScans < 255) completeCurrentScans++;
    lastFullCurrentScanAt = millis();
  }
}

void readPressure() {
  selectMux(14);
  const float result = ((static_cast<float>(analogRead(A0)) - 205.0f) * 6.0f) / (870.0f - 205.0f);
  if (result > 0) {
    value1 += result;
    Result1 = value1 / 16.0f;
    value1 -= Result1;
  }
}

void requestTemperature() {
  if (temperaturePending) return;
  sensors.requestTemperatures();
  temperaturePending = true;
  temperatureReadyAt = millis() + 800;
}

void serviceTemperature() {
  if (!temperaturePending || static_cast<int32_t>(millis() - temperatureReadyAt) < 0) return;
  temperaturePending = false;
  const float t1 = sensors.getTempC(temp);
  const float t2 = sensors.getTempC(temp2);
  const float t3 = sensors.getTempC(temp3);
  temp_1 = t1 > 0 && t1 < 125 ? t1 : 0;
  temp_2 = t2 > 0 && t2 < 125 ? t2 : 0;
  temp_3 = t3 > 0 && t3 < 125 ? t3 : 0;
}

bool isInsideSchedule(int nowSeconds) {
  if (data.start == data.stop) return false;
  if (data.start < data.stop) return nowSeconds > data.start && nowSeconds < data.stop;
  return nowSeconds > data.start || nowSeconds < data.stop;
}

void serviceAutoSchedule() {
  if (autoStage != AUTO_WAIT_B3 || static_cast<int32_t>(millis() - autoDeadline) < 0) return;
  autoStage = AUTO_IDLE;
  if (!localControlReady() || data.man != 1 || !scheduleWasInside || Irms2 < 3) return;
  if (Irms4 == 0 && !trip4) onG2();
  if (Irms5 == 0 && !trip5) onG3();
}

void rtctime() {
  if (!rtcAvailable) { rtcValid = false; return; }
  DateTime now = rtc_module.now();
  const bool blynkTimeValid = Blynk.connected() && year() >= 2024 && month() >= 1 && day() >= 1;
  bool clockChanged = false;
  if (blynkTimeValid) {
    DateTime cloudTime(year(), month(), day(), hour(), minute(), second());
    const int32_t difference = static_cast<int32_t>(now.unixtime() - cloudTime.unixtime());
    if (difference > 120 || difference < -120) {
      rtc_module.adjust(cloudTime);
      now = rtc_module.now();
      clockChanged = true;
    }
  }
  rtcValid = now.year() >= 2024 && now.year() <= 2099;
  if (!rtcValid) { scheduleInitialized = false; return; }
  timestamp = now.unixtime();
  if (lastRtcUnix) {
    const int32_t rtcElapsed = static_cast<int32_t>(timestamp - lastRtcUnix);
    const int32_t localElapsed = static_cast<int32_t>((millis() - lastRtcMillis) / 1000UL);
    const int32_t drift = rtcElapsed - localElapsed;
    if (drift > 120 || drift < -120) clockChanged = true;
  }
  lastRtcUnix = timestamp;
  lastRtcMillis = millis();
  if (clockChanged) scheduleInitialized = false;
  if (Blynk.connected())
    Blynk.virtualWrite(V1, daysOfTheWeek[now.dayOfTheWeek()], ", ", now.day(), "/", now.month(), "/",
                       now.year(), " - ", now.hour(), ":", now.minute(), ":", now.second());

  const bool inside = data.man == 1 && isInsideSchedule(now.hour() * 3600 + now.minute() * 60 + now.second());
  if (!scheduleInitialized) {
    scheduleWasInside = inside;
    scheduleInitialized = true;
    autoStage = AUTO_IDLE;
    return; // Never catch up a missed schedule after reset or clock correction.
  }
  if (inside && !scheduleWasInside && localControlReady() && !trip2 && Irms2 == 0) {
    if (on_Bom3(), pulseUntil[pin_on_Bom3]) {
      autoStage = AUTO_WAIT_B3;
      autoDeadline = millis() + 600000UL;
    }
  }
  if (!inside) autoStage = AUTO_IDLE;
  scheduleWasInside = inside;
}

void refreshRualoc() {
  if (data.rualoc && networkQuality > 0) send_rualoc(RUALOC_TOKEN, 2, data.rualoc);
}

BLYNK_WRITE(V2) // Rửa lọc
{
  if (key && networkQuality > 0) {
    switch (param.asInt()) {
    case 0: { // Tắt
      data.rualoc = 0;
      if ((data.LLG1_RL != 0) || (data.LLG2_RL != 0)) {
        if (data.LLG1_RL != 0) {
          Blynk.virtualWrite(V53, LLG1_1m3 - data.LLG1_RL);
          data.LLG1_RL = 0;
        }
        if (data.LLG2_RL != 0) {
          Blynk.virtualWrite(V56, LLG2_1m3 - data.LLG2_RL);
          data.LLG2_RL = 0;
        }
      }
      break;
    }
    case 1: { // RL 1
      data.rualoc = 1;
      if (data.LLG1_RL == 0) {
        data.LLG1_RL = LLG1_1m3;
      }
      break;
    }
    case 2: { // RL 2
      data.rualoc = 2;
      if (data.LLG2_RL == 0) {
        data.LLG2_RL = LLG2_1m3;
      }
      break;
    }
    case 3: { // RL 1+2
      data.rualoc = 3;
      if (data.LLG1_RL == 0) {
        data.LLG1_RL = LLG1_1m3;
      }
      if (data.LLG2_RL == 0) {
        data.LLG2_RL = LLG2_1m3;
      }
      break;
    }
    }
    savedata();
    send_rualoc(RUALOC_TOKEN, 2, data.rualoc);
  } else {
    Blynk.virtualWrite(V2, data.rualoc);
  }
}
BLYNK_WRITE(V3) // auto/man cap 1
{
  if (key) {
    if (param.asInt() == HIGH) {
      data.man = 1;
      scheduleInitialized = false;
    } else {
      data.man = 0;
      scheduleInitialized = false;
      autoStage = AUTO_IDLE;
    }
    savedata();
  } else
    Blynk.virtualWrite(V3, data.man);
}
BLYNK_WRITE(V4) // On/off chuc nang bao ve
{
  if (key) {
    if (param.asInt() == LOW) {
      data.keyp = 0;
    } else {
      data.keyp = 1;
    }
    savedata();
  } else {
    Blynk.virtualWrite(V4, data.keyp);
  }
}
BLYNK_WRITE(V5) // data string
{
  String dataS = param.asStr();
  if (dataS == "cc" || dataS == "CC") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Người vận hành: 'NM Cái Cát'\nMain kích hoạt trong 10s");
    key = true;
    keyExpiresAt = millis() + 10000UL;
  } else if (dataS == "active") {
    keyterminal.clear();
    visible();
    key = true;
    keyExpiresAt = millis() + 300000UL;
    Blynk.virtualWrite(V5, "Kích hoạt chế độ sửa lỗi trong 5 phút!");
  } else if (dataS == "deactive") {
    keyterminal.clear();
    hidden();
    key = false;
    keyExpiresAt = 0;
    Blynk.virtualWrite(V5, "Đã hủy!\n");
  } else if (dataS == "save") {
    keyterminal.clear();
    savedata();
    Blynk.virtualWrite(V5, "Đã lưu!\n");
  } else if (dataS == "reset") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Đã reset!");
    trip0 = trip1 = trip2 = trip3 = trip4 = trip5 = false;
    releaseAllRelays(); // Explicit reset command keeps the existing all-HIGH behavior.
    if (networkQuality > 0) reset_RL();
  } else if (dataS == "save_num") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Số lần ghi EEPROM: ", data.save_num);
  } else if (dataS == "rst") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "ESP Khởi động lại sau 3s");
    ESP.restart();
  } else if (dataS == "update") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Đã nhận lệnh OTA sạch; ESP sẽ khởi động lại.");
    if (!CleanOta::requestAndRestart()) {
      Blynk.virtualWrite(V5, "OTA lỗi: không ghi được yêu cầu vào RTC memory.");
    }
  } else if ((dataS == "ok") || (dataS == "Ok") || (dataS == "OK") || (dataS == "oK")) {
    if (clo_cache > 0) {
      data.clo = clo_cache;
      clo_cache = 0;
      data.time_clo = timestamp;
      Blynk.virtualWrite(V24, data.clo);
      savedata();
      keyterminal.clear();
      Blynk.virtualWrite(V5, "Đã lưu - CLO:", data.clo, "kg");
    }
  } else if (dataS == "update_RL") {
    keyterminal.clear();
    if (networkQuality > 0) {
      send_rualoc(RUALOC_TOKEN, 1, 7);
      Blynk.virtualWrite(V5, "UPDATE FIRMWARE...");
    } else {
      Blynk.virtualWrite(V5, "Lệnh bị khóa vì chất lượng mạng không đảm bảo.");
    }
  } else if (dataS == "code") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "cc-active-deactive-save-reset-save_num-rst-update-ok-update_RL");
  } else if (dataS == "clr") {
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Đã xoá dữ liệu!\n");
  } else {
    Blynk.virtualWrite(V5, "Mã không hợp lệ!\nVui lòng nhập lại.\n");
  }
}
BLYNK_WRITE(V6) // Chon máy cài đặt bảo vệ
{
  switch (param.asInt()) {
  case 0: {
    z = 0;
    Blynk.virtualWrite(V7, 0);
    Blynk.virtualWrite(V8, 0);
    break;
  }
  case 1: { // Bom 1
    z = 1;
    Blynk.virtualWrite(V7, data.SetAmpemin);
    Blynk.virtualWrite(V8, data.SetAmpemax);
    break;
  }
  case 2: { // Bom 2
    z = 2;
    Blynk.virtualWrite(V7, data.SetAmpe1min);
    Blynk.virtualWrite(V8, data.SetAmpe1max);
    break;
  }
  case 3: { // Bom 3
    z = 3;
    Blynk.virtualWrite(V7, data.SetAmpe2min);
    Blynk.virtualWrite(V8, data.SetAmpe2max);
    break;
  }
  case 4: { // G1
    z = 4;
    Blynk.virtualWrite(V7, data.SetAmpe3min);
    Blynk.virtualWrite(V8, data.SetAmpe3max);
    break;
  }
  case 5: { // G2
    z = 5;
    Blynk.virtualWrite(V7, data.SetAmpe4min);
    Blynk.virtualWrite(V8, data.SetAmpe4max);
    break;
  }
  case 6: { // G3
    z = 6;
    Blynk.virtualWrite(V7, data.SetAmpe5min);
    Blynk.virtualWrite(V8, data.SetAmpe5max);
    break;
  }
  }
}
BLYNK_WRITE(V7) // min
{
  if (key) {
    if (z == 1) {
      data.SetAmpemin = param.asInt();
    } else if (z == 2) {
      data.SetAmpe1min = param.asInt();
    } else if (z == 3) {
      data.SetAmpe2min = param.asInt();
    } else if (z == 4) {
      data.SetAmpe3min = param.asInt();
    } else if (z == 5) {
      data.SetAmpe4min = param.asInt();
    } else if (z == 6) {
      data.SetAmpe5min = param.asInt();
    }
  } else {
    Blynk.virtualWrite(V7, 0);
  }
}
BLYNK_WRITE(V8) // max
{
  if (key) {
    if (z == 1) {
      data.SetAmpemax = param.asInt();
    } else if (z == 2) {
      data.SetAmpe1max = param.asInt();
    } else if (z == 3) {
      data.SetAmpe2max = param.asInt();
    } else if (z == 4) {
      data.SetAmpe3max = param.asInt();
    } else if (z == 5) {
      data.SetAmpe4max = param.asInt();
    } else if (z == 6) {
      data.SetAmpe5max = param.asInt();
    }
  } else {
    Blynk.virtualWrite(V8, 0);
  }
}
BLYNK_WRITE(V9) // time input
{
  if (key) {
    TimeInputParam t(param);
    if (t.hasStartTime()) {
      data.start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
    }
    if (t.hasStopTime()) {
      data.stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      scheduleInitialized = false;
    }
    savedata();
  } else {
    Blynk.virtualWrite(V9, data.start, data.stop, tz);
  }
}
BLYNK_WRITE(V10) // On Bơm 1
{
  if (param.asInt() != HIGH) return;
  if (remoteControlReady(V10)) on_Bom1();
  else rejectRemoteCommand(V10);
}
BLYNK_WRITE(V11) // Off Bơm 1
{
  if (param.asInt() != HIGH) return;
  if (remoteControlReady(V11)) off_Bom1();
  else rejectRemoteCommand(V11);
}
BLYNK_WRITE(V12) // On Bơm 2
{
  if (param.asInt() != HIGH) return;
  if (remoteControlReady(V12)) on_Bom2();
  else rejectRemoteCommand(V12);
}
BLYNK_WRITE(V13) // Off Bơm 2
{
  if (param.asInt() != HIGH) return;
  if (remoteControlReady(V13)) off_Bom2();
  else rejectRemoteCommand(V13);
}
BLYNK_WRITE(V14) // On Bơm 3
{
  if (param.asInt() != HIGH) return;
  if (remoteControlReady(V14)) on_Bom3();
  else rejectRemoteCommand(V14);
}
BLYNK_WRITE(V15) // Off Bơm 3
{
  if (param.asInt() != HIGH) return;
  if (remoteControlReady(V15)) off_Bom3();
  else rejectRemoteCommand(V15);
}
BLYNK_WRITE(V16) // On Gieng 1
{
  if (param.asInt() != HIGH) return;
  if (remoteControlReady(V16)) onG1();
  else rejectRemoteCommand(V16);
}
BLYNK_WRITE(V17) // Off Gieng 1
{
  if (param.asInt() != HIGH) return;
  if (remoteControlReady(V17)) offG1();
  else rejectRemoteCommand(V17);
}
BLYNK_WRITE(V18) // On Gieng 2
{
  if (param.asInt() != HIGH) return;
  if (remoteControlReady(V18)) onG2();
  else rejectRemoteCommand(V18);
}
BLYNK_WRITE(V19) // Off Gieng 2
{
  if (param.asInt() != HIGH) return;
  if (remoteControlReady(V19)) offG2();
  else rejectRemoteCommand(V19);
}
BLYNK_WRITE(V20) // On Gieng 3
{
  if (param.asInt() != HIGH) return;
  if (remoteControlReady(V20)) onG3();
  else rejectRemoteCommand(V20);
}
BLYNK_WRITE(V21) // Off Gieng 3
{
  if (param.asInt() != HIGH) return;
  if (remoteControlReady(V21)) offG3();
  else rejectRemoteCommand(V21);
}
BLYNK_WRITE(V26) // The tich
{
  if (param.asFloat() >= 0) {
    conlai = param.asFloat();
    thetich = (dai * conlai * rong) / 1000000;
  }
}
BLYNK_WRITE(V40) // Nén Khí
{
  if (param.asInt() == 1) {
    if (localControlReady() && networkQuality > 0)
      writeRelay(pin_NK1, LOW);
    else rejectRemoteCommand(V40);
  }
}
BLYNK_WRITE(V43) // BTN NK1
{
  if (key && networkQuality > 0) {
    if (param.asInt() == HIGH) {
      on_nk1_RL();
    } else
      off_nk1_RL();
  } else if (networkQuality > 0)
    reset_btn_RL();
  else
    Blynk.virtualWrite(V43, 0);
}
BLYNK_WRITE(V44) // BTN NK2
{
  if (key && networkQuality > 0) {
    if (param.asInt() == HIGH) {
      on_nk2_RL();
    } else
      off_nk2_RL();
  } else if (networkQuality > 0)
    reset_btn_RL();
  else
    Blynk.virtualWrite(V44, 0);
}
BLYNK_WRITE(V45) // Teminal_volume
{
  String dataS = param.asStr();
  if ((dataS == "rst_G1") || (dataS == "update_G1") || (dataS == "rst_vl_G1") || (dataS == "i2c_G1")) {
    terminal_volume.clear();
    if (networkQuality > 0) sendVolumeCommand(VOLUME_TOKEN_G1, dataS);
    else Blynk.virtualWrite(V45, "Lệnh bị khóa vì chất lượng mạng không đảm bảo.");
  } else if ((dataS == "rst_G2") || (dataS == "update_G2") || (dataS == "rst_vl_G2") || (dataS == "i2c_G2")) {
    terminal_volume.clear();
    if (networkQuality > 0) sendVolumeCommand(VOLUME_TOKEN_G2, dataS);
    else Blynk.virtualWrite(V45, "Lệnh bị khóa vì chất lượng mạng không đảm bảo.");
  } else if ((dataS == "rst_G3") || (dataS == "update_G3") || (dataS == "rst_vl_G3") || (dataS == "i2c_G3")) {
    terminal_volume.clear();
    if (networkQuality > 0) sendVolumeCommand(VOLUME_TOKEN_G3, dataS);
    else Blynk.virtualWrite(V45, "Lệnh bị khóa vì chất lượng mạng không đảm bảo.");
  } else if (dataS == "code") {
    terminal_volume.clear();
    Blynk.virtualWrite(V45, "rst_Gn - update_Gn - rst_vl_Gn - i2c_Gn");
  } else if (dataS == "clr") {
    terminal_volume.clear();
    Blynk.virtualWrite(V45, "Terminal đã được xóa!");
  }
}
//----------------------------------------------------
BLYNK_WRITE(V49) // Clo Input
{
  if (param.asFloat() > 0) {
    keyterminal.clear();
    clo_cache = param.asFloat();
    Blynk.virtualWrite(V5, " Lượng CLO châm hôm nay:", clo_cache, "kg\n Vui lòng kiểm tra kỹ, nếu đúng hãy nhập 'OK' để lưu");
  }
}
BLYNK_WRITE(V50) // Check
{
  if (param.asInt() == 1) {
    DateTime dt(data.time_clo);
    keyterminal.clear();
    Blynk.virtualWrite(V5, "Châm CLO:", data.clo, "kg vào lúc", dt.hour(), ":", dt.minute(), "-", dt.day(), "/", dt.month(), "/", dt.year());
  }
}
BLYNK_WRITE(V51) // LLG1_1m3
{
  LLG1_1m3 = param.asInt();
}
BLYNK_WRITE(V54) // LLG2_1m3
{
  LLG2_1m3 = param.asInt();
}
BLYNK_WRITE(V57) // LLG3_1m3
{
  LLG3_1m3 = param.asInt();
}
//----------------------------------------------------

void setup() {
  Serial.begin(115200);
  if (CleanOta::handleBoot(ssid, password, URL_fw_Bin)) return;

  ESP.wdtDisable();
  ESP.wdtEnable(8000);
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  initializeRelays();
  eeprom.initialize();
  loadData();
  rtcAvailable = rtc_module.begin();

  sensors.begin();
  sensors.setWaitForConversion(false);
  requestTemperature();

  emon0.current(A0, 106);
  emon1.current(A0, 106);
  emon2.current(A0, 106);
  emon3.current(A0, 106);
  emon4.current(A0, 106);
  emon5.current(A0, 106);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);

  timer.setInterval(150L, sampleNextCurrent);
  timer.setInterval(303L, readPressure);
  timer.setInterval(250L, sendTelemetryStep);
  timer.setInterval(10000L, requestTemperature);
  timer.setInterval(15091L, []() { rtctime(); time_run_motor(); });
  timer.setInterval(15000L, probeNetwork);
  timer.setInterval(30000L, connectionstatus);
  timer.setInterval(60000L, refreshRualoc);
}

void loop() {
  ESP.wdtFeed();
  Blynk.run();
  timer.run();
  serviceRelayPulses();
  serviceTemperature();
  serviceAutoSchedule();
  if (key && keyExpiresAt && static_cast<int32_t>(millis() - keyExpiresAt) >= 0) {
    key = false;
    keyExpiresAt = 0;
    if (Blynk.connected()) keyterminal.clear();
  }
}
