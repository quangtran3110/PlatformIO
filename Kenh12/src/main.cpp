/*
#define BLYNK_TEMPLATE_ID "TMPL6PNR4qqpm"
#define BLYNK_TEMPLATE_NAME "Multi"
#define BLYNK_AUTH_TOKEN "LtMwtUXWi8mAX47_K17_z5rr8C3HNDjk"
*/
#define BLYNK_TEMPLATE_ID "TMPL6BUQS2c4x"
#define BLYNK_TEMPLATE_NAME "Áp lực tuyến"
#define BLYNK_AUTH_TOKEN "sVtMTLTQgaRjQTl31V7Qewtdv2KVs9ST"

#define BLYNK_FIRMWARE_VERSION "260818"
#define BLYNK_PRINT Serial
#define APP_DEBUG

#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <SimpleKalmanFilter.h>
#include <Wire.h>
#include <stddef.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
WiFiClient client;
HTTPClient http;
String server_name = "http://sgp1.blynk.cloud/external/api/";
String TanLap1 = "91thFYXxhfcs2ij5GDVg6NTjEgMqFBwi";
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/Kenh12/.pio/build/nodemcuv2/firmware.bin"

#include "PCF8575.h"
PCF8575 pcf8575_1(0x20);
const int S0pin = P3;
const int S1pin = P2;
const int S2pin = P1;
const int S3pin = P0;
const int XKC_in = P4;
const int pin_WATCHDOG = P8;

#define XKC_LOTTIE_VPIN V1
const char XKC_WATER_ANIMATION_URL[] =
    "https://cdn.jsdelivr.net/gh/quangtran3110/blynk-lottie-assets@main/Water%20Animation.json";
const char XKC_EMPTY_ANIMATION_URL[] =
    "https://cdn.jsdelivr.net/gh/quangtran3110/blynk-lottie-assets@main/WaterEmptyAnimation.json";
const unsigned long XKC_READ_INTERVAL_MS = 100UL;
const unsigned long XKC_DEBOUNCE_MS = 500UL;

OneWire oneWire(D5); // Chân D5 là chân dữ liệu của cảm biến DS18B20
DallasTemperature sensors(&oneWire);

// const char *ssid = "net";
// const char *password = "Password";
const char *ssid = "tram bom so 4";
const char *password = "0943950555";

float Result1 = 0.0f;
float temp[1], nhietdo;
float sensorValue = 0.0f;
byte reboot_num;
bool p = true, key_pluse = true;
int save_num;
unsigned long watchdogLastToggleMs = 0;
const unsigned long WATCHDOG_TOGGLE_INTERVAL_MS = 5000UL;
uint8_t watchdogOutputLevel = LOW;
int time1, time2, time3;
bool pcf8575Ready = false;
uint8_t xkcRawLevel = HIGH;
bool xkcCandidateHasWater = false;
bool xkcStableHasWater = false;
bool xkcCandidateKnown = false;
bool xkcStableStateKnown = false;
bool xkcWidgetNeedsUpdate = true;
unsigned long xkcCandidateChangedMs = 0;

// Chuoi xu ly ap suat: Median 5 mau -> Kalman -> noi suy hieu chuan da diem.
const uint8_t PRESSURE_MEDIAN_WINDOW_SIZE = 5;
int pressureMedianBuffer[PRESSURE_MEDIAN_WINDOW_SIZE] = {0};
uint8_t pressureMedianIndex = 0;
uint8_t pressureMedianCount = 0;
float filteredAdcPressure = 0.0f;
bool pressureFilterReady = false;
// Sai so uoc luong ban dau lon de bo loc bam nhanh vao mau ADC dau tien.
SimpleKalmanFilter pressureKalmanFilter(1.0f, 1000.0f, 0.01f);

const uint8_t MAX_PRESSURE_CALIB_POINTS = 5;
const uint16_t EEPROM_SIZE_BYTES = 512;
const uint16_t PRESSURE_CALIB_EEPROM_ADDRESS = 256;
const uint32_t PRESSURE_CALIB_MAGIC = 0x4B313250UL; // "K12P"
const uint8_t PRESSURE_CALIB_VERSION = 1;

