/*V0 - Button C1
 *V1 - Button C2-1
 *V2 - Button C2-2
 *V3 -
 *V4 -
 *V5 - MENU motor
 *V6 - min
 *V7 - max
 *V8 -
 *V9 - Ngày/Giờ
 *V10 - terminal key
 *V11 -
 *V12 -
 *V13 - Bảo vệ
 *V14 - Ap luc
 *V15 -
 *V16 - Thông báo
 *V17 -
 *V18 -
 *V19 - Thể tích
 *V20 - còn lại
 *V21 - Do Sau
 *V22 - Dung Tich
 *V23 -
 *V24 - I0 - Cap 1
 *V25 - I1 - Cap 2
 *V26 - I2
 *V27 - I3
 *V28 -
 *V29 - Info
 *V30 - I4
 *V31 - datas_volume
 *V32 - LL1m3
 *V33 - LL24h
 *V34 - LLG1_RL
 *V35 -
 *V36 -
 *V37 -
 *V40 - thời gian chạy G1
 *V41 - thời gian chạy G1-24h
 *V42 - thời gian chạy B1
 *V43 - thời gian chạy B1 - 24h

 *MCP------
 *pin8 - B0 - nối relay 1
 *pin9 - B1 - nối relay 2
 *pin10 - B2 - nối relay 3
 *
 *pin0 - A0 - tín hiệu kích R1
 *pin1 - A1 - tín hiệu kích R2
 *pin2 - A2 - tín hiệu kích R3
 *
 *pin12 - B4 - tín hiệu chạy/tắt
 *pin13 - B5 - tín hiệu chạy/tắt
 *pin14 - B6 - tín hiệu chạy/tắt
 *
 */

/*
#define BLYNK_TEMPLATE_ID "TMPLMrUg6ea8"
#define BLYNK_TEMPLATE_NAME "Tram BHĐ"
#define BLYNK_AUTH_TOKEN "FW_e5wcvT49ltI7RH6qf2v68F5xksayD"
*/
#define BLYNK_TEMPLATE_ID "TMPL6WyHMhloh"
#define BLYNK_TEMPLATE_NAME "TRẠM BHĐ"
#define BLYNK_AUTH_TOKEN "8rYwP5-2nYyA6G1txMqXMamUNITRd-k9"
#define VOLUME_TOKEN "PWYW_mopMTAAnpmZOeGH3h4D4QOzZi9X"

#define BLYNK_FIRMWARE_VERSION "251228"
const char *ssid = "Cap Nuoc";
const char *password = "0919126757";
// const char* ssid = "tram bom so 4";
// const char* password = "0943950555";

#define BLYNK_PRINT Serial
#define APP_DEBUG

#pragma region
#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <SimpleKalmanFilter.h>
#include <UrlEncode.h>
const int S0pin = 14;
const int S1pin = 12;
const int S2pin = 13;
const int S3pin = 15;
//-------------------
#include "PCF8575.h"
PCF8575 pcf8575(0x20);
//-------------------
const int pincap1 = P7;
const int pinbom1 = P6;
const int pinbom2 = P5;
const int pinUSB = 16;
//-------------------
#include "EmonLib.h"
EnergyMonitor emon0, emon1, emon2, emon3, emon4;
//-------------------
#include "RTClib.h"
#include <WidgetRTC.h>
RTC_DS3231 rtc_module;
char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";
//-------------------
#include <Eeprom24C32_64.h>
#define EEPROM_ADDRESS 0x57
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
const word address = 0;
//-------------------
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
WiFiClient client;
HTTPClient http;
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/CN_MocHoa/TramBHD/.pio/build/nodemcuv2/firmware.bin"
String server_name = "http://sgp1.blynk.cloud/external/api/";
//-------------------
int dai = 450, rong = 200, dosau = 175;
int bankinh = 1150;
int percent, percent1;
float volume, volume1, dungtich, dungtichtru, dungtichvuong;
float smoothDistance; // Chuyển sang float để chính xác hơn với logic mới
long distance, distance1, t;
long m = 60 * 1000;

bool key = false, keyp = true;
bool trip0 = false, trip1 = false, trip2 = false, trip3 = false, trip4 = false;
bool protect_cap2 = false, protect_cap1 = false;
bool noti_1 = true, noti_2 = true, noti_3 = true;
bool blynk_first_connect = false;
int c, b, reboot_num;
int LLG1_1m3;
int timer_1, timer_2, timer_3, timer_4, timer_5;
int RelayState = HIGH, RelayState1 = HIGH, RelayState2 = HIGH;
int xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0, xSetAmpe3 = 0, xSetAmpe4 = 0;
float Irms0, Irms1, Irms2, Irms3, Irms4, Result1;
unsigned long int xIrms0 = 0, xIrms1 = 0, xIrms2 = 0, xIrms3 = 0, xIrms4 = 0;
unsigned long int yIrms0 = 0, yIrms1 = 0, yIrms2 = 0, yIrms3 = 0, yIrms4 = 0;

int G1_start, B1_start;
bool G1_save = false, B1_save = false;

// --- CẤU TRÚC CHO HIỆU CHUẨN ĐA ĐIỂM ---
#define MAX_CALIB_POINTS 5
struct CalibPoint {
  uint16_t adc;   // Giá trị ADC
  uint16_t value; // Giá trị quy đổi (Áp suất * 100, Mực nước cm)
};

struct Data {
  float SetAmpemax, SetAmpemin;
  float SetAmpe1max, SetAmpe1min;
  float SetAmpe2max, SetAmpe2min;
  float SetAmpe3max, SetAmpe3min;
  byte key_noti;
  int save_num;
  byte reset_day;
  int timerun_G1, timerun_B1;

