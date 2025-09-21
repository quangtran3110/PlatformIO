#define BLYNK_TEMPLATE_ID "TMPL6ZpS1D0RR"
#define BLYNK_TEMPLATE_NAME "ĐỒNG HỒ UBND"
#define BLYNK_AUTH_TOKEN "oe2-Gx_w4LSfr7Ie1OsuUByCAKnpaX_Q"

#define BLYNK_FIRMWARE_VERSION "250719"
#define BLYNK_PRINT Serial
#define APP_DEBUG

const char *ssid = "PBV UB";
const char *password = "kientuong2022";
// const char *ssid = "tram bom so 4";
// const char *password = "0943950555";

#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
//-------------------
#include "RTClib.h"
#include <WidgetRTC.h>
#include <stdlib.h> // Thêm thư viện cho hàm abs()
RTC_DS3231 rtc_module;

char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";
//-------------------
#include <I2C_eeprom.h>
#include <I2C_eeprom_cyclic_store.h>
#define MEMORY_SIZE 0x4000 // Total capacity of the EEPROM
#define PAGE_SIZE 32
I2C_eeprom ee(0x57, MEMORY_SIZE);

struct Data {
  uint32_t unixtime_1, unixtime_2;
  uint32_t unixtime_3, unixtime_4;
};

I2C_eeprom_cyclic_store<Data> cs;
Data data, dataCheck;
const struct Data dataDefault = {0, 0, 0, 0};
//-------------------
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <SPI.h>
#include <UrlEncode.h>
#include <WiFiClientSecure.h>
WiFiClient client;
HTTPClient http;
String server_name = "http://sgp1.blynk.cloud/external/api/";

#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/DO_THI_KT/DONG_HO_UBND/.pio/build/nodemcuv2/firmware.bin"
//-------------------
#include "PCF8575.h"
PCF8575 pcf8575_1(0x20);
const int M1_FWD_PIN = P7;
const int M1_REV_PIN = P6;
const int M2_FWD_PIN = P5;
const int M2_REV_PIN = P4;
const int M3_FWD_PIN = P3;
const int M3_REV_PIN = P2;
const int M4_FWD_PIN = P1;
const int M4_REV_PIN = P0;
const int MANUAL_ONLINE_PIN = D4; // Pin D4 (GPIO2) trên ESP8266
// --- Sensor Pins ---
const int SENSOR_1_PIN = D3;
const int SENSOR_2_PIN = D5;
const int SENSOR_3_PIN = D6;
const int SENSOR_4_PIN = D7;
// --- State Machine and Sync Logic ---
enum State { STATE_OFFLINE_RUNNING,
             STATE_SYNCING,
             STATE_MANUAL_ONLINE };
State currentState = STATE_OFFLINE_RUNNING;

bool syncCompletedSuccessfully = false;
bool rtcSyncRequested = false;

unsigned long lastAdjust = 0;
unsigned long syncStartTime = 0;
const unsigned long SYNC_TIMEOUT = 5 * 60 * 1000;  // 5 phút timeout cho việc đồng bộ
const unsigned long SENSOR_TIMEOUT = 5000;         // 5 giây timeout cho cảm biến
const unsigned long CLOCK_ADJUST_INTERVAL = 30000; // 60 giây

int sync_face_index = 0; // Biến để theo dõi mặt đồng hồ đang được đồng bộ (1-4)
unsigned long lastSensorTriggerTime = 0;

// Cờ báo hiệu từ ISR, phải là 'volatile'
volatile bool sensor_1_triggered = false;
volatile bool sensor_2_triggered = false;
volatile bool sensor_3_triggered = false;
volatile bool sensor_4_triggered = false;
byte reboot_num, face_clock = 0, mode = 0;
volatile int dem1 = 0, dem2 = 0, dem3 = 0, dem4 = 0;
volatile bool eeprom_save_request = false;
uint32_t timestamp_now;

WidgetTerminal terminal(V3);
WidgetRTC rtc_widget;

// Khai báo trước
void print_clock_time(const char *clock_name, uint32_t unixtime);
void stop_all_motors();
void advanceToNextFace();
void startSequentialAdjustment();

BLYNK_CONNECTED() {
  Serial.println("Blynk connected.");
  rtc_widget.begin();
  rtcSyncRequested = true;
}

