/*-Color = 48c9b0
 *V0  - Nút Bơm 1
 *V1  - Nút Bơm 2
 *V2  - Ampe Bơm 1 (C2/emon0)
 *V3  - Ampe Bơm 2 (C3/emon1)
 *V4  - Áp suất (C0)
 *V5  - Mực nước (C1)
 *V6  - Thể tích bể
 *V7  - Chế độ bơm: thủ công/nghỉ đêm/theo lịch
 *V8  - Chọn Bơm 1/Bơm 2 để cài bảo vệ
 *V9  - Ngưỡng ampe thấp
 *V10 - Ngưỡng ampe cao
 *V11 - Terminal lệnh và thông báo
 *V12 - Chọn trống/nghỉ đêm/khung giờ Bơm 1/Bơm 2
 *V13 - Nhập giờ bắt đầu/kết thúc
 *V14 - Bật/tắt bảo vệ dòng điện
 *V15 - Bật/tắt thông báo
 *V17 - Xem thông tin chế độ và lịch bơm
 *V19 - Ampe Giếng (C4/emon2)
 *V20 - Ngày giờ RTC
 */

#define BLYNK_TEMPLATE_ID "TMPL6PHEaSQtI"
#define BLYNK_TEMPLATE_NAME "CT Tuyên Thạnh"
#define BLYNK_AUTH_TOKEN "-TEktfE94b5Z8XxPE7JglkNlhmp6t_Qd"

#define BLYNK_FIRMWARE_VERSION "260822"
//------------------
#define BLYNK_PRINT Serial
#include "EmonLib.h"
#include "PCF8575.h"
#include "RTClib.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <Eeprom24C32_64.h>
#include <I2C_eeprom_cyclic_store.h>
#include <SimpleKalmanFilter.h>
#include <WiFiClientSecure.h>
#include <WidgetRTC.h>
#include <Wire.h>
//-----------------------------
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/CONG_TRINH/Tram_So_Do_1/.pio/build/nodemcuv2/firmware.bin"
String server_name = "http://sgp1.blynk.cloud/external/api";
//-----------------------------
PCF8575 pcf8575_1(0x20);
RTC_DS3231 rtc_module;
EnergyMonitor emon0, emon1, emon2;
#define MEMORY_SIZE 4096
#define PAGE_SIZE 32
I2C_eeprom ee(0x57, MEMORY_SIZE);

WiFiClient client;
HTTPClient http;
//-----------------------------
const char *ssid = "Tram Cap Nuoc";
const char *password = "12345678";
// Hai bơm cấp 2 đang sử dụng.
const int pin_B1 = P7;
const int pin_B2 = P6;
// Tín hiệu heartbeat gửi sang mạch watchdog ngoài.
const int pin_WATCHDOG = P8;
// Ngõ ra dự phòng, chưa sử dụng.
const int pin_P5 = P5;
const int pin_P4 = P4;
const int pin_P3 = P3;
const int pin_P2 = P2;
const int pin_P1 = P1;
const int pin_P0 = P0;
// Trạng thái logic của bơm hiển thị trên Blynk.
const uint8_t PUMP_STATE_OFF = LOW;
const uint8_t PUMP_STATE_ON = HIGH;
// Mức điện xuất ra relay active-LOW.
const uint8_t RELAY_LEVEL_ON = LOW;
const uint8_t RELAY_LEVEL_OFF = HIGH;
// Hai bơm cấp 2 dùng tiếp điểm NC:
// relay nhả (HIGH) -> NC đóng -> bơm chạy;
// relay kích (LOW) -> NC mở -> bơm dừng.
const uint8_t PUMP_RELAY_RUN_LEVEL = RELAY_LEVEL_OFF;
const uint8_t PUMP_RELAY_STOP_LEVEL = RELAY_LEVEL_ON;

const int S0pin = P15;
const int S1pin = P14;
const int S2pin = P13;
const int S3pin = P12;
//-----------------------------
char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";

bool key = false;
bool trip1 = false, trip2 = false;
bool key_bom = true, key_bom2 = true;
bool blynk_first_connect = false;
// Trạng thái mặc định khớp với mạch fail-safe: relay nhả, hai bơm chạy.
bool status_b1 = PUMP_STATE_ON, status_b2 = PUMP_STATE_ON;
const unsigned long WATCHDOG_TOGGLE_INTERVAL_MS = 5000UL;
uint8_t watchdogOutputLevel = LOW;
unsigned long watchdogLastToggleMs = 0;

float volume, smoothDistance, smoothed_adc_level;
float Irms0, Irms1, Irms2;

unsigned long int yIrms0 = 0, yIrms1 = 0;

float Result, filtered_adc_pressure;
int c = 0, b = 0;
int xSetAmpe1 = 0, xSetAmpe2 = 0;
int timer_I = -1;
int unlockTimerId = -1;
const int dai = 1640;
const int rong = 640;

// --- Kalman Filters & Median ---
SimpleKalmanFilter pressureKalmanFilter(1.0, 1.0, 0.01);
SimpleKalmanFilter levelKalmanFilter(2.0, 2.0, 0.01);

const int MEDIAN_WINDOW_SIZE = 5;
int median_buffer[MEDIAN_WINDOW_SIZE];
int median_buffer_index = 0;

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

//-----------------------------
#define MAX_CALIB_POINTS 5
#define PUMP_SCHEDULE_COUNT 4
const uint8_t MODE_MANUAL = 0;
const uint8_t MODE_NIGHT_REST = 1;
const uint8_t MODE_SCHEDULE = 2;
const uint8_t EMPTY_SCHEDULE_MENU_INDEX = 0;
const uint8_t NIGHT_SCHEDULE_MENU_INDEX = 1;
const uint8_t PUMP_SCHEDULE_MENU_OFFSET = 2;
const uint8_t LAST_SCHEDULE_MENU_INDEX =
    PUMP_SCHEDULE_MENU_OFFSET + PUMP_SCHEDULE_COUNT * 2 - 1;

struct CalibPoint {
  uint16_t adc;   // Giá trị ADC (0-1023)
  uint16_t value; // Giá trị quy đổi (Áp suất * 100, Mực nước cm)
};

struct Data {
  byte SetAmpe1max, SetAmpe1min;
  byte SetAmpe2max, SetAmpe2min;
  byte mode_cap2;
  uint16_t b1_start[PUMP_SCHEDULE_COUNT], b1_stop[PUMP_SCHEDULE_COUNT];
  uint16_t b2_start[PUMP_SCHEDULE_COUNT], b2_stop[PUMP_SCHEDULE_COUNT];
  uint16_t night_start, night_stop;
  int save_num;
  byte key_noti;
  byte key_protect;
  CalibPoint pressure_points[MAX_CALIB_POINTS];
  uint8_t num_pressure_points;
  CalibPoint level_points[MAX_CALIB_POINTS];
  uint8_t num_level_points;
} data, dataCheck;
I2C_eeprom_cyclic_store<Data> cs;

WidgetTerminal terminal(V11);
WidgetRTC rtc_widget;
BlynkTimer timer, timer1;
void rtctime();

