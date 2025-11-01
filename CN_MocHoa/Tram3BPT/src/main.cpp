/**DC7633
 *V0 - Button C2 - 1
 *V1 - Button C2 - 2
 *V2 - Button C1 - 1
 *V3 - Button C1 - 2
 *V4 - Chọn người vận hành
 *V5 - MENU motor
 *V6 - min
 *V7 - max
 *V8 - info
 *V9 - Ngày/Giờ
 *V10 - terminal key
 *V11 - Thời gian chạy Bơm
 *V12 - Hide/visible
 *V13 - Bảo vệ
 *V14 - Ap luc
 *V15 - Rửa lọc
 *V16 -
 *V17 - Thông báo
 *V18 - time input
 *V19 -

 *V20 - I0 - Cấp 1 - 1
 *V21 - I1 - Cấp 1 - 2
 *V22 - I2 - Bơm 2
 *V23 - I3 - Bơm 2
 *V24 - Dung tich
 *V25 - Thể tích
 *V26 - Còn lại
 *V27 - Độ sâu
 *V28 - datas_volume
 *V29 - LL1m3
 *V30 - RAW AG1
 *V31 - LL24h
 *V32 - LLG1_RL
 *V33 -
 *V34 -
 *V35 -
 *V36 -
 *V37 -
 *V40 - thời gian chạy G1
 *V41 - thời gian chạy G1-24h
 *V42 - thời gian chạy G2
 *V43 - thời gian chạy G2-24h
 *V44 - thời gian chạy B1
 *V45 - thời gian chạy B1-24h
 *V46 - thời gian chạy B2
 *V47 - thời gian chạy B2-24h
 *
 */

/*
#define BLYNK_TEMPLATE_ID "TMPLbPLEi8uh"
#define BLYNK_TEMPLATE_NAME "Trạm Mộc Hóa"
#define BLYNK_AUTH_TOKEN "tKNZ99XnCSeCsoDEva3kx-O0YWw83nMn"
*/
#define BLYNK_TEMPLATE_ID "TMPL6coHtFMJ-"
#define BLYNK_TEMPLATE_NAME "TRẠM 3 BPT"
#define BLYNK_AUTH_TOKEN "Xd_XI0fm9nIsXBvvMZ6pjEtRd0irLLR2"
#define VOLUME_TOKEN "Q2KAjaqI3sWhET-Ax94VPYfIk2Fmsr36"

#define BLYNK_FIRMWARE_VERSION "250331"
#define BLYNK_PRINT Serial
#define APP_DEBUG
const char *ssid = "KwacoBlynk";
const char *password = "Password";
// const char* ssid = "Wifi";
// const char* password = "Password";

#pragma region
#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <UrlEncode.h>
//--------------
#include "PCF8575.h"
PCF8575 pcf8575(0x20);
const int btn_left = P7;
const int btn_right = P6;
const int btn_mid = P5;
const int btn_g1 = P4;
const int btn_g2 = P3;
const int pin_usb = P0;
const int pin4067 = P15;
//--------------
#include "EmonLib.h"
EnergyMonitor emon0, emon1, emon2, emon3;
//--------------
#include "RTClib.h"
#include <TimeLib.h>
#include <WidgetRTC.h>
RTC_DS3231 rtc_module;
//--------------
#include <OneWire.h>
#include <Wire.h>
OneWire oneWire(D3);
//--------------
#include <Eeprom24C32_64.h>
#define EEPROM_ADDRESS 0x57
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
//--------------
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
WiFiClient client;
HTTPClient http;
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/refs/heads/main/CN_MocHoa/Tram3BPT/.pio/build/nodemcuv2/firmware.bin"
String server_name = "http://sgp1.blynk.cloud/external/api/";
//--------------
#define filterSamples 121
const int S0pin = 14;
const int S1pin = 12;
const int S2pin = 13;
const int S3pin = 15;
const word address = 0;

const int dai = 750;
const int rong = 750;
const int dosau = 330;

const bool b1 = HIGH;
const bool b2 = LOW;
// long t;
long m = 60000;

