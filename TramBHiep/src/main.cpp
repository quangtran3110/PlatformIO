/*-Color = 48c9b0
 *V0 - Btn Cap 1
 *V1 - Btn Cap 2
 *V2 - I0 - Cap 1
 *V3 - I1 - Cap 2
 *V4 - Ap Luc
 *V5 - Nhiet Do
 *V6 -
 *V7 -
 *V8 - Menu
 *V9 - min A
 *V10 - max A
 *V11 - String
 *V12 -
 *V13 -
 *V14 - bao ve
 *V15 - thong bao
 *V16 -
 *V17 -
 *V18 -
 *V19 -
 *V20 - date/time
 *V21 -
 *V22 -
 *V23 -
 *V24 -
 *V25 -
 */
//-------------------------------------------------------------------
/*
#define BLYNK_TEMPLATE_ID "TMPLdGfzkVvi"
#define BLYNK_TEMPLATE_NAME "Bình Hiệp"
#define BLYNK_AUTH_TOKEN "tCAptndMM6EXqRkWvj_6tK76_mi7gbKf"
*/
#define BLYNK_TEMPLATE_ID "TMPL6Fz-ilSIi"
#define BLYNK_TEMPLATE_NAME "TRẠM BÌNH HIỆP"
#define BLYNK_AUTH_TOKEN "hTIQQoegUO-4hIC26z6pmy2lW9pHtv26"

const char *ssid = "net";
const char *password = "Abcd@1234";
//-------------------------------------------------------------------
#define BLYNK_PRINT Serial
#define BLYNK_FIRMWARE_VERSION "240827"
#define APP_DEBUG

#pragma region // Library
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
WiFiClient client;
HTTPClient http;

#include "PCF8575.h"
PCF8575 pcf8575(0x20);

#include "EmonLib.h"
EnergyMonitor emon0, emon1;

#include "RTClib.h"
#include <TimeLib.h>
RTC_DS3231 rtc;

#include <Eeprom24C32_64.h>
#define EEPROM_ADDRESS 0x57
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);

#include <DallasTemperature.h>
#define ONE_WIRE_BUS 0
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress temp = {0x28, 0x83, 0x8B, 0x79, 0xA2, 0x01, 0x03, 0xB2};

#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/TramBHiep/.pio/build/nodemcuv2/firmware.bin"

const int S0pin = 14;
const int S1pin = 12;
const int S2pin = 13;
const int S3pin = 15;
const int pincap1 = P7;
const int pinbom = P6;
const int pinUSB = 16;
const word address = 0;
long t;
char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";
bool key = false, keySet = false, keyp = true;
bool trip0 = false, trip1 = false, trip_mcp = false;
bool timer_I_status, connect = true;
int c, i = 0, reboot_num = 0;
int RelayState, RelayState1;
int timer_1, timer_2, timer_3;
int xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0;
int status_cap1 = HIGH, status_bom = HIGH;
int btnState = HIGH, btnState1 = HIGH;
float Irms0, Irms1, value, Result1, te;
unsigned long int xIrms0 = 0, xIrms1 = 0;
unsigned long int yIrms0 = 0, yIrms1 = 0;

struct Data {
  float SetAmpemax, SetAmpemin;
  float SetAmpe1max, SetAmpe1min;
  float pre_min;
  int save_num;
  byte noti, protect;
} data, dataCheck;
const struct Data dataDefault = {0, 0, 0, 0, 0, 0, 0, 0};
#pragma endregion

