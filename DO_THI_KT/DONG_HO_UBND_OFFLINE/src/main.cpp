#define BLYNK_TEMPLATE_ID "TMPL6ZpS1D0RR"
#define BLYNK_TEMPLATE_NAME "ĐỒNG HỒ UBND"
#define BLYNK_AUTH_TOKEN "oe2-Gx_w4LSfr7Ie1OsuUByCAKnpaX_Q"

#define BLYNK_FIRMWARE_VERSION "250826"
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
#include <stdlib.h> // Thêm thư viện cho hàm abs()
RTC_DS3231 rtc_module;
DateTime lastSampledTime;

//-------------------
#include <I2C_eeprom.h>
#include <I2C_eeprom_cyclic_store.h>
#define MEMORY_SIZE 4096 // Total capacity of the EEPROM
#define PAGE_SIZE 32
I2C_eeprom ee(0x57, MEMORY_SIZE);

struct Data {
  uint32_t unixtime[4];
};

I2C_eeprom_cyclic_store<Data> cs;
Data data, dataCheck;
// Khởi tạo giá trị mặc định cho mảng unixtime
const struct Data dataDefault = {{0, 0, 0, 0}};
//-------------------
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <SPI.h>
#include <UrlEncode.h>
#include <WiFiClientSecure.h>
WiFiClient client;
HTTPClient http;
String server_name = "http://sgp1.blynk.cloud/external/api/";

#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/refs/heads/main/DO_THI_KT/DONG_HO_UBND/.pio/build/nodemcuv2/firmware.bin"
//-------------------
#include "PCF8575.h"
#include <WidgetRTC.h>
PCF8575 pcf8575_1(0x20);

const int NUM_CLOCKS = 4;
const int FWD_PINS[NUM_CLOCKS] = {P7, P5, P3, P1}; // M1, M2, M3, M4
const int REV_PINS[NUM_CLOCKS] = {P6, P4, P2, P0}; // M1, M2, M3, M4

const int MANUAL_ONLINE_PIN = D4; // Pin D4 (GPIO2) trên ESP8266
// --- Sensor Pins ---
const int SENSOR_PINS[NUM_CLOCKS] = {D3, D5, D6, D7}; // S1, S2, S3, S4

// --- State Machine and Sync Logic ---
enum State { STATE_OFFLINE_RUNNING,
             STATE_SYNCING,
             STATE_MANUAL_ONLINE };
State currentState = STATE_OFFLINE_RUNNING;

bool rtcSyncRequested = false;

unsigned long lastAdjust = 0;
const unsigned long SENSOR_TIMEOUT = 5000;           // 5 giây timeout cho cảm biến
const unsigned long CLOCK_ADJUST_INTERVAL = 30000;   // 30 giây
const unsigned long MIN_VALID_TRIGGER_TIME_MS = 750; // (ms) Bỏ qua các trigger nhanh hơn khoảng thời gian này để lọc nhiễu

// Biến để theo dõi mặt đồng hồ đang được đồng bộ (0-3). -1 nghĩa là không có mặt nào.
int sync_face_index = -1;
unsigned long lastSensorTriggerTime = 0;

// Cờ báo hiệu từ ISR, phải là 'volatile'
volatile bool sensor_triggered[NUM_CLOCKS] = {false, false, false, false};

// Biến đếm số phút cần quay cho mỗi đồng hồ
volatile int dem[NUM_CLOCKS] = {0, 0, 0, 0};

// Biến lưu hướng quay của motor: 1 cho FWD, -1 cho REV
enum MotorDirection { STOP = 0,
                      FORWARD = 1,
                      BACKWARD = -1 };
MotorDirection motor_direction[NUM_CLOCKS] = {STOP, STOP, STOP, STOP};