struct PressureCalibPoint {
  uint16_t adc;
  uint16_t value; // Ap suat x 100 (bar).
};

struct PressureCalibStorage {
  uint32_t magic;
  uint8_t version;
  uint8_t numPoints;
  PressureCalibPoint points[MAX_PRESSURE_CALIB_POINTS];
  uint32_t checksum;
};

PressureCalibPoint pressureCalibPoints[MAX_PRESSURE_CALIB_POINTS];
uint8_t numPressureCalibPoints = 0;

static_assert(PRESSURE_CALIB_EEPROM_ADDRESS + sizeof(PressureCalibStorage) <= EEPROM_SIZE_BYTES,
              "Pressure calibration data exceeds EEPROM area");

BlynkTimer timer;

WidgetTerminal terminal(V0);
BLYNK_CONNECTED() {
  // Thuoc tinh widget can duoc gui lai sau moi lan thiet bi ket noi Blynk.
  xkcWidgetNeedsUpdate = true;
}
//-------------------------
void connectionstatus() {
  if ((WiFi.status() != WL_CONNECTED)) {
    Serial.println("Khong ket noi WIFI");
    WiFi.begin(ssid, password);
    reboot_num = reboot_num + 1;
    if (reboot_num % 5) {
      key_pluse = false;
    }
  }
  if ((WiFi.status() == WL_CONNECTED) && (!Blynk.connected())) {
    reboot_num = reboot_num + 1;
    if ((reboot_num == 1) || (reboot_num == 2)) {
      Serial.println("...");
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(ssid, password);
    }
    if (reboot_num % 5 == 0) {
      ESP.restart();
    }
  }
  if (Blynk.connected()) {
    key_pluse = true;
    if (reboot_num != 0) {
      reboot_num = 0;
    }
  }
}
void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}
void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}
void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}
void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}
void update_fw() {
  WiFiClientSecure client_;
  client_.setInsecure();
  Serial.print("Wait...");
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client_, URL_fw_Bin);
  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;
  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}
//-------------------------
void serviceExternalWatchdog() {
  unsigned long now = millis();
  if ((unsigned long)(now - watchdogLastToggleMs) < WATCHDOG_TOGGLE_INTERVAL_MS)
    return;

  watchdogLastToggleMs = now;
  uint8_t nextLevel = (watchdogOutputLevel == LOW) ? HIGH : LOW;
  if (pcf8575_1.digitalWrite(pin_WATCHDOG, nextLevel)) {
    watchdogOutputLevel = nextLevel;
  } else {
    Serial.println("Khong gui duoc heartbeat den watchdog");
  }
}
//-------------------------
void updateXkcLottieWidget() {
  if (!xkcStableStateKnown || !Blynk.connected()) {
    xkcWidgetNeedsUpdate = true;
    return;
  }

  const char *animationUrl =
      xkcStableHasWater ? XKC_WATER_ANIMATION_URL : XKC_EMPTY_ANIMATION_URL;
  Blynk.setProperty(XKC_LOTTIE_VPIN, "url", animationUrl);
  //Blynk.setProperty(XKC_LOTTIE_VPIN, "autoplay", "true");
  //Blynk.setProperty(XKC_LOTTIE_VPIN, "loop", "true");
  Blynk.virtualWrite(XKC_LOTTIE_VPIN, "play");
  xkcWidgetNeedsUpdate = false;
}