ICACHE_RAM_ATTR void sensor_1() {
  if (dem1 > 0) {
    sensor_1_triggered = true;
  }
}
ICACHE_RAM_ATTR void sensor_2() {
  if (dem2 > 0) {
    sensor_2_triggered = true;
  }
}
ICACHE_RAM_ATTR void sensor_3() {
  if (dem3 > 0) {
    sensor_3_triggered = true;
  }
}
ICACHE_RAM_ATTR void sensor_4() {
  if (dem4 > 0) {
    sensor_4_triggered = true;
  }
}
//-------------------------
void savedata() {
  if (memcmp(&data, &dataCheck, sizeof(data)) != 0) {
    Serial.println("\nData changed, writing to EEPROM...");
    if (cs.write(data)) {
      memcpy(&dataCheck, &data, sizeof(data));
      Serial.println("EEPROM write successful.");
    } else {
      Serial.println("EEPROM write failed!");
    }
  }
}
//-------------------------
void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
  terminal.println("OTA Update: Process started...");
  terminal.flush();
}
void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
  terminal.println("OTA Update: Finished. Rebooting...");
  terminal.flush();
}
void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
  static int last_percent = -1;
  int percent = (cur * 100) / total;
  if (percent > last_percent && percent % 5 == 0) {
    char buffer[30];
    sprintf(buffer, "Updating: %d%%", percent);
    Blynk.virtualWrite(V0, buffer);
    last_percent = percent;
  }
}
void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
  char buffer[50];
  sprintf(buffer, "OTA Update Error: %d", err);
  terminal.println(buffer);
  terminal.flush();
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
void disconnectWifiAndBlynk() {
  Blynk.disconnect();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi and Blynk disconnected.");
}
void processRtcSync() {
  if (rtcSyncRequested && year() > 2024) {
    Serial.println("Processing RTC sync request...");
    DateTime blynkLocalTime(year(), month(), day(), hour(), minute(), second());
    uint32_t new_unixtime = blynkLocalTime.unixtime();
    bool isTimeValid = true;
    char reason[128];
    if (data.unixtime_1 != 0 && new_unixtime <= data.unixtime_1) {
      isTimeValid = false;
      snprintf(reason, sizeof(reason), "Sync time (%lu) <= Clock 1 time (%lu)", new_unixtime, data.unixtime_1);
    } else if (data.unixtime_2 != 0 && new_unixtime <= data.unixtime_2) {
      isTimeValid = false;
      snprintf(reason, sizeof(reason), "Sync time (%lu) <= Clock 2 time (%lu)", new_unixtime, data.unixtime_2);
    } else if (data.unixtime_3 != 0 && new_unixtime <= data.unixtime_3) {
      isTimeValid = false;
      snprintf(reason, sizeof(reason), "Sync time (%lu) <= Clock 3 time (%lu)", new_unixtime, data.unixtime_3);
    } else if (data.unixtime_4 != 0 && new_unixtime <= data.unixtime_4) {
      isTimeValid = false;
      snprintf(reason, sizeof(reason), "Sync time (%lu) <= Clock 4 time (%lu)", new_unixtime, data.unixtime_4);
    }
    if (isTimeValid) {
      rtc_module.adjust(blynkLocalTime);
      Serial.println("RTC time synced with Blynk server.");
      print_clock_time("Thời gian đã đồng bộ: ", new_unixtime);
      terminal.println("RTC sync successful.");
      terminal.flush();
      syncCompletedSuccessfully = true;
    } else {
      Serial.println("RTC sync aborted. Reason:");
      Serial.println(reason);
      terminal.println("RTC sync aborted: Time is not valid.");
      terminal.flush();
    }
    rtcSyncRequested = false;
  }
}
void stop_all_motors() {
  pcf8575_1.digitalWrite(M1_FWD_PIN, HIGH);
  pcf8575_1.digitalWrite(M1_REV_PIN, HIGH);
  pcf8575_1.digitalWrite(M2_FWD_PIN, HIGH);
  pcf8575_1.digitalWrite(M2_REV_PIN, HIGH);
  pcf8575_1.digitalWrite(M3_FWD_PIN, HIGH);
  pcf8575_1.digitalWrite(M3_REV_PIN, HIGH);
  pcf8575_1.digitalWrite(M4_FWD_PIN, HIGH);
  pcf8575_1.digitalWrite(M4_REV_PIN, HIGH);
}
void check_and_adjust_single_clock(uint32_t current_unixtime, uint32_t clock_unixtime,
                                   volatile int &dem_counter,
                                   const int fwd_pin, const int rev_pin) {
  if (dem_counter > 0 || clock_unixtime == 0) {
    return;
  }
  const long SECONDS_IN_A_DAY = 86400L;
  long now_sec = current_unixtime % SECONDS_IN_A_DAY;
  long clk_sec = clock_unixtime % SECONDS_IN_A_DAY;
  long diff = now_sec - clk_sec;
  if (diff == 0) {
    pcf8575_1.digitalWrite(fwd_pin, HIGH);
    pcf8575_1.digitalWrite(rev_pin, HIGH);
    advanceToNextFace();
    return;
  }
  int forward_minutes = (diff > 0) ? diff / 60 : (SECONDS_IN_A_DAY + diff) / 60;
  int backward_minutes = (diff > 0) ? (SECONDS_IN_A_DAY - diff) / 60 : (-diff) / 60;
  if (min(forward_minutes, backward_minutes) < 1) {
    pcf8575_1.digitalWrite(fwd_pin, HIGH);
    pcf8575_1.digitalWrite(rev_pin, HIGH);
    advanceToNextFace();
    return;
  }
  pcf8575_1.digitalWrite(fwd_pin, HIGH);
  pcf8575_1.digitalWrite(rev_pin, HIGH);
  delay(100);
  if (forward_minutes <= backward_minutes) {
    dem_counter = forward_minutes;
    pcf8575_1.digitalWrite(rev_pin, HIGH);
    pcf8575_1.digitalWrite(fwd_pin, LOW);
  } else {
    dem_counter = backward_minutes;
    pcf8575_1.digitalWrite(fwd_pin, HIGH);
    pcf8575_1.digitalWrite(rev_pin, LOW);
  }
  lastSensorTriggerTime = millis();
}
void advanceToNextFace() {
  sync_face_index++;
  stop_all_motors();
  if (sync_face_index > 4) {
    Serial.println("All clocks adjusted. Stopping motors.");
    sync_face_index = 0; // Reset index
    return;
  }
  Serial.printf("Starting adjustment for Clock %d...\n", sync_face_index);
  DateTime now = rtc_module.now();
  timestamp_now = now.unixtime();
  switch (sync_face_index) {
  case 1:
    check_and_adjust_single_clock(timestamp_now, data.unixtime_1, dem1, M1_FWD_PIN, M1_REV_PIN);
    break;
  case 2:
    check_and_adjust_single_clock(timestamp_now, data.unixtime_2, dem2, M2_FWD_PIN, M2_REV_PIN);
    break;
  case 3:
    check_and_adjust_single_clock(timestamp_now, data.unixtime_3, dem3, M3_FWD_PIN, M3_REV_PIN);
    break;
  case 4:
    check_and_adjust_single_clock(timestamp_now, data.unixtime_4, dem4, M4_FWD_PIN, M4_REV_PIN);
    break;
  }
}
void startSequentialAdjustment() {
  Serial.println("Starting sequential adjustment...");
  sync_face_index = 0; // Reset và bắt đầu từ mặt 1
  advanceToNextFace();
}
void handleManualOnlineMode() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Manual Online Mode: Connecting to WiFi & Blynk...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Blynk.config(BLYNK_AUTH_TOKEN);
    unsigned long startConnectTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startConnectTime < 20000) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Blynk.connect();
      Serial.println("\nWiFi connected for Manual Mode.");
    } else {
      Serial.println("\nWiFi connection failed. Will retry...");
    }
  }
  Blynk.run();
  processRtcSync();
}
void processSingleSensor(volatile bool &sensor_triggered, volatile int &dem_counter, uint32_t &unixtime, const int fwd_pin, const int rev_pin) {
    if (sensor_triggered) {
        sensor_triggered = false;
        if (dem_counter > 0) {
            dem_counter--;
            unixtime += 60;
            eeprom_save_request = true;
            lastSensorTriggerTime = millis();
        }
        if (dem_counter <= 0) {
            pcf8575_1.digitalWrite(fwd_pin, HIGH);
            pcf8575_1.digitalWrite(rev_pin, HIGH);
            advanceToNextFace();
        }
    }
}