byte face_clock_selected = 0; // Mặt đồng hồ được chọn trên app (1-4), 0 là không chọn
byte mode = 0;                // 0: auto, 1: manual setting
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
  if (dem[0] > 0) {
    sensor_triggered[0] = true;
  }
}
ICACHE_RAM_ATTR void sensor_2() {
  if (dem[1] > 0) {
    sensor_triggered[1] = true;
  }
}
ICACHE_RAM_ATTR void sensor_3() {
  if (dem[2] > 0) {
    sensor_triggered[2] = true;
  }
}
ICACHE_RAM_ATTR void sensor_4() {
  if (dem[3] > 0) {
    sensor_triggered[3] = true;
  }
}
//-------------------------
void savedata() {
  if (eeprom_save_request) {
    if (memcmp(&data, &dataCheck, sizeof(data)) != 0) {
      Serial.println("\nData changed, writing to EEPROM...");
      if (cs.write(data)) {
        memcpy(&dataCheck, &data, sizeof(data));
        Serial.println("EEPROM write successful.");
      } else {
        Serial.println("EEPROM write failed!");
      }
    }
    eeprom_save_request = false;
  }
}
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
    rtc_module.adjust(blynkLocalTime);
    Serial.println("RTC time synced with Blynk server.");
    print_clock_time("Thời gian đã đồng bộ: ", new_unixtime);
    terminal.println("RTC sync successful.");
    terminal.flush();
    rtcSyncRequested = false;
  }
}
void stop_all_motors() {
  for (int i = 0; i < NUM_CLOCKS; i++) {
    pcf8575_1.digitalWrite(FWD_PINS[i], HIGH);
    pcf8575_1.digitalWrite(REV_PINS[i], HIGH);
  }
}

void check_and_adjust_single_clock(int clock_index, uint32_t current_unixtime) {
  uint32_t clock_unixtime = data.unixtime[clock_index];
  if (dem[clock_index] > 0 || clock_unixtime == 0) {
    return;
  }
  const long SECONDS_IN_A_DAY = 86400L;
  long now_sec = current_unixtime % SECONDS_IN_A_DAY;
  long clk_sec = clock_unixtime % SECONDS_IN_A_DAY;
  long diff = now_sec - clk_sec;
  if (diff == 0) {
    pcf8575_1.digitalWrite(FWD_PINS[clock_index], HIGH);
    pcf8575_1.digitalWrite(REV_PINS[clock_index], HIGH);
    advanceToNextFace();
    return;
  }
  int forward_minutes = (diff > 0) ? diff / 60 : (SECONDS_IN_A_DAY + diff) / 60;
  int backward_minutes = (diff > 0) ? (SECONDS_IN_A_DAY - diff) / 60 : (-diff) / 60;
  if (min(forward_minutes, backward_minutes) < 1) {
    pcf8575_1.digitalWrite(FWD_PINS[clock_index], HIGH);
    pcf8575_1.digitalWrite(REV_PINS[clock_index], HIGH);
    advanceToNextFace();
    return;
  }
  pcf8575_1.digitalWrite(FWD_PINS[clock_index], HIGH);
  pcf8575_1.digitalWrite(REV_PINS[clock_index], HIGH);
  delay(100);
  if (forward_minutes <= backward_minutes) {
    dem[clock_index] = forward_minutes;
    motor_direction[clock_index] = FORWARD;
    pcf8575_1.digitalWrite(REV_PINS[clock_index], HIGH);
    pcf8575_1.digitalWrite(FWD_PINS[clock_index], LOW);
  } else {
    dem[clock_index] = backward_minutes;
    motor_direction[clock_index] = BACKWARD;
    pcf8575_1.digitalWrite(FWD_PINS[clock_index], HIGH);
    pcf8575_1.digitalWrite(REV_PINS[clock_index], LOW);
  }
  lastSensorTriggerTime = millis();
}

void (*sensor_isrs[NUM_CLOCKS])() = {sensor_1, sensor_2, sensor_3, sensor_4};