void updateBlynkMenuLabels() {
  BlynkParamAllocated modeMenu(96);
  modeMenu.add("THỦ CÔNG");
  modeMenu.add("TĐ NGHỈ ĐÊM");
  modeMenu.add("TĐ THEO LỊCH");
  Blynk.setProperty(V7, "labels", modeMenu);

  BlynkParamAllocated scheduleMenu(255);
  scheduleMenu.add("...");
  scheduleMenu.add("GIỜ NGHỈ ĐÊM");
  scheduleMenu.add("BƠM 1 - LẦN 1");
  scheduleMenu.add("BƠM 2 - LẦN 1");
  scheduleMenu.add("BƠM 1 - LẦN 2");
  scheduleMenu.add("BƠM 2 - LẦN 2");
  scheduleMenu.add("BƠM 1 - LẦN 3");
  scheduleMenu.add("BƠM 2 - LẦN 3");
  scheduleMenu.add("BƠM 1 - LẦN 4");
  scheduleMenu.add("BƠM 2 - LẦN 4");
  Blynk.setProperty(V12, "labels", scheduleMenu);
}

BLYNK_CONNECTED() {
  rtc_widget.begin();
  blynk_first_connect = true;
  updateBlynkMenuLabels();
  b = EMPTY_SCHEDULE_MENU_INDEX;
  Blynk.virtualWrite(V0, status_b1);
  Blynk.virtualWrite(V1, status_b2);
  Blynk.virtualWrite(V7, data.mode_cap2);
  Blynk.virtualWrite(V12, EMPTY_SCHEDULE_MENU_INDEX);
  Blynk.virtualWrite(V14, data.key_protect);
  Blynk.virtualWrite(V15, data.key_noti);
}
//-------------------------------------------------------------------
void connectionstatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Mat ket noi WiFi");
    WiFi.reconnect();
    return;
  }
  if (!Blynk.connected()) {
    Serial.println("WiFi OK, khong ket noi duoc Blynk/Internet");
    Blynk.connect(1000);
    return;
  }
  // Serial.println("WiFi va Blynk binh thuong");
}

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

void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}
void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}
void update_progress(int cur, int total) {
  // OTA có thể giữ chương trình trong hàm update lâu hơn ngưỡng 20 giây.
  serviceExternalWatchdog();
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
//-------------------------------------------------------------------
void up() {
  if (WiFi.status() != WL_CONNECTED)
    return;
  String server_path = server_name + "/batch/update?token=" + BLYNK_AUTH_TOKEN +
                       "&V2=" + Irms0 +
                       "&V3=" + Irms1 +
                       "&V19=" + Irms2 +
                       "&V4=" + Result +
                       "&V5=" + smoothDistance +
                       "&V6=" + volume;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}
void savedata() {
  if (memcmp(&data, &dataCheck, sizeof(data)) != 0) {
    data.save_num++;
    if (cs.write(data)) {
      memcpy(&dataCheck, &data, sizeof(data));
      Blynk.setProperty(V11, "label", data.save_num);
    } else {
      data.save_num--;
    }
  }
}
/*
void on_cap1() {
  if (data.status_g1 != HIGH) {
    data.status_g1 = HIGH;
    savedata();
  }
  if (!trip0) {
    pcf8575_1.digitalWrite(pin_G1, data.status_g1 ? RELAY_LEVEL_ON : RELAY_LEVEL_OFF);
  }
  Blynk.virtualWrite(V0, data.status_g1);
}
void off_cap1() {
  if (data.status_g1 != LOW) {
    data.status_g1 = LOW;
    savedata();
  }
  yIrms0 = 0;
  pcf8575_1.digitalWrite(pin_G1, data.status_g1 ? RELAY_LEVEL_ON : RELAY_LEVEL_OFF);
  Blynk.virtualWrite(V0, data.status_g1);
}
void on_nenkhi() {
  if (data.status_nk1 != HIGH) {
    data.status_nk1 = HIGH;
    savedata();
  }
  if (!trip2) {
    pcf8575_1.digitalWrite(pin_NK1, data.status_nk1 ? RELAY_LEVEL_ON : RELAY_LEVEL_OFF);
  }
  Blynk.virtualWrite(V18, data.status_nk1);
}
void off_nenkhi() {
  if (data.status_nk1 != LOW) {
    data.status_nk1 = LOW;
    savedata();
  }
  pcf8575_1.digitalWrite(pin_NK1, data.status_nk1 ? RELAY_LEVEL_ON : RELAY_LEVEL_OFF);
  Blynk.virtualWrite(V18, data.status_nk1);
}
*/
void on_bom_1() {
  if (trip1) {
    Blynk.virtualWrite(V0, status_b1);
    return;
  }
  if (status_b1 != PUMP_STATE_ON) {
    yIrms0 = 0;
    xSetAmpe1 = 0;
    status_b1 = PUMP_STATE_ON;
    pcf8575_1.digitalWrite(pin_B1, PUMP_RELAY_RUN_LEVEL);
    Blynk.virtualWrite(V0, status_b1);
  }
}
void off_bom_1() {
  yIrms0 = 0;
  xSetAmpe1 = 0;
  if (status_b1 != PUMP_STATE_OFF) {
    status_b1 = PUMP_STATE_OFF;
    pcf8575_1.digitalWrite(pin_B1, PUMP_RELAY_STOP_LEVEL);
    Blynk.virtualWrite(V0, status_b1);
  }
}
void on_bom_2() {
  if (trip2) {
    Blynk.virtualWrite(V1, status_b2);
    return;
  }
  if (status_b2 != PUMP_STATE_ON) {
    yIrms1 = 0;
    xSetAmpe2 = 0;
    status_b2 = PUMP_STATE_ON;
    pcf8575_1.digitalWrite(pin_B2, PUMP_RELAY_RUN_LEVEL);
    Blynk.virtualWrite(V1, status_b2);
  }
}
void off_bom_2() {
  yIrms1 = 0;
  xSetAmpe2 = 0;
  if (status_b2 != PUMP_STATE_OFF) {
    status_b2 = PUMP_STATE_OFF;
    pcf8575_1.digitalWrite(pin_B2, PUMP_RELAY_STOP_LEVEL);
    Blynk.virtualWrite(V1, status_b2);
  }
}

bool isTimeInSchedule(uint16_t nowMinute, uint16_t startMinute, uint16_t stopMinute) {
  // start == stop được xem là khung giờ tắt.
  if (startMinute == stopMinute)
    return false;
  if (startMinute < stopMinute)
    return nowMinute >= startMinute && nowMinute < stopMinute;
  // Khung giờ đi qua 0 giờ, ví dụ 22:00 -> 02:00.
  return nowMinute >= startMinute || nowMinute < stopMinute;
}

bool isPumpScheduled(uint16_t nowMinute, const uint16_t starts[], const uint16_t stops[]) {
  for (uint8_t i = 0; i < PUMP_SCHEDULE_COUNT; i++) {
    if (isTimeInSchedule(nowMinute, starts[i], stops[i]))
      return true;
  }
  return false;
}

void applyPumpRequests(bool runPump1, bool runPump2) {
  if (runPump1 && !trip1)
    on_bom_1();
  else
    off_bom_1();

  if (runPump2 && !trip2)
    on_bom_2();
  else
    off_bom_2();
}

uint32_t getNightShiftDayIndex(const DateTime &now, uint16_t nowMinute) {
  uint32_t shiftDayIndex = now.unixtime() / 86400UL;
  // Với khung qua 0 giờ, phần sau 0 giờ vẫn thuộc ca bắt đầu từ tối hôm trước.
  if ((data.night_start > data.night_stop) && (nowMinute < data.night_stop) &&
      (shiftDayIndex > 0)) {
    shiftDayIndex--;
  }
  return shiftDayIndex;
}

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
  float x = current_adc;
  float x1 = p1->adc, y1 = p1->value, x2 = p2->adc, y2 = p2->value;

  if (abs(x2 - x1) < 0.001)
    return y1;
  return y1 + (x - x1) * (y2 - y1) / (x2 - x1);
}