void handleSensorTriggers() {
  // Chỉ xử lý cảm biến của mặt đồng hồ đang được điều chỉnh
  switch (sync_face_index) {
  case 1:
    processSingleSensor(sensor_1_triggered, dem1, data.unixtime_1, M1_FWD_PIN, M1_REV_PIN);
    break;
  case 2:
    processSingleSensor(sensor_2_triggered, dem2, data.unixtime_2, M2_FWD_PIN, M2_REV_PIN);
    break;
  case 3:
    processSingleSensor(sensor_3_triggered, dem3, data.unixtime_3, M3_FWD_PIN, M3_REV_PIN);
    break;
  case 4:
    processSingleSensor(sensor_4_triggered, dem4, data.unixtime_4, M4_FWD_PIN, M4_REV_PIN);
    break;
  }
}
void handleEepromSave() {
  if (eeprom_save_request) {
    savedata();
    eeprom_save_request = false;
  }
}
void handleTimeout() {
  if (sync_face_index > 0 && millis() - lastSensorTriggerTime > SENSOR_TIMEOUT) {
    Serial.printf("Sensor timeout for Clock %d! Skipping...\n", sync_face_index);
    terminal.printf("Clock %d timeout. Skipping.\n", sync_face_index);
    terminal.flush();
    // Reset bộ đếm của mặt đồng hồ hiện tại
    switch (sync_face_index) {
    case 1:
      dem1 = 0;
      break;
    case 2:
      dem2 = 0;
      break;
    case 3:
      dem3 = 0;
      break;
    case 4:
      dem4 = 0;
      break;
    }
    // Chuyển sang mặt đồng hồ tiếp theo
    advanceToNextFace();
  }
}
//-------------------------
bool isRtcConnected() {
  Wire.beginTransmission(0x68);
  byte error = Wire.endTransmission();
  if (error == 0) {
    return true;
  }
  Serial.printf("RTC I2C connection error: %d\n", error);
  return false;
}
bool isEepromConnected() {
  Wire.beginTransmission(0x57);
  byte error = Wire.endTransmission();
  if (error == 0) {
    return true;
  }
  Serial.printf("EEPROM I2C connection error: %d\n", error);
  return false;
}
bool isPcfConnected(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  if (error == 0) {
    return true;
  }
  return false;
}
void print_clock_time(const char *clock_name, uint32_t unixtime) {
  Serial.print(clock_name);
  if (unixtime == 0) {
    Serial.println("Chưa cài đặt.");
    return;
  }
  DateTime dt(unixtime);
  char buffer[25];
  sprintf(buffer, "%02d/%02d/%d %02d:%02d:%02d",
          dt.day(), dt.month(), dt.year(), dt.hour(), dt.minute(), dt.second());
  Serial.println(buffer);
}
//-------------------------
BLYNK_WRITE(V1) {
  TimeInputParam t(param);
  DateTime now = rtc_module.now();
  if (t.hasStartTime()) {
    DateTime startTime(now.year(), now.month(), now.day(),
                       t.getStartHour(), t.getStartMinute(), t.getStartSecond());
    if (mode == 1) {
      if (face_clock == 1) {
        data.unixtime_1 = startTime.unixtime();
      } else if (face_clock == 2) {
        data.unixtime_2 = startTime.unixtime();
      } else if (face_clock == 3) {
        data.unixtime_3 = startTime.unixtime();
      } else if (face_clock == 4) {
        data.unixtime_4 = startTime.unixtime();
      }
      savedata();
      Serial.print("Start Time Unixtime: ");
      Serial.println(startTime.unixtime());
    }
  }
}
BLYNK_WRITE(V2) {
  switch (param.asInt()) {
  case 0: {
    face_clock = 0;
    break;
  }
  case 1: {
    face_clock = 1;
    break;
  }
  case 2: {
    face_clock = 2;
    break;
  }
  case 3: {
    face_clock = 3;
    break;
  }
  case 4: {
    face_clock = 4;
    break;
  }
  }
}
BLYNK_WRITE(V3) {
  char infoBuffer[512];
  int offset = 0;
  String dataS = param.asStr();
  if (dataS == "update") {
    terminal.clear();
    terminal.println("Received 'update' command.");
    terminal.println("Starting OTA Firmware Update...");
    terminal.flush();
    Blynk.virtualWrite(V0, "Updating FW...");
    update_fw();
  } else if (dataS == "1") {
    terminal.clear();
    offset = snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "--- THÔNG TIN ĐỒNG HỒ ---\n\n");
    if (data.unixtime_1 == 0) {
      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 1: Chưa cài đặt.");
    } else {
      DateTime dt(data.unixtime_1);
      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 1: %02d:%02d", dt.hour(), dt.minute());
    }
    offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, " | Cần quay: %d phút\n", dem1);
    if (data.unixtime_2 == 0) {
      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 2: Chưa cài đặt.");
    } else {
      DateTime dt(data.unixtime_2);
      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 2: %02d:%02d", dt.hour(), dt.minute());
    }
    offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, " | Cần quay: %d phút\n", dem2);
    if (data.unixtime_3 == 0) {
      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 3: Chưa cài đặt.");
    } else {
      DateTime dt(data.unixtime_3);
      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 3: %02d:%02d", dt.hour(), dt.minute());
    }
    offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, " | Cần quay: %d phút\n", dem3);
    if (data.unixtime_4 == 0) {
      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 4: Chưa cài đặt.");
    } else {
      DateTime dt(data.unixtime_4);
      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 4: %02d:%02d", dt.hour(), dt.minute());
    }
    offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, " | Cần quay: %d phút\n", dem4);
    Blynk.virtualWrite(V3, infoBuffer);
  } else if (dataS == "wifi") {
    long rssi = WiFi.RSSI();
    snprintf(infoBuffer, sizeof(infoBuffer), "WiFi: %ld dBm\n", rssi);
    Blynk.virtualWrite(V3, infoBuffer);
  }
}
BLYNK_WRITE(V4) {
  if (param.asInt() == 0) {
    mode = 0;
  } else
    mode = 1;
}
//-------------------------
void setup() {
  Wire.begin();
  pcf8575_1.begin();
  for (int i = 0; i <= 15; i++) {
    pcf8575_1.pinMode(i, OUTPUT);
    pcf8575_1.digitalWrite(i, HIGH);
  }

  delay(5000);
  ESP.wdtDisable();
  ESP.wdtEnable(300000);
  Serial.begin(115200);
  rtc_module.begin();
  ee.begin();
  cs.begin(ee, PAGE_SIZE, MEMORY_SIZE / PAGE_SIZE);
  cs.read(data);
  memcpy(&dataCheck, &data, sizeof(data));

  Serial.println("-----------------------------------");
  Serial.println("\n--- Check Module I2C ---");
  while (!isRtcConnected()) {
    Serial.println("DS3231 not found! Please check wiring. Retrying in 5s...");
    delay(5000);
  }
  Serial.println("DS3231 OK!");
  while (!isEepromConnected()) {
    Serial.println("EEPROM not found! Please check wiring. Retrying in 5s...");
    delay(5000);
  }
  Serial.println("EEPROM OK!");
  if (!isPcfConnected(0x20)) {
    Serial.println("PCF8575 not found! Please check wiring. Halting.");
    while (1)
      delay(1000);
  }
  Serial.println("PCF8575 OK!");
  DateTime now = rtc_module.now();
  Serial.println("--- Last saved time from EEPROM ---");
  print_clock_time("Clock 1: ", data.unixtime_1);
  print_clock_time("Clock 2: ", data.unixtime_2);
  print_clock_time("Clock 3: ", data.unixtime_3);
  print_clock_time("Clock 4: ", data.unixtime_4);
  print_clock_time("Thời gian RTC hiện tại: ", now.unixtime());
  pinMode(MANUAL_ONLINE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_1_PIN), sensor_1, RISING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_2_PIN), sensor_2, RISING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_3_PIN), sensor_3, RISING);
  attachInterrupt(digitalPinToInterrupt(SENSOR_4_PIN), sensor_4, RISING);
  currentState = STATE_OFFLINE_RUNNING;
  Serial.println("Initialization complete. Entering OFFLINE mode.");
  Serial.println("-----------------------------------");
}
void loop() {
  ESP.wdtDisable();
  ESP.wdtEnable(300000);
  static unsigned long lastManualCheck = 0;
  if (millis() - lastManualCheck > 100) {
    lastManualCheck = millis();
    bool manualModeActive = (digitalRead(MANUAL_ONLINE_PIN) == LOW);
    if (manualModeActive && currentState != STATE_MANUAL_ONLINE) {
      Serial.println("D4 LOW: Kích hoạt chế độ ONLINE thủ công.");
      stop_all_motors();
      dem1 = dem2 = dem3 = dem4 = 0;
      currentState = STATE_MANUAL_ONLINE;
    } else if (!manualModeActive && currentState == STATE_MANUAL_ONLINE) {
      Serial.println("D4 HIGH: Thoát chế độ ONLINE.");
      disconnectWifiAndBlynk();
      currentState = STATE_OFFLINE_RUNNING;
      startSequentialAdjustment();
    }
  }
  switch (currentState) {
  case STATE_OFFLINE_RUNNING:
    if (millis() - lastAdjust > CLOCK_ADJUST_INTERVAL && sync_face_index == 0) {
      startSequentialAdjustment();
      lastAdjust = millis();
    }
    break;
  case STATE_MANUAL_ONLINE:
    handleManualOnlineMode();
    break;
  }
  handleSensorTriggers();
  handleEepromSave();
  handleTimeout();
}
