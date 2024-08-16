/*-Color = 9900FF
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
 *V21 - status pre
 *V22 - Do sau
 *V23 - Dung tich
 *V24 - String Clo
 *V25 - Clo input
 *V26 - Clo
 *V27 - check clo
 *V28 - Status Volume
 *V29 - LLG1_1m3
 *V30 - LLG1_24h
 *V31 - LLG1_RL
 */
//------------------
#define BLYNK_TEMPLATE_ID "TMPL6usi7FSqp"
#define BLYNK_TEMPLATE_NAME "TRẠM SỐ 3"
#define BLYNK_AUTH_TOKEN "R_V1bJ9xeyl6Yokg1rhlaLhK-NYcDVpx"
#define BLYNK_FIRMWARE_VERSION "240810"
//------------------
#include "EmonLib.h"
#include "PCF8575.h"
#include "RTClib.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <Eeprom24C32_64.h>
#include <SPI.h>
#include <WiFiClientSecure.h>
#include <WidgetRTC.h>
#include <Wire.h>
//------------------
#define APP_DEBUG
#define BLYNK_PRINT Serial
#define EEPROM_ADDRESS 0x57
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/Tram_So_3/.pio/build/nodemcuv2/firmware.bin"
String server_main = "http://sgp1.blynk.cloud/external/api/";
//------------------
PCF8575 pcf8575_1(0x20);
EnergyMonitor emon0, emon1, emon2;
RTC_DS3231 rtc_module;
WiFiClient client;
HTTPClient http;
WidgetRTC rtc_widget;
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
//-----------------------------
const char *ssid = "TTTV Xay Dung";
const char *password = "0723841249";
// const char* ssid = "iPhone 13";
// const char* password = "12345687";
const int pin_G1 = P1;
const int pin_B1 = P2;
const int pin_rst = P5;

const int S0pin = P15;
const int S1pin = P14;
const int S2pin = P13;
const int S3pin = P12;
const word address = 0;
//------------------
float Irms0, Irms1, Irms2;
float conlai, clo_cache = 0;
bool trip0 = false, trip1 = false, trip2 = false;
bool key = false, keytank = true;
bool status_pre = false, time_off_c1 = true;
bool noti_cap1 = true, maxtank = false, blynk_first_connect = false;
int xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0;
int c, b, i = 0;
int timer_I;
int LLG1_1m3, reboot_num = 0;

