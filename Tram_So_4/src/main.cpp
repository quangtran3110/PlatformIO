/*V0 - Button C2-1
 *V1 - Button C2-2
 *V2 - Button C1
 *V3 - Switch - chế độ Pump
 *V4 -
 *V5 - MENU motor
 *V6 - min
 *V7 - max
 *V8 -
 *V9 - Ngày/Giờ
 *V10 - terminal key
 *V11 - Thời gian chạy Bơm
 *V12 -
 *V13 - Bảo vệ
 *V14 - Ap luc
 *V15 - Nhiet do dong co 1
 *V16 - Thông báo
 *V17 - Van điện Rửa lọc
 *V18 - time input
 *V19 - Thể tích

 *V20 - haohut
 *V21 - Do Sau
 *V22 - Dung Tich
 *V23 - Nhiệt độ động cơ 2
 *V24 - I2 - Bơm 2
 *V25 - I4 - Van điện rửa lọc
 *V26 - I1 - Bơm 1
 *V27 - I0 - Giếng
 *V28 - DATAS_VOLUME
 *V29 - Info
 *V30 - I3 - Nén khí
 *V31 -
 *V32 - input luu luong
 *V33 - check luu luong
 *V34 -
 *V35 - Luu Luong G1 - 24h
 *V36 - Khoi luong Clo
 *V37 - Luu Luong G1 - 1m3
 *V38 - Luu Luong G1 rửa lọc
 *V39 - thời gian chạy G1
 *V40 - thời gian chạy G1-24h
 *V41 - thời gian chạy B1
 *V42 - thời gian chạy B1 - 24h
 *V43 - thời gian chạy B2
 *V44 - thời gian chạy B2 - 24h

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

#define BLYNK_PRINT Serial
/*
#define BLYNK_TEMPLATE_ID "TMPLJp_sN4GN"
#define BLYNK_TEMPLATE_NAME "Trạm Số 4"
#define BLYNK_AUTH_TOKEN "o-H-k28kNBIzgNIAP89f2AElv--eWuVO"
*/
#define BLYNK_TEMPLATE_ID "TMPL67lOs7dLq"
#define BLYNK_TEMPLATE_NAME "TRẠM SỐ 4"
#define BLYNK_AUTH_TOKEN "ra1gZtR0irrwiTH1L-L_nhXI6TMRH7M9"
#define VOLUME_TOKEN "fQeSuHadv_EFLjXPdqE-sV_lnZ6pXWfu"

#define BLYNK_FIRMWARE_VERSION "250330"
const char *ssid = "tram bom so 4";
const char *password = "0943950555";
#define APP_DEBUG

#pragma region
#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <UrlEncode.h>
//-------------------
#include "PCF8575.h"
PCF8575 pcf8575_1(0x20);
const int pin_G1 = P7;
const int pin_B2 = P6;
const int pin_B1 = P5;
const int pin_Vandien = P4;
const int pin_P3 = P3;
const int pin_rst = P2;

const int S0pin = P15;
const int S1pin = P14;
const int S2pin = P13;
const int S3pin = P12;
//-------------------
#include "EmonLib.h"
EnergyMonitor emon0, emon1, emon2, emon3, emon4;
bool trip0 = false, trip1 = false, trip2 = false, trip3 = false, trip4 = false;
int xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0, xSetAmpe3 = 0, xSetAmpe4 = 0;
int btnState = HIGH, btnState1 = HIGH, btnState2 = HIGH, AppState, AppState1, AppState2;
float Irms0, Irms1, Irms2, Irms3, Irms4;
unsigned long int xIrms0 = 0, xIrms1 = 0, xIrms2 = 0, xIrms3 = 0, xIrms4 = 0;
unsigned long int yIrms0 = 0, yIrms1 = 0, yIrms2 = 0, yIrms3 = 0, yIrms4 = 0;
//-------------------
#include "RTClib.h"
#include <WidgetRTC.h>
RTC_DS3231 rtc_module;
char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";
//-------------------
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>
OneWire oneWire(D5);
DallasTemperature sensors(&oneWire);
float temp[3];
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
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/Tram_So_4/.pio/build/nodemcuv2/firmware.bin"
String server_name = "http://sgp1.blynk.cloud/external/api/";
//-------------------
#define filterSamples 121
int dai = 800;
int rong = 800;
int dosau = 330;
int volume, volume1, percent, percent1, dungtich, smoothDistance;
int sensSmoothArray1[filterSamples];
int digitalSmooth(int rawIn, int *sensSmoothArray) {
  int j, k, temp, top, bottom;
  long total;
  static int i;
  static int sorted[filterSamples];
  boolean done;

  i = (i + 1) % filterSamples; // increment counter and roll over if necc. -  % (modulo operator) rolls over variable
  sensSmoothArray[i] = rawIn;  // input new data into the oldest slot

  // Serial.print("raw = ");

  for (j = 0; j < filterSamples; j++) // transfer data array into anther array for sorting and averaging
    sorted[j] = sensSmoothArray[j];

  done = 0;           // flag to know when we're done sorting
  while (done != 1) { // simple swap sort, sorts numbers from lowest to highest
    done = 1;
    for (j = 0; j < (filterSamples - 1); j++) {
      if (sorted[j] > sorted[j + 1]) { // numbers are out of order - swap
        temp = sorted[j + 1];
        sorted[j + 1] = sorted[j];
        sorted[j] = temp;
        done = 0;
      }
    }
  }
  bottom = max(((filterSamples * 20) / 100), 1);
  top = min((((filterSamples * 80) / 100) + 1), (filterSamples - 1)); // the + 1 is to make up for asymmetry caused by integer rounding
  k = 0;
  total = 0;
  for (j = bottom; j < top; j++) {
    total += sorted[j]; // total remaining indices
    k++;
  }
  return total / k; // divide by number of samples
}
long distance, distance1, t;
//-------------------