//-------------------------------------------------------------------
void readPressure() { // C0 - Ap Luc
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  int raw_adc = analogRead(A0);

  filtered_adc_pressure = pressureKalmanFilter.updateEstimate(raw_adc);
  float val = interpolate(filtered_adc_pressure, data.pressure_points, data.num_pressure_points);

  Result = val / 100.0f; // Quy đổi về bar
}
void MeasureCmForSmoothing() { // C1-  Muc Nuoc
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);

  int raw_adc = analogRead(A0);

  // 1. Median Filter
  median_buffer[median_buffer_index] = raw_adc;
  median_buffer_index = (median_buffer_index + 1) % MEDIAN_WINDOW_SIZE;
  int sorted_buffer[MEDIAN_WINDOW_SIZE];
  memcpy(sorted_buffer, median_buffer, sizeof(median_buffer));
  int median_value = getMedian(sorted_buffer, MEDIAN_WINDOW_SIZE);

  // 2. Kalman Filter
  smoothed_adc_level = levelKalmanFilter.updateEstimate(median_value);

  // Nội suy ra cm
  smoothDistance = interpolate(smoothed_adc_level, data.level_points, data.num_level_points);

  volume = (dai * smoothDistance * rong) / 1000000.0;
}
//-------------------------------------------------------------------
void readPowerPump1() { // C2 - Bơm 1 - Irms0/emon0
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms0 = emon0.calcIrms(1480);

  if (rms0 < 2) {
    Irms0 = 0;
    xSetAmpe1 = 0;
    yIrms0 = 0;
    return;
  }

  Irms0 = rms0;
  yIrms0++;
  if (yIrms0 <= 3)
    return;

  if ((status_b1 == PUMP_STATE_OFF) && data.key_noti && key_bom) {
    key_bom = false;
    Blynk.logEvent("info", String("Bơm 1 KHÔNG dừng. Xin kiểm tra."));
    timer1.setTimeout(900000L, []() { key_bom = true; });
  }
  if (status_b1 == PUMP_STATE_OFF) {
    xSetAmpe1 = 0;
    return;
  }

  if (!data.key_protect) {
    xSetAmpe1 = 0;
    return;
  }

  bool invalidThresholds = (data.SetAmpe1min > 0) &&
                           (data.SetAmpe1max > 0) &&
                           (data.SetAmpe1min >= data.SetAmpe1max);
  if (invalidThresholds) {
    xSetAmpe1 = 0;
    return;
  }

  bool overCurrent = (data.SetAmpe1max > 0) && (Irms0 >= data.SetAmpe1max);
  bool underCurrent = (data.SetAmpe1min > 0) && (Irms0 <= data.SetAmpe1min);
  if (overCurrent || underCurrent) {
    xSetAmpe1++;
    if (xSetAmpe1 >= 2) {
      off_bom_1();
      xSetAmpe1 = 0;
      trip1 = true;
      if (data.mode_cap2 == MODE_NIGHT_REST)
        timer1.setTimeout(1L, []() { rtctime(); });
      if (data.key_noti) {
        if (overCurrent) {
          Blynk.logEvent("error", String("Ampe BƠM 1 quá cao: ") + String(Irms0, 2) + String(" A. Đã tắt bơm!"));
        } else {
          Blynk.logEvent("error", String("Ampe BƠM 1 quá thấp: ") + String(Irms0, 2) + String(" A. Đã tắt bơm!"));
        }
      }
    }
  } else {
    xSetAmpe1 = 0;
  }
}

void readPowerPump2() { // C3 - Bơm 2 - Irms1/emon1
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms1 = emon1.calcIrms(1480);

  if (rms1 < 2) {
    Irms1 = 0;
    xSetAmpe2 = 0;
    yIrms1 = 0;
    return;
  }

  Irms1 = rms1;
  yIrms1++;
  if (yIrms1 <= 3)
    return;

  if ((status_b2 == PUMP_STATE_OFF) && data.key_noti && key_bom2) {
    key_bom2 = false;
    Blynk.logEvent("info", String("Bơm 2 KHÔNG dừng. Xin kiểm tra."));
    timer1.setTimeout(900000L, []() { key_bom2 = true; });
  }
  if (status_b2 == PUMP_STATE_OFF) {
    xSetAmpe2 = 0;
    return;
  }

  if (!data.key_protect) {
    xSetAmpe2 = 0;
    return;
  }

  bool invalidThresholds = (data.SetAmpe2min > 0) &&
                           (data.SetAmpe2max > 0) &&
                           (data.SetAmpe2min >= data.SetAmpe2max);
  if (invalidThresholds) {
    xSetAmpe2 = 0;
    return;
  }

  bool overCurrent = (data.SetAmpe2max > 0) && (Irms1 >= data.SetAmpe2max);
  bool underCurrent = (data.SetAmpe2min > 0) && (Irms1 <= data.SetAmpe2min);
  if (overCurrent || underCurrent) {
    xSetAmpe2++;
    if (xSetAmpe2 >= 2) {
      off_bom_2();
      xSetAmpe2 = 0;
      trip2 = true;
      if (data.mode_cap2 == MODE_NIGHT_REST)
        timer1.setTimeout(1L, []() { rtctime(); });
      if (data.key_noti) {
        if (overCurrent) {
          Blynk.logEvent("error", String("Ampe BƠM 2 quá cao: ") + String(Irms1, 2) + String(" A. Đã tắt bơm!"));
        } else {
          Blynk.logEvent("error", String("Ampe BƠM 2 quá thấp: ") + String(Irms1, 2) + String(" A. Đã tắt bơm!"));
        }
      }
    }
  } else {
    xSetAmpe2 = 0;
  }
}

void readPowerWell() { // C4 - Giếng - Irms2/emon2
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, HIGH);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms2 = emon2.calcIrms(1480);

  if (rms2 < 2) {
    Irms2 = 0;
    return;
  }

  Irms2 = rms2;
}

/* DỰ PHÒNG: bảo vệ dòng điện cho Giếng.
   Hiện tại Giếng chỉ được đọc và hiển thị ampe, không có relay để tắt/mở.
   Khi bổ sung relay, bỏ chú thích hàm này, thêm lệnh tắt Giếng và gọi hàm
   checkPowerWellProtection() ngay sau readPowerWell().

void checkPowerWellProtection() {
  static unsigned long yIrmsWell = 0;
  static int xSetAmpeWell = 0;

  if (Irms2 < 2) {
    yIrmsWell = 0;
    xSetAmpeWell = 0;
    return;
  }

  yIrmsWell++;
  if (yIrmsWell <= 3)
    return;

  bool overCurrent = (data.SetAmpeWellmax > 0) && (Irms2 >= data.SetAmpeWellmax);
  bool underCurrent = (data.SetAmpeWellmin > 0) && (Irms2 <= data.SetAmpeWellmin);
  if (overCurrent || underCurrent) {
    xSetAmpeWell++;
    if ((xSetAmpeWell >= 2) && data.key_protect) {
      // TODO: thêm lệnh tắt relay Giếng tại đây.
      xSetAmpeWell = 0;
      if (data.key_noti) {
        if (overCurrent) {
          Blynk.logEvent("error", String("Ampe GIẾNG quá cao: ") + String(Irms2, 2) + String(" A."));
        } else {
          Blynk.logEvent("error", String("Ampe GIẾNG quá thấp: ") + String(Irms2, 2) + String(" A."));
        }
      }
    }
  } else {
    xSetAmpeWell = 0;
  }
}
*/