unsigned long int xIrms0 = 0, xIrms1 = 0, xIrms2 = 0;
unsigned long int yIrms0 = 0, yIrms1 = 0, yIrms2 = 0;
unsigned long rest_time = 0, dem_bom = 0;
long t;
uint32_t timestamp;
char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";
//------------------
struct Data {
  byte SetAmpemax, SetAmpemin;
  byte SetAmpe1max, SetAmpe1min;
  byte SetAmpe2max, SetAmpe2min;
  byte mode_cap2;
  int t1_start, t1_stop;
  int save_num;
  byte status_g1, status_b1, status_nk1;
  byte key_noti, key_protect, rualoc;
  float clo;
  int time_clo, LLG1_RL;
} data, dataCheck;
const struct Data dataDefault = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//------------------
WidgetTerminal terminal(V11);
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
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur,
                total);
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
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n",
                  ESPhttpUpdate.getLastError(),
                  ESPhttpUpdate.getLastErrorString().c_str());
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
  String server_path = server_main + "batch/update?token=" + BLYNK_AUTH_TOKEN +
                       "&V2=" + Irms0 +
                       "&V3=" + Irms1 +
                       "&V19=" + Irms2;
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
    Blynk.setProperty(V11, "label",
                      data.save_num);
  }
}
void on_cap1() {
  if ((data.status_g1 != HIGH) && (trip0 == false)) {
    data.status_g1 = HIGH;
    savedata();
    Blynk.virtualWrite(V0, data.status_g1);
    pcf8575_1.digitalWrite(pin_G1, data.status_g1);
  }
}
void off_cap1() {
  if (data.status_g1 != LOW) {
    data.status_g1 = LOW;
    savedata();
    Blynk.virtualWrite(V0, data.status_g1);
  }
  pcf8575_1.digitalWrite(pin_G1, data.status_g1);
}
void on_bom() {
  if ((data.status_b1 != HIGH) && (trip1 == false)) {
    data.status_b1 = HIGH;
    savedata();
    Blynk.virtualWrite(V1, data.status_b1);
    pcf8575_1.digitalWrite(pin_B1, data.status_b1);
  }
}
void off_bom() {
  if (data.status_b1 != LOW) {
    data.status_b1 = LOW;
    savedata();
    Blynk.virtualWrite(V1, data.status_b1);
  }
  pcf8575_1.digitalWrite(pin_B1, data.status_b1);
}
void on_nenkhi() {
  if ((data.status_nk1 != HIGH) && (trip1 == false)) {
    data.status_nk1 = HIGH;
    savedata();
    Blynk.virtualWrite(V18, data.status_nk1);
    on_cap1();
  }
}
void off_nenkhi() {
  if (data.status_nk1 != LOW) {
    data.status_nk1 = LOW;
    savedata();
    Blynk.virtualWrite(V18, data.status_nk1);
  }
  off_cap1();
}
void hidden() {
  Blynk.setProperty(V12, V13, V15, V14, V10, V9, V8, "isHidden", true);
}
void visible() {
  Blynk.setProperty(V12, V13, V15, V14, V10, V9, V8, "isHidden", false);
}
void off_time() {
  time_off_c1 = true;
  if (Irms1 != 0) {
    off_bom();
  }
}
void on_time() {
  time_off_c1 = false;
  if ((!trip1) && (Irms1 == 0)) {
    on_bom();
  }
  if ((!trip0) && (Irms0 == 0)) {
    on_cap1();
  }
}
void off_time_cap1() {
  time_off_c1 = true;
  if (Irms0 != 0) {
    off_cap1();
  }
}
//-------------------------------------------------------------------
void readPower() { // C2 - Cấp 1  - I0
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms0 = emon0.calcIrms(740);
  if (rms0 < 2) {
    Irms0 = 0;
    yIrms0 = 0;
    if ((data.status_g1 == HIGH) && (conlai <= 190) && (status_pre)) {
      xIrms0++;
      if ((xIrms0 > 3) && (data.key_protect)) {
        xIrms0 = 0;
        off_cap1();
        trip0 = true;
        if (data.key_noti)
          Blynk.logEvent(
              "error", String("Giếng lỗi\nKhông đo được DÒNG ĐIỆN"));
      }
    }
    if ((noti_cap1) && (status_pre) && (conlai <= 80) && (!time_off_c1)) {
      if (data.key_noti) {
        noti_cap1 = false;
        Blynk.logEvent("info",
                       String("Bơm Giếng không chạy. Xin kiểm tra."));
      }
    }
  } else if (rms0 >= 2) {
    yIrms0 = yIrms0 + 1;
    Irms0 = rms0;
    if (yIrms0 > 3) {
      if ((Irms0 >= data.SetAmpemax) || (Irms0 <= data.SetAmpemin)) {
        xSetAmpe = xSetAmpe + 1;
        if ((xSetAmpe > 3) && (data.key_protect)) {
          off_cap1();
          xSetAmpe = 0;
          trip0 = true;
          if (data.key_noti) {
            Blynk.logEvent("error", String("Cấp 1 lỗi: ") + Irms0 +
                                        String(" A"));
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
  // Blynk.run();
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms1 = emon1.calcIrms(740);
  if (rms1 < 2) {
    Irms1 = 0;
    yIrms1 = 0;
    if ((data.status_b1 == HIGH) && (conlai > 180)) {
      xIrms1++;
      if ((xIrms1 > 3) && (data.key_protect)) {
        xIrms1 = 0;
        off_bom();
        trip1 = true;
        if (data.key_noti)
          Blynk.logEvent("error",
                         String("Bơm lỗi\nKhông đo được DÒNG ĐIỆN"));
      }
    }
  } else if (rms1 >= 2) {
    Irms1 = rms1;
    yIrms1 = yIrms1 + 1;
    if (yIrms1 > 3) {
      if ((Irms1 >= data.SetAmpe1max) || (Irms1 <= data.SetAmpe1min)) {
        xSetAmpe1 = xSetAmpe1 + 1;
        if ((xSetAmpe1 > 3) && (data.key_protect)) {
          off_bom();
          xSetAmpe1 = 0;
          trip1 = true;
          if (data.key_noti) {
            Blynk.logEvent("error", String("Cấp 2 lỗi: ") + Irms1 +
                                        String(" A"));
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
  // Blynk.run();
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, HIGH);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms2 = emon2.calcIrms(740);
  if (rms2 < 2) {
    Irms2 = 0;
    yIrms2 = 0;
  } else if (rms2 >= 2) {
    Irms2 = rms2;
    yIrms2 = yIrms2 + 1;
    if ((yIrms2 > 3) &&
        ((Irms2 >= data.SetAmpe2max) || (Irms2 <= data.SetAmpe2min))) {
      xSetAmpe2 = xSetAmpe2 + 1;
      if ((xSetAmpe2 > 3) && (data.key_protect)) {
        off_nenkhi();
        xSetAmpe2 = 0;
        trip1 = true; // Do Nén khí và Bơm 1 xài chung 1 khởi động từ
        if (data.key_noti) {
          Blynk.logEvent("error", String("Máy nén khí lỗi: ") +
                                      Irms2 + String(" A"));
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
    if ((now.day() != day()) || (now.hour() != hour()) ||
        ((now.minute() - minute() > 2) || (minute() - now.minute() > 2))) {
      rtc_module.adjust(
          DateTime(year(), month(), day(), hour(), minute(), second()));
    }
  }
  Blynk.virtualWrite(V20, daysOfTheWeek[now.dayOfTheWeek()], ", ", now.day(),
                     "/", now.month(), "/", now.year(), " - ", now.hour(),
                     ":", now.minute(), ":", now.second());
  timestamp = now.unixtime();

  int nowtime = (now.hour() * 3600 + now.minute() * 60);

  if (data.mode_cap2 == 1) {
    if (data.t1_start > data.t1_stop) {
      if (((nowtime + 1800) > data.t1_stop) &&
          (nowtime < data.t1_start)) {
        off_time_cap1();
        trip0 = true;
        if (!maxtank &&
            ((nowtime >= data.t1_stop) && (nowtime < data.t1_start))) {
          off_time();
          trip0 = false;
        }
      }
      if ((nowtime < data.t1_stop) || (nowtime > data.t1_start) ||
          maxtank) {
        on_time();
      }
    } else if (data.t1_start < data.t1_stop) {
      if (((nowtime + 1800) > data.t1_stop) ||
          (nowtime < data.t1_start)) {
        off_time_cap1();
        trip0 = true;
        if (!maxtank &&
            ((nowtime > data.t1_stop) || (nowtime < data.t1_start))) {
          off_time();
          trip0 = false;
        }
      }
      if (((nowtime < data.t1_stop) && (nowtime > data.t1_start)) ||
          maxtank) {
        on_time();
      }
    } else
      on_time();
  }
}
//-------------------------------------------------------------------
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
        Blynk.virtualWrite(V11, "I2C device found at address 0x0", stringOne, " !\n");
      Blynk.virtualWrite(V11, "I2C device found at address 0x", stringOne, " !\n");
      nDevices++;
    } else if (error == 4) {
      stringOne = String(address, HEX);

      if (address < 16)
        Blynk.virtualWrite(V11, "Unknown error at address 0x0", stringOne, " !\n");
      Blynk.virtualWrite(V11, "I2C device found at address 0x", stringOne, " !\n");
    }
  }
  if (nDevices == 0)
    Blynk.virtualWrite(V11, "No I2C devices found\n");
}
//-------------------------------------------------------------------
BLYNK_WRITE(V0) // Gieng
{
  if ((key) && (!trip0)) {
    if (param.asInt() == LOW) {
      off_cap1();
    } else {
      on_cap1();
      Serial.println("222");
    }
  }
  Blynk.virtualWrite(V0, data.status_g1);
}
BLYNK_WRITE(V1) // Bơm 1
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
BLYNK_WRITE(V5) // Con lai
{
  conlai = param.asFloat();
  if ((conlai > 220) && (status_pre)) {
    maxtank = true;
    if (keytank) {
      if (data.key_noti)
        Blynk.logEvent("info", String("Nước trong bể cao vượt mức ") +
                                   (conlai - 220) + String(" cm"));
      keytank = false;
      timer1.setTimeout(600000L, []() { keytank = true; });
    }
  }
  if ((conlai < 170) && (status_pre)) {
    maxtank = false;
  }
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
    }
  } else {
    Blynk.virtualWrite(V10, 0);
  }
}
BLYNK_WRITE(V11) // String
{
  String dataS = param.asStr();
  if (dataS == "ts3") {
    terminal.clear();
    key = true;
    Blynk.virtualWrite(V11,
                       "Người vận hành: 'V.Tài'\nKích hoạt trong 15s\n");
    timer1.setTimeout(15000, []() {
      key = false;
      terminal.clear();
    });
  } else if (dataS == "active") {
    terminal.clear();
    key = true;
    visible();
    Blynk.virtualWrite(
        V11, "KHÔNG sử dụng phần mềm cho đến khi thông báo này mất.\n");
  } else if (dataS == "deactive") {
    terminal.clear();
    key = false;
    hidden();
    Blynk.virtualWrite(V11, "Ok!\nNhập mã để điều khiển!\n");
  } else if (dataS == "save") {
    terminal.clear();
    savedata();
    Blynk.virtualWrite(V11, "Đã lưu cài đặt.\n");
  } else if (dataS == "reset") {
    terminal.clear();
    trip1 = false;
    trip0 = false;
    // on_cap1();
    Blynk.virtualWrite(V11, "Đã RESET! \nNhập mã để điều khiển!\n");
  } else if (dataS == "rst") {
    terminal.clear();
    Blynk.virtualWrite(V11, "ESP khởi động lại sau 3s");
    delay(3000);
    ESP.restart();
  } else if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V11, "UPDATE FIRMWARE...");
    update_fw();
  } else if ((dataS == "ok") || (dataS == "Ok") || (dataS == "OK") ||
             (dataS == "oK")) {
    if (clo_cache > 0) {
      data.clo = clo_cache;
      clo_cache = 0;
      data.time_clo = timestamp;
      Blynk.virtualWrite(V26, data.clo);
      savedata();
      terminal.clear();
      Blynk.virtualWrite(V11, "Đã lưu - CLO:", data.clo, "kg");
    }
  } else if (dataS == "i2c") {
    terminal.clear();
    i2c_scaner();
  } else {
    Blynk.virtualWrite(V11, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
BLYNK_WRITE(V13) // Time input
{
  if (key) {
    TimeInputParam t(param);
    if (t.hasStartTime()) {
      data.t1_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
    }
    if (t.hasStopTime()) {
      data.t1_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
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
BLYNK_WRITE(V16) // Rửa lọc
{
  if (key) {
    switch (param.asInt()) {
    case 0: { // Tắt
      data.rualoc = 0;
      if (data.LLG1_RL != 0) {
        Blynk.virtualWrite(V31, LLG1_1m3 - data.LLG1_RL);
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
      Blynk.virtualWrite(V11, "Mode: MAN");
    } else if (data.mode_cap2 == 1) {
      int moingay_start_h = data.t1_start / 3600;
      int moingay_start_m =
          (data.t1_start - (moingay_start_h * 3600)) / 60;
      int moingay_stop_h = data.t1_stop / 3600;
      int moingay_stop_m = (data.t1_stop - (moingay_stop_h * 3600)) / 60;
      Blynk.virtualWrite(V11, "Mode: AUTO - Mỗi ngày\nThời gian nghỉ: ",
                         moingay_stop_h, " : ", moingay_stop_m, " -> ",
                         moingay_start_h, " : ", moingay_start_m);
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
BLYNK_WRITE(V21) // Status_pre
{
  status_pre = param.asInt();
}
//-------------------------
BLYNK_WRITE(V25) // Clo input
{
  if (param.asFloat() > 0) {
    terminal.clear();
    clo_cache = param.asFloat();
    Blynk.virtualWrite(
        V11, " Lượng CLO châm hôm nay:", clo_cache,
        "kg\n Vui lòng kiểm tra kỹ, nếu đúng hãy nhập 'OK' để lưu");
  }
}
BLYNK_WRITE(V27) // Check Clo
{
  if (param.asInt() == 1) {
    DateTime dt(data.time_clo);
    terminal.clear();
    Blynk.virtualWrite(V11, "Châm CLO:", data.clo, "kg vào lúc", dt.hour(),
                       ":", dt.minute(), "-", dt.day(), "/", dt.month(),
                       "/", dt.year());
  }
}
BLYNK_WRITE(V29) // Lưu lượng G1_1m3
{
  LLG1_1m3 = param.asInt();
}
//-------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(7000);
  //------------------------------------------------------------------

  emon0.current(A0, 105);
  emon1.current(A0, 105);
  emon2.current(A0, 105);

  Wire.begin();
  rtc_module.begin();
  eeprom.initialize();
  eeprom.readBytes(address, sizeof(dataDefault), (byte *)&data);

  pcf8575_1.begin();
  pcf8575_1.pinMode(S0pin, OUTPUT);
  pcf8575_1.pinMode(S1pin, OUTPUT);
  pcf8575_1.pinMode(S2pin, OUTPUT);
  pcf8575_1.pinMode(S3pin, OUTPUT);

  pcf8575_1.pinMode(pin_G1, OUTPUT);
  pcf8575_1.digitalWrite(pin_G1, data.status_g1);
  pcf8575_1.pinMode(pin_B1, OUTPUT);
  pcf8575_1.digitalWrite(pin_B1, data.status_b1);
  pcf8575_1.pinMode(pin_rst, OUTPUT);
  pcf8575_1.digitalWrite(pin_rst, HIGH);

  //------------------------------------
  timer.setTimeout(5000L, []() {
    timer_I = timer1.setInterval(1083L, []() {
      readPower();
      readPower1();
      readPower2();
      up();
      timer1.restartTimer(timer_I);
    });
    timer1.setInterval(15006L, []() {
      rtctime();
      timer1.restartTimer(timer_I);
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
