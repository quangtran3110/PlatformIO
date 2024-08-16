/*V0 - Button C2-1
 *V1 - Button C2-2
 *V2 - Button C1
 *V3 - Switch - chế độ Pump
 *V4 - Chọn người vận hành
 *V5 - MENU motor
 *V6 - min
 *V7 - max
 *V8 - Info
 *V9 - Ngày/Giờ
 *V10 - terminal key
 *V11 - Thời gian chạy Bơm
 *V12 -
 *V13 - Bảo vệ
 *V14 - Ap luc
 *V15 - Nhiet do dong co 1
 *V16 - Nhiệt độ động cơ 2
 *V17 - Thông báo
 *V18 - time input
 *V19 -

 *V20 - I0 - Cấp 1
 *V21 - I1 - Bơm 1
 *V22 - I2 - Bơm 2
 *V23 -
 *V24 -
 *V25 -
 *V26 -
 *V27 -
 *V28 -
 *V29 -
 *V30 -
 *V31 -
 *V32 -
 *V33 -
 *V34 -
 *V35 -
 *V36 -
 *V37 -
 *V40 -
 *V41 -
 *V42 -
 *V43 -
 *
 */

/*
#define BLYNK_TEMPLATE_ID "TMPLRfSY8RNu"
#define BLYNK_TEMPLATE_NAME "Trạm Kinh Quận"
#define BLYNK_AUTH_TOKEN "KoAzw_0S67PUz85E83pj3O7uremeAPhS"
*/
#define BLYNK_TEMPLATE_ID "TMPL68fxciJDf"
#define BLYNK_TEMPLATE_NAME "TRẠM KINH QUẬN"
#define BLYNK_AUTH_TOKEN "YTaSFuqD9VYDpkceWb-a7P59cTbsYl9-"

#define BLYNK_FIRMWARE_VERSION "240816"
#define BLYNK_PRINT Serial
#define APP_DEBUG

#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <SPI.h>

#include "PCF8575.h"
PCF8575 pcf8575(0x20);

#include "EmonLib.h"
EnergyMonitor emon0, emon1, emon2, emon3, emon4;

#include "RTClib.h"
#include <TimeLib.h>
RTC_DS3231 rtc;

#include <Eeprom24C32_64.h>
#define EEPROM_ADDRESS 0x57
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);

#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>
OneWire oneWire(D3);
DallasTemperature sensors(&oneWire);

#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
// #define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/work/kwaco/kinhquan/main/main.ino.bin"
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/IOT/main/Arduino/TramKQuan/main/build/esp8266.esp8266.nodemcuv2/main.ino.bin"

const char *ssid = "net";
const char *password = "Password";

const int S0pin = 14;
const int S1pin = 12;
const int S2pin = 13;
const int S3pin = 15;
const word address = 0;
const int pincap1 = P7;
const int pinbom_1 = P6;
const int pinbom_2 = P5;
const int pin_usb = D0;
long t;
char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";
bool key = false, keySet = false, keyp = true, keynoti = true;
bool trip0 = false, trip1 = false, trip2 = false;
bool trip_run1 = false, trip_run2 = false;
bool time_run_1 = false, time_run_2 = false;
bool temp_0 = true, temp_1 = true;
int c, b, check_connect = 0;
int timer_2, timer_1, timer_3, timer_4, timer_5;
int RelayState = HIGH, RelayState1 = LOW, RelayState2 = LOW;
int xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0;
float Irms0, Irms1, Irms2, value, Result1, temp[3];
unsigned long int xIrms0 = 0, xIrms1 = 0, xIrms2 = 0;
unsigned long int yIrms0 = 0, yIrms1 = 0, yIrms2 = 0;