  // --- DỮ LIỆU HIỆU CHUẨN MỚI ---
  CalibPoint pressure_points[MAX_CALIB_POINTS];
  uint8_t num_pressure_points;
  CalibPoint level_points[MAX_CALIB_POINTS];
  uint8_t num_level_points;
} data, dataCheck;
const struct Data dataDefault = {0}; // Khởi tạo tất cả bằng 0

#pragma endregion

WidgetTerminal terminal(V10);
WidgetTerminal volume_terminal(V31);
WidgetRTC rtc_widget;

// --- Kalman Filters & Median ---
SimpleKalmanFilter pressureKalmanFilter(1.0, 1.0, 0.01);
SimpleKalmanFilter levelKalmanFilter(2.0, 2.0, 0.01);

const int MEDIAN_WINDOW_SIZE = 5;
int median_buffer[MEDIAN_WINDOW_SIZE];
int median_buffer_index = 0;
float filtered_adc_pressure, filtered_adc_level;

BlynkTimer timer, timer1;
BLYNK_CONNECTED() {
  rtc_widget.begin();
  blynk_first_connect = true;
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void connectionstatus() {
  if ((WiFi.status() != WL_CONNECTED)) {
    Serial.println("Khong ket noi WIFI");
    WiFi.begin(ssid, password);
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
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(ssid, password);
    }
  }
  if (Blynk.connected()) {
    if (reboot_num != 0) {
      reboot_num = 0;
    }
  }
}
//------------------------
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
//-------------------------------------------------------------------
void updata() {
  String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN + "&V14=" + Result1 + "&V24=" + Irms0 + "&V25=" + Irms1 + "&V19=" + volume1 + "&V20=" + smoothDistance + "&V40=" + float(data.timerun_G1) / 1000 / 60 / 60 + "&V42=" + float(data.timerun_B1) / 1000 / 60 / 60;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}
void savedata() {
  if (memcmp(&data, &dataCheck, sizeof(dataDefault)) == 0) {
    // Serial.println("structures same no need to write to EEPROM");
  } else {
    // Serial.println("\nWrite bytes to EEPROM memory...");
    data.save_num = data.save_num + 1;
    eeprom.writeBytes(address, sizeof(dataDefault), (byte *)&data);
    memcpy(&dataCheck, &data, sizeof(dataDefault)); // Cập nhật dataCheck sau khi ghi để tránh ghi lặp lại
    Blynk.setProperty(V10, "label", "EEPROM ", data.save_num);
  }
}
void oncap1() // Dùng chân thường đóng
{
  RelayState = HIGH;
  pcf8575.digitalWrite(pincap1, RelayState);
  Blynk.virtualWrite(V0, RelayState);
}
void offcap1() {
  RelayState = LOW;
  pcf8575.digitalWrite(pincap1, RelayState);
  Blynk.virtualWrite(V0, RelayState);
}
void onbom1() // Dùng chân thường đóng
{
  RelayState1 = HIGH;
  pcf8575.digitalWrite(pinbom1, RelayState1);
  Blynk.virtualWrite(V1, RelayState1);
}
void offbom1() {
  RelayState1 = LOW;
  pcf8575.digitalWrite(pinbom1, RelayState1);
  Blynk.virtualWrite(V1, RelayState1);
}
void onbom2() // Dùng chân thường hở
{
  RelayState2 = HIGH;
  pcf8575.digitalWrite(pinbom2, !RelayState2);
  Blynk.virtualWrite(V2, RelayState2);
}
void offbom2() {
  RelayState2 = LOW;
  pcf8575.digitalWrite(pinbom2, !RelayState2);
  Blynk.virtualWrite(V2, RelayState2);
}
void hidden() {
  Blynk.setProperty(V5, "isHidden", true);
  Blynk.setProperty(V6, "isHidden", true);
  Blynk.setProperty(V7, "isHidden", true);
  Blynk.setProperty(V16, "isHidden", true);
  Blynk.setProperty(V13, "isHidden", true);
}
void visible() {
  Blynk.setProperty(V5, "isHidden", false);
  Blynk.setProperty(V6, "isHidden", false);
  Blynk.setProperty(V7, "isHidden", false);
  Blynk.setProperty(V16, "isHidden", false);
  Blynk.setProperty(V13, "isHidden", false);
}
//-------------------------------------------------------------------
void readPower() // C3 - Giếng   - I0
{
  digitalWrite(S0pin, HIGH);
  digitalWrite(S1pin, HIGH);
  digitalWrite(S2pin, LOW);
  digitalWrite(S3pin, LOW);
  float rms0 = emon0.calcIrms(1480);
  if (rms0 < 3) {
    Irms0 = 0;
    yIrms0 = 0;
    if (G1_start != 0) {
      data.timerun_G1 = data.timerun_G1 + (millis() - G1_start);
      savedata();
      G1_start = 0;
    }
  } else if (rms0 >= 3) {
    Irms0 = rms0;
    yIrms0 = yIrms0 + 1;
    if (yIrms0 > 3) {
      if (G1_start >= 0) {
        if (G1_start == 0)
          G1_start = millis();
        else if (millis() - G1_start > 60000) {
          G1_save = true;
        } else
          G1_save = false;
      }
      if ((Irms0 >= data.SetAmpemax) || (Irms0 <= data.SetAmpemin)) {
        xSetAmpe = xSetAmpe + 1;
        if ((xSetAmpe >= 2) && (keyp)) {
          offcap1();
          xSetAmpe = 0;
          trip0 = true;
          if (data.key_noti) {
            Blynk.logEvent("error", String("Bơm GIẾNG lỗi: ") + Irms0 + String(" A"));
          }
        }
      } else
        xSetAmpe = 0;
    }
  }
}
void readPower1() // C4 - Bơm 1   - I1
{
  digitalWrite(S0pin, LOW);
  digitalWrite(S1pin, LOW);
  digitalWrite(S2pin, HIGH);
  digitalWrite(S3pin, LOW);
  float rms1 = emon1.calcIrms(1480);
  if (rms1 < 3) {
    Irms1 = 0;
    yIrms1 = 0;
    if (B1_start != 0) {
      data.timerun_B1 = data.timerun_B1 + (millis() - B1_start);
      savedata();
      B1_start = 0;
    }
  } else if (rms1 >= 3) {
    Irms1 = rms1;
    yIrms1 = yIrms1 + 1;
    if (yIrms1 > 3) {
      if (B1_start >= 0) {
        if (B1_start == 0)
          B1_start = millis();
        else if (millis() - B1_start > 60000) {
          B1_save = true;
        } else
          B1_save = false;
      }
      if ((Irms1 >= data.SetAmpe1max) || (Irms1 <= data.SetAmpe1min)) {
        xSetAmpe1 = xSetAmpe1 + 1;
        if ((xSetAmpe1 >= 2) && (keyp)) {
          offbom1();
          xSetAmpe1 = 0;
          trip1 = true;
          if (data.key_noti) {
            Blynk.logEvent("error", String("Bơm CẤP 2 lỗi: ") + Irms1 + String(" A"));
          }
        }
      } else
        xSetAmpe1 = 0;
    }
  }
}
void readPower2() // C5 -   - I2
{
  Blynk.run();
  digitalWrite(S0pin, HIGH);
  digitalWrite(S1pin, LOW);
  digitalWrite(S2pin, LOW);
  digitalWrite(S3pin, LOW);
  float rms2 = emon2.calcIrms(740);
  if (rms2 < 1) {
    Irms2 = 0;
    yIrms2 = 0;
  } else if (rms2 >= 1) {
    Irms2 = rms2;
    yIrms2 = yIrms2 + 1;
    if ((yIrms2 > 3) && ((Irms2 >= data.SetAmpe2max) || (Irms2 <= data.SetAmpe2min))) {
      xSetAmpe2 = xSetAmpe2 + 1;
      if ((xSetAmpe2 >= 2) && (keyp)) {
        offbom1();
        xSetAmpe2 = 0;
        trip2 = true;
        if (data.key_noti) {
          Blynk.logEvent("error", String("Bơm 1 lỗi: ") + Irms2 + String(" A"));
        }
      }
    } else {
      xSetAmpe2 = 0;
    }
  }
  // Blynk.virtualWrite(V26, Irms2);
}
void readPower3() // C6 -   - I3
{
  digitalWrite(S0pin, LOW);
  digitalWrite(S1pin, HIGH);
  digitalWrite(S2pin, LOW);
  digitalWrite(S3pin, LOW);
  float rms3 = emon3.calcIrms(740);
  if (rms3 < 1) {
    Irms3 = 0;
    yIrms3 = 0;
  } else if (rms3 >= 1) {
    Irms3 = rms3;
    yIrms3 = yIrms3 + 1;
    if ((yIrms3 > 3) && ((Irms3 >= data.SetAmpe3max) || (Irms3 <= data.SetAmpe3min))) {
      xSetAmpe3 = xSetAmpe3 + 1;
      if ((xSetAmpe3 >= 3) && (keyp)) {
        offcap1();
        trip3 = true;
        xSetAmpe3 = 0;
        if (data.key_noti) {
          Blynk.logEvent("error", String("Giếng lỗi: ") + Irms3 + String(" A"));
        }
      }
    } else {
      xSetAmpe3 = 0;
    }
  }
  // Blynk.virtualWrite(V27, Irms3);
}
//-------------------
void up_timerun_motor() {
  String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN + "&V41=" + float(data.timerun_G1) / 1000 / 60 / 60 + "&V43=" + float(data.timerun_B1) / 1000 / 60 / 60;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}