WidgetTerminal terminal(V11);

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
    Blynk.setProperty(V11, "label", BLYNK_FIRMWARE_VERSION, "-EEPROM ", data.save_num);
  }
}
//-------------------------
void reset_USB() {
  digitalWrite(pinUSB, LOW);
  delay(2000);
  digitalWrite(pinUSB, HIGH);
}
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
    } else if (reboot_num == 5) {
      reset_USB();
    } else if (reboot_num == 10) {
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
//-------------------------
void on_cap1() // Kích thường đ��ng
{
  status_cap1 = HIGH;
  pcf8575.digitalWrite(pincap1, status_cap1);
}
void off_cap1() {
  status_cap1 = LOW;
  pcf8575.digitalWrite(pincap1, status_cap1);
}
void on_bom() // Kích thường đóng
{
  status_bom = HIGH;
  pcf8575.digitalWrite(pinbom, status_bom);
}
void off_bom() {
  status_bom = LOW;
  pcf8575.digitalWrite(pinbom, status_bom);
}
void hidden() {
  Blynk.setProperty(V8, V9, V10, V14, V15, "isHidden", true);
}
void visible() {
  Blynk.setProperty(V8, V9, V10, V14, V15, "isHidden", false);
}
void syncstatus() {
  Blynk.virtualWrite(V0, status_cap1);
  Blynk.virtualWrite(V1, status_bom);
}
//-------------------------
void updata() {
  Blynk.virtualWrite(V4, Result1);
}
//-------------------------
void temperature() {
  sensors.requestTemperatures();
  te = sensors.getTempC(temp);
  // Serial.println(te);
}
//-------------------------
void readPower() // C3 - Cấp 1  - I0
{
  Blynk.run();
  digitalWrite(S0pin, HIGH);
  digitalWrite(S1pin, HIGH);
  digitalWrite(S2pin, LOW);
  digitalWrite(S3pin, LOW);
  float rms0 = emon0.calcIrms(1480);
  if (rms0 < 2) {
    Irms0 = 0;
    yIrms0 = 0;
  } else if (rms0 >= 2) {
    yIrms0 = yIrms0 + 1;
    Irms0 = rms0;
    if (yIrms0 > 3) {
      if ((Irms0 >= data.SetAmpemax) || (Irms0 <= data.SetAmpemin)) {
        xSetAmpe = xSetAmpe + 1;
        if ((xSetAmpe >= 2) && (keyp)) {
          off_cap1();
          xSetAmpe = 0;
          trip0 = true;
          if (data.noti) {
            Blynk.logEvent("error", String("Cấp 1 lỗi: ") + Irms0 + String(" A"));
          }
        }
      } else {
        xSetAmpe = 0;
      }
    }
  }
  Blynk.virtualWrite(V2, Irms0);
}
void readPower1() // C4 - Bơm    - I1
{
  if (te > 0) {
    Blynk.virtualWrite(V5, te);
  }
  Blynk.run();
  digitalWrite(S0pin, LOW);
  digitalWrite(S1pin, LOW);
  digitalWrite(S2pin, HIGH);
  digitalWrite(S3pin, LOW);
  float rms1 = emon1.calcIrms(1480);
  if (rms1 < 2) {
    Irms1 = 0;
    yIrms1 = 0;
  } else if (rms1 >= 2) {
    Irms1 = rms1;
    yIrms1 = yIrms1 + 1;
    if (yIrms1 > 3) {
      if ((Irms1 >= data.SetAmpe1max) || (Irms1 <= data.SetAmpe1min)) {
        xSetAmpe1 = xSetAmpe1 + 1;
        if ((xSetAmpe1 >= 2) && (keyp)) {
          off_bom();
          xSetAmpe1 = 0;
          trip1 = true;
          if (data.noti) {
            Blynk.logEvent("error", String("Cấp 2 lỗi: ") + Irms1 + String(" A"));
          }
        }
      } else {
        xSetAmpe1 = 0;
      }
    }
  }
  Blynk.virtualWrite(V3, Irms1);
}
//-------------------------
void readPressure() // C2 - Ap Luc
{
  digitalWrite(S0pin, LOW);
  digitalWrite(S1pin, HIGH);
  digitalWrite(S2pin, LOW);
  digitalWrite(S3pin, LOW);
  float sensorValue = analogRead(A0);
  // Serial.print("sensorValue: ");
  // Serial.println(sensorValue);
  float Result;
  Result = (((sensorValue - 190) * 10) / (985 - 190));
  if (Result > 0) {
    value += Result;
    Result1 = value / 16;
    value -= Result1;
  }
  // Serial.print("Result1: ");
  // Serial.println(Result1);
}
//-------------------------
void rtctime() {
  DateTime now = rtc.now();
  if (Blynk.connected() == true) {
    Blynk.sendInternal("rtc", "sync");
    setTime(t);
    if ((now.day() != day()) || (now.hour() != hour()) || ((now.minute() - minute() > 2) || (minute() - now.minute() > 2))) {
      rtc.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
      DateTime now = rtc.now();
    }
  }
  Blynk.virtualWrite(V20, daysOfTheWeek[now.dayOfTheWeek()], ", ", now.day(), "/", now.month(), "/", now.year(), " - ", now.hour(), ":", now.minute(), ":", now.second());

  if (Result1 < data.pre_min) {
    i = i + 1;
    if ((i >= 2) && (Irms1 == 0) && (data.noti)) {
      Blynk.logEvent("info", String("Lỗi BƠM không chạy, xin kiểm tra."));
    }
  } else {
    i = 0;
  }
}
//-------------------------
BLYNK_WRITE(InternalPinRTC) // check the value of InternalPinRTC
{
  t = param.asLong();
}
BLYNK_WRITE(V0) // Gieng
{
  if ((key) && (!trip0)) {
    RelayState = param.asInt();
    if (RelayState == LOW) {
      off_cap1();
    } else {
      on_cap1();
    }
  }
  Blynk.virtualWrite(V0, status_cap1);
}
BLYNK_WRITE(V1) // Bơm
{
  if ((key) && (!trip1)) {
    RelayState1 = param.asInt();
    if (RelayState1 == LOW) {
      off_bom();
    } else {
      on_bom();
    }
  }
  Blynk.virtualWrite(V1, status_bom);
}
BLYNK_WRITE(V8) // Chon máy cài đặt bảo vệ
{
  switch (param.asInt()) {
  case 0: { // Gieng
    c = 0;
    Blynk.virtualWrite(V9, data.SetAmpemin);
    Blynk.virtualWrite(V10, data.SetAmpemax);
    break;
  }
  case 1: { // Bom
    c = 1;
    Blynk.virtualWrite(V9, data.SetAmpe1min);
    Blynk.virtualWrite(V10, data.SetAmpe1max);
    break;
  }
  case 2: { // Áp lực thấp
    c = 2;
    Blynk.virtualWrite(V9, data.pre_min);
    break;
  }
  }
}
BLYNK_WRITE(V9) // min
{
  if (keySet) {
    if (c == 0) {
      data.SetAmpemin = param.asFloat();
    } else if (c == 1) {
      data.SetAmpe1min = param.asFloat();
    } else if (c == 2) {
      data.pre_min = param.asFloat();
    }
  } else {
    Blynk.virtualWrite(V9, 0);
  }
}
BLYNK_WRITE(V10) // max
{
  if (keySet) {
    if (c == 0) {
      data.SetAmpemax = param.asFloat();
    } else if (c == 1) {
      data.SetAmpe1max = param.asFloat();
    } else if (c == 2) {
      Blynk.virtualWrite(V10, 0);
    }
  } else {
    Blynk.virtualWrite(V10, 0);
  }
}
BLYNK_WRITE(V11) // String
{
  String dataS = param.asStr();
  if (dataS == "M") {
    terminal.clear();
    key = true;
    keySet = true;
    Blynk.virtualWrite(V11, "Người vận hành: 'M.Quang'\nKích hoạt trong 10s\n");
    timer1.setTimeout(10000, []() {
      key = false;
      keySet = false;
      terminal.clear();
    });
  } else if (dataS == "active") {
    terminal.clear();
    key = true;
    keySet = true;
    visible();
    Blynk.virtualWrite(V11, "KHÔNG sử dụng phần mềm cho đến khi thông báo này mất.\n");
  } else if (dataS == "deactive") {
    terminal.clear();
    key = false;
    keySet = false;
    hidden();
    Blynk.virtualWrite(V11, "Ok!\nNhập mã đ��� điều khiển!\n");
  } else if (dataS == "save") {
    terminal.clear();
    savedata();
    Blynk.virtualWrite(V11, "Đã lưu cài đặt.\n");
  } else if (dataS == "reset") {
    terminal.clear();
    trip1 = false;
    trip0 = false;
    on_cap1();
    on_bom();
    Blynk.virtualWrite(V11, "Đã RESET! \nNhập mã để điều khiển!\n");
  } else if (dataS == "rst") {
    terminal.clear();
    Blynk.virtualWrite(V11, "ESP khởi động lại sau 3s");
    delay(3000);
    ESP.restart();
  } else if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V11, "ESP UPDATE...");
    update_fw();
  } else {
    Blynk.virtualWrite(V11, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
BLYNK_WRITE(V14) // Bảo vệ
{
  if (keySet) {
    if (param.asInt() == LOW) {
      data.protect = false;
      savedata();
    } else {
      data.protect = true;
      savedata();
    }
  } else
    Blynk.virtualWrite(V14, data.protect);
}
BLYNK_WRITE(V15) // Thông báo
{
  if (keySet) {
    if (param.asInt() == LOW) {
      data.noti = false;
      savedata();
    } else {
      data.noti = true;
      savedata();
    }
  } else
    Blynk.virtualWrite(V15, data.noti);
}
//-------------------------------------------------------------------
void setup() {
  pinMode(S0pin, OUTPUT);
  pinMode(S1pin, OUTPUT);
  pinMode(S2pin, OUTPUT);
  pinMode(S3pin, OUTPUT);
  pinMode(pinUSB, OUTPUT);
  digitalWrite(pinUSB, HIGH);
  delay(15000);
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(5000);
  //---------------------------------------------------------------------------------
  emon0.current(A0, 130);
  emon1.current(A0, 130);

  pcf8575.pinMode(pincap1, OUTPUT);
  pcf8575.pinMode(pinbom, OUTPUT);
  pcf8575.pinMode(P5, OUTPUT);
  pcf8575.pinMode(P4, OUTPUT);
  pcf8575.pinMode(P3, OUTPUT);
  pcf8575.begin();
  pcf8575.digitalWrite(pincap1, HIGH); // Bom 1
  pcf8575.digitalWrite(pinbom, HIGH);  // Bom 2
  pcf8575.digitalWrite(P5, HIGH);
  pcf8575.digitalWrite(P4, HIGH);
  pcf8575.digitalWrite(P3, HIGH);

  sensors.begin();
  rtc.begin();
  eeprom.initialize();
  eeprom.readBytes(address, sizeof(dataDefault), (byte *)&data);

  terminal.clear();
  //------------------------------------
  timer.setTimeout(5000, []() {
    timer_1 = timer1.setInterval(200L, []() {
      readPressure();
      temperature();
      timer.restartTimer(timer_1);
    });
    timer_2 = timer1.setInterval(1783L, []() {
      readPower();
      readPower1();
      updata();
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
    });
    timer1.setInterval(589011L, []() {
      syncstatus();
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
    });
    timer1.setInterval(60131L, []() {
      connectionstatus();
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
    });
    timer1.setInterval(95231L, []() {
      rtctime();
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