void readXkcSensor() {
  if (!pcf8575Ready)
    return;

  xkcRawLevel = pcf8575_1.digitalRead(XKC_in, true);
  // Y26-NPN de chan MODE ho: co nuoc thi OUT keo xuong LOW.
  bool hasWaterNow = (xkcRawLevel == LOW);
  unsigned long now = millis();

  if (!xkcCandidateKnown) {
    xkcCandidateHasWater = hasWaterNow;
    xkcCandidateChangedMs = now;
    xkcCandidateKnown = true;
    return;
  }

  if (hasWaterNow != xkcCandidateHasWater) {
    xkcCandidateHasWater = hasWaterNow;
    xkcCandidateChangedMs = now;
    return;
  }

  if ((!xkcStableStateKnown || xkcStableHasWater != xkcCandidateHasWater) &&
      (unsigned long)(now - xkcCandidateChangedMs) >= XKC_DEBOUNCE_MS) {
    xkcStableHasWater = xkcCandidateHasWater;
    xkcStableStateKnown = true;
    xkcWidgetNeedsUpdate = true;
    //Serial.printf("XKC: %s\n", xkcStableHasWater ? "CO NUOC" : "KHONG CO NUOC");
  }

  if (xkcWidgetNeedsUpdate)
    updateXkcLottieWidget();
}

//-------------------------
void send_data() {
}

uint32_t pressureCalibChecksum(const PressureCalibStorage &storage) {
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&storage);
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < offsetof(PressureCalibStorage, checksum); i++) {
    hash ^= bytes[i];
    hash *= 16777619UL;
  }
  return hash;
}

void sortPressureCalibPoints() {
  for (uint8_t i = 1; i < numPressureCalibPoints; i++) {
    PressureCalibPoint key = pressureCalibPoints[i];
    int8_t j = i - 1;
    while (j >= 0 && pressureCalibPoints[j].adc > key.adc) {
      pressureCalibPoints[j + 1] = pressureCalibPoints[j];
      j--;
    }
    pressureCalibPoints[j + 1] = key;
  }
}

void setDefaultPressureCalibration() {
  // Giu gan nguyen dac tuyen cu: ADC 197.5..1005 tuong ung 0..5 bar.
  pressureCalibPoints[0] = {198, 0};
  pressureCalibPoints[1] = {1005, 500};
  numPressureCalibPoints = 2;
}

bool isPressureCalibStorageValid(const PressureCalibStorage &storage) {
  if (storage.magic != PRESSURE_CALIB_MAGIC ||
      storage.version != PRESSURE_CALIB_VERSION ||
      storage.numPoints < 2 ||
      storage.numPoints > MAX_PRESSURE_CALIB_POINTS ||
      storage.checksum != pressureCalibChecksum(storage)) {
    return false;
  }

  for (uint8_t i = 0; i < storage.numPoints; i++) {
    if (storage.points[i].adc > 1023)
      return false;
    if (i > 0 && storage.points[i].adc <= storage.points[i - 1].adc)
      return false;
  }
  return true;
}

void loadPressureCalibration() {
  PressureCalibStorage storage = {};
  EEPROM.begin(EEPROM_SIZE_BYTES);
  EEPROM.get(PRESSURE_CALIB_EEPROM_ADDRESS, storage);
  EEPROM.end();

  if (!isPressureCalibStorageValid(storage)) {
    setDefaultPressureCalibration();
    Serial.println("Chua co calib ap suat hop le, dung mac dinh 0..5 bar");
    return;
  }

  numPressureCalibPoints = storage.numPoints;
  memcpy(pressureCalibPoints, storage.points,
         numPressureCalibPoints * sizeof(PressureCalibPoint));
  Serial.printf("Da nap %u diem calib ap suat tu EEPROM\n", numPressureCalibPoints);
}

bool savePressureCalibration() {
  PressureCalibStorage storage = {};
  storage.magic = PRESSURE_CALIB_MAGIC;
  storage.version = PRESSURE_CALIB_VERSION;
  storage.numPoints = numPressureCalibPoints;
  memcpy(storage.points, pressureCalibPoints,
         numPressureCalibPoints * sizeof(PressureCalibPoint));
  storage.checksum = pressureCalibChecksum(storage);

  EEPROM.begin(EEPROM_SIZE_BYTES);
  EEPROM.put(PRESSURE_CALIB_EEPROM_ADDRESS, storage);
  bool saved = EEPROM.commit();
  EEPROM.end();
  return saved;
}

