/*V0 - Btn Bom 1
 *V1 - Btn Bom 2
 *V2 - I0 - Bom 1
 *V3 - I1 - Bom 2
 *V4 - Ap Luc
 *V5 - Con lai
 *V6 - The tich
 *V7 - Che do Cap 2
 *V8 - Chon may bao ve
 *V9 - min
 *V10 - max
 *V11 - String
 *V12 - Chon thoi gian chay bom
 *V13 - timeinput
 *V14 - bao ve
 *V15 - thong bao
 *V16 -
 *V17 - info
 *V18 - min pre
 *V19 - max pre
 *V20 - date/time
 *V21 - Báo lỗi
 *V22 - Do Sau
 *V23 - Dung tich
 *V24 - Reset
 *V25 - Kich bien tan
 *V26 - Status Ap 5
 --------------------------

 *V30 - Dien ap
 */

/*
#define BLYNK_TEMPLATE_ID "TMPLFpa9NbqF"
#define BLYNK_TEMPLATE_NAME "Tân Lập"
#define BLYNK_AUTH_TOKEN "0mWyl3eT8xTCqSlVecGw6DzzNPhTadoa"
*/
#define BLYNK_TEMPLATE_ID "TMPL6TClJ6X_r"
#define BLYNK_TEMPLATE_NAME "TRẠM TÂN LẬP"
#define BLYNK_AUTH_TOKEN "91thFYXxhfcs2ij5GDVg6NTjEgMqFBwi"

#define BLYNK_FIRMWARE_VERSION "2409011"
#define BLYNK_PRINT Serial
#define APP_DEBUG
const char *ssid = "Tram Nuoc";
const char *password = "0917173339";

#pragma region
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <UrlEncode.h>
//------------------
#include "PCF8575.h"
PCF8575 pcf8575_1(0x20);
const int pin_B1 = P1;
const int pin_B2 = P2;
const int pin_rst = P4;
const int pin_VFD = P5;

const int S0pin = P15;
const int S1pin = P14;
const int S2pin = P13;
const int S3pin = P12;
//------------------
#include "EmonLib.h"
EnergyMonitor emon0, emon1;
//------------------
#include "RTClib.h"
#include <WidgetRTC.h>
RTC_DS3231 rtc_module;
char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";
//------------------
#include <Eeprom24C32_64.h>
#define EEPROM_ADDRESS 0x57
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
const word address = 0;
//------------------
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
WiFiClient client;
HTTPClient http;
String server_name = "http://sgp1.blynk.cloud/external/api/";
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/TanLap1/.pio/build/nodemcuv2/firmware.bin"
//------------------
int dai = 640;
int rong = 590;
int dosau = 240;
int volume, volume1, percent, percent1, dungtich, smoothDistance;
long distance, distance1, t;
#define filterSamples 121
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

  for (j = 0; j < filterSamples; j++) { // transfer data array into anther array for sorting and averaging
    sorted[j] = sensSmoothArray[j];
  }

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

bool key = false, keyp = true, keynoti = true, keytank = true, keyPRE = true, keytime = false;
bool trip0 = false, trip1 = false;
bool key_memory = true, timer_I_status;
bool rtc1 = true, mintank = false;
bool blynk_first_connect = false;
bool pre_raw = false, tank_raw = false;

int c, b, j = 0, i = 0, e = 0, check_connect = 0;
int RelayState, RelayState1;
int timer_2, timer_1, timer_3, timer_4, timer_5, timer_6, timer_7;
int xSetAmpe = 0, xSetAmpe1 = 0;
int status_bom1 = HIGH, status_bom2 = HIGH;
float Irms0, Irms1, value, Result1, sensorValue_pre, sensorValue_tank;
unsigned long int xIrms0 = 0, xIrms1 = 0;
unsigned long int yIrms0 = 0, yIrms1 = 0;

struct Data {
  byte SetAmpemax, SetAmpemin;
  byte SetAmpe1max, SetAmpe1min;
  byte mode_cap2;
  int t1_start, t1_stop, t2_start, t2_stop, t3_start, t3_stop, t4_start, t4_stop;
  float min_pre, max_pre;
  int reboot_num, save_num;
} data, dataCheck;
const struct Data dataDefault = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