/* DỰ PHÒNG: logic đọc C2/C3/C4/C5 của cấu hình thiết bị cũ.
void readPower() {    // C2 - Cấp 1  - I0
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms0 = emon0.calcIrms(1480);
  if (rms0 < 2) {
    Irms0 = 0;
    if ((data.status_g1 == HIGH) && (yIrms0 > 3)) {
      if (smoothDistance > data.phao_max) {
        yIrms0 = 0;
      } else if (smoothDistance < data.phao_max) {
        xIrms0++;
        if ((xIrms0 > 3) && (data.key_protect)) {
          off_cap1();
          trip0 = true;
          xIrms0 = 0;
          if (data.key_noti)
            Blynk.logEvent("error", String("Giếng lỗi\nKhông đo được DÒNG ĐIỆN"));
        }
      }
    }
    if (rest_time == 0) {
      if (dem_cap1 == 0) {
        dem_cap1 = millis();
      }
      if ((unsigned long)(millis() - dem_cap1) > 900000) { // 15p
        dem_cap1 = 0;
        if (data.key_noti) {
          // Blynk.logEvent("info", String("Bơm Giếng không chạy. Xin kiểm tra."));
        }
      }
    }
  } else if (rms0 >= 2) {
    yIrms0 = yIrms0 + 1;
    Irms0 = rms0;
    if (yIrms0 > 3) {
      dem_cap1 = 0;
      if ((data.status_g1 == LOW) && data.key_noti && key_gieng) {
        key_gieng = false;
        Blynk.logEvent("info", String("Bơm Giếng không tắt. Xin kiểm tra."));
        timer1.setTimeout(900000L, []() { // 15p
          key_gieng = true;
        });
      }
      if ((Irms0 >= data.SetAmpemax) || (Irms0 <= data.SetAmpemin)) {
        xSetAmpe = xSetAmpe + 1;
        if ((xSetAmpe >= 2) && (data.key_protect)) {
          off_cap1();
          xSetAmpe = 0;
          trip0 = true;
          if (data.key_noti) {
            Blynk.logEvent("error", String("Cấp 1 lỗi: ") + Irms0 + String(" A"));
          }
        }
      } else {
        xSetAmpe = 0;
      }
    }
  }
  // Blynk.virtualWrite(V2, Irms0);
}
void readPower1() {   // C3 - Bơm    - I1
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms1 = emon1.calcIrms(1480);
  if (rms1 < 2) {
    Irms1 = 0;
    if (status_b1 == HIGH) {
      if (smoothDistance < data.phao_min) {
        xIrms1 = 0;
      } else {
        xIrms1++;
        if ((xIrms1 > 3) && (data.key_protect)) {
          off_bom_1();
          trip1 = true;
          xIrms1 = 0;
          if (data.key_noti)
            Blynk.logEvent("error", String("Bơm 1 lỗi\nKhông đo được DÒNG ĐIỆN"));
        }
      }
    }
    if (rest_time == 0) {
      if (dem_bom == 0) {
        dem_bom = millis();
      }
      if ((unsigned long)(millis() - dem_bom) > 1800000) { // 30p
        dem_bom = 0;
        if (data.key_noti) {
          // Blynk.logEvent("info", String("Bơm cấp 2 không chạy. Xin kiểm tra."));
        }
      }
    }
  } else if (rms1 >= 2) {
    Irms1 = rms1;
    yIrms1 = yIrms1 + 1;
    if (yIrms1 > 3) {
      dem_bom = 0;
      if ((status_b1 == LOW) && data.key_noti && key_bom) {
        key_bom = false;
        Blynk.logEvent("info", String("Bơm 1 cấp 2 không tắt. Xin kiểm tra."));
        timer1.setTimeout(900000L, []() { // 15p
          key_bom = true;
        });
      }
      if (((data.SetAmpe1max > 0) && (Irms1 >= data.SetAmpe1max)) ||
          ((data.SetAmpe1min > 0) && (Irms1 <= data.SetAmpe1min))) {
        xSetAmpe1 = xSetAmpe1 + 1;
        if ((xSetAmpe1 >= 2) && (data.key_protect)) {
          off_bom_1();
          xSetAmpe1 = 0;
          trip1 = true;
          if (data.key_noti) {
            Blynk.logEvent("error", String("Bơm 1 cấp 2 lỗi: ") + Irms1 + String(" A"));
          }
        }
      } else {
        xSetAmpe1 = 0;
      }
    }
  }
  // Blynk.virtualWrite(V3, Irms1);
}
void readPower2() {   // C4 - Nen khi- I2
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, HIGH);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms2 = emon2.calcIrms(1480);
  if (rms2 < 2) {
    Irms2 = 0;
    yIrms2 = 0;
  } else if (rms2 >= 2) {
    Irms2 = rms2;
    yIrms2 = yIrms2 + 1;
    if ((yIrms2 > 3) && ((Irms2 >= data.SetAmpe2max) || (Irms2 <= data.SetAmpe2min))) {
      xSetAmpe2 = xSetAmpe2 + 1;
      if ((xSetAmpe2 >= 2) && (data.key_protect)) {
        off_nenkhi();
        xSetAmpe2 = 0;
        trip2 = true;
        if (data.key_noti) {
          Blynk.logEvent("error", String("Máy nén khí lỗi: ") + Irms2 + String(" A"));
        }
      }
    } else {
      xSetAmpe2 = 0;
    }
  }
  // Blynk.virtualWrite(V19, Irms2);
}
void readPowerB2() {  // C5 - Bơm 2 - I3
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, HIGH);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rmsB2 = emon3.calcIrms(1480);

  if (rmsB2 < 2) {
    IrmsB2 = 0;
    if (status_b2 == HIGH) {
      if (smoothDistance < data.phao_min) {
        xIrmsB2 = 0;
      } else {
        xIrmsB2++;
        if ((xIrmsB2 > 3) && data.key_protect) {
          off_bom_2();
          trip_b2 = true;
          xIrmsB2 = 0;
          if (data.key_noti)
            Blynk.logEvent("error", String("Bơm 2 lỗi\nKhông đo được DÒNG ĐIỆN"));
        }
      }
    } else {
      xIrmsB2 = 0;
    }
  } else {
    IrmsB2 = rmsB2;
    yIrmsB2++;
    xIrmsB2 = 0;
    if (yIrmsB2 > 3) {
      if ((status_b2 == LOW) && data.key_noti && key_bom2) {
        key_bom2 = false;
        Blynk.logEvent("info", String("Bơm 2 cấp 2 không tắt. Xin kiểm tra."));
        timer1.setTimeout(900000L, []() { key_bom2 = true; });
      }
      bool overCurrent = (data.SetAmpeB2max > 0) && (IrmsB2 >= data.SetAmpeB2max);
      bool underCurrent = (data.SetAmpeB2min > 0) && (IrmsB2 <= data.SetAmpeB2min);
      if (overCurrent || underCurrent) {
        xSetAmpeB2++;
        if ((xSetAmpeB2 >= 2) && data.key_protect) {
          off_bom_2();
          xSetAmpeB2 = 0;
          trip_b2 = true;
          if (data.key_noti)
            Blynk.logEvent("error", String("Bơm 2 cấp 2 lỗi: ") + IrmsB2 + String(" A"));
        }
      } else {
        xSetAmpeB2 = 0;
      }
    }
  }
}
*/
//-------------------------------------------------------------------
void rtctime() {
  DateTime now = rtc_module.now();
  if (blynk_first_connect && year() >= 2026) {
    DateTime networkTime(year(), month(), day(), hour(), minute(), second());
    int32_t clockDifference = (int32_t)networkTime.unixtime() - (int32_t)now.unixtime();
    if (clockDifference > 120 || clockDifference < -120) {
      rtc_module.adjust(networkTime);
      now = networkTime;
    }
  }
  Blynk.virtualWrite(V20, daysOfTheWeek[now.dayOfTheWeek()], ", ", now.day(), "/", now.month(), "/", now.year(), " - ", now.hour(), ":", now.minute(), ":", now.second());
  bool automaticMode = (data.mode_cap2 == MODE_SCHEDULE) ||
                       (data.mode_cap2 == MODE_NIGHT_REST);
  if (automaticMode) {
    if (now.year() < 2026) {
      applyPumpRequests(false, false);
      return;
    }

    uint16_t nowMinute = now.hour() * 60U + now.minute();

    if (data.mode_cap2 == MODE_SCHEDULE) {
      bool runPump1 = isPumpScheduled(nowMinute, data.b1_start, data.b1_stop);
      bool runPump2 = isPumpScheduled(nowMinute, data.b2_start, data.b2_stop);

      // Hai lịch được xử lý độc lập nên hai bơm có thể chạy đồng thời.
      applyPumpRequests(runPump1, runPump2);
      return;
    }

    bool nightWindowActive = isTimeInSchedule(nowMinute, data.night_start,
                                              data.night_stop);
    if (!nightWindowActive) {
      // Ngoài giờ nghỉ đêm: yêu cầu cả hai bơm cùng chạy.
      applyPumpRequests(true, true);
      return;
    }

    uint32_t shiftDayIndex = getNightShiftDayIndex(now, nowMinute);
    bool restPump1 = (shiftDayIndex % 2UL) == 0;
    bool runPump1 = false;
    bool runPump2 = false;

    if (restPump1) {
      // Bơm 1 nghỉ. Nếu Bơm 2 lỗi thì Bơm 1 chạy thay.
      runPump2 = !trip2;
      runPump1 = trip2 && !trip1;
    } else {
      // Bơm 2 nghỉ. Nếu Bơm 1 lỗi thì Bơm 2 chạy thay.
      runPump1 = !trip1;
      runPump2 = trip1 && !trip2;
    }
    applyPumpRequests(runPump1, runPump2);
  }
}
void i2c_scaner() {
  byte error, address;
  int nDevices = 0;
  terminal.clear();
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      char message[48];
      snprintf(message, sizeof(message), "I2C: tìm thấy thiết bị tại 0x%02X\n", address);
      Blynk.virtualWrite(V11, message);
      nDevices++;
    } else if (error == 4) {
      char message[48];
      snprintf(message, sizeof(message), "I2C: lỗi không xác định tại 0x%02X\n", address);
      Blynk.virtualWrite(V11, message);
    }
  }
  if (nDevices == 0)
    Blynk.virtualWrite(V11, "Không tìm thấy thiết bị I2C.\n");
}
//-------------------------------------------------------------------
BLYNK_WRITE(V0) // Bơm 1
{
  if (key && !trip1 && (data.mode_cap2 == MODE_MANUAL)) {
    if (param.asInt() == PUMP_STATE_OFF) {
      off_bom_1();
    } else {
      on_bom_1();
    }
  }
  Blynk.virtualWrite(V0, status_b1);
}
BLYNK_WRITE(V1) // Bơm 2
{
  if (key && !trip2 && (data.mode_cap2 == MODE_MANUAL)) {
    if (param.asInt() == PUMP_STATE_OFF) {
      off_bom_2();
    } else {
      on_bom_2();
    }
  }
  Blynk.virtualWrite(V1, status_b2);
}
/* DỰ PHÒNG: V21 không dùng trong cấu hình hai bơm hiện tại.
BLYNK_WRITE(V21) // Bơm 2
{
  if ((key) && (!trip_b2)) {
    if (param.asInt() == LOW)
      off_bom_2();
    else
      on_bom_2();
  }
  Blynk.virtualWrite(V21, status_b2);
}
*/
BLYNK_WRITE(V7) // Chọn chế độ Cấp 2
{
  if (key) {
    int requestedMode = param.asInt();
    if ((requestedMode >= MODE_MANUAL) &&
        (requestedMode <= MODE_SCHEDULE)) {
      data.mode_cap2 = requestedMode;
      savedata();
      if (data.mode_cap2 != MODE_MANUAL)
        rtctime();
    } else {
      Blynk.virtualWrite(V7, data.mode_cap2);
    }
  } else {
    Blynk.virtualWrite(V7, data.mode_cap2);
  }
}
BLYNK_WRITE(V8) // Chon máy cài đặt bảo vệ
{
  switch (param.asInt()) {
  case 0: { // ....
    c = 0;
    Blynk.virtualWrite(V9, 0);
    Blynk.virtualWrite(V10, 0);
    break;
  }
  case 1: { // Bơm 1
    c = 1;
    Blynk.virtualWrite(V9, data.SetAmpe1min);
    Blynk.virtualWrite(V10, data.SetAmpe1max);
    break;
  }
  case 2: { // Bơm 2
    c = 2;
    Blynk.virtualWrite(V9, data.SetAmpe2min);
    Blynk.virtualWrite(V10, data.SetAmpe2max);
    break;
  }
  }
}
BLYNK_WRITE(V9) // min
{
  if (key) {
    int newMin = constrain(param.asInt(), 0, 255);
    if (c == 1) {
      if ((data.SetAmpe1max > 0) && (newMin >= data.SetAmpe1max)) {
        Blynk.virtualWrite(V9, data.SetAmpe1min);
        Blynk.virtualWrite(V11, "Ampe thấp phải nhỏ hơn ampe cao.\n");
        return;
      }
      data.SetAmpe1min = newMin;
      savedata();
    } else if (c == 2) {
      if ((data.SetAmpe2max > 0) && (newMin >= data.SetAmpe2max)) {
        Blynk.virtualWrite(V9, data.SetAmpe2min);
        Blynk.virtualWrite(V11, "Ampe thấp phải nhỏ hơn ampe cao.\n");
        return;
      }
      data.SetAmpe2min = newMin;
      savedata();
    }
  } else {
    Blynk.virtualWrite(V9, 0);
  }
}
BLYNK_WRITE(V10) // max
{
  if (key) {
    int newMax = constrain(param.asInt(), 0, 255);
    if (c == 1) {
      if ((newMax > 0) && (data.SetAmpe1min > 0) && (newMax <= data.SetAmpe1min)) {
        Blynk.virtualWrite(V10, data.SetAmpe1max);
        Blynk.virtualWrite(V11, "Ampe cao phải lớn hơn ampe thấp.\n");
        return;
      }
      data.SetAmpe1max = newMax;
      savedata();
    } else if (c == 2) {
      if ((newMax > 0) && (data.SetAmpe2min > 0) && (newMax <= data.SetAmpe2min)) {
        Blynk.virtualWrite(V10, data.SetAmpe2max);
        Blynk.virtualWrite(V11, "Ampe cao phải lớn hơn ampe thấp.\n");
        return;
      }
      data.SetAmpe2max = newMax;
      savedata();
    }
  } else {
    Blynk.virtualWrite(V10, 0);
  }
}
BLYNK_WRITE(V11) // String
{
  String dataS = param.asStr();
  dataS.trim();

  bool validTimedUnlock = false;
  int unlockSeconds = 0;
  if ((dataS.length() >= 2) && (dataS.length() <= 3) &&
      ((dataS[0] == 't') || (dataS[0] == 'T'))) {
    validTimedUnlock = true;
    for (unsigned int i = 1; i < dataS.length(); i++) {
      if ((dataS[i] < '0') || (dataS[i] > '9')) {
        validTimedUnlock = false;
        break;
      }
    }
    if (validTimedUnlock) {
      unlockSeconds = dataS.substring(1).toInt();
      validTimedUnlock = (unlockSeconds >= 1) && (unlockSeconds <= 99);
    }
  }

  if (validTimedUnlock) {
    terminal.clear();
    if (unlockTimerId >= 0)
      timer1.deleteTimer(unlockTimerId);
    key = true;
    Blynk.virtualWrite(V11, String("Mở khoá điều khiển trong ") + unlockSeconds + String("s\n"));
    unlockTimerId = timer1.setTimeout((unsigned long)unlockSeconds * 1000L, []() {
      key = false;
      unlockTimerId = -1;
      terminal.clear();
    });
  } else if ((dataS.length() > 0) && ((dataS[0] == 't') || (dataS[0] == 'T'))) {
    terminal.clear();
    Blynk.virtualWrite(V11, "Sai cú pháp. Chỉ cho phép sử dụng lệnh t1 đến t99.\n");
  } else if (dataS == "active") {
    terminal.clear();
    if (unlockTimerId >= 0) {
      timer1.deleteTimer(unlockTimerId);
      unlockTimerId = -1;
    }
    key = true;
    Blynk.virtualWrite(V11, "KHÔNG sử dụng phần mềm cho đến khi thông báo này mất.\n");
  } else if (dataS == "deactive") {
    terminal.clear();
    if (unlockTimerId >= 0) {
      timer1.deleteTimer(unlockTimerId);
      unlockTimerId = -1;
    }
    key = false;
    Blynk.virtualWrite(V11, "Ok!\nNhập mã để điều khiển!\n");
  } else if (dataS == "save") {
    terminal.clear();
    savedata();
    Blynk.virtualWrite(V11, "Đã lưu cài đặt.\n");
  } else if (dataS == "help") {
    terminal.clear();
    Blynk.virtualWrite(V11, "--- DANH SÁCH LỆNH ---\n"
                            "pre_X.X   : Calib áp suất tại X.X bar\n"
                            "pre_N_X.X : Thay điểm áp suất thứ N\n"
                            "level_YYY : Calib mực nước tại YYY cm\n"
                            "level_N_YYY: Thay điểm mực nước thứ N\n"
                            "calib_pre : Xem thong tin calib ap suat\n"
                            "calib_level : Xem thong tin calib muc nuoc\n"
                            "pre_clear : Xóa calib áp suất\n"
                            "level_clear : Xóa calib mực nước\n"
                            "t1...t99 : Mở khoá điều khiển từ 1 đến 99 giây\n"
                            "save      : Luu cai dat\n"
                            "reset     : Reset lỗi\n"
                            "update    : Cập nhật firmware\n"
                            "rst       : Khởi động lại ESP\n"
                            "i2c_scan  : Quét thiết bị I2C\n");
  } else if (dataS == "reset") {
    terminal.clear();
    trip2 = false;
    trip1 = false;
    if (data.mode_cap2 != MODE_MANUAL)
      timer1.setTimeout(1L, []() { rtctime(); });
    Blynk.virtualWrite(V11, "Đã RESET lỗi!\n");
  } else if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V11, "UPDATE FIRMWARE...");
    update_fw();
  } else if (dataS == "rst") {
    terminal.clear();
    Blynk.virtualWrite(V11, "ESP khởi động lại sau 3s");
    delay(3000);
    ESP.restart();
  } else if (dataS == "calib_pre") {
    terminal.clear();
    Blynk.virtualWrite(V11, "--- CALIB ÁP SUẤT ---\n");
    char buff[64];
    snprintf(buff, sizeof(buff), "Points: %d/%d\n", data.num_pressure_points, MAX_CALIB_POINTS);
    Blynk.virtualWrite(V11, buff);
    for (uint8_t i = 0; i < data.num_pressure_points; i++) {
      snprintf(buff, sizeof(buff), "#%d: ADC=%d -> %.2f bar\n", i + 1, data.pressure_points[i].adc, data.pressure_points[i].value / 100.0f);
      Blynk.virtualWrite(V11, buff);
    }
    snprintf(buff, sizeof(buff), "ADC: %.2f -> %.2f bar\n", filtered_adc_pressure, Result);
    Blynk.virtualWrite(V11, buff);
  } else if (dataS == "calib_level") {
    terminal.clear();
    Blynk.virtualWrite(V11, "--- CALIB MỰC NƯỚC ---\n");
    char buff[64];
    snprintf(buff, sizeof(buff), "Points: %d/%d\n", data.num_level_points, MAX_CALIB_POINTS);
    Blynk.virtualWrite(V11, buff);
    for (uint8_t i = 0; i < data.num_level_points; i++) {
      snprintf(buff, sizeof(buff), "#%d: ADC=%d -> %d cm\n", i + 1, data.level_points[i].adc, data.level_points[i].value);
      Blynk.virtualWrite(V11, buff);
    }
    snprintf(buff, sizeof(buff), "ADC: %.0f -> %.1f cm\n", smoothed_adc_level, smoothDistance);
    Blynk.virtualWrite(V11, buff);
  } else if (dataS == "pre_clear") {
    data.num_pressure_points = 0;
    savedata();
    Blynk.virtualWrite(V11, "Đã xóa calib áp suất.\n");
  } else if (dataS == "level_clear") {
    data.num_level_points = 0;
    savedata();
    Blynk.virtualWrite(V11, "Đã xóa calib mực nước.\n");
  } else if (dataS.startsWith("pre_") && dataS.indexOf('_', 4) >= 0) {
    int separator = dataS.indexOf('_', 4);
    String point_text = dataS.substring(4, separator);
    String value_text = dataS.substring(separator + 1);

    if (!isNonNegativeNumber(point_text, false) || !isNonNegativeNumber(value_text, true)) {
      Blynk.virtualWrite(V11, "Sai định dạng. Dùng pre_N_X.X (ví dụ pre_3_2.5).\n");
    } else {
      int point_number = point_text.toInt();
      float val = value_text.toFloat();
      if (point_number < 1 || point_number > data.num_pressure_points) {
        Blynk.virtualWrite(V11, "Điểm áp suất không tồn tại. Xem danh sách bằng calib_pre.\n");
      } else if (val > 655.35f) {
        Blynk.virtualWrite(V11, "Giá trị áp suất vượt giới hạn lưu trữ.\n");
      } else {
        CalibPoint pt;
        pt.adc = (uint16_t)round(filtered_adc_pressure);
        pt.value = (uint16_t)(val * 100);
        replaceCalibPoint((uint8_t)point_number, pt, data.pressure_points, data.num_pressure_points);
        savedata();
        Blynk.virtualWrite(V11, "Đã thay điểm áp suất đã chọn.\n");
      }
    }
  } else if (dataS.startsWith("level_") && dataS.indexOf('_', 6) >= 0) {
    int separator = dataS.indexOf('_', 6);
    String point_text = dataS.substring(6, separator);
    String value_text = dataS.substring(separator + 1);

    if (!isNonNegativeNumber(point_text, false) || !isNonNegativeNumber(value_text, false)) {
      Blynk.virtualWrite(V11, "Sai định dạng. Dùng level_N_YYY (ví dụ level_4_180).\n");
    } else {
      int point_number = point_text.toInt();
      unsigned long val = value_text.toInt();
      if (point_number < 1 || point_number > data.num_level_points) {
        Blynk.virtualWrite(V11, "Điểm mực nước không tồn tại. Xem danh sách bằng calib_level.\n");
      } else if (val > 65535UL) {
        Blynk.virtualWrite(V11, "Giá trị mực nước vượt giới hạn lưu trữ.\n");
      } else {
        CalibPoint pt;
        pt.adc = (uint16_t)round(smoothed_adc_level);
        pt.value = (uint16_t)val;
        replaceCalibPoint((uint8_t)point_number, pt, data.level_points, data.num_level_points);
        savedata();
        Blynk.virtualWrite(V11, "Đã thay điểm mực nước đã chọn.\n");
      }
    }
  } else if (dataS.startsWith("pre_")) {
    String valueText = dataS.substring(4);
    if (!isNonNegativeNumber(valueText, true)) {
      Blynk.virtualWrite(V11, "Sai định dạng. Dùng pre_X.X (ví dụ pre_2.5).\n");
    } else {
      float val = valueText.toFloat();
      if (val > 655.35f) {
        Blynk.virtualWrite(V11, "Giá trị áp suất vượt giới hạn lưu trữ.\n");
      } else {
        CalibPoint pt;
        pt.adc = (uint16_t)round(filtered_adc_pressure);
        pt.value = (uint16_t)(val * 100);
        addOrUpdateCalibPoint(pt, data.pressure_points, data.num_pressure_points);
        savedata();
        Blynk.virtualWrite(V11, "Đã lưu điểm áp suất.\n");
      }
    }
  } else if (dataS.startsWith("level_")) {
    String valueText = dataS.substring(6);
    if (!isNonNegativeNumber(valueText, false)) {
      Blynk.virtualWrite(V11, "Sai định dạng. Dùng level_YYY (ví dụ level_180).\n");
    } else {
      unsigned long val = valueText.toInt();
      if (val > 65535UL) {
        Blynk.virtualWrite(V11, "Giá trị mực nước vượt giới hạn lưu trữ.\n");
      } else {
        CalibPoint pt;
        pt.adc = (uint16_t)round(smoothed_adc_level);
        pt.value = (uint16_t)val;
        addOrUpdateCalibPoint(pt, data.level_points, data.num_level_points);
        savedata();
        Blynk.virtualWrite(V11, "Đã lưu điểm mực nước.\n");
      }
    }
  } else if ((dataS == "i2c") || (dataS == "i2c_scan")) {
    i2c_scaner();
  } else {
    Blynk.virtualWrite(V11, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
BLYNK_WRITE(V12) // Menu Timer Bơm Cấp 2
{
  updateBlynkMenuLabels();

  int selected = param.asInt();
  if ((selected < EMPTY_SCHEDULE_MENU_INDEX) ||
      (selected > LAST_SCHEDULE_MENU_INDEX)) {
    b = EMPTY_SCHEDULE_MENU_INDEX;
    return;
  }

  b = selected;
  if (b == EMPTY_SCHEDULE_MENU_INDEX)
    return;

  if (b == NIGHT_SCHEDULE_MENU_INDEX) {
    Blynk.virtualWrite(V13, data.night_start * 60UL,
                       data.night_stop * 60UL, tz);
    return;
  }

  uint8_t pumpScheduleIndex = b - PUMP_SCHEDULE_MENU_OFFSET;
  uint8_t slot = pumpScheduleIndex / 2;
  bool pump2Selected = (pumpScheduleIndex % 2) == 1;
  uint16_t startMinute = pump2Selected ? data.b2_start[slot] : data.b1_start[slot];
  uint16_t stopMinute = pump2Selected ? data.b2_stop[slot] : data.b1_stop[slot];
  Blynk.virtualWrite(V13, startMinute * 60UL, stopMinute * 60UL, tz);
}

BLYNK_WRITE(V13) // Time input
{
  if (key) {
    TimeInputParam t(param);

    if (b == EMPTY_SCHEDULE_MENU_INDEX) {
      Blynk.virtualWrite(V11,
                         "Chưa chọn khung giờ. V13 không thay đổi dữ liệu.\n");
      return;
    }

    if (b == NIGHT_SCHEDULE_MENU_INDEX) {
      if (t.hasStartTime())
        data.night_start = t.getStartHour() * 60U + t.getStartMinute();
      if (t.hasStopTime())
        data.night_stop = t.getStopHour() * 60U + t.getStopMinute();
      savedata();
      if (data.mode_cap2 == MODE_NIGHT_REST)
        rtctime();
      return;
    }

    if ((b < PUMP_SCHEDULE_MENU_OFFSET) ||
        (b > LAST_SCHEDULE_MENU_INDEX))
      return;

    uint8_t pumpScheduleIndex = b - PUMP_SCHEDULE_MENU_OFFSET;
    uint8_t slot = pumpScheduleIndex / 2;
    bool pump2Selected = (pumpScheduleIndex % 2) == 1;
    if (slot >= PUMP_SCHEDULE_COUNT)
      return;

    if (t.hasStartTime()) {
      uint16_t startMinute = t.getStartHour() * 60U + t.getStartMinute();
      if (pump2Selected)
        data.b2_start[slot] = startMinute;
      else
        data.b1_start[slot] = startMinute;
    }
    if (t.hasStopTime()) {
      uint16_t stopMinute = t.getStopHour() * 60U + t.getStopMinute();
      if (pump2Selected)
        data.b2_stop[slot] = stopMinute;
      else
        data.b1_stop[slot] = stopMinute;
    }
    savedata();
  } else {
    Blynk.virtualWrite(V13, 0);
  }
}
BLYNK_WRITE(V14) // Bảo vệ
{
  if (key) {
    data.key_protect = (param.asInt() != LOW);
    xSetAmpe1 = 0;
    xSetAmpe2 = 0;
    savedata();
  } else
    Blynk.virtualWrite(V14, data.key_protect);
}
BLYNK_WRITE(V15) // Thông báo
{
  if (key) {
    if (param.asInt() == LOW) {
      data.key_noti = false;
    } else {
      data.key_noti = true;
    }
    savedata();
  } else
    Blynk.virtualWrite(V15, data.key_noti);
}
BLYNK_WRITE(V17) // Info
{
  if (param.asInt() == 1) {
    terminal.clear();
    if (data.mode_cap2 == MODE_MANUAL) {
      Blynk.virtualWrite(V11, "Chế độ bơm: THỦ CÔNG.");
    } else if (data.mode_cap2 == MODE_SCHEDULE) {
      Blynk.virtualWrite(V11, "Chế độ bơm: TỰ ĐỘNG THEO LỊCH\n");
      for (uint8_t slot = 0; slot < PUMP_SCHEDULE_COUNT; slot++) {
        Blynk.virtualWrite(V11,
                           "B1-", slot + 1, ": ", data.b1_start[slot] / 60, ":", data.b1_start[slot] % 60,
                           " -> ", data.b1_stop[slot] / 60, ":", data.b1_stop[slot] % 60, "\n",
                           "B2-", slot + 1, ": ", data.b2_start[slot] / 60, ":", data.b2_start[slot] % 60,
                           " -> ", data.b2_stop[slot] / 60, ":", data.b2_stop[slot] % 60, "\n");
      }
    } else if (data.mode_cap2 == MODE_NIGHT_REST) {
      Blynk.virtualWrite(V11,
                         "Chế độ bơm: TỰ ĐỘNG NGHỈ ĐÊM\n",
                         "Giờ nghỉ: ", data.night_start / 60, ":", data.night_start % 60,
                         " -> ", data.night_stop / 60, ":", data.night_stop % 60, "\n");

      DateTime now = rtc_module.now();
      uint16_t nowMinute = now.hour() * 60U + now.minute();
      if (isTimeInSchedule(nowMinute, data.night_start, data.night_stop)) {
        bool restPump1 = (getNightShiftDayIndex(now, nowMinute) % 2UL) == 0;
        Blynk.virtualWrite(V11, restPump1 ? "Ca hiện tại: BƠM 2 chạy.\n"
                                          : "Ca hiện tại: BƠM 1 chạy.\n");
      } else {
        Blynk.virtualWrite(V11, "Ngoài giờ nghỉ: Chạy 2 bơm.\n");
      }
    }
  } else {
    terminal.clear();
  }
}
/* DỰ PHÒNG: điều khiển máy nén khí chưa sử dụng.
BLYNK_WRITE(V18) // Nen Khi
{
  if ((key) && (!trip2)) {
    if (param.asInt() == LOW) {
      off_nenkhi();
    } else
      on_nenkhi();
  }
  Blynk.virtualWrite(V18, data.status_nk1);
}
*/
//-------------------------
//-------------------------
//----------------------------------------------------
//-------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pcf8575_1.begin();
  // Khởi tạo heartbeat sớm; sau đó P8 phải đổi mức mỗi 5 giây.
  pcf8575_1.pinMode(pin_WATCHDOG, OUTPUT, watchdogOutputLevel);
  pcf8575_1.digitalWrite(pin_WATCHDOG, watchdogOutputLevel);
  watchdogLastToggleMs = millis();

  pcf8575_1.pinMode(S0pin, OUTPUT);
  pcf8575_1.pinMode(S1pin, OUTPUT);
  pcf8575_1.pinMode(S2pin, OUTPUT);
  pcf8575_1.pinMode(S3pin, OUTPUT);
  // Bơm cấp 2 dùng tiếp điểm NC nên mặc định để relay nhả (HIGH): bơm chạy.
  // PCF8575 cũng khởi động ở mức HIGH, phù hợp trạng thái fail-safe này.
  pcf8575_1.pinMode(pin_B1, OUTPUT, PUMP_RELAY_RUN_LEVEL);
  pcf8575_1.pinMode(pin_B2, OUTPUT, PUMP_RELAY_RUN_LEVEL);
  pcf8575_1.digitalWrite(pin_B1, PUMP_RELAY_RUN_LEVEL);
  pcf8575_1.digitalWrite(pin_B2, PUMP_RELAY_RUN_LEVEL);

  // Các ngõ ra dự phòng vẫn được để relay nhả.
  pcf8575_1.pinMode(pin_P5, OUTPUT);
  pcf8575_1.digitalWrite(pin_P5, RELAY_LEVEL_OFF);
  pcf8575_1.pinMode(pin_P4, OUTPUT);
  pcf8575_1.digitalWrite(pin_P4, RELAY_LEVEL_OFF);
  pcf8575_1.pinMode(pin_P3, OUTPUT);
  pcf8575_1.digitalWrite(pin_P3, RELAY_LEVEL_OFF);
  pcf8575_1.pinMode(pin_P2, OUTPUT);
  pcf8575_1.digitalWrite(pin_P2, RELAY_LEVEL_OFF);
  pcf8575_1.pinMode(pin_P1, OUTPUT);
  pcf8575_1.digitalWrite(pin_P1, RELAY_LEVEL_OFF);
  pcf8575_1.pinMode(pin_P0, OUTPUT);
  pcf8575_1.digitalWrite(pin_P0, RELAY_LEVEL_OFF);

  emon0.current(A0, 105);
  emon1.current(A0, 105);
  emon2.current(A0, 120);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);

  rtc_module.begin();
  ee.begin();
  bool currentStoreReady = cs.begin(ee, PAGE_SIZE, MEMORY_SIZE / PAGE_SIZE);
  bool currentDataValid = currentStoreReady && cs.read(data);

  if (!currentStoreReady) {
    Serial.println("EEPROM store initialization failed. Using RAM defaults...");
    memset(&data, 0, sizeof(data));
    memcpy(&dataCheck, &data, sizeof(data));
  } else if (!currentDataValid) {
    Serial.println("EEPROM data invalid. Creating defaults...");
    cs.format();
    memset(&data, 0, sizeof(data));
    // Buộc savedata() ghi bản mặc định đầu tiên sau khi format.
    memset(&dataCheck, 0xFF, sizeof(dataCheck));
    savedata();
  }

  bool dataCorrected = false;
  if (data.mode_cap2 > MODE_SCHEDULE) {
    data.mode_cap2 = MODE_MANUAL;
    dataCorrected = true;
  }
  if (data.key_protect > 1) {
    data.key_protect = 1;
    dataCorrected = true;
  }
  if (data.key_noti > 1) {
    data.key_noti = 1;
    dataCorrected = true;
  }
  for (uint8_t slot = 0; slot < PUMP_SCHEDULE_COUNT; slot++) {
    if ((data.b1_start[slot] > 1439) || (data.b1_stop[slot] > 1439)) {
      data.b1_start[slot] = 0;
      data.b1_stop[slot] = 0;
      dataCorrected = true;
    }
    if ((data.b2_start[slot] > 1439) || (data.b2_stop[slot] > 1439)) {
      data.b2_start[slot] = 0;
      data.b2_stop[slot] = 0;
      dataCorrected = true;
    }
  }
  if ((data.night_start > 1439) || (data.night_stop > 1439)) {
    data.night_start = 0;
    data.night_stop = 0;
    dataCorrected = true;
  }
  if (data.num_pressure_points > MAX_CALIB_POINTS) {
    data.num_pressure_points = 0;
    dataCorrected = true;
  }
  if (data.num_level_points > MAX_CALIB_POINTS) {
    data.num_level_points = 0;
    dataCorrected = true;
  }
  if (dataCorrected && currentStoreReady)
    savedata();
  else
    memcpy(&dataCheck, &data, sizeof(data));

  //------------------------------------
  timer.setTimeout(5000L, []() {
    timer.setInterval(121L, []() {
      readPressure();
      MeasureCmForSmoothing();
    });
    timer_I = timer.setInterval(1283L, []() {
      readPowerPump1();
      readPowerPump2();
      readPowerWell();
      up();
      // Chỉ bắt đầu đếm 1,283 giây sau khi đọc và gửi HTTP đã hoàn tất.
      timer.restartTimer(timer_I);
    });
    rtctime();
    timer.setInterval(15006L, []() {
      rtctime();
    });
    timer.setInterval(30000L, []() {
      connectionstatus();
    });
    terminal.clear();
  });
}
void loop() {
  serviceExternalWatchdog();
  Blynk.run();
  timer.run();
  timer1.run();
}