float interpolatePressure(float currentAdc) {
  if (numPressureCalibPoints == 0)
    return 0.0f;
  if (numPressureCalibPoints == 1)
    return pressureCalibPoints[0].value / 100.0f;

  const PressureCalibPoint *p1;
  const PressureCalibPoint *p2;
  if (currentAdc <= pressureCalibPoints[0].adc) {
    p1 = &pressureCalibPoints[0];
    p2 = &pressureCalibPoints[1];
  } else if (currentAdc >= pressureCalibPoints[numPressureCalibPoints - 1].adc) {
    p1 = &pressureCalibPoints[numPressureCalibPoints - 2];
    p2 = &pressureCalibPoints[numPressureCalibPoints - 1];
  } else {
    uint8_t i = 0;
    while (i < numPressureCalibPoints - 1 &&
           currentAdc > pressureCalibPoints[i + 1].adc) {
      i++;
    }
    p1 = &pressureCalibPoints[i];
    p2 = &pressureCalibPoints[i + 1];
  }

  float x1 = p1->adc;
  float x2 = p2->adc;
  float y1 = p1->value;
  float y2 = p2->value;
  if (x2 == x1)
    return y1 / 100.0f;
  return (y1 + (currentAdc - x1) * (y2 - y1) / (x2 - x1)) / 100.0f;
}

int getPressureMedian(const int values[], uint8_t size) {
  int sorted[PRESSURE_MEDIAN_WINDOW_SIZE];
  memcpy(sorted, values, size * sizeof(int));
  for (uint8_t i = 1; i < size; i++) {
    int key = sorted[i];
    int8_t j = i - 1;
    while (j >= 0 && sorted[j] > key) {
      sorted[j + 1] = sorted[j];
      j--;
    }
    sorted[j + 1] = key;
  }
  return sorted[size / 2];
}

bool isNonNegativeNumber(const String &text, bool allowDecimal) {
  if (text.length() == 0)
    return false;

  bool hasDigit = false;
  bool hasDecimal = false;
  for (uint16_t i = 0; i < text.length(); i++) {
    char ch = text.charAt(i);
    if (ch >= '0' && ch <= '9') {
      hasDigit = true;
    } else if (allowDecimal && ch == '.' && !hasDecimal) {
      hasDecimal = true;
    } else {
      return false;
    }
  }
  return hasDigit;
}

void addOrUpdatePressureCalibPoint(const PressureCalibPoint &newPoint) {
  // Cung mot ADC thi cap nhat diem cu de tranh hai moc co cung hoanh do.
  for (uint8_t i = 0; i < numPressureCalibPoints; i++) {
    if (pressureCalibPoints[i].adc == newPoint.adc) {
      pressureCalibPoints[i] = newPoint;
      sortPressureCalibPoints();
      return;
    }
  }

  if (numPressureCalibPoints < MAX_PRESSURE_CALIB_POINTS) {
    pressureCalibPoints[numPressureCalibPoints++] = newPoint;
  } else {
    uint8_t closestIndex = 0;
    uint16_t minDifference = 65535;
    for (uint8_t i = 0; i < numPressureCalibPoints; i++) {
      uint16_t difference = abs((int)pressureCalibPoints[i].value -
                                (int)newPoint.value);
      if (difference < minDifference) {
        minDifference = difference;
        closestIndex = i;
      }
    }
    pressureCalibPoints[closestIndex] = newPoint;
  }
  sortPressureCalibPoints();
}

void printPressureCalibration() {
  terminal.clear();
  Blynk.virtualWrite(V0, "--- CALIB AP SUAT KENH 12 ---\n");
  char line[80];
  snprintf(line, sizeof(line), "Points: %u/%u\n", numPressureCalibPoints,
           MAX_PRESSURE_CALIB_POINTS);
  Blynk.virtualWrite(V0, line);
  for (uint8_t i = 0; i < numPressureCalibPoints; i++) {
    snprintf(line, sizeof(line), "#%u: ADC=%u -> %.2f bar\n", i + 1,
             pressureCalibPoints[i].adc,
             pressureCalibPoints[i].value / 100.0f);
    Blynk.virtualWrite(V0, line);
  }
  snprintf(line, sizeof(line), "ADC raw=%.0f, filtered=%.2f -> %.2f bar\n",
           sensorValue, filteredAdcPressure, Result1);
  Blynk.virtualWrite(V0, line);
}

