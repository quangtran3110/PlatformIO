/*-Color = 48c9b0
 *V0 - Btn Cap 1
 *V1 - Btn Cap 2
 *V2 - I0 - Cap 1
 *V3 - I1 - Cap 2
 *V4 - Ap Luc
 *V5 - Con lai
 *V6 - The tich
 *V7 - Che do Cap 2
 *V8 - Chon may bao ve
 *V9 - min A
 *V10 - max A
 *V11 - String
 *V12 -
 *V13 - timeinput
 *V14 - bao ve
 *V15 - thong bao
 *V16 - Rửa lọc
 *V17 - info
 *V18 - Btn Nen Khi
 *V19 - I2 - Nen Khi
 *V20 - date/time
 *V21 -
 *V22 -
 *V23 -
 *V24 - LLG1_1m3
 *V25 - LLG1_24h
 *V26 - LLG1_RL
 *V27 - Status_VLG1
 *V28 - CLO input
 *V29 - Clo
 *V30 - check clo
 */

#define BLYNK_TEMPLATE_ID "TMPL67KBbwuNv"
#define BLYNK_TEMPLATE_NAME "TRẠM SỐ 1"
#define BLYNK_AUTH_TOKEN "SZfJItqPgAVkiB8VdBuzyl5f94BU3E4x"

#define BLYNK_FIRMWARE_VERSION "260813"
//------------------
#include "EmonLib.h"
#include "PCF8575.h"
#include "RTClib.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <Eeprom24C32_64.h>
#include <I2C_eeprom_cyclic_store.h>
#include <SPI.h>
#include <SimpleKalmanFilter.h>
#include <WiFiClientSecure.h>
#include <WidgetRTC.h>
#include <Wire.h>
//-----------------------------
#define APP_DEBUG
#define BLYNK_PRINT Serial
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/Tram_So_1/.pio/build/nodemcuv2/firmware.bin"
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
const char *ssid = "Tram Bom So 1";
const char *password = "0943950555";
const int pin_G1 = P1;
const int pin_B1 = P2;
const int pin_NK1 = P3;
const int pin_rst = P4;

const int S0pin = P15;
const int S1pin = P14;
const int S2pin = P13;
const int S3pin = P12;
//-----------------------------
char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";

byte calib_pre_sta = 0, range_pre = 10;
bool key = false, keytank = true;
bool trip0 = false, trip1 = false, trip2 = false, trip_mcp = false;
bool key_memory = true, timer_I_status;
bool key_bom = true, key_gieng = true;
bool blynk_first_connect = false;

float volume, percent, percent1, dungtich, smoothDistance, smoothed_adc_level;
float Irms0, Irms1, Irms2, value, clo_cache = 0;

long distance, distance1, t;

unsigned long int xIrms0 = 0, xIrms1 = 0, xIrms2 = 0;
unsigned long int yIrms0 = 0, yIrms1 = 0, yIrms2 = 0;
unsigned long rest_time = 0, dem_bom = 0, dem_cap1 = 0;
uint32_t timestamp;

float sensor_pre, Result;
float filtered_adc_pressure, filtered_adc_level;
int LLG1_1m3;
int reboot_num = 0;
int c, b, i = 0;
int xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0;
int btnState = HIGH, btnState1 = HIGH;
int timer_I;
int dai = 566;
int rong = 297;
int dosau = 130;

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
#define DATA_VERSION 1
#define MAX_CALIB_POINTS 5

struct CalibPoint {
  uint16_t adc;   // Giá trị ADC (0-1023)
  uint16_t value; // Giá trị quy đổi (Áp suất * 100, Mực nước cm)
};