char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";
bool key = false, keyp = true, keynoti = true;
bool trip0 = false, trip1 = false, trip2 = false, trip3 = false;
bool temp_0 = true, temp_1 = true;
bool noti_1 = true, noti_2 = true;
bool blynk_first_connect = false;
int timer_I, timer_run_main;
int i = 0; // Biến đếm áp lực
int j = 0; // Biến đếm cấp 1 không chạy
int c, b, check_connect = 0, reboot_num;
int status_g1 = HIGH, status_g2 = HIGH;
int timer_1, timer_2;
int xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0, xSetAmpe3 = 0;
int G1_start, G2_start, B1_start, B2_start;
int LLG1_1m3;
bool G1_save = false, G2_save = false, B1_save = false, B2_save = false;
float Irms0, Irms1, Irms2, Irms3, value, Result1;
unsigned long int xIrms0 = 0, xIrms1 = 0, xIrms2 = 0, xIrms3 = 0;
unsigned long int yIrms0 = 0, yIrms1 = 0, yIrms2 = 0, yIrms3 = 0;
unsigned long timerun;

int volume, volume1, smoothDistance;
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
long distance, distance1;

struct Data {
  byte SetAmpemax, SetAmpemin;
  byte SetAmpe1max, SetAmpe1min;
  byte SetAmpe2max, SetAmpe2min;
  byte SetAmpe3max, SetAmpe3min;
  byte mode_cap2;
  int start_time, stop_time;
  int save_num;
  byte check_changeday;
  byte status_btn_mid;
  byte status_btn_left;
  byte status_direct;
  byte reset_day;
  int timerun_G1, timerun_G2, timerun_B1, timerun_B2;
  int LLG1_RL;
  byte rualoc;
} data, dataCheck;
const struct Data dataDefault = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#pragma endregion

WidgetTerminal terminal(V10);
WidgetTerminal volume_terminal(V28);
WidgetRTC rtc;
BlynkTimer timer, timeout;
BLYNK_CONNECTED() {
  rtc.begin();
  blynk_first_connect = true;
  Blynk.virtualWrite(V4, data.mode_cap2);
}
//-------------------------------------------------------------------
void savedata() {
  if (memcmp(&data, &dataCheck, sizeof(dataDefault)) == 0) {
    // Serial.println("structures same no need to write to EEPROM");
  } else {
    // Serial.println("\nWrite bytes to EEPROM memory...");
    data.save_num = data.save_num + 1;
    eeprom.writeBytes(address, sizeof(dataDefault), (byte *)&data);
    Blynk.setProperty(V10, "label", "EEPROM ", data.save_num);
  }
}
//-----------------------------
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
//-----------------------------
void up() {
  String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN +
                       "&V14=" + Result1 +
                       "&V21=" + Irms1 +
                       "&V22=" + Irms2 +
                       "&V23=" + Irms3 +
                       "&V25=" + volume1 +
                       "&V26=" + smoothDistance +
                       "&V40=" + float(data.timerun_G1) / 1000 / 60 / 60 +
                       "&V42=" + float(data.timerun_G2) / 1000 / 60 / 60 +
                       "&V44=" + float(data.timerun_B1) / 1000 / 60 / 60 +
                       "&V46=" + float(data.timerun_B2) / 1000 / 60 / 60;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}