void savedata() {
  save_num = save_num + 1;
  EEPROM.begin(EEPROM_SIZE_BYTES);
  delay(10);
  EEPROM.put(156, reboot_num);
  EEPROM.put(160, save_num);
  EEPROM.commit();
  EEPROM.end();
}

void updata() {
}
void tem() {
  sensors.requestTemperatures();
  // Serial.println(sensors.getDeviceCount());
  for (byte i = 0; i < sensors.getDeviceCount(); i++) {
    temp[i] = sensors.getTempCByIndex(i);
    nhietdo = temp[i];
    // Blynk.virtualWrite(V36, temp[i]);
  }
  if (temp[0] > 42) {
    Blynk.logEvent("error-5", String("Nhiệt độ tủ cao: ") + temp[0] + String("°C"));
  }
}

void pressure() {
  // Kenh C0 cua CD74HC4067: S0=S1=S2=S3=LOW.
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  delayMicroseconds(200);

  // Bo mau dau sau khi chuyen MUX de tu lay mau ADC nap lai theo kenh C0.
  analogRead(A0);
  delayMicroseconds(50);
  int rawAdc = analogRead(A0);
  sensorValue = rawAdc;

  pressureMedianBuffer[pressureMedianIndex] = rawAdc;
  pressureMedianIndex = (pressureMedianIndex + 1) % PRESSURE_MEDIAN_WINDOW_SIZE;
  if (pressureMedianCount < PRESSURE_MEDIAN_WINDOW_SIZE)
    pressureMedianCount++;

  int medianAdc = getPressureMedian(pressureMedianBuffer, pressureMedianCount);
  filteredAdcPressure = pressureKalmanFilter.updateEstimate(medianAdc);
  Result1 = interpolatePressure(filteredAdcPressure);
  pressureFilterReady = true;
}