struct Data {
  uint8_t version;
  byte SetAmpemax, SetAmpemin;
  byte SetAmpe1max, SetAmpe1min;
  byte SetAmpe2max, SetAmpe2min;
  byte mode_cap2;
  int bom_chanle_start, bom_chanle_stop;
  int bom_moingay_start, bom_moingay_stop;
  byte status_g1, status_b1, status_nk1;
  int save_num;
  byte key_noti, rualoc;
  float clo;
  int time_clo, LLG1_RL;
  byte key_protect;
  int phao_max, phao_min;
  byte pre_zero;
  int pre_num;
  CalibPoint pressure_points[MAX_CALIB_POINTS];
  uint8_t num_pressure_points;
  CalibPoint level_points[MAX_CALIB_POINTS];
  uint8_t num_level_points;
} data, dataCheck;
I2C_eeprom_cyclic_store<Data> cs;

WidgetTerminal terminal(V11);
WidgetRTC rtc_widget;
BlynkTimer timer, timer1;
BLYNK_CONNECTED() {
  rtc_widget.begin();
  blynk_first_connect = true;
}
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
void up() {
  String server_path = server_name + "/batch/update?token=" + BLYNK_AUTH_TOKEN +
                       "&V2=" + Irms0 +
                       "&V3=" + Irms1 +
                       "&V4=" + float(Result) +
                       "&V5=" + smoothDistance +
                       "&V6=" + volume +
                       "&V19=" + Irms2;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
  if (calib_pre_sta == 1) {
    Blynk.virtualWrite(V21, sensor_pre);
  }
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
void on_cap1() {
  if (data.status_g1 != HIGH) {
    data.status_g1 = HIGH;
    savedata();
  }
  if (!trip0) {
    pcf8575_1.digitalWrite(pin_G1, data.status_g1);
  }
  Blynk.virtualWrite(V0, data.status_g1);
}
void off_cap1() {
  if (data.status_g1 != LOW) {
    data.status_g1 = LOW;
    savedata();
  }
  yIrms0 = 0;
  pcf8575_1.digitalWrite(pin_G1, data.status_g1);
  Blynk.virtualWrite(V0, data.status_g1);
}
void on_bom() {
  if (data.status_b1 != HIGH) {
    data.status_b1 = HIGH;
    savedata();
  }
  if (!trip1) {
    pcf8575_1.digitalWrite(pin_B1, data.status_b1);
  }
  Blynk.virtualWrite(V1, data.status_b1);
}
void off_bom() {
  if (data.status_b1 != LOW) {
    data.status_b1 = LOW;
    savedata();
  }
  yIrms1 = 0;
  pcf8575_1.digitalWrite(pin_B1, data.status_b1);
  Blynk.virtualWrite(V1, data.status_b1);
}
void on_nenkhi() {
  if (data.status_nk1 != HIGH) {
    data.status_nk1 = HIGH;
    savedata();
  }
  if (!trip2) {
    pcf8575_1.digitalWrite(pin_NK1, data.status_nk1);
  }
  Blynk.virtualWrite(V18, data.status_nk1);
}
void off_nenkhi() {
  if (data.status_nk1 != LOW) {
    data.status_nk1 = LOW;
    savedata();
  }
  pcf8575_1.digitalWrite(pin_NK1, data.status_nk1);
  Blynk.virtualWrite(V18, data.status_nk1);
}
void hidden() {
  Blynk.setProperty(V12, V13, V15, V14, V10, V9, V8, "isHidden", true);
}
void visible() {
  Blynk.setProperty(V12, V13, V15, V14, V10, V9, V8, "isHidden", false);
}
void off_time() {
  rest_time = rest_time + 1;
  if (Irms1 != 0) {
    off_bom();
  }
  if (Irms0 != 0) {
    off_cap1();
  }
}
void on_time() {
  rest_time = 0;
  if ((!trip1) && (Irms1 == 0)) {
    on_bom();
  }
  if ((!trip0) && (Irms0 == 0)) {
    on_cap1();
  }
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
  if ((smoothDistance - dosau >= 20) && (data.key_noti) && (keytank)) {
    Blynk.logEvent("info", String("Nước trong bể cao vượt mức ") + (smoothDistance - dosau) + String(" cm"));
    keytank = false;
    timer1.setTimeout(1800000L, []() { keytank = true; });
  }
}
//-------------------------------------------------------------------
void readPower() { // C2 - Cấp 1  - I0
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
      if ((rest_time > 2) && (data.key_noti) && (key_gieng)) {
        key_gieng = false;
        Blynk.logEvent("info", String("Bơm Giếng không tắt. Xin kiểm tra."));
        timer1.setInterval(900000L, []() { // 15p
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
void readPower1() { // C3 - Bơm    - I1
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms1 = emon1.calcIrms(1480);
  if (rms1 < 2) {
    Irms1 = 0;
    if ((data.status_b1 == HIGH) && (yIrms1 > 3)) {
      if (smoothDistance < data.phao_min) {
        yIrms1 = 0;
      } else if (smoothDistance > data.phao_min) {
        xIrms1++;
        if ((xIrms1 > 3) && (data.key_protect)) {
          off_bom();
          trip1 = true;
          xIrms1 = 0;
          if (data.key_noti)
            Blynk.logEvent("error", String("Bơm lỗi\nKhông đo được DÒNG ĐIỆN"));
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
      if ((rest_time > 2) && (data.key_noti) && (key_bom)) {
        key_bom = false;
        Blynk.logEvent("info", String("Bơm cấp 2 không tắt. Xin kiểm tra."));
        timer1.setInterval(900000L, []() { // 15p
          key_bom = true;
        });
      }
      if ((Irms1 >= data.SetAmpe1max) || (Irms1 <= data.SetAmpe1min)) {
        xSetAmpe1 = xSetAmpe1 + 1;
        if ((xSetAmpe1 >= 2) && (data.key_protect)) {
          off_bom();
          xSetAmpe1 = 0;
          trip1 = true;
          if (data.key_noti) {
            Blynk.logEvent("error", String("Cấp 2 lỗi: ") + Irms1 + String(" A"));
          }
        }
      } else {
        xSetAmpe1 = 0;
      }
    }
  }
  // Blynk.virtualWrite(V3, Irms1);
}
void readPower2() { // C4 - Nen khi- I2
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
//-------------------------------------------------------------------
void rtctime() {
  DateTime now = rtc_module.now();
  if (blynk_first_connect == true) {
    if ((now.day() != day()) || (now.hour() != hour()) || ((now.minute() - minute() > 2) || (minute() - now.minute() > 2))) {
      rtc_module.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
    }
  }
  Blynk.virtualWrite(V20, daysOfTheWeek[now.dayOfTheWeek()], ", ", now.day(), "/", now.month(), "/", now.year(), " - ", now.hour(), ":", now.minute(), ":", now.second());
  timestamp = now.unixtime();

  int nowtime = (now.hour() * 3600 + now.minute() * 60);

  if (data.mode_cap2 == 1) { // Chạy Tu Dong
    if (data.bom_moingay_start > data.bom_moingay_stop) {
      if ((nowtime > data.bom_moingay_stop) && (nowtime < data.bom_moingay_start)) {
        off_time();
      }
      if ((nowtime < data.bom_moingay_stop) || (nowtime > data.bom_moingay_start)) {
        on_time();
      }
    } else if (data.bom_moingay_start < data.bom_moingay_stop) {
      if ((nowtime > data.bom_moingay_stop) || (nowtime < data.bom_moingay_start)) {
        off_time();
      }
      if ((nowtime < data.bom_moingay_stop) && (nowtime > data.bom_moingay_start)) {
        on_time();
      }
    } else if (data.bom_moingay_start == data.bom_moingay_stop) {
      on_time();
    }
  }
}
void i2c_scaner() {
  byte error, address;
  int nDevices;
  String stringOne;

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
//-------------------------------------------------------------------
BLYNK_WRITE(V0) // Gieng
{
  if ((key) && (!trip0)) {
    if (param.asInt() == LOW) {
      off_cap1();
    } else {
      on_cap1();
    }
  }
  Blynk.virtualWrite(V0, data.status_g1);
}
BLYNK_WRITE(V1) // Bơm
{
  if ((key) && (!trip1)) {
    if (param.asInt() == LOW) {
      off_bom();
    } else {
      on_bom();
    }
  }
  Blynk.virtualWrite(V1, data.status_b1);
}
BLYNK_WRITE(V7) // Chọn chế độ Cấp 2
{
  if (key) {
    switch (param.asInt()) {
    case 0: { // Man
      data.mode_cap2 = 0;
      break;
    }
    case 1: { // Auto
      data.mode_cap2 = 1;
      break;
    }
    }
  } else
    Blynk.virtualWrite(V7, data.mode_cap2);
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
  case 1: { // Gieng
    c = 1;
    Blynk.virtualWrite(V9, data.SetAmpemin);
    Blynk.virtualWrite(V10, data.SetAmpemax);
    break;
  }
  case 2: { // Bom
    c = 2;
    Blynk.virtualWrite(V9, data.SetAmpe1min);
    Blynk.virtualWrite(V10, data.SetAmpe1max);
    break;
  }
  case 3: { // Nén khí
    c = 3;
    Blynk.virtualWrite(V9, data.SetAmpe2min);
    Blynk.virtualWrite(V10, data.SetAmpe2max);
    break;
  }
  case 4: { // Phao
    c = 4;
    Blynk.virtualWrite(V9, data.phao_min);
    Blynk.virtualWrite(V10, data.phao_max);
    break;
  }
  }
}
BLYNK_WRITE(V9) // min
{
  if (key) {
    if (c == 1) {
      data.SetAmpemin = param.asInt();
    } else if (c == 2) {
      data.SetAmpe1min = param.asInt();
    } else if (c == 3) {
      data.SetAmpe2min = param.asInt();
    } else if (c == 4) {
      data.phao_min = param.asInt();
    }
  } else {
    Blynk.virtualWrite(V9, 0);
  }
}
BLYNK_WRITE(V10) // max
{
  if (key) {
    if (c == 1) {
      data.SetAmpemax = param.asInt();
    } else if (c == 2) {
      data.SetAmpe1max = param.asInt();
    } else if (c == 3) {
      data.SetAmpe2max = param.asInt();
    } else if (c == 4) {
      data.phao_max = param.asInt();
    }
  } else {
    Blynk.virtualWrite(V10, 0);
  }
}
BLYNK_WRITE(V11) // String
{
  String dataS = param.asStr();
  if (dataS == "ts1") {
    terminal.clear();
    key = true;
    Blynk.virtualWrite(V11, "Người vận hành: 'V.Tài'\nKích hoạt trong 15s\n");
    timer1.setTimeout(15000, []() {
      key = false;
      terminal.clear();
    });
  } else if (dataS == "active") {
    terminal.clear();
    key = true;
    visible();
    Blynk.virtualWrite(V11, "KHÔNG sử dụng phần mềm cho đến khi thông báo này mất.\n");
  } else if (dataS == "deactive") {
    terminal.clear();
    key = false;
    hidden();
    Blynk.virtualWrite(V11, "Ok!\nNhập mã để điều khiển!\n");
  } else if (dataS == "save") {
    terminal.clear();
    savedata();
    Blynk.virtualWrite(V11, "Đã lưu cài đặt.\n");
  } else if (dataS == "help") {
    terminal.clear();
    Blynk.virtualWrite(V11, "--- DANH SÁCH LỆNH ---\n"
                            "pre_X.X   : Calib áp suất tại X.X bar\n"
                            "level_YYY : Calib mực nước tại YYY cm\n"
                            "calib_pre : Xem thong tin calib ap suat\n"
                            "calib_level : Xem thong tin calib muc nuoc\n"
                            "pre_clear : Xóa calib áp suất\n"
                            "level_clear : Xóa calib mực nước\n"
                            "save      : Luu cai dat\n");
  } else if (dataS == "reset") {
    terminal.clear();
    trip2 = false;
    trip1 = false;
    trip0 = false;
    on_cap1();
    Blynk.virtualWrite(V11, "Đã RESET! \nNhập mã để điều khiển!\n");
  } else if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V11, "UPDATE FIRMWARE...");
    update_fw();
  } else if (dataS == "rst") {
    terminal.clear();
    Blynk.virtualWrite(V11, "ESP khởi động lại sau 3s");
    delay(3000);
    ESP.restart();
  } else if ((dataS == "ok") || (dataS == "Ok") || (dataS == "OK") || (dataS == "oK")) {
    if (clo_cache > 0) {
      data.clo = clo_cache;
      clo_cache = 0;
      data.time_clo = timestamp;
      Blynk.virtualWrite(V29, data.clo);
      savedata();
      terminal.clear();
      Blynk.virtualWrite(V11, "Đã lưu - CLO:", data.clo, "kg");
    }
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
  } else if (dataS.startsWith("pre_")) {
    float val = dataS.substring(4).toFloat();
    CalibPoint pt;
    pt.adc = (uint16_t)round(filtered_adc_pressure);
    pt.value = (uint16_t)(val * 100);
    addOrUpdateCalibPoint(pt, data.pressure_points, data.num_pressure_points);
    savedata();
    Blynk.virtualWrite(V11, "Đã lưu điểm áp suất.\n");
  } else if (dataS.startsWith("level_")) {
    float val = dataS.substring(6).toFloat();
    CalibPoint pt;
    pt.adc = (uint16_t)round(smoothed_adc_level);
    pt.value = (uint16_t)val;
    addOrUpdateCalibPoint(pt, data.level_points, data.num_level_points);
    savedata();
    Blynk.virtualWrite(V11, "Đã lưu điểm mực nước.\n");
  } else if (dataS == "i2c") {
    i2c_scaner();
  } else {
    Blynk.virtualWrite(V11, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
BLYNK_WRITE(V12) { // Tắt mở cân chỉnh áp lực
  calib_pre_sta = param.asInt();
}
BLYNK_WRITE(V13) // Time input
{
  if (key) {
    TimeInputParam t(param);
    if (t.hasStartTime()) {
      data.bom_moingay_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
    }
    if (t.hasStopTime()) {
      data.bom_moingay_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
    }
  } else
    Blynk.virtualWrite(V13, 0);
}
BLYNK_WRITE(V14) // Bảo vệ
{
  if (key) {
    int data13 = param.asInt();
    if (data13 == LOW) {
      data.key_protect = false;
    } else {
      data.key_protect = true;
    }
  } else
    Blynk.virtualWrite(V14, data.key_protect);
}
BLYNK_WRITE(V15) // Thông báo
{
  if (key) {
    if (param.asInt() == LOW) {
      data.key_noti = false;
      savedata();
    } else {
      data.key_noti = true;
      savedata();
    }
  } else
    Blynk.virtualWrite(V15, data.key_noti);
}
BLYNK_WRITE(V16) // Rửa lọc
{
  if (key) {
    switch (param.asInt()) {
    case 0: { // Tắt
      data.rualoc = 0;
      if (data.LLG1_RL != 0) {
        Blynk.virtualWrite(V26, LLG1_1m3 - data.LLG1_RL);
        data.LLG1_RL = 0;
        savedata();
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
    }
    savedata();
  } else {
    Blynk.virtualWrite(V16, data.rualoc);
  }
}
BLYNK_WRITE(V17) // Info
{
  if (param.asInt() == 1) {
    terminal.clear();
    if (data.mode_cap2 == 0) {
      Blynk.virtualWrite(V11, "Chế độ bơm: Vận hành THỦ CÔNG.");
    } else if (data.mode_cap2 == 1) {
      int moingay_start_h = data.bom_moingay_start / 3600;
      int moingay_start_m = (data.bom_moingay_start - (moingay_start_h * 3600)) / 60;
      int moingay_stop_h = data.bom_moingay_stop / 3600;
      int moingay_stop_m = (data.bom_moingay_stop - (moingay_stop_h * 3600)) / 60;
      Blynk.virtualWrite(V11, "Chế độ bơm: Tự động - Mỗi ngày\nThời gian: ", moingay_stop_h, " : ", moingay_stop_m, " -> ", moingay_start_h, " : ", moingay_start_m);
    }
  } else {
    terminal.clear();
  }
}
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
//-------------------------
BLYNK_WRITE(V24) // Lưu lượng G1_1m3
{
  LLG1_1m3 = param.asInt();
}
//-------------------------
BLYNK_WRITE(V28) // Clo input
{
  if (param.asFloat() > 0) {
    terminal.clear();
    clo_cache = param.asFloat();
    Blynk.virtualWrite(V11, " Lượng CLO châm hôm nay:", clo_cache, "kg\n Vui lòng kiểm tra kỹ, nếu đúng hãy nhập 'OK' để lưu");
  }
}
BLYNK_WRITE(V30) // Check Clo
{
  if (param.asInt() == 1) {
    DateTime dt(data.time_clo);
    terminal.clear();
    Blynk.virtualWrite(V11, "Châm CLO:", data.clo, "kg vào lúc", dt.hour(), ":", dt.minute(), "-", dt.day(), "/", dt.month(), "/", dt.year());
  }
}
//----------------------------------------------------
//-------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(7000);
  //---------------------------------------------------------------------------------

  emon0.current(A0, 105);
  emon1.current(A0, 105);
  emon2.current(A0, 105);

  pcf8575_1.begin();
  pcf8575_1.pinMode(S0pin, OUTPUT);
  pcf8575_1.pinMode(S1pin, OUTPUT);
  pcf8575_1.pinMode(S2pin, OUTPUT);
  pcf8575_1.pinMode(S3pin, OUTPUT);
  pcf8575_1.pinMode(pin_G1, OUTPUT);
  pcf8575_1.digitalWrite(pin_G1, LOW);
  pcf8575_1.pinMode(pin_B1, OUTPUT);
  pcf8575_1.digitalWrite(pin_B1, LOW);
  pcf8575_1.pinMode(pin_NK1, OUTPUT);
  pcf8575_1.digitalWrite(pin_NK1, HIGH);
  pcf8575_1.pinMode(pin_rst, OUTPUT);
  pcf8575_1.digitalWrite(pin_rst, HIGH);

  rtc_module.begin();
  ee.begin();
  cs.begin(ee, PAGE_SIZE, MEMORY_SIZE / PAGE_SIZE);

  // Kiểm tra phiên bản dữ liệu
  if (!cs.read(data) || data.version != DATA_VERSION) {
    Serial.println("EEPROM version mismatch. Resetting...");
    memset(&data, 0, sizeof(data));
    data.version = DATA_VERSION;
    savedata();
  }
  // Kiểm tra an toàn mảng
  if (data.num_pressure_points > MAX_CALIB_POINTS)
    data.num_pressure_points = 0;
  if (data.num_level_points > MAX_CALIB_POINTS)
    data.num_level_points = 0;
  memcpy(&dataCheck, &data, sizeof(data));

  //------------------------------------
  timer.setTimeout(5000L, []() {
    timer.setInterval(121L, []() {
      readPressure();
      MeasureCmForSmoothing();
    });
    timer_I = timer.setInterval(1283L, []() {
      readPower();
      readPower1();
      readPower2();
      up();
      // Blynk.virtualWrite(V11, sensor_pre, "-", sensor_tank, "\n");
      timer.restartTimer(timer_I);
    });
    timer.setInterval(15006L, []() {
      rtctime();
      timer.restartTimer(timer_I);
    });
    timer.setInterval(900005L, []() {
      connectionstatus();
      timer.restartTimer(timer_I);
    });
    terminal.clear();
  });
}
void loop() {
  Blynk.run();
  timer.run();
  timer1.run();
}