void up_timerun_motor() {
  String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN +
                       "&V41=" + float(data.timerun_G1) / 1000 / 60 / 60 +
                       "&V43=" + float(data.timerun_G2) / 1000 / 60 / 60 +
                       "&V45=" + float(data.timerun_B1) / 1000 / 60 / 60 +
                       "&V47=" + float(data.timerun_B2) / 1000 / 60 / 60;
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
        data.timerun_G2 = 0;
        data.timerun_B1 = 0;
        data.timerun_B2 = 0;
        data.reset_day = day();
        savedata();
      }
    }
  }
  if (G1_save || G2_save || B1_save || B2_save) {
    if (G1_start != 0) {
      data.timerun_G1 = data.timerun_G1 + (millis() - G1_start);
      G1_start = millis();
      G1_save = false;
    }
    if (G2_start != 0) {
      data.timerun_G2 = data.timerun_G2 + (millis() - G2_start);
      G2_start = millis();
      G2_save = false;
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
void reset_4067() {
  pcf8575.digitalWrite(pin4067, HIGH);
  delay(3000);
  pcf8575.digitalWrite(pin4067, LOW);
}
//-----------------------------
void on_G1() { // Dùng thường đóng
  status_g1 = HIGH;
  pcf8575.digitalWrite(btn_g1, status_g1);
}
void off_G1() {
  status_g1 = LOW;
  pcf8575.digitalWrite(btn_g1, status_g1);
}
void on_G2() { // Dùng thường đóng
  status_g2 = HIGH;
  pcf8575.digitalWrite(btn_g2, status_g2);
}
void off_G2() {
  status_g2 = LOW;
  pcf8575.digitalWrite(btn_g2, status_g2);
}
void run_main() {
  long time_start;
  if (timerun >= 60000) {
    time_start = 1000;
  } else
    time_start = 40000;
  data.status_btn_left = HIGH;
  pcf8575.digitalWrite(btn_left, data.status_btn_left);
  pcf8575.digitalWrite(btn_mid, data.status_btn_mid);
  timer_run_main = timeout.setTimeout(time_start, []() {
    pcf8575.digitalWrite(btn_left, !data.status_btn_left);
  });
  savedata();
}
void off_main() {
  timeout.deleteTimer(timer_run_main);
  data.status_btn_left = LOW;
  pcf8575.digitalWrite(btn_left, !data.status_btn_left);
  savedata();
}
void on_direct() { // Dùng thường hở
  data.status_direct = HIGH;
  pcf8575.digitalWrite(btn_right, !data.status_direct);
  savedata();
}
void off_direct() {
  data.status_direct = LOW;
  pcf8575.digitalWrite(btn_right, !data.status_direct);
  savedata();
}
//-----------------------------
void hidden() {
  Blynk.setProperty(V11, "isHidden", true);
  Blynk.setProperty(V18, "isHidden", true);
  Blynk.setProperty(V7, "isHidden", true);
  Blynk.setProperty(V6, "isHidden", true);
  Blynk.setProperty(V5, "isHidden", true);
  Blynk.setProperty(V17, "isHidden", true);
  Blynk.setProperty(V13, "isHidden", true);
}
void visible() {
  Blynk.setProperty(V11, "isHidden", false);
  Blynk.setProperty(V18, "isHidden", false);
  Blynk.setProperty(V7, "isHidden", false);
  Blynk.setProperty(V6, "isHidden", false);
  Blynk.setProperty(V5, "isHidden", false);
  Blynk.setProperty(V17, "isHidden", false);
  Blynk.setProperty(V13, "isHidden", false);
}
void syncstatus() {
  Blynk.virtualWrite(V4, data.mode_cap2);
  Blynk.virtualWrite(V2, status_g1);
  Blynk.virtualWrite(V3, status_g2);
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
        Blynk.virtualWrite(V10, "I2C device found at address 0x0", stringOne, " !\n");
      Blynk.virtualWrite(V10, "I2C device found at address 0x", stringOne, " !\n");
      nDevices++;
    } else if (error == 4) {
      stringOne = String(address, HEX);

      if (address < 16)
        Blynk.virtualWrite(V10, "Unknown error at address 0x0", stringOne, " !\n");
      Blynk.virtualWrite(V10, "I2C device found at address 0x", stringOne, " !\n");
    }
  }
  if (nDevices == 0)
    Blynk.virtualWrite(V10, "No I2C devices found\n");
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
BLYNK_WRITE(V20) { // Cấp 1 - 1 - I0
  float rms0 = param.asFloat();
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
        if ((xSetAmpe >= 4) && (keyp)) {
          off_G1();
          xSetAmpe = 0;
          trip0 = true;
          if (keynoti) {
            Blynk.logEvent("error", String("Giếng 1 lỗi: ") + Irms0 + String(" A"));
          }
        }
      } else
        xSetAmpe = 0;
    }
  }
}
void readPower1() // C3 - Cấp 1 - 2 - I1
{
  digitalWrite(S0pin, HIGH);
  digitalWrite(S1pin, HIGH);
  digitalWrite(S2pin, LOW);
  digitalWrite(S3pin, LOW);
  float rms1 = emon1.calcIrms(740);
  if (rms1 < 3) {
    Irms1 = 0;
    yIrms1 = 0;
    if (G2_start != 0) {
      data.timerun_G2 = data.timerun_G2 + (millis() - G2_start);
      savedata();
      G2_start = 0;
    }
  } else if (rms1 >= 3) {
    Irms1 = rms1;
    yIrms1 = yIrms1 + 1;
    if (yIrms1 > 3) {
      if (G2_start >= 0) {
        if (G2_start == 0)
          G2_start = millis();
        else if (millis() - G2_start > 60000) {
          G2_save = true;
        } else
          G2_save = false;
      }
      if ((Irms1 >= data.SetAmpe1max) || (Irms1 <= data.SetAmpe1min)) {
        xSetAmpe1 = xSetAmpe1 + 1;
        if ((xSetAmpe1 >= 3) && (keyp)) {
          off_G2();
          trip1 = true;
          xSetAmpe1 = 0;
          if (keynoti) {
            Blynk.logEvent("error", String("Giếng 2 lỗi: ") + Irms1 + String(" A"));
          }
        }
      } else
        xSetAmpe1 = 0;
    }
  }
}
void readPower2() // C4 - Bơm 1  - I2
{
  digitalWrite(S0pin, LOW);
  digitalWrite(S1pin, LOW);
  digitalWrite(S2pin, HIGH);
  digitalWrite(S3pin, LOW);
  float rms2 = emon2.calcIrms(740);
  if (rms2 < 3) {
    Irms2 = 0;
    yIrms2 = 0;
    if (B1_start != 0) {
      data.timerun_B1 = data.timerun_B1 + (millis() - B1_start);
      savedata();
      B1_start = 0;
    }
  } else if (rms2 >= 3) {
    Irms2 = rms2;
    yIrms2 = yIrms2 + 1;
    if (yIrms2 > 15) {
      if (B1_start >= 0) {
        if (B1_start == 0)
          B1_start = millis();
        else if (millis() - B1_start > 60000) {
          B1_save = true;
        } else
          B1_save = false;
      }
      if ((Irms2 >= data.SetAmpe2max) || ((Irms2 <= data.SetAmpe2min) && (Result1 < 2.4))) {
        xSetAmpe2 = xSetAmpe2 + 1;
        if ((xSetAmpe2 >= 2) && (keyp)) {
          off_main();
          xSetAmpe2 = 0;
          trip2 = true;
          if (keynoti) {
            Blynk.logEvent("error", String("Bơm 1 lỗi: ") + Irms2 + String(" A"));
          }
        }
      } else
        xSetAmpe2 = 0;
    }
  }
}
void readPower3() // C5 - Bơm 2  - I3
{
  digitalWrite(S0pin, HIGH);
  digitalWrite(S1pin, LOW);
  digitalWrite(S2pin, HIGH);
  digitalWrite(S3pin, LOW);
  float rms3 = emon3.calcIrms(740);
  if (rms3 < 3) {
    Irms3 = 0;
    yIrms3 = 0;
    if (B2_start != 0) {
      data.timerun_B2 = data.timerun_B2 + (millis() - B2_start);
      savedata();
      B2_start = 0;
    }
  } else if (rms3 >= 3) {
    Irms3 = rms3;
    yIrms3 = yIrms3 + 1;
    if (yIrms3 > 3) {
      if (B2_start >= 0) {
        if (B2_start == 0)
          B2_start = millis();
        else if (millis() - B2_start > 60000) {
          B2_save = true;
        } else
          B2_save = false;
      }
      if ((Irms3 >= data.SetAmpe3max) || ((Irms3 <= data.SetAmpe3min) && (Result1 < 2.4))) {
        xSetAmpe3 = xSetAmpe3 + 1;
        if ((xSetAmpe3 >= 15) && (keyp)) {
          off_main();
          xSetAmpe3 = 0;
          trip3 = true;
          if (keynoti) {
            Blynk.logEvent("error", String("Bơm 2 lỗi: ") + Irms3 + String(" A"));
          }
        }
      } else
        xSetAmpe3 = 0;
    }
  }
}
//-------------------------------------------------------------------
void readPressure() // C1 - Ap Luc
{
  digitalWrite(S0pin, HIGH);
  digitalWrite(S1pin, LOW);
  digitalWrite(S2pin, LOW);
  digitalWrite(S3pin, LOW);
  float sensorValue = analogRead(A0);
  float Result;
  Result = ((sensorValue - 193) * 10) / (925 - 193);
  if (Result > 0) {
    value += Result;
    Result1 = value / 16.0;
    value -= Result1;
  }
}
void MeasureCmForSmoothing() {
  digitalWrite(S0pin, LOW);
  digitalWrite(S1pin, LOW);
  digitalWrite(S2pin, LOW);
  digitalWrite(S3pin, LOW);
  float sensorValue = analogRead(A0);
  // Serial.print("Nuoc: ");
  // Serial.println(sensorValue);
  distance1 = (((sensorValue - 190) * 800) / (890 - 190));
  // Serial.print("Do sau: ");
  // Serial.println(distance1);
  if (distance1 > 0) {
    smoothDistance = digitalSmooth(distance1, sensSmoothArray1);
    volume1 = (dai * smoothDistance * rong) / 1000000;
  }
  // Serial.println(sensorValue);
}
//-------------------------------------------------------------------
void rtctime() {
  timerun = millis();
  DateTime now = rtc_module.now();
  if (blynk_first_connect == true) {
    if ((now.day() != day()) || (now.hour() != hour()) || ((now.minute() - minute() > 2) || (minute() - now.minute() > 2))) {
      rtc_module.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
    }
  }
  Blynk.virtualWrite(V9, daysOfTheWeek[now.dayOfTheWeek()], ", ", now.day(), "/", now.month(), "/", now.year(), " - ", now.hour(), ":", now.minute(), ":", now.second());

  int nowtime = (now.hour() * 3600 + now.minute() * 60);

  if (data.mode_cap2 == 2) {                // Auto
    if (data.start_time < data.stop_time) { // Nếu thời gian nghỉ lớn hơn thời gian chạy
      if ((nowtime > data.stop_time) || (nowtime < data.start_time)) {
        if (data.check_changeday == 0) {
          data.check_changeday = 1;
          data.status_btn_mid = !data.status_btn_mid;
          timerun = 0;
          if (data.status_direct == HIGH)
            off_direct();
          run_main();
        } else {
          if (data.status_direct == HIGH)
            off_direct();
          if (data.status_btn_mid == b1) {
            if ((Irms2 == 0) && (!trip2))
              run_main();
          } else if (data.status_btn_mid == b2) {
            if ((Irms3 == 0) && (!trip3))
              run_main();
          }
        }
      }
      if ((nowtime > data.start_time) && (nowtime < data.stop_time)) {
        if (data.check_changeday == 1) {
          data.check_changeday = 0;
          savedata();
        }
        if (data.status_btn_mid == b1) {
          if ((Irms2 == 0) && (!trip2))
            run_main();
          if (data.status_direct == LOW && !trip3)
            on_direct();
        } else if (data.status_btn_mid == b2) {
          if ((Irms3 == 0) && (!trip3))
            run_main();
          if (data.status_direct == LOW && !trip2)
            on_direct();
        }
      }
    }
    if (data.start_time > data.stop_time) { // Nếu thời gian nghỉ nhỏ hơn thời gian chạy
      if ((nowtime > data.stop_time) && (nowtime < data.start_time)) {
        if (data.check_changeday == 0) {
          data.check_changeday = 1;
          data.status_btn_mid = !data.status_btn_mid;
          if (data.status_direct == HIGH)
            off_direct();
          run_main();
        } else {
          if (data.status_direct == HIGH)
            off_direct();
          if (data.status_btn_mid == b1) {
            if ((Irms2 == 0) && (!trip2))
              run_main();
          } else if (data.status_btn_mid == b2) {
            if ((Irms3 == 0) && (!trip3))
              run_main();
          }
        }
      }
      if ((nowtime > data.start_time) || (nowtime < data.stop_time)) {
        if (data.check_changeday == 1) {
          data.check_changeday = 0;
          savedata();
        }
        if (data.status_btn_mid == b1) {
          if ((Irms2 == 0) && (!trip2))
            run_main();
          if (data.status_direct == LOW && !trip3)
            on_direct();
        } else if (data.status_btn_mid == b2) {
          if ((Irms3 == 0) && (!trip3))
            run_main();
          if (data.status_direct == LOW && !trip2)
            on_direct();
        }
      }
    }
  }
  // Cảnh báo áp lực thấp

  // Cảnh báo cấp 1 không chạy
}
//-------------------------------------------------------------------
BLYNK_WRITE(V0) // Bơm 1
{
  if ((key) && (data.mode_cap2 == 1)) {
    if (param.asInt() == HIGH) {
      if ((Irms2 == 0) && (!trip2)) {
        if (Irms3 == 0) {
          data.status_btn_mid = b1;
          run_main();
        } else {
          if (data.status_direct == LOW)
            on_direct();
        }
      }
    } else {
      if (data.status_btn_mid == b1) {
        off_main();
      } else
        off_direct();
    }
  } else {
    if ((data.status_btn_mid == b1) && (data.status_btn_left == HIGH))
      Blynk.virtualWrite(V0, HIGH);
    else if ((data.status_btn_mid == b2) && (data.status_btn_left == HIGH) && (data.status_direct = HIGH))
      Blynk.virtualWrite(V0, HIGH);
    else
      Blynk.virtualWrite(V0, LOW);
  }
}
BLYNK_WRITE(V1) // Bơm 2
{
  if ((key) && (data.mode_cap2 == 1)) {
    if (param.asInt() == HIGH) {
      if ((Irms3 == 0) && (!trip3)) {
        if (Irms2 == 0) {
          data.status_btn_mid = b2;
          run_main();
        } else {
          if (data.status_direct == LOW)
            on_direct();
        }
      }
    } else {
      if (data.status_btn_mid == b2) {
        off_main();
      } else
        off_direct();
    }
  } else {
    if ((data.status_btn_mid == b2) && (data.status_btn_left == HIGH))
      Blynk.virtualWrite(V1, HIGH);
    else if ((data.status_btn_mid == b1) && (data.status_btn_left == HIGH) && (data.status_direct == HIGH))
      Blynk.virtualWrite(V1, HIGH);
    else
      Blynk.virtualWrite(V1, LOW);
  }
}
BLYNK_WRITE(V2) // Giếng 1
{
  if (key) {
    if ((param.asInt() == HIGH) && (!trip0)) {
      on_G1();
    } else
      off_G1();
  } else {
    Blynk.virtualWrite(V2, status_g1);
  }
}
BLYNK_WRITE(V3) // Giếng 2
{
  if (key) {
    if ((param.asInt() == HIGH) && (!trip1)) {
      on_G2();
    } else
      off_G2();
  } else {
    Blynk.virtualWrite(V3, status_g2);
  }
}
BLYNK_WRITE(V4) // Chọn chế độ Cấp 2
{
  if (key) {
    switch (param.asInt()) {
    case 0: { // Tắt máy
      data.mode_cap2 = 0;
      off_main();
      off_direct();
      break;
    }
    case 1: { // Man
      data.mode_cap2 = 1;
      Blynk.setProperty(V0, "isHidden", false);
      Blynk.setProperty(V1, "isHidden", false);
      break;
    }
    case 2: { // Auto
      data.mode_cap2 = 2;
      Blynk.setProperty(V0, "isHidden", true);
      Blynk.setProperty(V1, "isHidden", true);
      break;
    }
    }
  } else
    Blynk.virtualWrite(V4, data.mode_cap2);
}
BLYNK_WRITE(V5) // Chon máy cài đặt bảo vệ
{
  switch (param.asInt()) {
  case 0: { // ...
    c = 0;
    Blynk.virtualWrite(V6, 0);
    Blynk.virtualWrite(V7, 0);
    break;
  }
  case 1: { // Cấp 1 - 1
    c = 1;
    Blynk.virtualWrite(V6, data.SetAmpemin);
    Blynk.virtualWrite(V7, data.SetAmpemax);
    break;
  }
  case 2: { // Cấp 1 - 2
    c = 2;
    Blynk.virtualWrite(V6, data.SetAmpe1min);
    Blynk.virtualWrite(V7, data.SetAmpe1max);
    break;
  }
  case 3: { // Cap 2 - 1
    c = 3;
    Blynk.virtualWrite(V6, data.SetAmpe2min);
    Blynk.virtualWrite(V7, data.SetAmpe2max);
    break;
  }
  case 4: { // Cấp 2 - 2
    c = 4;
    Blynk.virtualWrite(V6, data.SetAmpe3min);
    Blynk.virtualWrite(V7, data.SetAmpe3max);
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
    } else if (c == 3) {
      data.SetAmpe2min = param.asInt();
    } else if (c == 4) {
      data.SetAmpe3min = param.asInt();
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
    } else if (c == 3) {
      data.SetAmpe2max = param.asInt();
    } else if (c == 4) {
      data.SetAmpe3max = param.asInt();
    }
  } else {
    Blynk.virtualWrite(V7, 0);
  }
}
BLYNK_WRITE(V8) // info
{
  if (param.asInt() == 1) {
    terminal.clear();
    if (data.mode_cap2 == 1) {
      Blynk.virtualWrite(V10, "MODE: MAN");
    } else if (data.mode_cap2 == 2) {
      int hour_start = data.start_time / 3600;
      int minute_start = (data.start_time - (hour_start * 3600)) / 60;
      int hour_stop = data.stop_time / 3600;
      int minute_stop = (data.stop_time - (hour_stop * 3600)) / 60;
      Blynk.virtualWrite(V10, "MODE: AUTO\nThời gian chạy 2 bơm: ", hour_start, ":", minute_start, " -> ", hour_stop, " : ", minute_stop);
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
  if (dataS == "mh" || dataS == "MH") {
    terminal.clear();
    key = true;
    Blynk.virtualWrite(V10, "Đơn vị vận hành: CN-Mộc Hóa\nKích hoạt trong 15s\n");
    timeout.setTimeout(15000, []() {
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
    on_G1();
    on_G2();
    Blynk.virtualWrite(V10, "Đã RESET! \nNhập mã để điều khiển!\n");
  } else if (dataS == "save_num") {
    terminal.clear();
    Blynk.virtualWrite(V10, "Số lần ghi EEPROM: ", data.save_num);
  } else if (dataS == "rst") {
    terminal.clear();
    Blynk.virtualWrite(V10, "ESP khởi động lại sau 3s");
    reset_4067();
    ESP.restart();
  } else if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V10, "UPDATE FIRMWARE...");
    update_fw();
  } else if (dataS == "i2c") {
    i2c_scaner();
  } else {
    Blynk.virtualWrite(V10, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
BLYNK_WRITE(V11) // Chọn thời gian chạy 2 Bơm
{
  switch (param.asInt()) {
  case 0: { // ...
    b = 0;
    Blynk.virtualWrite(V18, 0);
    break;
  }
  case 1: {
    b = 1;
    Blynk.virtualWrite(V18, data.start_time, data.stop_time, tz);
    break;
  }
  }
}
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
BLYNK_WRITE(V15) // Rửa lọc
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
    Blynk.virtualWrite(V15, data.rualoc);
  }
}
BLYNK_WRITE(V17) // Thông báo
{
  if (key) {
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
  if (key) {
    TimeInputParam t(param);
    if (t.hasStartTime()) {
      if (b == 1) {
        data.start_time = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
    }
    if (t.hasStopTime()) {
      if (b == 1) {
        data.stop_time = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
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
BLYNK_WRITE(V29) // Lưu lượng G1_1m3
{
  LLG1_1m3 = param.asInt();
}
//-------------------------------------------------------------------
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

  emon1.current(A0, 112);
  emon2.current(A0, 112);
  emon3.current(A0, 112);

  eeprom.initialize();
  eeprom.readBytes(address, sizeof(dataDefault), (byte *)&data);
  rtc_module.begin();

  pcf8575.begin();
  pcf8575.pinMode(btn_left, OUTPUT);
  pcf8575.digitalWrite(btn_left, !data.status_btn_left);
  pcf8575.pinMode(btn_mid, OUTPUT);
  pcf8575.digitalWrite(btn_mid, data.status_btn_mid);
  pcf8575.pinMode(btn_right, OUTPUT);
  pcf8575.digitalWrite(btn_right, !data.status_direct);
  pcf8575.pinMode(btn_g1, OUTPUT);
  pcf8575.digitalWrite(btn_g1, HIGH);
  pcf8575.pinMode(btn_g2, OUTPUT);
  pcf8575.digitalWrite(btn_g2, HIGH);
  pcf8575.pinMode(pin4067, OUTPUT);
  pcf8575.digitalWrite(pin4067, LOW);
  pcf8575.pinMode(pin_usb, OUTPUT);
  pcf8575.pinMode(P2, OUTPUT);

  terminal.clear();
  //---------------------------------------------------------------
  timeout.setTimeout(5000, []() {
    timer_1 = timer.setInterval(1183L, []() {
      // readPower();
      readPower1();
      readPower2();
      readPower3();
      up();
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
    });
    timer_2 = timer.setInterval(251L, []() {
      readPressure();
      MeasureCmForSmoothing();
    });
    //-------------------------------
    timer.setInterval(30016L, []() {
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
  });
}

void loop() {
  ESP.wdtFeed();
  Blynk.run();
  timeout.run();
  timer.run();
}