void time_run_motor() {
  if (blynk_first_connect) {
    if (data.reset_day != day()) {
      if (Blynk.connected()) {
        up_timerun_motor();
        data.timerun_G1 = 0;
        data.timerun_B1 = 0;
        data.reset_day = day();
        savedata();
      }
    }
  }
  if (G1_save || B1_save) {
    if (G1_start != 0) {
      data.timerun_G1 = data.timerun_G1 + (millis() - G1_start);
      G1_start = millis();
      G1_save = false;
    }
    if (B1_start != 0) {
      data.timerun_B1 = data.timerun_B1 + (millis() - B1_start);
      B1_start = millis();
      B1_save = false;
    }
    savedata();
  }
}
//-------------------------------------------------------------------
// --- CÁC HÀM HỖ TRỢ HIỆU CHUẨN ---
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
  if (p2->adc == p1->adc)
    return p1->value;
  return p1->value + (current_adc - p1->adc) * (float)(p2->value - p1->value) / (p2->adc - p1->adc);
}

void readPressure() // C2 - Ap Luc
{
  digitalWrite(S0pin, LOW);
  digitalWrite(S1pin, HIGH);
  digitalWrite(S2pin, LOW);
  digitalWrite(S3pin, LOW);
  int raw_adc = analogRead(A0);

  filtered_adc_pressure = pressureKalmanFilter.updateEstimate(raw_adc);
  float val = interpolate(filtered_adc_pressure, data.pressure_points, data.num_pressure_points);
  Result1 = val / 100.0f; // Quy đổi về Bar
  Result1 = constrain(Result1, 0.0f, 10.0f);
}
//-------------------------------------------------------------------
void readWaterLevel() { // Thay thế MeasureCmForSmoothing
  digitalWrite(S0pin, HIGH);
  digitalWrite(S1pin, LOW);
  digitalWrite(S2pin, LOW);
  digitalWrite(S3pin, LOW);
  int raw_adc = analogRead(A0);

  // 1. Median Filter
  median_buffer[median_buffer_index] = raw_adc;
  median_buffer_index = (median_buffer_index + 1) % MEDIAN_WINDOW_SIZE;
  int sorted_buffer[MEDIAN_WINDOW_SIZE];
  memcpy(sorted_buffer, median_buffer, sizeof(median_buffer));
  int median_value = getMedian(sorted_buffer, MEDIAN_WINDOW_SIZE);

  // 2. Kalman Filter
  filtered_adc_level = levelKalmanFilter.updateEstimate(median_value);

  // 3. Interpolate
  smoothDistance = interpolate(filtered_adc_level, data.level_points, data.num_level_points);
  smoothDistance = constrain(smoothDistance, 0.0f, (float)dosau * 1.5f);

  // 4. Tính thể tích
  dungtichtru = (smoothDistance * 3.14 * bankinh * bankinh) / 100000000;
  dungtichvuong = (dai * smoothDistance * rong) / 1000000;
  volume1 = dungtichtru + dungtichvuong;
}
//-------------------------------------------------------------------
void rtctime() {
  DateTime now = rtc_module.now();
  if (blynk_first_connect == true) {
    if ((now.day() != day()) || (now.hour() != hour()) || ((now.minute() - minute() > 2) || (minute() - now.minute() > 2))) {
      rtc_module.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
    }
  }
  Blynk.virtualWrite(V9, daysOfTheWeek[now.dayOfTheWeek()], ", ", now.day(), "/", now.month(), "/", now.year(), " - ", now.hour(), ":", now.minute(), ":", now.second());

  // int nowtime = (now.hour() * 3600 + now.minute() * 60);
}
//-------------------------------------------------------------------
BLYNK_WRITE(V0) // Giếng
{
  if ((key) && (!trip0)) {
    if (param.asInt() == LOW) {
      offcap1();
    } else
      oncap1();
  } else {
    Blynk.virtualWrite(V0, RelayState);
  }
}
BLYNK_WRITE(V1) // Bơm 1
{
  if ((key) && (!trip1)) {
    if (param.asInt() == LOW) {
      offbom1();
    } else
      onbom1();
  } else {
    Blynk.virtualWrite(V1, RelayState1);
  }
}
BLYNK_WRITE(V2) // Bơm 2
{
  if ((key) && (!trip3)) {
    if (param.asInt() == LOW) {
      offbom2();
    } else
      onbom2();
  } else {
    Blynk.virtualWrite(V2, RelayState2);
  }
}
/*
BLYNK_WRITE(V3) // Chọn chế độ Cấp 2
{
  if (key) {
    switch (param.asInt()) {
    case 0:
    { // Man
      data.mode_cap2 = 0;
      break;
    }
    case 1:
    { // Bơm 1
      data.mode_cap2 = 1;
      break;
    }
    case 2:
    { // Bơm 2
      data.mode_cap2 = 2;
      break;
    }
    case 3:
    { // Bơm 1 + 2
      data.mode_cap2 = 3;
      break;
    }
    }
  }
  else
    Blynk.virtualWrite(V3, data.mode_cap2);
}
*/
BLYNK_WRITE(V5) // Chon máy cài đặt bảo vệ
{
  switch (param.asInt()) {
  case 0: { // ...
    c = 0;
    Blynk.virtualWrite(V6, 0);
    Blynk.virtualWrite(V7, 0);
    break;
  }
  case 1: { // Giếng
    c = 1;
    Blynk.virtualWrite(V6, data.SetAmpemin);
    Blynk.virtualWrite(V7, data.SetAmpemax);
    break;
  }
  case 2: { // Bơm 1
    c = 2;
    Blynk.virtualWrite(V6, data.SetAmpe1min);
    Blynk.virtualWrite(V7, data.SetAmpe1max);
    break;
  }
  }
}
BLYNK_WRITE(V6) // min
{
  if (key) {
    if (c == 1) {
      data.SetAmpemin = param.asInt();
    } else if (c == 2) {
      data.SetAmpe1min = param.asInt();
    }
  } else {
    Blynk.virtualWrite(V6, 0);
  }
}
BLYNK_WRITE(V7) // max
{
  if (key) {
    if (c == 1) {
      data.SetAmpemax = param.asInt();
    } else if (c == 2) {
      data.SetAmpe1max = param.asInt();
    }
  } else {
    Blynk.virtualWrite(V7, 0);
  }
}
BLYNK_WRITE(V10) // String
{
  String dataS = param.asStr();
  if (dataS == "help") {
    terminal.clear();
    Blynk.virtualWrite(V10,
                       "--- DANH SÁCH LỆNH ---\n"
                       "BHD       : Kich hoat che do cai dat (15s)\n"
                       "active    : Kich hoat che do cai dat (vo han)\n"
                       "deactive  : Thoat che do cai dat\n"
                       "save      : Luu tat ca cai dat\n"
                       "reset     : Xoa trang thai loi\n"
                       "rst       : Khoi dong lai thiet bi\n"
                       "update    : Cap nhat firmware (OTA)\n"
                       "--- HIỆU CHUẨN ---\n"
                       "calib     : Xem thong so calib\n"
                       "pre_X.X   : Calib ap suat X.X bar\n"
                       "level_YYY : Calib muc nuoc YYY cm\n"
                       "pre_clear : Xoa calib ap suat\n"
                       "level_clear : Xoa calib muc nuoc\n");
  } else if ((dataS == "BHD") || (dataS == "bhd")) {
    terminal.clear();
    key = true;
    Blynk.virtualWrite(V10, "Đơn vị vận hành: 'Mộc Hóa'\nKích hoạt trong 15s\n");
    timer1.setTimeout(15000, []() {
      key = false;
      terminal.clear();
    });
  } else if (dataS == "active") {
    terminal.clear();
    key = true;
    visible();
    Blynk.virtualWrite(V10, "KHÔNG sử dụng phần mềm cho đến khi thông báo này mất.\n");
  } else if (dataS == "deactive") {
    terminal.clear();
    key = false;
    hidden();
    Blynk.virtualWrite(V10, "Ok!\nNhập mã để điều khiển!\n");
  } else if (dataS == "save") {
    terminal.clear();
    savedata();
    Blynk.virtualWrite(V10, "Đã lưu cài đặt.\n");
  } else if (dataS == "reset") {
    terminal.clear();
    trip0 = false;
    trip1 = false;
    trip2 = false;
    trip3 = false;
    pcf8575.digitalWrite(pincap1, HIGH);
    pcf8575.digitalWrite(pinbom1, HIGH);
    Blynk.virtualWrite(V10, "Đã RESET! \nNhập mã để điều khiển!\n");
  } else if (dataS == "rst") {
    Blynk.virtualWrite(V10, "MODULE KHỞI ĐỘNG LẠI SAU 3S");
    delay(3000);
    ESP.restart();
  } else if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V10, "UPDATE FIRMWARE...");
    update_fw();
  } else if (dataS == "save_num") {
    terminal.clear();
    Blynk.virtualWrite(V10, "Số lần ghi EEPROM: ", data.save_num);
  } else if (dataS == "calib") {
    terminal.clear();
    Blynk.virtualWrite(V10, "--- THÔNG TIN HIỆU CHUẨN ---\n");

    // In thông tin áp suất
    Blynk.virtualWrite(V10, "[CẢM BIẾN ÁP SUẤT]\n");
    char buff[100];
    snprintf(buff, sizeof(buff), " - Số điểm: %d/%d\n", data.num_pressure_points, MAX_CALIB_POINTS);
    Blynk.virtualWrite(V10, buff);
    for (uint8_t i = 0; i < data.num_pressure_points; i++) {
      snprintf(buff, sizeof(buff), " #%d: ADC=%d -> %.2f bar\n", i + 1, data.pressure_points[i].adc, data.pressure_points[i].value / 100.0f);
      Blynk.virtualWrite(V10, buff);
    }
    snprintf(buff, sizeof(buff), " - ADC đã lọc hiện tại: %.2f\n", filtered_adc_pressure);
    Blynk.virtualWrite(V10, buff);
    snprintf(buff, sizeof(buff), " => Áp suất tính toán: %.2f bar\n", Result1);
    Blynk.virtualWrite(V10, buff);

    // In thông tin mực nước
    Blynk.virtualWrite(V10, "[CẢM BIẾN MỰC NƯỚC]\n");
    snprintf(buff, sizeof(buff), " - Số điểm: %d/%d\n", data.num_level_points, MAX_CALIB_POINTS);
    Blynk.virtualWrite(V10, buff);
    for (uint8_t i = 0; i < data.num_level_points; i++) {
      snprintf(buff, sizeof(buff), " #%d: ADC=%d -> %d cm\n", i + 1, data.level_points[i].adc, data.level_points[i].value);
      Blynk.virtualWrite(V10, buff);
    }
    snprintf(buff, sizeof(buff), " - ADC đã lọc hiện tại: %.2f\n", filtered_adc_level);
    Blynk.virtualWrite(V10, buff);
    snprintf(buff, sizeof(buff), " => Mực nước tính toán: %.1f cm\n", smoothDistance);
    Blynk.virtualWrite(V10, buff);
  } else if (dataS == "pre_clear") {
    data.num_pressure_points = 0;
    savedata();
    Blynk.virtualWrite(V10, "Cleared Pressure Calib.\n");
  } else if (dataS.startsWith("pre_")) {
    float val = dataS.substring(4).toFloat();
    CalibPoint pt;
    pt.adc = (uint16_t)round(filtered_adc_pressure);
    pt.value = (uint16_t)(val * 100);
    addOrUpdateCalibPoint(pt, data.pressure_points, data.num_pressure_points);
    savedata();
    Blynk.virtualWrite(V10, "Saved Pressure Point.\n");
  } else if (dataS == "level_clear") {
    data.num_level_points = 0;
    savedata();
    Blynk.virtualWrite(V10, "Cleared Level Calib.\n");
  } else if (dataS.startsWith("level_")) {
    float val = dataS.substring(6).toFloat();
    CalibPoint pt;
    pt.adc = (uint16_t)round(filtered_adc_level);
    pt.value = (uint16_t)val;
    addOrUpdateCalibPoint(pt, data.level_points, data.num_level_points);
    savedata();
    Blynk.virtualWrite(V10, "Saved Level Point.\n");
  } else {
    Blynk.virtualWrite(V10, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
/*
BLYNK_WRITE(V11) // Chọn thời gian chạy 2 Bơm
{
  if ((data.mode_cap2 == 1) || (data.mode_cap2 == 2)) {
    BlynkParamAllocated menu(128); // list length, in bytes
    menu.add("NGÀY CHẴN");
    menu.add("NGÀY LẺ");
    menu.add("CHÂM CLO");
    Blynk.setProperty(V11, "labels", menu);
    switch (param.asInt()) {
    case 0:
    {
      if (key) {
        data.cap2_chanle = 0;
        b = 9;
      }
      Blynk.virtualWrite(V18, data.bom_chanle_start, data.bom_chanle_stop, tz);
      break;
    }
    case 1:
    {
      if (key) {
        data.cap2_chanle = 1;
        b = 9;
      }
      Blynk.virtualWrite(V18, data.bom_chanle_start, data.bom_chanle_stop, tz);
      break;
    }
    case 2:
    { // Châm Clo
      if (key)
        b = 8;
      Blynk.virtualWrite(V18, data.clo_start, data.clo_stop, tz);
      break;
    }
    }
  }
  else if (data.mode_cap2 == 3) {
    BlynkParamAllocated menu(255); // list length, in bytes
    menu.add("BƠM 1 - LẦN 1");
    menu.add("BƠM 2 - LẦN 1");
    menu.add("BƠM 1 - LẦN 2");
    menu.add("BƠM 2 - LẦN 2");
    menu.add("BƠM 1 - LẦN 3");
    menu.add("BƠM 2 - LẦN 3");
    menu.add("BƠM 1 - LẦN 4");
    menu.add("BƠM 2 - LẦN 4");
    menu.add("CHÂM CLO");
    Blynk.setProperty(V11, "labels", menu);
    switch (param.asInt()) {
    case 0:
    { // Bơm 1 - Lần 1
      if (key)
        b = 0;
      Blynk.virtualWrite(V18, data.b1_1_start, data.b1_1_stop, tz);
      break;
    }
    case 1:
    { // Bơm 2 - Lần 1
      if (key)
        b = 1;
      Blynk.virtualWrite(V18, data.b2_1_start, data.b2_1_stop, tz);
      break;
    }
    case 2:
    { // Bơm 1 - Lần 2
      if (key)
        b = 2;
      Blynk.virtualWrite(V18, data.b1_2_start, data.b1_2_stop, tz);
      break;
    }
    case 3:
    { // Bơm 2 - Lần 2
      if (key)
        b = 3;
      Blynk.virtualWrite(V18, data.b2_2_start, data.b2_2_stop, tz);
      break;
    }
    case 4:
    { // Bơm 1 - Lần 3
      if (key)
        b = 4;
      Blynk.virtualWrite(V18, data.b1_3_start, data.b1_3_stop, tz);
      break;
    }
    case 5:
    { // Bơm 2 - Lần 3
      if (key)
        b = 5;
      Blynk.virtualWrite(V18, data.b2_3_start, data.b2_3_stop, tz);
      break;
    }
    case 6:
    { // Bơm 1 - Lần 4
      if (key)
        b = 6;
      Blynk.virtualWrite(V18, data.b1_4_start, data.b1_4_stop, tz);
      break;
    }
    case 7:
    { // Bơm 2 - Lần 4
      if (key)
        b = 7;
      Blynk.virtualWrite(V18, data.b2_4_start, data.b2_4_stop, tz);
      break;
    }
    case 8:
    { // Châm Clo
      if (key)
        b = 8;
      Blynk.virtualWrite(V18, data.clo_start, data.clo_stop, tz);
      break;
    }
    }
  }
}
*/
BLYNK_WRITE(V13) // Bảo vệ
{
  if (key) {
    int data13 = param.asInt();
    if (data13 == LOW) {
      keyp = false;
    } else {
      keyp = true;
    }
  } else
    Blynk.virtualWrite(V13, keyp);
}
BLYNK_WRITE(V16) // Thông báo
{
  if (key) {
    int data16 = param.asInt();
    if (data16 == LOW) {
      data.key_noti = false;
    } else {
      data.key_noti = true;
    }
  } else
    Blynk.virtualWrite(V16, data.key_noti);
}
/*
BLYNK_WRITE(V18) // Time input
{
  if (key) {
    TimeInputParam t(param);
    if (t.hasStartTime()) {
      if (b == 0) {
        data.b1_1_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 1) {
        data.b2_1_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 2) {
        data.b1_2_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 3) {
        data.b2_2_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 4) {
        data.b1_3_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 5) {
        data.b2_3_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 6) {
        data.b1_4_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 7) {
        data.b2_4_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 8) {
        data.clo_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 9) {
        data.bom_chanle_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
    }
    if (t.hasStopTime()) {
      if (b == 0) {
        data.b1_1_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 1) {
        data.b2_1_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 2) {
        data.b1_2_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 3) {
        data.b2_2_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 4) {
        data.b1_3_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 5) {
        data.b2_3_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 6) {
        data.b1_4_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 7) {
        data.b2_4_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 8) {
        data.clo_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 9) {
        data.bom_chanle_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
    }
  }
  else
    Blynk.virtualWrite(V18, 0);
}
*/
/*
BLYNK_WRITE(V29) // Info
{
  int hour_clo_start = data.clo_start / 3600;
  int minute_clo_start = (data.clo_start - (hour_clo_start * 3600)) / 60;
  int hour_clo_stop = data.clo_stop / 3600;
  int minute_clo_stop = (data.clo_stop - (hour_clo_stop * 3600)) / 60;
  if (param.asInt() == 1) {
    terminal.clear();
    if ((data.mode_cap2 == 0) && (data.mode_clo == 0)) {
      Blynk.virtualWrite(V10, "Chế độ bơm: Vận hành THỦ CÔNG.\nChế độ clo: TỰ ĐỘNG\nThời gian châm clo: Từ ", hour_clo_start, ":", minute_clo_start, " đến ", hour_clo_stop, ":", minute_clo_stop);
    }
    else if ((data.mode_cap2 == 0) && (data.mode_clo == 1)) {
      Blynk.virtualWrite(V10, "Chế độ bơm: Vận hành THỦ CÔNG.\nChế độ clo: THỦ CÔNG");
    }
    else if ((data.mode_cap2 == 1) || (data.mode_cap2 == 2)) {
      int hour_start = data.bom_chanle_start / 3600;
      int minute_start = (data.bom_chanle_start - (hour_start * 3600)) / 60;
      int hour_stop = data.bom_chanle_stop / 3600;
      int minute_stop = (data.bom_chanle_stop - (hour_stop * 3600)) / 60;
      if (data.mode_cap2 == 1) {
        if ((data.cap2_chanle == 0) && (data.mode_clo == 0)) {
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 1 tự động\nTắt máy vào: NGÀY CHẴN\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop, "\nChế độ clo: TỰ ĐỘNG\nThời gian châm clo: Từ ", hour_clo_start, ":", minute_clo_start, " đến ", hour_clo_stop, ":", minute_clo_stop);
        }
        else if ((data.cap2_chanle == 0) && (data.mode_clo == 1)) {
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 1 tự động\nTắt máy vào: NGÀY CHẴN\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop, "\nChế độ clo: THỦ CÔNG");
        }
        else if ((data.cap2_chanle == 1) && (data.mode_clo == 0)) {
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 1 tự động\nTắt máy vào: NGÀY LẺ\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop, "\nChế độ clo: TỰ ĐỘNG\nThời gian châm clo: Từ ", hour_clo_start, ":", minute_clo_start, " đến ", hour_clo_stop, ":", minute_clo_stop);
        }
        else if ((data.cap2_chanle == 1) && (data.mode_clo == 1)) {
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 1 tự động\nTắt máy vào: NGÀY LẺ\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop, "\nChế độ clo: THỦ CÔNG");
        }
      }
      else if (data.mode_cap2 == 2) {
        if ((data.cap2_chanle == 0) && (data.mode_clo == 0)) {
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 2 tự động\nTắt máy vào: NGÀY CHẴN\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop, "\nChế độ clo: TỰ ĐỘNG\nThời gian châm clo: Từ ", hour_clo_start, ":", minute_clo_start, " đến ", hour_clo_stop, ":", minute_clo_stop);
        }
        else if ((data.cap2_chanle == 0) && (data.mode_clo == 1)) {
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 2 tự động\nTắt máy vào: NGÀY CHẴN\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop, "\nChế độ clo: THỦ CÔNG");
        }
        else if ((data.cap2_chanle == 1) && (data.mode_clo == 0)) {
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 2 tự động\nTắt máy vào: NGÀY LẺ\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop, "\nChế độ clo: TỰ ĐỘNG\nThời gian châm clo: Từ ", hour_clo_start, ":", minute_clo_start, " đến ", hour_clo_stop, ":", minute_clo_stop);
        }
        else if ((data.cap2_chanle == 1) && (data.mode_clo == 1)) {
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 2 tự động\nTắt máy vào: NGÀY LẺ\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop, "\nChế độ clo: THỦ CÔNG");
        }
      }
    }
    else if (data.mode_cap2 == 3) {
      int hour_start_b1_1 = data.b1_1_start / 3600;
      int minute_start_b1_1 = (data.b1_1_start - (hour_start_b1_1 * 3600)) / 60;
      int hour_stop_b1_1 = data.b1_1_stop / 3600;
      int minute_stop_b1_1 = (data.b1_1_stop - (hour_stop_b1_1 * 3600)) / 60;

      int hour_start_b2_1 = data.b2_1_start / 3600;
      int minute_start_b2_1 = (data.b2_1_start - (hour_start_b2_1 * 3600)) / 60;
      int hour_stop_b2_1 = data.b2_1_stop / 3600;
      int minute_stop_b2_1 = (data.b2_1_stop - (hour_stop_b2_1 * 3600)) / 60;

      int hour_start_b1_2 = data.b1_2_start / 3600;
      int minute_start_b1_2 = (data.b1_2_start - (hour_start_b1_2 * 3600)) / 60;
      int hour_stop_b1_2 = data.b1_2_stop / 3600;
      int minute_stop_b1_2 = (data.b1_2_stop - (hour_stop_b1_2 * 3600)) / 60;

      int hour_start_b2_2 = data.b2_2_start / 3600;
      int minute_start_b2_2 = (data.b2_2_start - (hour_start_b2_2 * 3600)) / 60;
      int hour_stop_b2_2 = data.b2_2_stop / 3600;
      int minute_stop_b2_2 = (data.b2_2_stop - (hour_stop_b2_2 * 3600)) / 60;

      int hour_start_b1_3 = data.b1_3_start / 3600;
      int minute_start_b1_3 = (data.b1_3_start - (hour_start_b1_3 * 3600)) / 60;
      int hour_stop_b1_3 = data.b1_3_stop / 3600;
      int minute_stop_b1_3 = (data.b1_3_stop - (hour_stop_b1_3 * 3600)) / 60;

      int hour_start_b2_3 = data.b2_3_start / 3600;
      int minute_start_b2_3 = (data.b2_3_start - (hour_start_b2_3 * 3600)) / 60;
      int hour_stop_b2_3 = data.b2_3_stop / 3600;
      int minute_stop_b2_3 = (data.b2_3_stop - (hour_stop_b2_3 * 3600)) / 60;

      int hour_start_b1_4 = data.b1_4_start / 3600;
      int minute_start_b1_4 = (data.b1_4_start - (hour_start_b1_4 * 3600)) / 60;
      int hour_stop_b1_4 = data.b1_4_stop / 3600;
      int minute_stop_b1_4 = (data.b1_4_stop - (hour_stop_b1_4 * 3600)) / 60;

      int hour_start_b2_4 = data.b2_4_start / 3600;
      int minute_start_b2_4 = (data.b2_4_start - (hour_start_b2_4 * 3600)) / 60;
      int hour_stop_b2_4 = data.b2_4_stop / 3600;
      int minute_stop_b2_4 = (data.b2_4_stop - (hour_stop_b2_4 * 3600)) / 60;

      if (data.mode_clo == 0) {
        Blynk.virtualWrite(V10, "Mode: Auto\nTimer:\nPump 1: ", hour_start_b1_1, "h", minute_start_b1_1, " -> ", hour_stop_b1_1, "h", minute_stop_b1_1, "\nPump 2: ", hour_start_b2_1, "h", minute_start_b2_1, " -> ", hour_stop_b2_1, "h", minute_stop_b2_1, "\nPump 1: ", hour_start_b1_2, "h", minute_start_b1_2, " -> ", hour_stop_b1_2, "h", minute_stop_b1_2, "\nPump 2: ", hour_start_b2_2, "h", minute_start_b2_2, " -> ", hour_stop_b2_2, "h", minute_stop_b2_2, "\nPump 1: ", hour_start_b1_3, "h", minute_start_b1_3, " -> ", hour_stop_b1_3, "h", minute_stop_b1_3, "\nPump 2: ", hour_start_b2_3, "h", minute_start_b2_3, " -> ", hour_stop_b2_3, "h", minute_stop_b2_3, "\nPump 1: ", hour_start_b1_4, "h", minute_start_b1_4, " -> ", hour_stop_b1_4, "h", minute_stop_b1_4, "\nPump 2: ", hour_start_b2_4, "h", minute_start_b2_4, " -> ", hour_stop_b2_4, "h", minute_stop_b2_4, "\nClo mode: Auto\nTimer: ", hour_clo_start, ":", minute_clo_start, " -> ", hour_clo_stop, ":", minute_clo_stop);
      }
      else if (data.mode_clo == 1) {
        Blynk.virtualWrite(V10, "Mode: Auto\nTimer:\nPump 1: ", hour_start_b1_1, "h", minute_start_b1_1, " -> ", hour_stop_b1_1, "h", minute_stop_b1_1, "\nPump 2: ", hour_start_b2_1, "h", minute_start_b2_1, " -> ", hour_stop_b2_1, "h", minute_stop_b2_1, "\nPump 1: ", hour_start_b1_2, "h", minute_start_b1_2, " -> ", hour_stop_b1_2, "h", minute_stop_b1_2, "\nPump 2: ", hour_start_b2_2, "h", minute_start_b2_2, " -> ", hour_stop_b2_2, "h", minute_stop_b2_2, "\nPump 1: ", hour_start_b1_3, "h", minute_start_b1_3, " -> ", hour_stop_b1_3, "h", minute_stop_b1_3, "\nPump 2: ", hour_start_b2_3, "h", minute_start_b2_3, " -> ", hour_stop_b2_3, "h", minute_stop_b2_3, "\nPump 1: ", hour_start_b1_4, "h", minute_start_b1_4, " -> ", hour_stop_b1_4, "h", minute_stop_b1_4, "\nPump 2: ", hour_start_b2_4, "h", minute_start_b2_4, " -> ", hour_stop_b2_4, "h", minute_stop_b2_4, "\nClo mode: Man");
      }

    }
  }
  else {
    terminal.clear();
  }
  timer.restartTimer(timer_1);
  timer.restartTimer(timer_2);
}
*/
BLYNK_WRITE(V31) {
  String dataS = param.asStr();
  if ((dataS == "rst") || (dataS == "update") || (dataS == "rst_vl") || (dataS == "i2c")) {
    volume_terminal.clear();
    String server_path = server_name + "batch/update?token=" + VOLUME_TOKEN +
                         "&V0=" + urlEncode(dataS);
    http.begin(client, server_path.c_str());
    http.GET();
    http.end();
  }
}
BLYNK_WRITE(V32) // Lưu lượng G1_1m3
{
  LLG1_1m3 = param.asInt();
}
//-------------------------------------------------------------------
void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(300000);
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(5000);
  //---------------------------------------------------------------------------------
  pinMode(S0pin, OUTPUT);
  pinMode(S1pin, OUTPUT);
  pinMode(S2pin, OUTPUT);
  pinMode(S3pin, OUTPUT);

  emon0.current(A0, 107);
  emon1.current(A0, 90);
  // emon2.current(A0, 96);
  // emon3.current(A0, 100);
  // emon4.current(A0, 85);

  rtc_module.begin();

  pcf8575.pinMode(pincap1, OUTPUT);
  pcf8575.pinMode(pinbom1, OUTPUT);
  pcf8575.pinMode(pinbom2, OUTPUT);
  pcf8575.pinMode(P4, OUTPUT);
  pcf8575.pinMode(P3, OUTPUT);
  pcf8575.begin();
  pcf8575.digitalWrite(pincap1, HIGH); // Bom 1
  pcf8575.digitalWrite(pinbom1, HIGH); // Bom 2
  pcf8575.digitalWrite(pinbom2, HIGH);
  pcf8575.digitalWrite(P4, HIGH);
  pcf8575.digitalWrite(P3, HIGH);

  eeprom.initialize();
  eeprom.readBytes(address, sizeof(dataDefault), (byte *)&data);
  memcpy(&dataCheck, &data, sizeof(dataDefault)); // Đồng bộ dataCheck với dữ liệu vừa đọc

  terminal.clear();

  timer_2 = timer.setInterval(253L, []() {
    readPressure();
    readWaterLevel();
  });
  timer_1 = timer.setInterval(1283L, []() {
    readPower();
    readPower1();
    updata();
    timer.restartTimer(timer_1);
    timer.restartTimer(timer_2);
  });
  timer_5 = timer.setInterval(15006L, []() {
    rtctime();
    time_run_motor();
    timer.restartTimer(timer_1);
    timer.restartTimer(timer_2);
  });
  timer.setInterval(900005L, []() {
    connectionstatus();
    timer.restartTimer(timer_1);
    timer.restartTimer(timer_2);
  });
}

void loop() {
  ESP.wdtFeed();
  Blynk.run();
  timer.run();
  timer1.run();
}