long m = 60 * 1000;
bool key = false, keyp = true, keytank = true;
bool timer_updata_status, timer_I_status;
bool time_run1 = false, time_run2 = false;
bool noti_1 = true, noti_2 = true, noti_3 = true, noti_4 = true, noti_5 = true, noti_6 = true;
bool blynk_first_connect = false, pre_raw = false, tank_raw = false;
float clo_cache = 0, value, Result1, sensorValue_pre, sensorValue_tank;
uint32_t timestamp;
int a, c, b, f = 0;
int timer_2, timer_1, timer_3, timer_4, timer_5;
int time_cycle, timer_cycle;
int LLG1_1m3, reboot_num;
int time_run_nenkhi = 5 * 60;
int time_stop_nenkhi = 10 * 60;
byte status_b1, status_b2, status_g1, stastus_read_current = false;
int G1_start, B1_start, B2_start;
bool G1_save = false, B1_save = false, B2_save = false;
//-------------------
struct Data {
  byte SetAmpemax, SetAmpemin;
  byte SetAmpe1max, SetAmpe1min;
  byte SetAmpe2max, SetAmpe2min;
  byte SetAmpe3max, SetAmpe3min;
  byte SetAmpe4max, SetAmpe4min;
  byte man, mode_cap2;
  int b1_1_start, b1_1_stop, b1_2_start, b1_2_stop, b1_3_start, b1_3_stop, b1_4_start, b1_4_stop;
  int b2_1_start, b2_1_stop, b2_2_start, b2_2_stop, b2_3_start, b2_3_stop, b2_4_start, b2_4_stop;
  int save_num;
  byte clo;
  int time_clo, LLG1_RL;
  byte statusRualoc;
  byte key_noti;
  byte reset_day;
  int timerun_G1, timerun_B1, timerun_B2;

} data, dataCheck;
const struct Data dataDefault = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#pragma endregion

WidgetTerminal volume_terminal(V28);
WidgetTerminal terminal(V10);
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
      ESP.restart();
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
  String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN +
                       "&V14=" + float(Result1) +
                       //"&V15=" + temp[1] +
                       "&V19=" + volume1 +
                       "&V20=" + smoothDistance +
                       //"&V23=" + temp[0] +
                       "&V27=" + Irms0 +
                       "&V26=" + Irms1 +
                       "&V24=" + Irms2 +
                       "&V30=" + Irms3 +
                       "&V25=" + Irms4 +
                       "&V39=" + float(data.timerun_G1) / 1000 / 60 / 60 +
                       "&V41=" + float(data.timerun_B1) / 1000 / 60 / 60 +
                       "&V43=" + float(data.timerun_B2) / 1000 / 60 / 60;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}