WidgetTerminal terminal(V11);
WidgetRTC rtc_widget;
BlynkTimer timer, timer1, timer2;
BLYNK_CONNECTED() {
  rtc_widget.begin();
  blynk_first_connect = true;
  Blynk.virtualWrite(V0, !status_bom1);
  Blynk.virtualWrite(V1, !status_bom2);
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
//------------------
void connectionstatus() {
  if ((WiFi.status() != WL_CONNECTED)) {
    // WiFi.disconnect();
    // WiFi.mode(WIFI_STA);
    // WiFi.begin(ssid, password);
    // Blynk.config(BLYNK_AUTH_TOKEN);
    Serial.println("Khong ket noi WIFI");
  }
  if ((WiFi.status() == WL_CONNECTED) && (!Blynk.connected())) {
    data.reboot_num = data.reboot_num + 1;
    savedata();
    // Serial.print("data.reboot_num: ");
    // Serial.println(data.reboot_num);
    if ((data.reboot_num == 1) || (data.reboot_num == 2)) {
      // Serial.println("Restart...");
      // Serial.print("data.reboot_num: ");
      // Serial.println(data.reboot_num);
      delay(1000);
      ESP.restart();
    }
    if (data.reboot_num % 5 == 0) {
      // Serial.print("data.reboot_num: ");
      // Serial.println(data.reboot_num);
      delay(1000);
      ESP.restart();
    }
  }
  if (Blynk.connected()) {
    if (data.reboot_num != 0) {
      data.reboot_num = 0;
      savedata();
    }
    // Serial.print("data.reboot_num: ");
    // Serial.println(data.reboot_num);
    // Serial.println("Blynk OK");
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
//------------------
void onbom1() {
  status_bom1 = LOW;
  pcf8575_1.digitalWrite(pin_B1, status_bom1);
  if (e == 0) {
    e = 1;
    timer2.setTimeout(30000L, []() {
      e = 0;
      if ((Irms0 == 0) && (Result1 < data.min_pre)) {
        trip0 = true;
        if (keynoti) {
          Blynk.logEvent("error", String("Bơm 1 không chạy kìa..Kiểm tra lẹ... "));
        }
      }
    });
  }
}
void offbom1() {
  status_bom1 = HIGH;
  pcf8575_1.digitalWrite(pin_B1, status_bom1);
}
void onbom2() {
  status_bom2 = LOW;
  pcf8575_1.digitalWrite(pin_B2, status_bom2);
}
void offbom2() {
  status_bom2 = HIGH;
  pcf8575_1.digitalWrite(pin_B2, status_bom2);
}
void hidden() {
  Blynk.setProperty(V15, "isHidden", true);
  Blynk.setProperty(V12, "isHidden", true);
  Blynk.setProperty(V13, "isHidden", true);
  Blynk.setProperty(V19, "isHidden", true);
  Blynk.setProperty(V18, "isHidden", true);
  Blynk.setProperty(V14, "isHidden", true);
  Blynk.setProperty(V8, "isHidden", true);
  Blynk.setProperty(V9, "isHidden", true);
  Blynk.setProperty(V10, "isHidden", true);
}
void visible() {
  Blynk.setProperty(V15, "isHidden", false);
  Blynk.setProperty(V12, "isHidden", false);
  Blynk.setProperty(V13, "isHidden", false);
  Blynk.setProperty(V19, "isHidden", false);
  Blynk.setProperty(V18, "isHidden", false);
  Blynk.setProperty(V14, "isHidden", false);
  Blynk.setProperty(V8, "isHidden", false);
  Blynk.setProperty(V9, "isHidden", false);
  Blynk.setProperty(V10, "isHidden", false);
}
//------------------
void updata() {
  String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN +
                       "&V4=" + float(Result1) +
                       "&V6=" + volume1 +
                       "&V5=" + smoothDistance +
                       "&V2=" + Irms0 +
                       "&V3=" + Irms1;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}
//------------------
void readPower() // C2 - Bơm 1  - I0
{
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms0 = emon0.calcIrms(1480);
  if (rms0 < 2) {
    Irms0 = 0;
    yIrms0 = 0;
  } else if (rms0 >= 2) {
    Irms0 = rms0;
    yIrms0 = yIrms0 + 1;
    if ((yIrms0 > 3) && ((Irms0 >= data.SetAmpemax) || (Irms0 <= data.SetAmpemin))) {
      xSetAmpe = xSetAmpe + 1;
      if ((xSetAmpe >= 2) && (keyp)) {
        offbom1();
        xSetAmpe = 0;
        trip0 = true;
        if (keynoti) {
          Blynk.logEvent("error", String("Bơm 1 lỗi: ") + Irms0 + String(" A"));
        }
      }
    } else {
      xSetAmpe = 0;
    }
  }
}
void readPower1() // C3 - Bom 2  - I1
{
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms1 = emon1.calcIrms(1480);
  if (rms1 < 2) {
    Irms1 = 0;
    yIrms1 = 0;
  } else if (rms1 >= 2) {
    Irms1 = rms1;
    yIrms1 = yIrms1 + 1;
    if ((yIrms1 > 3) && ((Irms1 >= data.SetAmpe1max) || (Irms1 <= data.SetAmpe1min))) {
      xSetAmpe1 = xSetAmpe1 + 1;
      if ((xSetAmpe1 >= 2) && (keyp)) {
        offbom2();
        xSetAmpe1 = 0;
        trip1 = true;
        if (keynoti) {
          Blynk.logEvent("error", String("Bơm 2 lỗi: ") + Irms1 + String(" A"));
        }
      }
    } else {
      xSetAmpe1 = 0;
    }
  }
}
void readPressure() // C0 - Ap Luc
{
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  sensorValue_pre = analogRead(A0);
  float Result;
  Result = ((sensorValue_pre - 186.2) * 10) / (931 - 186.2);
  if (Result > 0) {
    value += Result;
    Result1 = value / 4;
    value -= Result1;
  }
  if (Result1 >= 4) {
    Blynk.logEvent("error", String("Áp lực Tân Lập lỗi ") + Result1);
    // pcf8575_1.digitalWrite(pin_rst, LOW);
  }
}
void MeasureCmForSmoothing() { // C1 - Muc nuoc
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  sensorValue_tank = analogRead(A0);
  distance1 = (((sensorValue_tank - 196.5) * 500) / (980 - 196.5));
  if (distance1 > 0) {
    smoothDistance = digitalSmooth(distance1, sensSmoothArray1);
    volume1 = (dai * smoothDistance * rong) / 1000000;

    if (volume1 < 33) {
      mintank = true;
      keytime = false;
    }
    if ((volume1 > 50) && (mintank)) {
      mintank = false;
    }
    if ((smoothDistance - dosau >= 20) && (keynoti) && (keytank)) {
      Blynk.logEvent("error", String("Nước trong bể cao vượt mức ") + (smoothDistance - dosau) + String(" cm"));
      keytank = false;
      timer1.setTimeout(600000L, []() {
        keytank = true;
      });
    }
  }
}
void sensor_raw() {
  if (pre_raw) {
    Blynk.virtualWrite(V11, "PRE: ", sensorValue_pre, "\n");
  }
  if (tank_raw) {
    Blynk.virtualWrite(V11, "TANK: ", sensorValue_tank, "\n");
  }
}
//------------------
void rtctime() {
  DateTime now = rtc_module.now();
  if (blynk_first_connect == true) {
    if ((now.day() != day()) || (now.hour() != hour()) || ((now.minute() - minute() > 2) || (minute() - now.minute() > 2))) {
      rtc_module.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
      DateTime now = rtc_module.now();
    }
  }
  int nowtime = (now.hour() * 3600 + now.minute() * 60);

  if ((data.mode_cap2 == 1) && (mintank == false)) { // Chạy Tu Dong
    if ((keytime) && (rtc1)) {
      rtc1 = false;
      timer2.setTimeout(3600000, []() {
        rtc1 = true;
      });
      timer2.setTimeout(600000L, []() {
        if ((Result1 < (data.min_pre - 0.2)) && (keytime)) {
          if (keynoti) {
            Blynk.logEvent("error", String("Áp lực thấp: ") + Result1 + String(" bar"));
          }
        }
      });
    }
    if ((nowtime > data.t1_start && nowtime < data.t1_stop) || (nowtime > data.t2_start && nowtime < data.t2_stop) || (nowtime > data.t3_start && nowtime < data.t3_stop) || (nowtime > data.t4_start && nowtime < data.t4_stop)) {
      keytime = true;
      if ((Irms0 == 0) && (!trip0) && (Result1 < data.min_pre)) { // Chạy bơm 1
        onbom1();
      }
      if (Result1 < (data.min_pre - 0.2)) {
        j++;
        if ((j > 20) && (keyPRE == true)) // 5p
        {
          if ((Irms1 == 0) && (!trip1)) {
            onbom2();
            timer2.setTimeout(5000L, []() {
              if (Irms1 != 0) {
                timer2.setTimeout(1800000L, []() {
                  offbom2();
                });
              } else {
                trip1 = true;
                if (keynoti) {
                  Blynk.logEvent("error", String("Không thể chạy Bơm 2 tăng cường."));
                }
              }
            });
          }
          keyPRE = false;
          timer2.setTimeout(900000L, []() { // 15p
            j = 0;
            keyPRE = true;
          });
        }
      }
      if (Result1 >= (data.max_pre + 0.3)) {
        i++;
        if (i > 4) {
          i = 0;
          if (keynoti) {
            Blynk.logEvent("error", String("Áp lực cao: ") + Result1 + String(" bar"));
          }
        }
      }
      if ((Result1 >= (data.min_pre - 0.2)) && (Result1 <= (data.max_pre + 0.2))) {
        j = 0;
        i = 0;
        keyPRE = true;
      }
    } else {
      keytime = false;
    }
  } else if (data.mode_cap2 == 0) {
    keytime = false;
  }
}
void controlmotor() { // Điều khiển biến tần
  if (data.mode_cap2 == 1) {
    if (keytime) {
      if (Result1 < data.min_pre) {
        pcf8575_1.digitalWrite(pin_VFD, LOW); // kích
      }
      if (Result1 > data.max_pre) {
        pcf8575_1.digitalWrite(pin_VFD, HIGH); // nhả
      }
    } else {
      offbom2();
      if (status_bom1 == LOW) {
        status_bom1 = HIGH;
        pcf8575_1.digitalWrite(pin_VFD, HIGH);
        timer2.setTimeout(180000, []() { // 3p
          offbom1();
        });
      }
    }
  }
}
//------------------
BLYNK_WRITE(V0) // Bơm 1
{
  if ((key) && (!trip0)) {
    RelayState = param.asInt();
    if (RelayState == LOW) {
      offbom1();
    } else
      onbom1();
  } else {
    Blynk.virtualWrite(V0, !status_bom1);
  }
}
BLYNK_WRITE(V1) // Bơm 2
{
  if ((key) && (!trip1)) {
    RelayState1 = param.asInt();
    if (RelayState1 == LOW) {
      offbom2();
    } else
      onbom2();
  } else {
    Blynk.virtualWrite(V1, !status_bom2);
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
  case 0: { // Bom 1
    c = 0;
    Blynk.virtualWrite(V9, data.SetAmpemin);
    Blynk.virtualWrite(V10, data.SetAmpemax);
    break;
  }
  case 1: { // Bom 2
    c = 1;
    Blynk.virtualWrite(V9, data.SetAmpe1min);
    Blynk.virtualWrite(V10, data.SetAmpe1max);
    break;
  }
  }
}
BLYNK_WRITE(V9) // min
{
  if (key) {
    if (c == 0) {
      data.SetAmpemin = param.asInt();
    } else if (c == 1) {
      data.SetAmpe1min = param.asInt();
    }
  } else {
    Blynk.virtualWrite(V9, 0);
  }
}
BLYNK_WRITE(V10) // max
{
  if (key) {
    if (c == 0) {
      data.SetAmpemax = param.asInt();
    } else if (c == 1) {
      data.SetAmpe1max = param.asInt();
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
    Blynk.virtualWrite(V11, "Người vận hành: 'M.Quang'\nKích hoạt trong 10s\n");
    timer1.setTimeout(10000, []() {
      key = false;
      terminal.clear();
    });
  } else if (dataS == "active") {
    terminal.clear();
    visible();
    key = true;
    Blynk.virtualWrite(V11, "KHÔNG sử dụng phần mềm cho đến khi thông báo này mất.\n");
  } else if (dataS == "deactive") {
    terminal.clear();
    hidden();
    key = false;
    Blynk.virtualWrite(V11, "Ok!\nNhập mã để điều khiển!\n");
  } else if (dataS == "save") {
    terminal.clear();
    savedata();
    Blynk.virtualWrite(V11, "Đã lưu cài đặt.\n");
  } else if (dataS == "reset") {
    terminal.clear();
    trip1 = false;
    trip0 = false;
    Blynk.virtualWrite(V21, LOW);
    Blynk.virtualWrite(V11, "Đã RESET! \nNhập mã để điều khiển!\n");
  } else if (dataS == "save_num") {
    terminal.clear();
    Blynk.virtualWrite(V11, "Số lần ghi EEPROM: ", data.save_num);
  } else if (dataS == "rst") {
    terminal.clear();
    Blynk.virtualWrite(V11, "ESP khởi động lại sau 3s");
    delay(3000);
    ESP.restart();
  } else if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V11, "UPDATE FIRMWARE...");
    update_fw();
  } else if (dataS == "pre") {
    terminal.clear();
    pre_raw = !pre_raw;
  } else if (dataS == "tank") {
    terminal.clear();
    tank_raw = !tank_raw;
  } else if (dataS == "clr") {
    terminal.clear();
  } else {
    Blynk.virtualWrite(V11, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
BLYNK_WRITE(V12) // Chọn thời gian chạy Bơm
{
  switch (param.asInt()) {
  case 0: { // Lần 1
    if (key)
      b = 0;
    Blynk.virtualWrite(V13, data.t1_start, data.t1_stop, tz);
    break;
  }
  case 1: { // Lần 2
    if (key)
      b = 1;
    Blynk.virtualWrite(V13, data.t2_start, data.t2_stop, tz);
    break;
  }
  case 2: { // Lần 3
    if (key)
      b = 2;
    Blynk.virtualWrite(V13, data.t3_start, data.t3_stop, tz);
    break;
  }
  case 3: { // Lần 4
    if (key)
      b = 3;
    Blynk.virtualWrite(V13, data.t4_start, data.t4_stop, tz);
    break;
  }
  }
}
BLYNK_WRITE(V13) // Time input
{
  if (key) {
    TimeInputParam t(param);
    if (t.hasStartTime()) {
      if (b == 0) {
        data.t1_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 1) {
        data.t2_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 2) {
        data.t3_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 3) {
        data.t4_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
    }
    if (t.hasStopTime()) {
      if (b == 0) {
        data.t1_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 1) {
        data.t2_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 2) {
        data.t3_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 3) {
        data.t4_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
    }
  } else
    Blynk.virtualWrite(V13, 0);
}
BLYNK_WRITE(V14) // Bảo vệ
{
  if (key) {
    int data13 = param.asInt();
    if (data13 == LOW) {
      keyp = false;
    } else {
      keyp = true;
    }
  } else
    Blynk.virtualWrite(V14, keyp);
}
BLYNK_WRITE(V15) // Thông báo
{
  if (key) {
    int data16 = param.asInt();
    if (data16 == LOW) {
      keynoti = false;
    } else {
      keynoti = true;
    }
  } else
    Blynk.virtualWrite(V15, keynoti);
}
BLYNK_WRITE(V17) // Info
{
  if (param.asInt() == 1) {
    terminal.clear();
    if (data.mode_cap2 == 0) {
      Blynk.virtualWrite(V11, "Mode: MAN");
    } else if (data.mode_cap2 == 1) {
      int t1_start_h = data.t1_start / 3600;
      int t1_start_m = (data.t1_start - (t1_start_h * 3600)) / 60;
      int t1_stop_h = data.t1_stop / 3600;
      int t1_stop_m = (data.t1_stop - (t1_stop_h * 3600)) / 60;
      int t2_start_h = data.t2_start / 3600;
      int t2_start_m = (data.t2_start - (t2_start_h * 3600)) / 60;
      int t2_stop_h = data.t2_stop / 3600;
      int t2_stop_m = (data.t2_stop - (t2_stop_h * 3600)) / 60;
      int t3_start_h = data.t3_start / 3600;
      int t3_start_m = (data.t3_start - (t3_start_h * 3600)) / 60;
      int t3_stop_h = data.t3_stop / 3600;
      int t3_stop_m = (data.t3_stop - (t3_stop_h * 3600)) / 60;
      int t4_start_h = data.t4_start / 3600;
      int t4_start_m = (data.t4_start - (t4_start_h * 3600)) / 60;
      int t4_stop_h = data.t4_stop / 3600;
      int t4_stop_m = (data.t4_stop - (t4_stop_h * 3600)) / 60;

      Blynk.virtualWrite(V11, "Mode: AUTO\nLần 1: ", t1_start_h, ":", t1_start_m, " -> ", t1_stop_h, ":", t1_stop_m, "\n- Lần 2: ", t2_start_h, ":", t2_start_m, " -> ", t2_stop_h, ":", t2_stop_m, "\n- Lần 3: ", t3_start_h, ":", t3_start_m, " -> ", t3_stop_h, ":", t3_stop_m, "\n- Lần 4: ", t4_start_h, ":", t4_start_m, " -> ", t4_stop_h, ":", t4_stop_m);
      Blynk.virtualWrite(V11, "\nMức áp thấp: ", data.min_pre, "\nMức áp cao: ", data.max_pre);
    }
  } else {
    terminal.clear();
  }
  timer.restartTimer(timer_1);
  timer.restartTimer(timer_2);
}
BLYNK_WRITE(V18) // Min pressure
{
  if (key) {
    data.min_pre = param.asFloat();
  } else
    Blynk.virtualWrite(V18, data.min_pre);
}
BLYNK_WRITE(V19) // Max pressure
{
  if (key) {
    data.max_pre = param.asFloat();
  } else
    Blynk.virtualWrite(V19, data.max_pre);
}
BLYNK_WRITE(V24) {
  if (key) {
    if (param.asInt() == LOW) {
      pcf8575_1.digitalWrite(pin_rst, LOW);
    }
  }
}
BLYNK_WRITE(V25) // Kich BT
{
  if (key) {
    if (param.asInt() == LOW) {
      pcf8575_1.digitalWrite(pin_VFD, HIGH);
    } else
      (pcf8575_1.digitalWrite(pin_VFD, LOW));
  }
}
//-------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(7000);
  //---------------------------------------------------------------------------------
  emon0.current(A0, 105);
  emon1.current(A0, 105);

  rtc_module.begin();
  eeprom.initialize();
  eeprom.readBytes(address, sizeof(dataDefault), (byte *)&data);

  pcf8575_1.begin();
  pcf8575_1.pinMode(S0pin, OUTPUT);
  pcf8575_1.pinMode(S1pin, OUTPUT);
  pcf8575_1.pinMode(S2pin, OUTPUT);
  pcf8575_1.pinMode(S3pin, OUTPUT);

  pcf8575_1.pinMode(pin_B1, OUTPUT);
  pcf8575_1.digitalWrite(pin_B1, HIGH);
  pcf8575_1.pinMode(pin_B2, OUTPUT);
  pcf8575_1.digitalWrite(pin_B2, HIGH);
  pcf8575_1.pinMode(pin_rst, OUTPUT);
  pcf8575_1.digitalWrite(pin_rst, HIGH);
  pcf8575_1.pinMode(pin_VFD, OUTPUT);
  pcf8575_1.digitalWrite(pin_VFD, HIGH);

  timer.setTimeout(2000L, []() {
    timer_1 = timer1.setInterval(301L, []() {
      readPressure();
      MeasureCmForSmoothing();
      controlmotor();
    });
    timer_2 = timer1.setInterval(1283L, []() {
      readPower();
      readPower1();
      sensor_raw();
      updata();
      timer.restartTimer(timer_1);
    });
    timer_3 = timer1.setInterval(15006L, []() {
      rtctime();
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
    });
    timer.setInterval(900005L, []() {
      connectionstatus();
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
    });
    terminal.clear();
  });
}
void loop() {
  Blynk.run();
  timer.run();
  timer1.run();
  timer2.run();
}