void enableSensorInterrupt(int clock_index) {
  // Tắt hết tất cả các ngắt cảm biến trước
  for (int i = 0; i < NUM_CLOCKS; i++) {
    detachInterrupt(digitalPinToInterrupt(SENSOR_PINS[i]));
  }
  // Chỉ bật ngắt cho mặt đồng hồ được chỉ định (nếu có)
  if (clock_index >= 0 && clock_index < NUM_CLOCKS) {
    attachInterrupt(digitalPinToInterrupt(SENSOR_PINS[clock_index]), sensor_isrs[clock_index], RISING);
  }
}
void advanceToNextFace() {
  sync_face_index++;
  stop_all_motors();
  if (sync_face_index >= NUM_CLOCKS) {
    Serial.println("All clocks adjusted. Stopping motors.");
    sync_face_index = -1; // Reset index, -1 nghĩa là không đồng bộ
    // Reset lại thời gian của trigger cuối cùng. Điều này rất quan trọng để đảm bảo
    // lần điều chỉnh tiếp theo (có thể là 1 phút sau) sẽ không bị ảnh hưởng bởi giá trị cũ.
    lastSensorTriggerTime = 0;
    enableSensorInterrupt(-1); // Tắt hết ngắt
    return;
  }
  enableSensorInterrupt(sync_face_index); // Bật ngắt cho mặt hiện tại
  Serial.printf("Starting adjustment for Clock %d...\n", sync_face_index + 1);
  motor_direction[sync_face_index] = STOP; // Reset hướng
  check_and_adjust_single_clock(sync_face_index, timestamp_now);
}
void startSequentialAdjustment() {
  Serial.println("Starting sequential adjustment...");
  sync_face_index = -1; // Bắt đầu từ -1 để advanceToNextFace sẽ tăng lên 0
  DateTime now = rtc_module.now();
  timestamp_now = now.unixtime();
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
void processSingleSensor(int clock_index) {
  if (sensor_triggered[clock_index]) {
    // Lọc nhiễu: Bỏ qua các xung kích hoạt quá nhanh.
    // Một xung chỉ được coi là hợp lệ nếu nó xảy ra sau một khoảng thời gian tối thiểu
    // kể từ khi motor bắt đầu quay hoặc kể từ xung hợp lệ cuối cùng.
    if (millis() - lastSensorTriggerTime < MIN_VALID_TRIGGER_TIME_MS) {
      sensor_triggered[clock_index] = false; // Hạ cờ, bỏ qua xung nhiễu
      Serial.printf("Clock %d: Ignored noisy trigger (too fast).\n", clock_index + 1);
      return; // Thoát, không xử lý gì thêm
    }

    sensor_triggered[clock_index] = false; // Xung hợp lệ, hạ cờ để xử lý
    if (dem[clock_index] > 0) {
      dem[clock_index]--;
      // Cập nhật thời gian dựa trên hướng quay
      if (motor_direction[clock_index] == FORWARD) {
        data.unixtime[clock_index] += 60;
      } else if (motor_direction[clock_index] == BACKWARD) {
        // Khi quay lùi, thời gian của đồng hồ phải giảm đi để khớp với chuyển động cơ khí
        data.unixtime[clock_index] -= 60;
      }
      eeprom_save_request = true;
      lastSensorTriggerTime = millis(); // Cập nhật lại thời điểm của xung hợp lệ cuối cùng
    }
    if (dem[clock_index] <= 0) {
      pcf8575_1.digitalWrite(FWD_PINS[clock_index], HIGH);
      pcf8575_1.digitalWrite(REV_PINS[clock_index], HIGH);
      advanceToNextFace();
    }
  }
}

void handleSensorTriggers() {
  if (sync_face_index >= 0 && sync_face_index < NUM_CLOCKS) {
    processSingleSensor(sync_face_index);
  }
}
void handleTimeout() {
  if (currentState == STATE_MANUAL_ONLINE) {
    return;
  }
  // Chỉ xử lý timeout khi đang trong quá trình đồng bộ (sync_face_index hợp lệ)
  if (sync_face_index >= 0 && sync_face_index < NUM_CLOCKS && millis() - lastSensorTriggerTime > SENSOR_TIMEOUT) {
    // Serial.printf("Sensor timeout for Clock %d! Skipping...\n", sync_face_index);
    // terminal.printf("Clock %d timeout. Skipping.\n", sync_face_index);
    // terminal.flush();
    dem[sync_face_index] = 0; // Reset bộ đếm của mặt đồng hồ hiện tại
    motor_direction[sync_face_index] = STOP;
    // Chuyển sang mặt đồng hồ tiếp theo
    advanceToNextFace();
  }
} //-------------------------

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

BLYNK_WRITE(V1) {
  TimeInputParam t(param);
  DateTime now = rtc_module.now();
  if (t.hasStartTime()) {
    DateTime startTime(now.year(), now.month(), now.day(),
                       t.getStartHour(), t.getStartMinute(), t.getStartSecond());
    if (mode == 1 && face_clock_selected >= 1 && face_clock_selected <= NUM_CLOCKS) {
      data.unixtime[face_clock_selected - 1] = startTime.unixtime();
      eeprom_save_request = true; // Yêu cầu lưu
      Serial.printf("Set time for clock %d. Unixtime: ", face_clock_selected);
      Serial.println(startTime.unixtime());
    }
  }
}
BLYNK_WRITE(V2) {
  switch (param.asInt()) {
  case 0: {
    face_clock_selected = 0;
    break;
  }
  case 1: {
    face_clock_selected = 1;
    break;
  }
  case 2: {
    face_clock_selected = 2;
    break;
  }
  case 3: {
    face_clock_selected = 3;
    break;
  }
  case 4: {
    face_clock_selected = 4;
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
    for (int i = 0; i < NUM_CLOCKS; i++) {
      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt %d: ", i + 1);
      if (data.unixtime[i] == 0) {
        offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Chưa cài đặt.");
      } else {
        DateTime dt(data.unixtime[i]);
        offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "%02d:%02d", dt.hour(), dt.minute());
      }
      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, " | Cần quay: %d phút\n", dem[i]);
    }
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
  // ESP.wdtDisable();
  // ESP.wdtEnable(300000);
  Serial.begin(115200);
  Wire.begin();

  for (int i = 0; i < NUM_CLOCKS; i++) {
    pinMode(SENSOR_PINS[i], INPUT_PULLUP);
  }

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
  pcf8575_1.begin();
  for (int i = 0; i <= 15; i++) {
    pcf8575_1.pinMode(i, OUTPUT);
    pcf8575_1.digitalWrite(i, HIGH);
  }
  delay(3000);
  rtc_module.begin();
  ee.begin();
  cs.begin(ee, PAGE_SIZE, MEMORY_SIZE / PAGE_SIZE);
  cs.read(data);
  memcpy(&dataCheck, &data, sizeof(data));

  DateTime now = rtc_module.now();
  Serial.println("--- Last saved time from EEPROM ---");
  for (int i = 0; i < NUM_CLOCKS; i++) {
    char clock_name[10];
    sprintf(clock_name, "Clock %d: ", i + 1);
    print_clock_time(clock_name, data.unixtime[i]);
  }
  print_clock_time("Thời gian RTC hiện tại: ", now.unixtime());
  pinMode(MANUAL_ONLINE_PIN, INPUT_PULLUP);
  enableSensorInterrupt(-1); // Đảm bảo không có ngắt nào được kích hoạt lúc khởi động
  currentState = STATE_OFFLINE_RUNNING;
  Serial.println("Initialization complete. Entering OFFLINE mode.");
  Serial.println("-----------------------------------");
}
void loop() {
  // ESP.wdtDisable();
  // ESP.wdtEnable(300000);
  static unsigned long lastManualCheck = 0;
  static unsigned long lastSaveData = 0;
  if (millis() - lastManualCheck > 1000) {
    lastManualCheck = millis();
    bool manualModeActive = (digitalRead(MANUAL_ONLINE_PIN) == LOW);
    if (manualModeActive && currentState != STATE_MANUAL_ONLINE) {
      Serial.println("D4 LOW: Kích hoạt chế độ ONLINE thủ công.");
      stop_all_motors();
      for (int i = 0; i < NUM_CLOCKS; i++) {
        dem[i] = 0;
      }
      currentState = STATE_MANUAL_ONLINE;
    } else if (!manualModeActive && currentState == STATE_MANUAL_ONLINE) {
      Serial.println("D4 HIGH: Thoát chế độ ONLINE.");
      disconnectWifiAndBlynk();
      currentState = STATE_OFFLINE_RUNNING;
      delay(2000);
      startSequentialAdjustment();
    }
  }
  switch (currentState) {
  case STATE_OFFLINE_RUNNING:
    if (millis() - lastAdjust > CLOCK_ADJUST_INTERVAL && sync_face_index == -1) {
      startSequentialAdjustment();
      lastAdjust = millis();
    }
    break;
  case STATE_MANUAL_ONLINE:
    handleManualOnlineMode();
    break;
  }
  handleSensorTriggers();
  if (millis() - lastSaveData > 30000) {
    savedata();
    lastSaveData = millis();
  }
  handleTimeout();
}