void up_timerun_motor() {
  String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN + "&V40=" + float(data.timerun_G1) / 1000 / 60 / 60 + "&V42=" + float(data.timerun_B1) / 1000 / 60 / 60 + "&V44=" + float(data.timerun_B2) / 1000 / 60 / 60;
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
    Blynk.setProperty(V10, "label", data.save_num);
  }
}
void time_run_motor() {
  if (blynk_first_connect) {
    if (data.reset_day != day()) {
      if (Blynk.connected()) {
        up_timerun_motor();
        data.timerun_G1 = 0;
        data.timerun_B1 = 0;
        data.timerun_B2 = 0;
        data.reset_day = day();
        savedata();
      }
    }
  }
  if (G1_save || B1_save || B2_save) {
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
    if (B2_start != 0) {
      data.timerun_B2 = data.timerun_B2 + (millis() - B2_start);
      B2_start = millis();
      B2_save = false;
    }
    savedata();
  }
}
void rualoc() {
  if (data.statusRualoc == HIGH) {
    pcf8575_1.digitalWrite(pin_Vandien, !data.statusRualoc);
    timer1.setTimeout(long(time_run_nenkhi * 1000), []() { pcf8575_1.digitalWrite(pin_Vandien, HIGH); });
  }
}
void oncap1() {
  if (status_g1 != HIGH) {
    status_g1 = HIGH;
    Blynk.virtualWrite(V2, status_g1);
    pcf8575_1.digitalWrite(pin_G1, status_g1);
  }
}
void offcap1() {
  if (status_g1 != LOW) {
    status_g1 = LOW;
    Blynk.virtualWrite(V2, status_g1);
    pcf8575_1.digitalWrite(pin_G1, status_g1);
  }
}
void onbom1() {
  if (status_b1 != HIGH) {
    status_b1 = HIGH;
    Blynk.virtualWrite(V0, status_b1);
    pcf8575_1.digitalWrite(pin_B1, !status_b1);
  }
}
void offbom1() {
  if (status_b1 != LOW) {
    status_b1 = LOW;
    Blynk.virtualWrite(V0, status_b1);
    pcf8575_1.digitalWrite(pin_B1, !status_b1);
  }
}
void onbom2() {
  if (status_b2 != HIGH) {
    status_b2 = HIGH;
    Blynk.virtualWrite(V1, status_b2);
    pcf8575_1.digitalWrite(pin_B2, !status_b2);
  }
}
void offbom2() {
  if (status_b2 != LOW) {
    status_b2 = LOW;
    Blynk.virtualWrite(V1, status_b2);
    pcf8575_1.digitalWrite(pin_B2, !status_b2);
  }
}
void hidden_all() {
  Blynk.setProperty(V18, V11, V16, V13, V8, V5, V6, V7, "isHidden", true);
}
void visible_all() {
  Blynk.setProperty(V18, V11, V16, V13, V8, V5, V6, V7, "isHidden", false);
}
void hidden_auto() {
  Blynk.setProperty(V1, V0, "isHidden", true);
}
void visible_man() {
  Blynk.setProperty(V1, V0, "isHidden", false);
}
void rst_board() {
  pcf8575_1.digitalWrite(pin_rst, LOW);
}
//-------------------------------------------------------------------
void readPower() // C2 - Giếng    - I0
{
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms0 = emon0.calcIrms(1480);
  if (rms0 < 2) {
    Irms0 = 0;
    yIrms0 = 0;
    xIrms0++;
    if (xIrms0 > 3) {
      if ((status_g1 == HIGH) && (keyp) && (volume1 < 170)) {
        offcap1();
        trip0 = true;
        if (data.key_noti)
          Blynk.logEvent("error", String("Bơm GIẾNG lỗi không đo được DÒNG ĐIỆN!"));
      }
    }
    if (G1_start != 0) {
      data.timerun_G1 = data.timerun_G1 + (millis() - G1_start);
      savedata();
      G1_start = 0;
    }
  } else if (rms0 >= 2) {
    Irms0 = rms0;
    yIrms0 = yIrms0 + 1;
    xIrms0 = 0;
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
        if ((xSetAmpe >= 4) && (keyp)) {
          offcap1();
          xSetAmpe = 0;
          trip0 = true;
          if (data.key_noti)
            Blynk.logEvent("error", String("Bơm GIẾNG lỗi: ") + Irms0 + String(" A"));
        }
      } else
        xSetAmpe = 0;
    }
  }
  // Blynk.virtualWrite(V27, Irms0);
}
void readPower1() // C3 - Bơm 1    - I1
{
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms1 = emon1.calcIrms(1480);
  if (rms1 < 2) {
    Irms1 = 0;
    yIrms1 = 0;
    xIrms1++;
    if (xIrms1 > 3) {
      if ((status_b1 == HIGH) && (keyp)) {
        offbom1();
        trip1 = true;
        if (data.key_noti)
          Blynk.logEvent("error", String("Bơm 1 lỗi không đo được DÒNG ĐIỆN!"));
      }
    }
    if (B1_start != 0) {
      data.timerun_B1 = data.timerun_B1 + (millis() - B1_start);
      savedata();
      B1_start = 0;
    }
  } else if (rms1 >= 2) {
    Irms1 = rms1;
    yIrms1 = yIrms1 + 1;
    xIrms1 = 0;
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
          if (data.key_noti)
            Blynk.logEvent("error", String("Bơm 1 lỗi: ") + Irms1 + String(" A"));
        }
      } else
        xSetAmpe1 = 0;
    }
  }
  // Blynk.virtualWrite(V26, Irms1);
}
void readPower2() // C4 - Bơm 2    - I2
{
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, HIGH);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms2 = emon2.calcIrms(1480);
  if (rms2 < 2) {
    Irms2 = 0;
    yIrms2 = 0;
    xIrms2++;
    if ((xIrms2 > 3) && (keyp)) {
      if (status_b2 == HIGH) {
        offbom2();
        trip2 = true;
        if (data.key_noti)
          Blynk.logEvent("error", String("Bơm 2 lỗi không đo được DÒNG ĐIỆN!"));
      }
    }
    if (B2_start != 0) {
      data.timerun_B2 = data.timerun_B2 + (millis() - B2_start);
      savedata();
      B2_start = 0;
    }
  } else if (rms2 >= 2) {
    Irms2 = rms2;
    yIrms2 = yIrms2 + 1;
    xIrms2 = 0;
    if (yIrms2 > 3) {
      if (B2_start >= 0) {
        if (B2_start == 0)
          B2_start = millis();
        else if (millis() - B2_start > 60000) {
          B2_save = true;
        } else
          B2_save = false;
      }
      if ((Irms2 >= data.SetAmpe2max) || (Irms2 <= data.SetAmpe2min)) {
        xSetAmpe2 = xSetAmpe2 + 1;
        if ((xSetAmpe2 >= 2) && (keyp)) {
          offbom2();
          xSetAmpe2 = 0;
          trip2 = true;
          if (data.key_noti)
            Blynk.logEvent("error", String("Cảnh báo dòng điện Bơm 2 quá cao: ") + Irms2 + String(" A\nĐã ngưng máy, xin kiểm tra!"));
        }
      }
    }
  }
  // Blynk.virtualWrite(V24, Irms2);
}
void readPower3() // C5 - Nén khí  - I3
{
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, HIGH);
  pcf8575_1.digitalWrite(S3pin, LOW);
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
        if (data.key_noti)
          Blynk.logEvent("error", String("Máy NÉN KHÍ lỗi: ") + Irms3 + String(" A"));
      }
    } else {
      xSetAmpe3 = 0;
    }
  }
  // Blynk.virtualWrite(V30, Irms3);
}
void readPower4() // C6 - Van điện - I4
{
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, HIGH);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms4 = emon4.calcIrms(740);
  if (rms4 < 1) {
    Irms4 = 0;
    yIrms4 = 0;
  } else if (rms4 >= 1) {
    Irms4 = rms4;
    yIrms4 = yIrms4 + 1;
    if ((yIrms4 > 3) && ((Irms4 >= data.SetAmpe4max) || (Irms4 <= data.SetAmpe4min))) {
      xSetAmpe4 = xSetAmpe4 + 1;
      if ((xSetAmpe4 >= 2) && (keyp)) {
        data.statusRualoc = LOW;
        savedata();
        pcf8575_1.digitalWrite(pin_Vandien, !data.statusRualoc);
        xSetAmpe4 = 0;
        trip4 = true;
        if (data.key_noti)
          Blynk.logEvent("error", String("Van điện lỗi: ") + Irms4 + String(" A"));
      }
    } else {
      xSetAmpe4 = 0;
    }
  }
  // Blynk.virtualWrite(V25, Irms4);
}
void temperature() { // Nhiệt độ
  sensors.requestTemperatures();
  // Serial.println(sensors.getDeviceCount());
  for (byte i = 0; i < sensors.getDeviceCount(); i++)
    temp[i] = sensors.getTempCByIndex(i);
  // Blynk.virtualWrite(V15, temp[1]);
  // Blynk.virtualWrite(V23, temp[0]);
}
void up_cycle() {
  if (Irms0 != 0 || Irms1 != 0 || Irms2 != 0 || Irms3 != 0 || Irms4 != 0) {
    if (time_cycle != 1503) {
      time_cycle = 1503;
      up();
      timer.deleteTimer(timer_cycle);
      timer_cycle = timer.setInterval(time_cycle, []() {
        up();
        timer.restartTimer(timer_1);
      });
    }
  } else {
    if (time_cycle != 5001) {
      time_cycle = 5001;
      up();
      timer.deleteTimer(timer_cycle);
      timer_cycle = timer.setInterval(time_cycle, []() {
        up();
        timer.restartTimer(timer_1);
      });
    }
  }
}
//-------------------------------------------------------------------
void readPressure() // C0 - Ap Luc
{
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  sensorValue_pre = analogRead(A0);
  float Result;
  Result = (((sensorValue_pre - 186) * 6) / (805 - 186));
  if (Result > 0) {
    value += Result;
    Result1 = value / 16.0;
    value -= Result1;
  }
}
void MeasureCmForSmoothing() // C1 - Muc Nuoc
{
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float sensorValue = analogRead(A0);
  distance1 = (((sensorValue - 196.5) * 500) / (750 - 196.5)); // 915,74 (R=147.7)
  if (distance1 > 0) {
    smoothDistance = digitalSmooth(distance1, sensSmoothArray1);
    volume1 = (dai * smoothDistance * rong) / 1000000;
    if ((smoothDistance < (dosau / 2)) && (Irms3 == 0) && !trip3 && data.key_noti && keytank) {
      Blynk.logEvent("info", String("Mực nước thấp nhưng cấp 1 không chạy: ") + smoothDistance + String(" cm"));
      keytank = false;
      timer1.setTimeout(10 * m, []() { keytank = true; });
    } else if ((smoothDistance - dosau >= 20) && (data.key_noti) && (keytank)) {
      Blynk.logEvent("info", String("Nước trong bể cao vượt mức ") + (smoothDistance - dosau) + String(" cm"));
      keytank = false;
      timer1.setTimeout(15 * m, []() { keytank = true; });
    } else
      keytank = true;
  }
}
void sensor_raw() {
  if (pre_raw) {
    Blynk.virtualWrite(V10, "PRE: ", sensorValue_pre, "\n");
  }
  if (tank_raw) {
    Blynk.virtualWrite(V10, "TANK: ", sensorValue_tank, "\n");
  }
}
//-------------------------------------------------------------------
void rtctime() {
  DateTime now = rtc_module.now();
  if (blynk_first_connect == true) {
    if ((now.day() != day()) || (now.hour() != hour()) || ((now.minute() - minute() > 2) || (minute() - now.minute() > 2))) {
      rtc_module.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
    }
  }
  timestamp = now.unixtime();
  Blynk.virtualWrite(V9, daysOfTheWeek[now.dayOfTheWeek()], ", ", now.day(), "/", now.month(), "/", now.year(), " - ", now.hour(), ":", now.minute(), ":", now.second());
  int nowtime = (now.hour() * 3600 + now.minute() * 60);
  if (data.mode_cap2 == 1) {                                                                                                                                                                                                                        // Chọn chế độ chạy 2 bơm
    if ((nowtime > data.b1_1_start && nowtime < data.b1_1_stop) || (nowtime > data.b1_2_start && nowtime < data.b1_2_stop) || (nowtime > data.b1_3_start && nowtime < data.b1_3_stop) || (nowtime > data.b1_4_start && nowtime < data.b1_4_stop)) { // Chạy bơm 1
      if (Irms1 == 0 && !trip1) {                                                                                                                                                                                                                   // Nếu bơm 1 đang tắt và không lỗi
        if ((Irms2 == 0 && !time_run2) || (time_run2))
          onbom1(); // Chạy bơm 1
        if (time_run1 && noti_3) {
          noti_3 = false;
          if (data.key_noti)
            Blynk.logEvent("error", String("LỖI: Bơm 1 không chạy."));
          timer1.setTimeout(30 * m, []() { noti_3 = true; });
        }
      }
      time_run1 = true;
    } else {
      if (Irms1 != 0) {
        offbom1();
        if (!time_run1 && noti_1) {
          noti_1 = false;
          if (data.key_noti)
            Blynk.logEvent("error", String("Lỗi lịch chạy!\nBơm 1 đang chạy: ") + Irms1 + String(" A"));
          timer1.setTimeout(5 * m, []() { noti_1 = true; });
        }
      }
      time_run1 = false;
    }
    if ((nowtime > data.b2_1_start && nowtime < data.b2_1_stop) || (nowtime > data.b2_2_start && nowtime < data.b2_2_stop) || (nowtime > data.b2_3_start && nowtime < data.b2_3_stop) || (nowtime > data.b2_4_start && nowtime < data.b2_4_stop)) {
      if (Irms2 == 0 && !trip2) { // Nếu bơm 2 đang tắt và không lỗi
        if ((Irms1 == 0 && !time_run1) || (time_run1))
          onbom2(); // Chạy bơm 2
        if (time_run2 && noti_4) {
          noti_4 = false;
          if (data.key_noti)
            Blynk.logEvent("error", String("LỖI: Bơm 2 không chạy."));
          timer1.setTimeout(30 * m, []() { noti_4 = true; });
        }
      }
      time_run2 = true;
    } else {
      if (Irms2 != 0) {
        offbom2();
        if (!time_run2 && noti_2) {
          noti_2 = false;
          if (data.key_noti)
            Blynk.logEvent("error", String("Lỗi lịch chạy!\nBơm 2 đang chạy: ") + Irms2 + String(" A"));
          timer1.setTimeout(5 * m, []() { noti_2 = true; });
        }
      }
      time_run2 = false;
    }
  } /*else if (data.mode_cap2 == 1) {  // Chạy bơm 1
    if (Irms0 != 0) {                // Tắt Bơm 2
      offbom2();
      timer1.setTimeout(3000L, []() {
        if (Irms0 != 0) {
          offbom2();
          if (data.key_noti) Blynk.logEvent("error", String("Lỗi lịch chạy!\nBơm 2 đang chạy: ") + Irms0 + String(" A"));
        }
      });
    }
    if (data.cap2_chanle == 0) {  // Chọn ngày chẵn tắt máy
      if (now.day() % 2 == 0) {
        if ((nowtime > data.bom_chanle_stop) && (Irms2 != 0))
          offbom1();
        if ((nowtime < data.bom_chanle_stop) && (trip2 == false) && (Irms2 == 0)) {
          onbom1();
          if (Irms2 == 0) {
            f = f + 1;
            if ((f == 3) && (data.key_noti))
              Blynk.logEvent("error", String("Bơm 1 bị lỗi không chạy kìa.\nKiểm tra lẹ."));
          }
        }
      }
      if (now.day() % 2 != 0) {
        if (((nowtime >= data.bom_chanle_start) && (trip2 == false)) && (Irms2 == 0)) {
          onbom1();
          if (Irms2 == 0) {
            f = f + 1;
            if ((f == 3) && (data.key_noti))
              Blynk.logEvent("error", String("Bơm 1 lỗi không thể chạy.\nXin hãy kiểm tra. "));
          }
        }
      }
    }
    if (data.cap2_chanle == 1) {  // Chon ngay le tat may
      if (now.day() % 2 != 0) {
        if ((nowtime > data.bom_chanle_stop) && (Irms2 != 0))
          offbom1();
        if ((nowtime < data.bom_chanle_stop) && (trip2 == false) && (Irms2 == 0)) {
          onbom1();
          if (Irms2 == 0) {
            f = f + 1;
            if ((f == 3) && (data.key_noti))
              Blynk.logEvent("error", String("Bơm 1 lỗi không thể chạy.\nXin hãy kiểm tra. "));
          }
        }
      }
      if (now.day() % 2 == 0) {
        if (((nowtime >= data.bom_chanle_start) && (trip2 == false)) && (Irms2 == 0)) {
          onbom1();
          if (Irms2 == 0) {
            f = f + 1;
            if ((f == 3) && (data.key_noti))
              Blynk.logEvent("error", String("Bơm 1 lỗi không thể chạy.\nXin hãy kiểm tra. "));
          }
        }
      }
    }
  }  else if (data.mode_cap2 == 2) {  // Chạy bơm 2
    if (Irms2 != 0) {                // Tắt Bơm 1
      offbom1();
      timer1.setTimeout(3000L, []() {
        if (Irms2 != 0) {
          offbom1();
          if (data.key_noti) Blynk.logEvent("error", String("Lỗi lịch chạy!\nBơm 1 đang chạy: ") + Irms2 + String(" A"));
        }
      });
    }
    if (data.cap2_chanle == 0) {  // Chọn ngày chẵn tắt máy
      if (now.day() % 2 == 0) {
        if ((nowtime > data.bom_chanle_stop) && (Irms0 != 0))
          offbom2();
        if ((nowtime < data.bom_chanle_stop) && (trip0 == false) && (Irms0 == 0)) {
          onbom2();
          if (Irms0 == 0) {
            f = f + 1;
            if ((f == 3) && (data.key_noti))
              Blynk.logEvent("error", String("Bơm 2 bị lỗi không chạy kìa.\nKiểm tra lẹ."));
          }
        }
      }
      if (now.day() % 2 != 0) {
        if (((nowtime >= data.bom_chanle_start) && (trip0 == false)) && (Irms0 == 0)) {
          onbom2();
          if (Irms0 == 0) {
            f = f + 1;
            if ((f == 3) && (data.key_noti))
              Blynk.logEvent("error", String("Bơm 2 lỗi không thể chạy.\nXin hãy kiểm tra. "));
          }
        }
      }
    }
    if (data.cap2_chanle == 1) {  // Chon ngay le tat may
      if (now.day() % 2 != 0) {
        if ((nowtime > data.bom_chanle_stop) && (Irms0 != 0))
          offbom2();
        if ((nowtime < data.bom_chanle_stop) && (trip0 == false) && (Irms0 == 0)) {
          onbom2();
          if (Irms0 == 0) {
            f = f + 1;
            if ((f == 3) && (data.key_noti))
              Blynk.logEvent("error", String("Bơm 2 lỗi không thể chạy.\nXin hãy kiểm tra. "));
          }
        }
      }
      if (now.day() % 2 == 0) {
        if (((nowtime >= data.bom_chanle_start) && (trip0 == false)) && (Irms0 == 0)) {
          onbom2();
          if (Irms0 == 0) {
            f = f + 1;
            if ((f == 3) && (data.key_noti))
              Blynk.logEvent("error", String("Bơm 2 lỗi không thể chạy.\nXin hãy kiểm tra. "));
          }
        }
      }
    }
  }*/
}
//-------------------------------------------------------------------
BLYNK_WRITE(V0) // Bơm 1
{
  if (key && !trip2)
    if (param.asInt() == LOW)
      offbom1();
    else
      onbom1();
  else
    Blynk.virtualWrite(V0, status_b1);
}
BLYNK_WRITE(V1) // Bơm 2
{
  if (key && !trip0)
    if (param.asInt() == LOW)
      offbom2();
    else
      onbom2();
  else
    Blynk.virtualWrite(V1, status_b2);
}
BLYNK_WRITE(V2) // Giếng
{
  if (key && !trip3)
    if (param.asInt() == LOW)
      offcap1();
    else
      oncap1();
  else
    Blynk.virtualWrite(V2, status_g1);
}
BLYNK_WRITE(V3) // Chọn chế độ Cấp 2
{
  if (key) {
    switch (param.asInt()) {
    case 0: { // Man
      data.mode_cap2 = 0;
      visible_man();
      break;
    }
    case 1: {
      data.mode_cap2 = 1;
      hidden_auto();
      break;
    }
    }
  } else
    Blynk.virtualWrite(V3, data.mode_cap2);
}
BLYNK_WRITE(V5) // Chon máy cài đặt bảo vệ
{
  switch (param.asInt()) {
  case 0: { // Cấp 1
    c = 0;
    Blynk.virtualWrite(V6, data.SetAmpemin);
    Blynk.virtualWrite(V7, data.SetAmpemax);
    break;
  }
  case 1: { // Cap 2 - 1
    c = 1;
    Blynk.virtualWrite(V6, data.SetAmpe1min);
    Blynk.virtualWrite(V7, data.SetAmpe1max);
    break;
  }
  case 2: { // Cap 2 - 2
    c = 2;
    Blynk.virtualWrite(V6, data.SetAmpe2min);
    Blynk.virtualWrite(V7, data.SetAmpe2max);
    break;
  }
  case 3: { // Clo
    c = 3;
    Blynk.virtualWrite(V6, data.SetAmpe3min);
    Blynk.virtualWrite(V7, data.SetAmpe3max);
    break;
  }
  case 4: { // NK
    c = 4;
    Blynk.virtualWrite(V6, data.SetAmpe4min);
    Blynk.virtualWrite(V7, data.SetAmpe4max);
    break;
  }
  }
}
BLYNK_WRITE(V6) // min
{
  if (key) {
    if (c == 0) // Giếng
      data.SetAmpemin = param.asInt();
    else if (c == 1) // Bơm 1
      data.SetAmpe1min = param.asInt();
    else if (c == 2) // Bơm 2
      data.SetAmpe2min = param.asInt();
    else if (c == 3) // Nén khí
      data.SetAmpe3min = param.asInt();
    else if (c == 4) // Van điện
      data.SetAmpe4min = param.asInt();
  } else {
    Blynk.virtualWrite(V6, 0);
  }
}
BLYNK_WRITE(V7) // max
{
  if (key) {
    if (c == 0) // Giếng
      data.SetAmpemax = param.asInt();
    else if (c == 1) // Bơm 1
      data.SetAmpe1max = param.asInt();
    else if (c == 2) // Bơm 2
      data.SetAmpe2max = param.asInt();
    else if (c == 3) // Nén khí
      data.SetAmpe3max = param.asInt();
    else if (c == 4) // Van điện
      data.SetAmpe4max = param.asInt();
  } else {
    Blynk.virtualWrite(V7, 0);
  }
}
BLYNK_WRITE(V10) // String
{
  String dataS = param.asStr();
  if (dataS == "M") {
    terminal.clear();
    key = true;
    Blynk.virtualWrite(V10, "Người vận hành: 'M.Quang'\nKích hoạt trong 10s\n");
    timer1.setTimeout(10000L, []() {
      key = false;
      terminal.clear(); });
  } else if (dataS == "active") {
    terminal.clear();
    key = true;
    visible_all();
    Blynk.virtualWrite(V10, "KHÔNG sử dụng phần mềm cho đến khi thông báo này mất.\n");
  } else if (dataS == "deactive") {
    terminal.clear();
    key = false;
    hidden_all();
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
    trip4 = false;
    pcf8575_1.digitalWrite(pin_G1, HIGH);
    Blynk.virtualWrite(V10, "Đã RESET! \nNhập mã để điều khiển!\n");
  } else if (dataS == "save_num") {
    terminal.clear();
    Blynk.virtualWrite(V10, "Số lần ghi EEPROM: ", data.save_num);
  } else if (dataS == "rst") {
    terminal.clear();
    Blynk.virtualWrite(V10, "ESP khởi động lại sau 3s");
    delay(3000);
    // ESP.restart();
    rst_board();
  } else if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V10, "UPDATE FIRMWARE...");
    update_fw();
  } else if ((dataS == "ok") || (dataS == "Ok") || (dataS == "OK") || (dataS == "oK")) {
    if (clo_cache > 0) {
      data.clo = clo_cache;
      clo_cache = 0;
      data.time_clo = timestamp;
      Blynk.virtualWrite(V36, data.clo);
      savedata();
      terminal.clear();
      Blynk.virtualWrite(V10, "Đã lưu - CLO:", data.clo, "kg");
    }
  } else if (dataS == "pre") {
    terminal.clear();
    pre_raw = !pre_raw;
  } else if (dataS == "tank") {
    terminal.clear();
    tank_raw = !tank_raw;
  } else if (dataS == "clr") {
    terminal.clear();
  } else {
    Blynk.virtualWrite(V10, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
BLYNK_WRITE(V11) // Chọn thời gian chạy 2 Bơm
{
  if (data.mode_cap2 == 1) {
    BlynkParamAllocated menu(255); // list length, in bytes
    menu.add("BƠM 1 - LẦN 1");
    menu.add("BƠM 2 - LẦN 1");
    menu.add("BƠM 1 - LẦN 2");
    menu.add("BƠM 2 - LẦN 2");
    menu.add("BƠM 1 - LẦN 3");
    menu.add("BƠM 2 - LẦN 3");
    menu.add("BƠM 1 - LẦN 4");
    menu.add("BƠM 2 - LẦN 4");
    menu.add("THỜI GIAN RỬA LỌC");
    Blynk.setProperty(V11, "labels", menu);
    switch (param.asInt()) {
    case 0: { // Bơm 1 - Lần 1
      if (key)
        b = 0;
      Blynk.virtualWrite(V18, data.b1_1_start, data.b1_1_stop, tz);
      break;
    }
    case 1: { // Bơm 2 - Lần 1
      if (key)
        b = 1;
      Blynk.virtualWrite(V18, data.b2_1_start, data.b2_1_stop, tz);
      break;
    }
    case 2: { // Bơm 1 - Lần 2
      if (key)
        b = 2;
      Blynk.virtualWrite(V18, data.b1_2_start, data.b1_2_stop, tz);
      break;
    }
    case 3: { // Bơm 2 - Lần 2
      if (key)
        b = 3;
      Blynk.virtualWrite(V18, data.b2_2_start, data.b2_2_stop, tz);
      break;
    }
    case 4: { // Bơm 1 - Lần 3
      if (key)
        b = 4;
      Blynk.virtualWrite(V18, data.b1_3_start, data.b1_3_stop, tz);
      break;
    }
    case 5: { // Bơm 2 - Lần 3
      if (key)
        b = 5;
      Blynk.virtualWrite(V18, data.b2_3_start, data.b2_3_stop, tz);
      break;
    }
    case 6: { // Bơm 1 - Lần 4
      if (key)
        b = 6;
      Blynk.virtualWrite(V18, data.b1_4_start, data.b1_4_stop, tz);
      break;
    }
    case 7: { // Bơm 2 - Lần 4
      if (key)
        b = 7;
      Blynk.virtualWrite(V18, data.b2_4_start, data.b2_4_stop, tz);
      break;
    }
    }
  }
}
BLYNK_WRITE(V13) // Bảo vệ
{
  if (key) {
    int data13 = param.asInt();
    if (data13 == LOW)
      keyp = false;
    else
      keyp = true;
  } else
    Blynk.virtualWrite(V13, keyp);
}
BLYNK_WRITE(V16) // Thông báo
{
  if (key) {
    if (param.asInt() == LOW)
      data.key_noti = false;
    else
      data.key_noti = true;
    savedata();
  } else
    Blynk.virtualWrite(V16, data.key_noti);
}
BLYNK_WRITE(V17) // Chế độ rửa lọc
{
  if (key) {
    if (param.asInt() == LOW) {
      data.statusRualoc = LOW;
      pcf8575_1.digitalWrite(pin_Vandien, !data.statusRualoc);
      if (data.LLG1_RL != 0) {
        Blynk.virtualWrite(V38, LLG1_1m3 - data.LLG1_RL);
        data.LLG1_RL = 0;
        savedata();
      }
    } else {
      data.statusRualoc = HIGH;
      if (data.LLG1_RL == 0) {
        data.LLG1_RL = LLG1_1m3;
        savedata();
      }
    }
  } else {
    Blynk.virtualWrite(V17, data.statusRualoc);
  }
}
BLYNK_WRITE(V18) // Time input
{
  if (key) {
    TimeInputParam t(param);
    if (t.hasStartTime()) {
      if (b == 0)
        data.b1_1_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      else if (b == 1)
        data.b2_1_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      else if (b == 2)
        data.b1_2_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      else if (b == 3)
        data.b2_2_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      else if (b == 4)
        data.b1_3_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      else if (b == 5)
        data.b2_3_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      else if (b == 6)
        data.b1_4_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      else if (b == 7)
        data.b2_4_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
    }
    if (t.hasStopTime()) {
      if (b == 0)
        data.b1_1_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      else if (b == 1)
        data.b2_1_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      else if (b == 2)
        data.b1_2_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      else if (b == 3)
        data.b2_2_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      else if (b == 4)
        data.b1_3_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      else if (b == 5)
        data.b2_3_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      else if (b == 6)
        data.b1_4_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      else if (b == 7)
        data.b2_4_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
    }
  } else
    Blynk.virtualWrite(V18, 0);
}
BLYNK_WRITE(V28) {
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
BLYNK_WRITE(V29) // Info
{
  if (param.asInt() == 1) {
    terminal.clear();
    if (data.mode_cap2 == 0) {
      Blynk.virtualWrite(V10, "Chế độ bơm: Vận hành THỦ CÔNG");
    } else if (data.mode_cap2 == 1) {
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

      Blynk.virtualWrite(V10, "Mode: Auto\nPump 1: ", hour_start_b1_1, "h", minute_start_b1_1, " -> ", hour_stop_b1_1, "h", minute_stop_b1_1, "\nPump 2: ", hour_start_b2_1, "h", minute_start_b2_1, " -> ", hour_stop_b2_1, "h", minute_stop_b2_1, "\nPump 1: ", hour_start_b1_2, "h", minute_start_b1_2, " -> ", hour_stop_b1_2, "h", minute_stop_b1_2, "\nPump 2: ", hour_start_b2_2, "h", minute_start_b2_2, " -> ", hour_stop_b2_2, "h", minute_stop_b2_2);
      Blynk.virtualWrite(V10, "\nPump 1: ", hour_start_b1_3, "h", minute_start_b1_3, " -> ", hour_stop_b1_3, "h", minute_stop_b1_3, "\nPump 2: ", hour_start_b2_3, "h", minute_start_b2_3, " -> ", hour_stop_b2_3, "h", minute_stop_b2_3, "\nPump 1: ", hour_start_b1_4, "h", minute_start_b1_4, " -> ", hour_stop_b1_4, "h", minute_stop_b1_4, "\nPump 2: ", hour_start_b2_4, "h", minute_start_b2_4, " -> ", hour_stop_b2_4, "h", minute_stop_b2_4);
    } /* else if ((data.mode_cap2 == 1) || (data.mode_cap2 == 2)) {
      int hour_start = data.bom_chanle_start / 3600;
      int minute_start = (data.bom_chanle_start - (hour_start * 3600)) / 60;
      int hour_stop = data.bom_chanle_stop / 3600;
      int minute_stop = (data.bom_chanle_stop - (hour_stop * 3600)) / 60;
      if (data.mode_cap2 == 1) {
        if (data.cap2_chanle == 0)
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 1 tự động\nTắt máy vào: NGÀY CHẴN\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop);
        else if (data.cap2_chanle == 1)
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 1 tự động\nTắt máy vào: NGÀY LẺ\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop);
      } else if (data.mode_cap2 == 2) {
        if (data.cap2_chanle == 0)
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 2 tự động\nTắt máy vào: NGÀY CHẴN\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop);
        else if (data.cap2_chanle == 1)
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 2 tự động\nTắt máy vào: NGÀY LẺ\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop);
      }
    } */
  } else
    terminal.clear();
  timer.restartTimer(timer_1);
  timer.restartTimer(timer_2);
}
//-------------------------------------------------------------------
BLYNK_WRITE(V32) { // Clo Input
  if (param.asFloat() > 0) {
    terminal.clear();
    clo_cache = param.asFloat();
    Blynk.virtualWrite(V10, " Lượng CLO châm hôm nay:", clo_cache, "kg\n Vui lòng kiểm tra kỹ, nếu đúng hãy nhập 'OK' để lưu");
  }
}
BLYNK_WRITE(V33) { // Check Clo
  if (param.asInt() == 1) {
    DateTime dt(data.time_clo);
    terminal.clear();
    Blynk.virtualWrite(V10, "Châm CLO: ", data.clo, " kg vào lúc ", dt.hour(), ":", dt.minute(), "-", dt.day(), "/", dt.month(), "/", dt.year());
  }
}
BLYNK_WRITE(V37) { // LLG1_1m3
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
  delay(7000);
  //---------------------------------------------------------------------------------
  emon0.current(A0, 105); // Giếng
  emon1.current(A0, 105); // Bơm 1
  emon2.current(A0, 105); // Bơm 2
  emon3.current(A0, 105); // Nén khí
  emon4.current(A0, 105); // Van điện

  Wire.begin();
  sensors.begin(); // DS18B20 start
  rtc_module.begin();
  eeprom.initialize();
  eeprom.readBytes(address, sizeof(dataDefault), (byte *)&data);

  pcf8575_1.begin();
  pcf8575_1.pinMode(S0pin, OUTPUT);
  pcf8575_1.pinMode(S1pin, OUTPUT);
  pcf8575_1.pinMode(S2pin, OUTPUT);
  pcf8575_1.pinMode(S3pin, OUTPUT);

  pcf8575_1.pinMode(pin_G1, OUTPUT);
  pcf8575_1.digitalWrite(pin_G1, HIGH);
  pcf8575_1.pinMode(pin_B1, OUTPUT);
  pcf8575_1.digitalWrite(pin_B1, HIGH);
  pcf8575_1.pinMode(pin_B2, OUTPUT);
  pcf8575_1.digitalWrite(pin_B2, HIGH);
  pcf8575_1.pinMode(pin_Vandien, OUTPUT);
  pcf8575_1.digitalWrite(pin_Vandien, HIGH);
  pcf8575_1.pinMode(pin_rst, OUTPUT);
  pcf8575_1.digitalWrite(pin_rst, HIGH);
  pcf8575_1.pinMode(pin_P3, OUTPUT);
  pcf8575_1.digitalWrite(pin_P3, HIGH);

  timer.setTimeout(5000L, []() {
    timer_2 = timer.setInterval(243L, []() {
      readPressure();
      MeasureCmForSmoothing();
    });
    timer_1 = timer.setInterval(1103L, []() {
      readPower();
      readPower1();
      readPower2();
      readPower3();
      readPower4();
      //temperature();
      up_cycle();
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
    timer.setInterval(long(((time_run_nenkhi + time_stop_nenkhi) * 1000) + 500), rualoc);
    terminal.clear(); });
}
void loop() {
  ESP.wdtFeed();
  Blynk.run();
  timer.run();
  timer1.run();
}