struct Data {
  byte SetAmpemax, SetAmpemin;
  byte SetAmpe1max, SetAmpe1min;
  byte SetAmpe2max, SetAmpe2min;
  byte man, mode_cap2;
  int b1_1_start, b1_1_stop, b1_2_start, b1_2_stop, b1_3_start, b1_3_stop, b1_4_start, b1_4_stop;
  int b2_1_start, b2_1_stop, b2_2_start, b2_2_stop, b2_3_start, b2_3_stop, b2_4_start, b2_4_stop;
  byte reboot_num;
  int save_num;
  int set_temp0, set_temp1;
} data, dataCheck;
const struct Data dataDefault = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

WidgetTerminal terminal(V10);

BlynkTimer timer, timer1;
BLYNK_CONNECTED() {
  Blynk.sendInternal("rtc", "sync");
}
//-------------------------------------------------------------------
void savedata() {
  if (memcmp(&data, &dataCheck, sizeof(dataDefault)) == 0) {
    // Serial.println("structures same no need to write to EEPROM");
  } else {
    // Serial.println("\nWrite bytes to EEPROM memory...");
    data.save_num = data.save_num + 1;
    eeprom.writeBytes(address, sizeof(dataDefault), (byte *)&data);
  }
}
//-------------------------
void connectionstatus() {
  if ((WiFi.status() != WL_CONNECTED)) {
    // WiFi.disconnect();
    // WiFi.mode(WIFI_STA);
    // WiFi.begin(ssid, password);
    // Blynk.config(auth);
    Serial.println("Khong ket noi WIFI");
  }
  if ((WiFi.status() == WL_CONNECTED) && (!Blynk.connected())) {
    data.reboot_num = data.reboot_num + 1;
    savedata();
    Serial.print("data.reboot_num: ");
    Serial.println(data.reboot_num);
    if (data.reboot_num == 1) {
      Serial.println("Restart...");
      Serial.print("data.reboot_num: ");
      Serial.println(data.reboot_num);
      delay(1000);
      ESP.restart();
    }
    if (data.reboot_num % 5 == 0) {
      Serial.print("data.reboot_num: ");
      Serial.println(data.reboot_num);
      delay(1000);
      ESP.restart();
    }
  }
  if (Blynk.connected()) {
    if (data.reboot_num != 0) {
      data.reboot_num = 0;
      savedata();
    }
    Serial.print("data.reboot_num: ");
    Serial.println(data.reboot_num);
    Serial.println("Blynk OK");
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
void oncap1() {
  RelayState = HIGH;
  pcf8575.digitalWrite(pincap1, RelayState);
  Blynk.virtualWrite(V2, RelayState);
}
void offcap1() {
  RelayState = LOW;
  pcf8575.digitalWrite(pincap1, RelayState);
  Blynk.virtualWrite(V2, RelayState);
}
void onbom1() {
  RelayState1 = HIGH;
  pcf8575.digitalWrite(pinbom_1, !RelayState1);
  Blynk.virtualWrite(V0, RelayState1);
}
void offbom1() {
  RelayState1 = LOW;
  pcf8575.digitalWrite(pinbom_1, !RelayState1);
  Blynk.virtualWrite(V0, RelayState1);
}
void onbom2() {
  RelayState2 = HIGH;
  pcf8575.digitalWrite(pinbom_2, !RelayState2);
  Blynk.virtualWrite(V1, RelayState2);
}
void offbom2() {
  RelayState2 = LOW;
  pcf8575.digitalWrite(pinbom_2, !RelayState2);
  Blynk.virtualWrite(V1, RelayState2);
}
void hidden() {
  Blynk.setProperty(V3, "isHidden", true);
  Blynk.setProperty(V11, "isHidden", true);
  Blynk.setProperty(V18, "isHidden", true);
  Blynk.setProperty(V7, "isHidden", true);
  Blynk.setProperty(V6, "isHidden", true);
  Blynk.setProperty(V5, "isHidden", true);
  Blynk.setProperty(V17, "isHidden", true);
  Blynk.setProperty(V13, "isHidden", true);
}
void visible() {
  Blynk.setProperty(V3, "isHidden", false);
  Blynk.setProperty(V11, "isHidden", false);
  Blynk.setProperty(V18, "isHidden", false);
  Blynk.setProperty(V7, "isHidden", false);
  Blynk.setProperty(V6, "isHidden", false);
  Blynk.setProperty(V5, "isHidden", false);
  Blynk.setProperty(V17, "isHidden", false);
  Blynk.setProperty(V13, "isHidden", false);
}
void syncstatus() {
  Blynk.virtualWrite(V0, RelayState1);
  Blynk.virtualWrite(V1, RelayState2);
  Blynk.virtualWrite(V2, RelayState);
}
//-------------------------
void updata() {
  Blynk.virtualWrite(V14, Result1);
}
//-------------------------
void readPower() // C3 - Cấp 1  - I0
{
  digitalWrite(S0pin, HIGH);
  digitalWrite(S1pin, HIGH);
  digitalWrite(S2pin, LOW);
  digitalWrite(S3pin, LOW);
  float rms0 = emon0.calcIrms(1480);
  if (rms0 < 3) {
    Irms0 = 0;
    yIrms0 = 0;
  } else if (rms0 >= 3) {
    Irms0 = rms0;
    yIrms0 = yIrms0 + 1;
    if ((yIrms0 > 3) && ((Irms0 >= data.SetAmpemax) || (Irms0 <= data.SetAmpemin))) {
      xSetAmpe = xSetAmpe + 1;
      if ((xSetAmpe >= 2) && (keyp)) {
        offcap1();
        xSetAmpe = 0;
        trip0 = true;
        if (keynoti) {
          Blynk.logEvent("error", String("Giếng lỗi: ") + Irms0 + String(" A"));
        }
      }
    } else {
      xSetAmpe = 0;
    }
  }
  Blynk.virtualWrite(V20, Irms0);
}
void readPower1() // C4 - Bơm 1  - I1
{
  digitalWrite(S0pin, LOW);
  digitalWrite(S1pin, LOW);
  digitalWrite(S2pin, HIGH);
  digitalWrite(S3pin, LOW);
  float rms1 = emon1.calcIrms(1480);
  if (rms1 < 3) {
    Irms1 = 0;
    yIrms1 = 0;
  } else if (rms1 >= 3) {
    Irms1 = rms1;
    yIrms1 = yIrms1 + 1;
    if ((yIrms1 > 3) && ((Irms1 >= data.SetAmpe1max) || (Irms1 <= data.SetAmpe1min))) {
      xSetAmpe1 = xSetAmpe1 + 1;
      if ((xSetAmpe1 >= 3) && (keyp)) {
        offbom1();
        trip1 = true;
        xSetAmpe1 = 0;
        if (keynoti) {
          Blynk.logEvent("error", String("Bơm 1 lỗi: ") + Irms1 + String(" A"));
        }
      }
    } else {
      xSetAmpe1 = 0;
      trip_run1 = false;
    }
  }
  Blynk.virtualWrite(V21, Irms1);
}
void readPower2() // C5 - Bơm 2  - I2
{
  digitalWrite(S0pin, HIGH);
  digitalWrite(S1pin, LOW);
  digitalWrite(S2pin, HIGH);
  digitalWrite(S3pin, LOW);
  float rms2 = emon2.calcIrms(1480);
  if (rms2 < 3) {
    Irms2 = 0;
    yIrms2 = 0;
  } else if (rms2 >= 3) {
    Irms2 = rms2;
    yIrms2 = yIrms2 + 1;
    if ((yIrms2 > 3) && ((Irms2 >= data.SetAmpe2max) || (Irms2 <= data.SetAmpe2min))) {
      xSetAmpe2 = xSetAmpe2 + 1;
      if ((xSetAmpe2 >= 2) && (keyp)) {
        offbom2();
        xSetAmpe2 = 0;
        trip2 = true;
        if (keynoti) {
          Blynk.logEvent("error", String("Bơm 2 lỗi: ") + Irms2 + String(" A"));
        }
      }
    } else {
      xSetAmpe2 = 0;
    }
  }
  Blynk.virtualWrite(V22, Irms2);
}
void temperature() { // Nhiệt độ
  Blynk.run();
  sensors.requestTemperatures();
  // Serial.println(sensors.getDeviceCount());
  for (byte i = 0; i < sensors.getDeviceCount(); i++) {
    temp[i] = sensors.getTempCByIndex(i);
  }
  if (temp[0] > 0) {
    Blynk.virtualWrite(V15, temp[0]);
    if ((temp[0] > data.set_temp0) && (temp_0)) {
      temp_0 = false;
      Blynk.logEvent("info", String("Nhiệt độ Bơm 2 quá cao: ") + temp[0] + String(" oC"));
    }
    if (temp[0] < 50) {
      temp_0 = true;
    }
  }
  if (temp[1] > 0) {
    Blynk.virtualWrite(V16, temp[1]);
    if ((temp[1] > data.set_temp1) && (temp_1)) {
      temp_1 = false;
      Blynk.logEvent("info", String("Nhiệt độ Bơm 1 quá cao: ") + temp[1] + String(" oC"));
    }
    if (temp[1] < 50) {
      temp_1 = true;
    }
  }
}
//-------------------------
void readPressure() // C2 - Ap Luc
{
  digitalWrite(S0pin, LOW);
  digitalWrite(S1pin, HIGH);
  digitalWrite(S2pin, LOW);
  digitalWrite(S3pin, LOW);
  float sensorValue = analogRead(A0);
  float Result;
  Result = (((sensorValue - 190) * 10) / (960 - 190));
  if (Result > 0) {
    value += Result;
    Result1 = value / 16.0;
    value -= Result1;
  }
}
//-------------------------
void rtctime() {
  DateTime now = rtc.now();
  if (Blynk.connected() == true) {
    Blynk.sendInternal("rtc", "sync");
    setTime(t);
    if ((now.day() != day()) || (now.hour() != hour()) || ((now.minute() - minute() > 1) || (minute() - now.minute() > 1))) {
      rtc.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
      DateTime now = rtc.now();
    }
  }
  Blynk.virtualWrite(V9, daysOfTheWeek[now.dayOfTheWeek()], ", ", now.day(), "/", now.month(), "/", now.year(), " - ", now.hour(), ":", now.minute(), ":", now.second());

  int nowtime = (now.hour() * 3600 + now.minute() * 60);
  bool wait = false;

  if (data.mode_cap2 == 1) { // Chạy 2 bơm
    if ((nowtime > data.b1_1_start && nowtime < data.b1_1_stop) || (nowtime > data.b1_2_start && nowtime < data.b1_2_stop) || (nowtime > data.b1_3_start && nowtime < data.b1_3_stop) || (nowtime > data.b1_4_start && nowtime < data.b1_4_stop)) {
      if (Irms1 == 0 && !trip1) { // Nếu bơm 1 đang tắt và không lỗi
        if (Irms2 == 0 && !time_run_2) {
          onbom1();
        } else if (time_run_2) {
          onbom1();
        }
      }
      if (Irms1 == 0 && !trip1 && time_run_1 && !trip_run1) {
        trip_run1 = true;
        if (keynoti)
          Blynk.logEvent("error", String("Bơm 1 bị lỗi không chạy kìa.\nKiểm tra lẹ."));
      }
      time_run_1 = true;
    } else {
      if (Irms1 != 0) {
        offbom1();
        if (!time_run_1) {
          if (keynoti)
            Blynk.logEvent("error", String("Lỗi lịch chạy!\nBơm 1 đang chạy: ") + Irms1 + String(" A"));
        }
      }
      time_run_1 = false;
    }
    if ((nowtime > data.b2_1_start && nowtime < data.b2_1_stop) || (nowtime > data.b2_2_start && nowtime < data.b2_2_stop) || (nowtime > data.b2_3_start && nowtime < data.b2_3_stop) || (nowtime > data.b2_4_start && nowtime < data.b2_4_stop)) {
      if (Irms2 == 0 && !trip2) {        // Nếu bơm 2 đang tắt và không lỗi
        if (Irms1 == 0 && !time_run_1) { // Nếu bơm 1 đang tắt và ngoài giờ chạy
          onbom2();
        } else if (time_run_1) { // Nếu bơm 1 trong giờ chạy
          onbom2();
        }
      }
      if (Irms2 == 0 && !trip2 && time_run_2 && !trip_run2) {
        trip_run2 = true;
        if (keynoti)
          Blynk.logEvent("error", String("Bơm 2 bị lỗi không chạy kìa.\nKiểm tra lẹ."));
        timer1.setTimeout(30 * 60 * 1000, []() {
          trip_run2 = false;
        });
      }
      time_run_2 = true;
    } else {
      if (Irms2 != 0) {
        offbom2();
        if (!time_run_2) {
          if (keynoti)
            Blynk.logEvent("error", String("Lỗi lịch chạy!\nBơm 2 đang chạy: ") + Irms2 + String(" A"));
        }
      }
      time_run_2 = false;
    }
  }
}
//-------------------------------------------------------------------
BLYNK_WRITE(InternalPinRTC) // check the value of InternalPinRTC
{
  t = param.asLong();
}
BLYNK_WRITE(V0) // Bơm 1
{
  if ((key) && (!trip1)) {
    if (param.asInt() == LOW) {
      offbom1();
    } else
      onbom1();
  } else {
    Blynk.virtualWrite(V0, RelayState1);
  }
}
BLYNK_WRITE(V1) // Bơm 2
{
  if ((key) && (!trip2)) {
    if (param.asInt() == LOW) {
      offbom2();
    } else
      onbom2();
  } else {
    Blynk.virtualWrite(V1, RelayState2);
  }
}
BLYNK_WRITE(V2) // Giếng
{
  if ((key) && (!trip0)) {
    if (param.asInt() == LOW) {
      offcap1();
    } else
      oncap1();
  } else {
    Blynk.virtualWrite(V2, RelayState);
  }
}
BLYNK_WRITE(V3) // Chọn chế độ Cấp 2
{
  if (keySet) {
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
    Blynk.virtualWrite(V3, data.mode_cap2);
}
BLYNK_WRITE(V4) // Chọn người trực
{
  if (key) {
    switch (param.asInt()) {
    case 0: { // Quang
      data.man = 0;
      break;
    }
    case 1: { // GUEST
      data.man = 1;
      break;
    }
    }
  } else
    Blynk.virtualWrite(V4, data.man);
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
  case 3: { // Nhiet do 1
    c = 3;
    Blynk.virtualWrite(V7, data.set_temp0);
    Blynk.virtualWrite(V6, 0);
    break;
  }
  case 4: { // Nhiet do 2
    c = 4;
    Blynk.virtualWrite(V7, data.set_temp1);
    Blynk.virtualWrite(V6, 0);
    break;
  }
  }
}
BLYNK_WRITE(V6) // min
{
  if (keySet) {
    if (c == 0) {
      data.SetAmpemin = param.asInt();
    } else if (c == 1) {
      data.SetAmpe1min = param.asInt();
    } else if (c == 2) {
      data.SetAmpe2min = param.asInt();
    }
  } else {
    Blynk.virtualWrite(V6, 0);
  }
}
BLYNK_WRITE(V7) // max
{
  if (keySet) {
    if (c == 0) {
      data.SetAmpemax = param.asInt();
    } else if (c == 1) {
      data.SetAmpe1max = param.asInt();
    } else if (c == 2) {
      data.SetAmpe2max = param.asInt();
    } else if (c == 3) {
      data.set_temp0 = param.asInt();
    } else if (c == 4) {
      data.set_temp1 = param.asInt();
    }
  } else {
    Blynk.virtualWrite(V7, 0);
  }
}
BLYNK_WRITE(V8) // Info
{
  if (param.asInt() == 1) {
    terminal.clear();
    if (data.mode_cap2 == 0) {
      Blynk.virtualWrite(V10, "Mode: MAN");
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

      Blynk.virtualWrite(V10, "Mode: AUTO\nTimer:\nPump 1: ", hour_start_b1_1, "h", minute_start_b1_1, " -> ", hour_stop_b1_1, "h", minute_stop_b1_1, "\nPump 2: ", hour_start_b2_1, "h", minute_start_b2_1, " -> ", hour_stop_b2_1, "h", minute_stop_b2_1, "\nPump 1: ", hour_start_b1_2, "h", minute_start_b1_2, " -> ", hour_stop_b1_2, "h", minute_stop_b1_2, "\nPump 2: ", hour_start_b2_2, "h", minute_start_b2_2, " -> ", hour_stop_b2_2, "h", minute_stop_b2_2, "\nPump 1: ", hour_start_b1_3, "h", minute_start_b1_3, " -> ", hour_stop_b1_3, "h", minute_stop_b1_3, "\nPump 2: ", hour_start_b2_3, "h", minute_start_b2_3, " -> ", hour_stop_b2_3, "h", minute_stop_b2_3, "\nPump 1: ", hour_start_b1_4, "h", minute_start_b1_4, " -> ", hour_stop_b1_4, "h", minute_stop_b1_4, "\nPump 2: ", hour_start_b2_4, "h", minute_start_b2_4, " -> ", hour_stop_b2_4, "h", minute_stop_b2_4);
    }
  } else {
    terminal.clear();
  }
  timer.restartTimer(timer_1);
  timer.restartTimer(timer_2);
}
BLYNK_WRITE(V10) // String
{
  String dataS = param.asStr();
  if (dataS == "M") {
    terminal.clear();
    key = true;
    Blynk.virtualWrite(V10, "Người vận hành: 'M.Quang'\nKích hoạt trong 10s\n");
    timer1.setTimeout(10000, []() {
      key = false;
      terminal.clear();
    });
  } else if (dataS == "active") {
    terminal.clear();
    key = true;
    keySet = true;
    visible();
    Blynk.virtualWrite(V10, "KHÔNG sử dụng phần mềm cho đến khi thông báo này mất.\n");
  } else if (dataS == "deactive") {
    terminal.clear();
    key = false;
    keySet = false;
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
    pcf8575.digitalWrite(pincap1, HIGH);
    Blynk.virtualWrite(V10, "Đã RESET! \nNhập mã để điều khiển!\n");
  } else if (dataS == "save_num") {
    terminal.clear();
    Blynk.virtualWrite(V10, "Số lần ghi EEPROM: ", data.save_num);
  } else if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V10, "ESP UPDATE...");
    update_fw();
  } else if (dataS == "rst") {
    terminal.clear();
    Blynk.virtualWrite(V10, "ESP RESTART...");
    ESP.restart();
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
    Blynk.setProperty(V11, "labels", menu);
    switch (param.asInt()) {
    case 0: { // Bơm 1 - Lần 1
      if (keySet)
        b = 0;
      Blynk.virtualWrite(V18, data.b1_1_start, data.b1_1_stop, tz);
      break;
    }
    case 1: { // Bơm 2 - Lần 1
      if (keySet)
        b = 1;
      Blynk.virtualWrite(V18, data.b2_1_start, data.b2_1_stop, tz);
      break;
    }
    case 2: { // Bơm 1 - Lần 2
      if (keySet)
        b = 2;
      Blynk.virtualWrite(V18, data.b1_2_start, data.b1_2_stop, tz);
      break;
    }
    case 3: { // Bơm 2 - Lần 2
      if (keySet)
        b = 3;
      Blynk.virtualWrite(V18, data.b2_2_start, data.b2_2_stop, tz);
      break;
    }
    case 4: { // Bơm 1 - Lần 3
      if (keySet)
        b = 4;
      Blynk.virtualWrite(V18, data.b1_3_start, data.b1_3_stop, tz);
      break;
    }
    case 5: { // Bơm 2 - Lần 3
      if (keySet)
        b = 5;
      Blynk.virtualWrite(V18, data.b2_3_start, data.b2_3_stop, tz);
      break;
    }
    case 6: { // Bơm 1 - Lần 4
      if (keySet)
        b = 6;
      Blynk.virtualWrite(V18, data.b1_4_start, data.b1_4_stop, tz);
      break;
    }
    case 7: { // Bơm 2 - Lần 4
      if (keySet)
        b = 7;
      Blynk.virtualWrite(V18, data.b2_4_start, data.b2_4_stop, tz);
      break;
    }
    }
  }
}
BLYNK_WRITE(V13) // Bảo vệ
{
  if (keySet) {
    int data13 = param.asInt();
    if (data13 == LOW) {
      keyp = false;
    } else {
      keyp = true;
    }
  } else
    Blynk.virtualWrite(V13, keyp);
}
BLYNK_WRITE(V17) // Thông báo
{
  if (keySet) {
    int data17 = param.asInt();
    if (data17 == LOW) {
      keynoti = false;
    } else {
      keynoti = true;
    }
  } else
    Blynk.virtualWrite(V17, keynoti);
}
BLYNK_WRITE(V18) // Time input
{
  if (keySet) {
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
    }
  } else
    Blynk.virtualWrite(V18, 0);
}
//-------------------------------------------------------------------
void setup() {
  pinMode(S0pin, OUTPUT);
  pinMode(S1pin, OUTPUT);
  pinMode(S2pin, OUTPUT);
  pinMode(S3pin, OUTPUT);
  pinMode(pin_usb, OUTPUT);
  digitalWrite(pin_usb, LOW);
  delay(1000);
  digitalWrite(pin_usb, HIGH);
  delay(15000);
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(5000);
  //---------------------------------------------------------------------------------

  emon0.current(A0, 117);
  emon1.current(A0, 117);
  emon2.current(A0, 117);

  // Wire.begin();
  sensors.begin(); // DS18B20 start

  pcf8575.pinMode(pincap1, OUTPUT);
  pcf8575.pinMode(pinbom_1, OUTPUT);
  pcf8575.pinMode(pinbom_2, OUTPUT);
  pcf8575.pinMode(P4, OUTPUT);
  pcf8575.pinMode(P3, OUTPUT);
  pcf8575.begin();
  pcf8575.digitalWrite(pincap1, HIGH);  // Gieng
  pcf8575.digitalWrite(pinbom_1, HIGH); // Bom 1
  pcf8575.digitalWrite(pinbom_2, HIGH); // Bom 2
  pcf8575.digitalWrite(P4, HIGH);
  pcf8575.digitalWrite(P3, HIGH);

  rtc.begin();

  eeprom.initialize();
  eeprom.readBytes(address, sizeof(dataDefault), (byte *)&data);

  terminal.clear();

  timer.setTimeout(5000, []() {
    timer_2 = timer.setInterval(301L, []() {
      readPressure();
      timer.restartTimer(timer_2);
    });
    timer_1 = timer.setInterval(993L, []() {
      readPower();
      readPower1();
      readPower2();
      temperature();
      updata();
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
    });
    timer_5 = timer.setInterval(15006L, []() {
      rtctime();
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
    });
    timer_3 = timer.setInterval(599011L, []() {
      syncstatus();
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
    });
    timer.setInterval(900005L, []() {
      connectionstatus();
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
    });
  });
}

void loop() {
  Blynk.run();
  timer.run();
  timer1.run();
}