BLYNK_WRITE(V0) {
  String dataS = param.asStr();
  dataS.trim();
  if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V0, "ESP UPDATE...");
    update_fw();
  } else if (dataS == "help") {
    terminal.clear();
    Blynk.virtualWrite(V0,
                       "pre_X.X    : Them diem calib ap suat\n"
                       "pre_N_X.X  : Thay diem calib thu N\n"
                       "calib_pre  : Xem cac diem calib\n"
                       "pre_clear  : Khoi phuc calib mac dinh\n"
                       "update     : Cap nhat firmware\n");
  } else if (dataS == "calib_pre") {
    printPressureCalibration();
  } else if (dataS == "pre_clear") {
    setDefaultPressureCalibration();
    bool saved = savePressureCalibration();
    Blynk.virtualWrite(V0, saved ? "Da khoi phuc calib mac dinh 0..5 bar.\n"
                                 : "Loi ghi EEPROM, calib chi co hieu luc den khi khoi dong lai.\n");
  } else if (dataS.startsWith("pre_") && dataS.indexOf('_', 4) >= 0) {
    int separator = dataS.indexOf('_', 4);
    String pointText = dataS.substring(4, separator);
    String valueText = dataS.substring(separator + 1);

    if (!isNonNegativeNumber(pointText, false) ||
        !isNonNegativeNumber(valueText, true)) {
      Blynk.virtualWrite(V0, "Sai dinh dang. Dung pre_N_X.X, vi du pre_3_2.5.\n");
      return;
    }

    int pointNumber = pointText.toInt();
    float pressureValue = valueText.toFloat();
    if (pointNumber < 1 || pointNumber > numPressureCalibPoints) {
      Blynk.virtualWrite(V0, "Diem khong ton tai. Dung calib_pre de xem danh sach.\n");
    } else if (pressureValue > 655.35f) {
      Blynk.virtualWrite(V0, "Gia tri ap suat vuot gioi han luu tru.\n");
    } else if (!pressureFilterReady) {
      Blynk.virtualWrite(V0, "Bo loc chua co mau ADC. Hay cho vai giay roi thu lai.\n");
    } else {
      PressureCalibPoint newPoint = {
          (uint16_t)round(filteredAdcPressure),
          (uint16_t)round(pressureValue * 100.0f)};
      bool duplicateAdc = false;
      for (uint8_t i = 0; i < numPressureCalibPoints; i++) {
        if (i != (uint8_t)(pointNumber - 1) &&
            pressureCalibPoints[i].adc == newPoint.adc) {
          duplicateAdc = true;
          break;
        }
      }
      if (duplicateAdc) {
        Blynk.virtualWrite(V0, "ADC nay da thuoc diem calib khac. Hay thay doi ap suat roi thu lai.\n");
      } else {
        pressureCalibPoints[pointNumber - 1] = newPoint;
        sortPressureCalibPoints();
        bool saved = savePressureCalibration();
        Blynk.virtualWrite(V0, saved ? "Da thay va luu diem calib ap suat. Chay lai calib_pre.\n"
                                     : "Da thay trong RAM nhung loi ghi EEPROM.\n");
      }
    }
  } else if (dataS.startsWith("pre_")) {
    String valueText = dataS.substring(4);
    if (!isNonNegativeNumber(valueText, true)) {
      Blynk.virtualWrite(V0, "Sai dinh dang. Dung pre_X.X, vi du pre_2.5.\n");
      return;
    }

    float pressureValue = valueText.toFloat();
    if (pressureValue > 655.35f) {
      Blynk.virtualWrite(V0, "Gia tri ap suat vuot gioi han luu tru.\n");
    } else if (!pressureFilterReady) {
      Blynk.virtualWrite(V0, "Bo loc chua co mau ADC. Hay cho vai giay roi thu lai.\n");
    } else {
      PressureCalibPoint newPoint = {
          (uint16_t)round(filteredAdcPressure),
          (uint16_t)round(pressureValue * 100.0f)};
      addOrUpdatePressureCalibPoint(newPoint);
      bool saved = savePressureCalibration();
      Blynk.virtualWrite(V0, saved ? "Da them/cap nhat diem calib ap suat.\n"
                                   : "Da cap nhat trong RAM nhung loi ghi EEPROM.\n");
    }
  }
}

//-------------------------
void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(300000);
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(5000);

  Wire.begin();
  sensors.begin();

  // Khoi dong MUX ngay tai C0 de ADC khong doc nham kenh C15.
  pcf8575_1.pinMode(S0pin, OUTPUT, LOW);
  pcf8575_1.pinMode(S1pin, OUTPUT, LOW);
  pcf8575_1.pinMode(S2pin, OUTPUT, LOW);
  pcf8575_1.pinMode(S3pin, OUTPUT, LOW);
  pcf8575_1.pinMode(XKC_in, INPUT_PULLUP);
  pcf8575_1.pinMode(pin_WATCHDOG, OUTPUT, LOW);

  pcf8575Ready = pcf8575_1.begin();
  if (!pcf8575Ready) {
    Serial.println("Khong tim thay PCF8575");
  }

  EEPROM.begin(EEPROM_SIZE_BYTES);
  delay(10);
  EEPROM.get(156, reboot_num);
  EEPROM.get(160, save_num);
  // EEPROM.commit();
  EEPROM.end();
  loadPressureCalibration();

  timer.setInterval(XKC_READ_INTERVAL_MS, readXkcSensor);
  timer.setTimeout(5000, []() {
    time1 = timer.setInterval(5612, []() {
      tem();
      send_data();
      // updata();
      timer.restartTimer(time1);
      timer.restartTimer(time2);
    });
    time2 = timer.setInterval(123, pressure);
    timer.setInterval(600005, []() { // 10p
      connectionstatus();
      timer.restartTimer(time1);
      timer.restartTimer(time2);
    });
  });
}
void loop() {
  ESP.wdtFeed();
  serviceExternalWatchdog();
  Blynk.run();
  timer.run();
}
