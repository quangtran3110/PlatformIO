/*
V0 - Btn Cap 1
V1 - Btn Bom 1
V2 - Btn Bom 2
V3 - Btn Nen khi
V4 -
V5 - Irms 0
V6 - I_vdf
V7 - Irms 2
V8 - Irms 3
V3 - Chon máy cài đặt bảo vệ
V10- Min
V11- Max
V12- String
V13- Tan so Bom 1
V14- Ap luc
V15- Con lai
V16- The tich
V17- Key protect
V18- Set ref
V19- Nhiệt độ biên tần
V20- date/time
V21- Nhiệt độ động cơ
V22- Nhiệt độ tủ điện
V23- datas_volume
V24- LLG1_1m3
V25- LLG1_24H
V26- LLG1_RL
V27- RỬA LỌC
V40- thời gian chạy G1
V41- thời gian chạy G1-24h
V42- thời gian chạy B1
V43- thời gian chạy B1 - 24h
*/
/*
#define BLYNK_TEMPLATE_ID "TMPL6sp_uYXmC"
#define BLYNK_TEMPLATE_NAME "MH TRAM 2 BPT"
#define BLYNK_AUTH_TOKEN "CJNSfOtHYJ0poN7g4Qaswwqopwzko_Ux"
*/
/*
#define BLYNK_TEMPLATE_ID "TMPL6D2ion8Uo"
#define BLYNK_TEMPLATE_NAME "TRẠM 2 BPT"
#define BLYNK_AUTH_TOKEN "YZXkYAgH44t-kjJPKapydw5vMlR7MGAC"
*/
#define BLYNK_TEMPLATE_ID "TMPL6D2ion8Uo"
#define BLYNK_TEMPLATE_NAME "TRẠM 2 BPT"
#define BLYNK_AUTH_TOKEN "YZXkYAgH44t-kjJPKapydw5vMlR7MGAC"

#define VOLUME_TOKEN "dXbklpLJKTZQ5hK9Qpy7Sg5DdwgmQ8z"
#define BLYNK_FIRMWARE_VERSION "240910"

const char *ssid = "BPT2";
const char *password = "0919126757";
//-------------------------------------------------------------------
#define BLYNK_PRINT Serial
#define APP_DEBUG

#pragma region
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <UrlEncode.h>
//-----------------------------
#include "PCF8575.h"
PCF8575 pcf8575_1(0x20);
//-----------------------------
#include "EmonLib.h"
EnergyMonitor emon0, emon1, emon2, emon3;
//-----------------------------
#include "RTClib.h"
#include <WidgetRTC.h>
RTC_DS3231 rtc_module;
//-----------------------------
#include <Eeprom24C32_64.h>
#include <Wire.h>
#define EEPROM_ADDRESS 0x57
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
const word address = 0;
//-----------------------------
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
//-----------------------------
#include <DallasTemperature.h>
#include <OneWire.h>
#define ONE_WIRE_BUS D1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float temp[3];
//-----------------------------
WiFiClient client;
HTTPClient http;
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/CN_MocHoa/Tram2BPT/.pio/build/nodemcuv2/firmware.bin"
String server_name = "http://sgp1.blynk.cloud/external/api/";
//-----------------------------
#include <ModbusRTU.h>
#include <SoftwareSerial.h>
// connect TX to D4 (GPIO0), RX to D3 (GPIO2)/SoftwareSerial S(TX, RX);
SoftwareSerial S(2, 0);
ModbusRTU mb;
bool key_read = true;
int time_delay;
int32_t int32_2int16(int int1, int int2) {
  union i32_2i16 {
    int32_t f;
    uint16_t i[2];
  };
  union i32_2i16 f_number;
  f_number.i[0] = int1;
  f_number.i[1] = int2;
  return f_number.f;
}
bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
  }
  return true;
}
//-----------------------------
const int S0 = 14;
const int S1 = 12;
const int S2 = 13;
const int S3 = 15;

const int pin_G1 = P7;
const int pin_B1 = P6;
const int pin_B2 = P5;
const int pin_NK = P4;
const int pin_Fan = P3;
const int pin_rst = P2;
//-----------------------------
char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
int xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0, xSetAmpe3 = 0;
int timer_I;
unsigned long int yIrms0 = 0, yIrms1 = 0, yIrms2 = 0, yIrms3 = 0;
float Irms0, Irms1, Irms2, Irms3, I_vdf, pre, ref_percent, ref_blynk, hz;
bool trip0 = false, trip1 = false, trip2 = false, trip3 = false;
bool key = false, blynk_first_connect = false, status_fan = HIGH;
byte c;
byte reboot_num;
int LLG1_1m3;
int temp_vdf;
int G1_start, B1_start;
bool G1_save = false, B1_save = false;
//-----------------------------
#define filterSamples 121
int dai = 510;
int rong = 510;
int dosau = 260;
int volume, dungtich, smoothDistance;
long distance;
int zeropointTank = 189, fullpointTank = 850;
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
//-----------------------------
struct Data {
  byte SetAmpemax, SetAmpemin;
  byte SetAmpe1max, SetAmpe1min;
  byte SetAmpe2max, SetAmpe2min;
  byte SetAmpe3max, SetAmpe3min;
  int startH, startM;
  int stopH, stopM;
  byte mode_run;
  int save_num;
  byte keyp;
  byte status_b1, status_b2, status_cap1, status_nenkhi;
  float pre_set;
  byte reset_day;
  int timerun_G1, timerun_B1;
  int LLG1_RL;
  byte rualoc;
} data, dataCheck;
const struct Data dataDefault = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#pragma endregion
//-----------------------------
WidgetRTC rtc_widget;
WidgetTerminal terminal(V12);
WidgetTerminal volume_terminal(V23);
BlynkTimer timer, timer1;
BLYNK_CONNECTED() {
  rtc_widget.begin();
  blynk_first_connect = true;
  Blynk.syncVirtual(V18);
}
//-------------------------------------------------------------------
//-------------------------
void savedata() {
  if (memcmp(&data, &dataCheck, sizeof(dataDefault)) == 0) {
    // Serial.println("structures same no need to write to EEPROM");
  } else {
    // Serial.println("\nWrite bytes to EEPROM memory...");
    data.save_num = data.save_num + 1;
    eeprom.writeBytes(address, sizeof(dataDefault), (byte *)&data);
  }
  Blynk.setProperty(V12, "label", BLYNK_FIRMWARE_VERSION, "-EEPROM ", data.save_num);
}
//-------------------------
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
//----------------------------------------------------------------
void up() {
  String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN + "&V5=" + Irms0 + "&V6=" + I_vdf + "&V8=" + Irms3 + "&V13=" + hz + "&V14=" + pre + "&V15=" + smoothDistance + "&V16=" + volume + "&V19=" + temp_vdf + "&V40=" + float(data.timerun_G1) / 1000 / 60 / 60 + "&V42=" + float(data.timerun_B1) / 1000 / 60 / 60;
  //+ "&V21=" + temp[0]
  http.begin(client, server_path.c_str());
  int httpResponseCode = http.GET();
  http.end();
}
void up_timerun_motor() {
  String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN + "&V41=" + float(data.timerun_G1) / 1000 / 60 / 60 + "&V43=" + float(data.timerun_B1) / 1000 / 60 / 60;
  http.begin(client, server_path.c_str());
  int httpResponseCode = http.GET();
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
//----------------------------------------------------------------
void on_cap1() { // NC
  data.status_cap1 = HIGH;
  pcf8575_1.digitalWrite(pin_G1, data.status_cap1);
  Blynk.virtualWrite(V0, data.status_cap1);
  savedata();
}
void off_cap1() {
  data.status_cap1 = LOW;
  pcf8575_1.digitalWrite(pin_G1, data.status_cap1);
  Blynk.virtualWrite(V0, data.status_cap1);
  savedata();
}
void on_b1() { // NC
  data.status_b1 = HIGH;
  pcf8575_1.digitalWrite(pin_B1, data.status_b1);
  Blynk.virtualWrite(V1, data.status_b1);
  savedata();
}
void off_b1() {
  data.status_b1 = LOW;
  pcf8575_1.digitalWrite(pin_B1, data.status_b1);
  Blynk.virtualWrite(V1, data.status_b1);
  savedata();
}
void on_b2() { // NO
  data.status_b2 = HIGH;
  pcf8575_1.digitalWrite(pin_B2, data.status_b2);
  Blynk.virtualWrite(V2, data.status_b2);
  savedata();
}
void off_b2() {
  data.status_b2 = LOW;
  pcf8575_1.digitalWrite(pin_B2, data.status_b2);
  Blynk.virtualWrite(V2, data.status_b2);
  savedata();
}
void on_nenkhi() { // NC
  data.status_nenkhi = HIGH;
  pcf8575_1.digitalWrite(pin_NK, data.status_nenkhi);
  Blynk.virtualWrite(V3, data.status_nenkhi);
  savedata();
}
void off_nenkhi() {
  data.status_nenkhi = LOW;
  pcf8575_1.digitalWrite(pin_NK, data.status_nenkhi);
  Blynk.virtualWrite(V3, data.status_nenkhi);
  savedata();
}
void on_fan() { // NC
  status_fan = HIGH;
  pcf8575_1.digitalWrite(pin_Fan, status_fan);
}
void off_fan() {
  status_fan = LOW;
  pcf8575_1.digitalWrite(pin_Fan, status_fan);
}
void rst_module() {
  pcf8575_1.digitalWrite(pin_rst, LOW);
}
void hidden() {
  Blynk.setProperty(V9, "isHidden", true);
  Blynk.setProperty(V10, "isHidden", true);
  Blynk.setProperty(V11, "isHidden", true);
  Blynk.setProperty(V17, "isHidden", true);
}
void visible() {
  Blynk.setProperty(V9, "isHidden", false);
  Blynk.setProperty(V10, "isHidden", false);
  Blynk.setProperty(V11, "isHidden", false);
  Blynk.setProperty(V17, "isHidden", false);
}
//-------------------------------------------------------------------
void MeasureCmForSmoothing() // C1
{
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  float sensorValue = analogRead(A0);
  distance = (((sensorValue - zeropointTank) * 500) / (fullpointTank - zeropointTank));
  Serial.print("sensorValue ");
  Serial.println(distance);
  if (distance > 0) {
    smoothDistance = digitalSmooth(distance, sensSmoothArray1);
    volume = (dai * smoothDistance * rong) / 1000000;
    // Serial.print("\nsmoothDistance ");
    // Serial.println(smoothDistance);
  }
  // Blynk.virtualWrite(V15, smoothDistance);
  // Blynk.virtualWrite(V16, volume);
}
void temperature() { // Nhiệt độ
  sensors.requestTemperatures();
  for (byte i = 0; i < sensors.getDeviceCount(); i++) {
    temp[i] = sensors.getTempCByIndex(i);
    if (temp[i] < 0)
      temp[i] = 0;
    // Serial.println(temp[0]);
  }
}
//-------------------------------------------------------------------
void readcurrent() // C2 - Cấp 1   - I0
{
  digitalWrite(S0, LOW);
  digitalWrite(S1, HIGH);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  float rms0 = emon0.calcIrms(740);
  if (rms0 < 2) {
    Irms0 = 0;
    yIrms0 = 0;
    if (G1_start != 0) {
      data.timerun_G1 = data.timerun_G1 + (millis() - G1_start);
      savedata();
      G1_start = 0;
    }
  } else if (rms0 >= 2) {
    yIrms0 = yIrms0 + 1;
    Irms0 = rms0;
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
        if ((xSetAmpe > 3) && (data.keyp)) {
          off_cap1();
          xSetAmpe = 0;
          trip0 = true;
          Blynk.logEvent("error", String("Cấp 1 lỗi: ") + Irms0 + String(" A"));
        }
      } else {
        xSetAmpe = 0;
      }
    }
  }
}
void readcurrent1() // C3 - Bơm 1   - I1
{
  digitalWrite(S0, HIGH);
  digitalWrite(S1, HIGH);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  float rms1 = emon1.calcIrms(740);
  if (rms1 < 2) {
    Irms1 = 0;
    yIrms1 = 0;
  } else if (rms1 >= 2) {
    Irms1 = rms1;
    yIrms1 = yIrms1 + 1;
    if (yIrms1 > 3) {
      if ((Irms1 >= data.SetAmpe1max) || (Irms1 <= data.SetAmpe1min)) {
        xSetAmpe1 = xSetAmpe1 + 1;
        if ((xSetAmpe1 > 3) & (data.keyp)) {
          off_b1();
          xSetAmpe1 = 0;
          trip1 = true;
          Blynk.logEvent("error", String("Bơm 1 lỗi: ") + Irms1 + String(" A"));
        }
      } else {
        xSetAmpe1 = 0;
      }
    }
  }
}
void readcurrent2() // C4 - Bơm 2   - I2
{
  // Blynk.run();
  digitalWrite(S0, LOW);
  digitalWrite(S1, LOW);
  digitalWrite(S2, HIGH);
  digitalWrite(S3, LOW);
  float rms2 = emon2.calcIrms(740);
  if (rms2 < 2) {
    Irms2 = 0;
    yIrms2 = 0;
  } else if (rms2 >= 2) {
    Irms2 = rms2;
    yIrms2 = yIrms2 + 1;
    if ((yIrms2 > 3) && ((Irms2 >= data.SetAmpe2max) || (Irms2 <= data.SetAmpe2min))) {
      xSetAmpe2 = xSetAmpe2 + 1;
      if ((xSetAmpe2 > 3) & (data.keyp)) {
        off_b2();
        xSetAmpe2 = 0;
        trip2 = true;
        Blynk.logEvent("error", String("Bơm 2 lỗi: ") + Irms2 + String(" A"));
      }
    } else {
      xSetAmpe2 = 0;
    }
  }
  // Blynk.virtualWrite(V7, Irms2);
}
void readcurrent3() // C5 - NenKhi  - I3
{
  // Blynk.run();
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);
  digitalWrite(S2, HIGH);
  digitalWrite(S3, LOW);
  float rms3 = emon3.calcIrms(740);
  if (rms3 < 2) {
    Irms3 = 0;
    yIrms3 = 0;
  } else if (rms3 >= 2) {
    Irms3 = rms3;
    yIrms3 = yIrms3 + 1;
    if ((yIrms3 > 3) && ((Irms3 >= data.SetAmpe3max) || (Irms3 <= data.SetAmpe3min))) {
      xSetAmpe3 = xSetAmpe3 + 1;
      if ((xSetAmpe3 > 3) & (data.keyp)) {
        off_nenkhi();
        xSetAmpe3 = 0;
        trip3 = true;
        Blynk.logEvent("error", String("Máy nén khí lỗi: ") + Irms3 + String(" A"));
      }
    } else {
      xSetAmpe3 = 0;
    }
  }
  // Blynk.virtualWrite(V8, Irms3);
}
void read_timerun_b1() {
  if (I_vdf <= 0) {
    if (B1_start != 0) {
      data.timerun_B1 = data.timerun_B1 + (millis() - B1_start);
      savedata();
      B1_start = 0;
    }
  } else {
    if (B1_start >= 0) {
      if (B1_start == 0)
        B1_start = millis();
      else if (millis() - B1_start > 60000) {
        B1_save = true;
      } else
        B1_save = false;
    }
  }
}
//-------------------------------------------------------------------
void rtctime() {
  DateTime now = rtc_module.now();
  if (blynk_first_connect == true) {
    if ((now.day() != day()) || (now.hour() != hour()) || ((now.minute() - minute() > 2) || (minute() - now.minute() > 2))) {
      rtc_module.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
      DateTime now = rtc_module.now();
    }
  }
  Blynk.virtualWrite(V20, daysOfTheWeek[now.dayOfTheWeek()], ", ", now.day(), "/", now.month(), "/", now.year(), " - ", now.hour(), ":", now.minute(), ":", now.second());

  /*
  int nowtime = (now.hour() * 3600 + now.minute() * 60);

  if (data.mode_cap2 == 1) {   // Chạy Tu Dong
    if (data.mode_run == 0) {  //Ngay chan tat may
      if (now.day() % 2 == 0) {
        if ((nowtime + 1800) > data.bom_chanle_stop) {  //30p
          off_time_cap1();
          trip0 = true;
          if (nowtime > data.bom_chanle_stop) {
            trip0 = false;
            off_time();
          }
        }
        if (nowtime < data.bom_chanle_stop) {
          on_time();
        }
      }
      if (now.day() % 2 != 0) {
        if ((nowtime > data.bom_chanle_start)) {
          on_time();
        }
      }
    }
    if (data.mode_run == 1) {  //Ngay le tat may
      if (now.day() % 2 != 0) {
        if ((nowtime + 1200) > data.bom_chanle_stop) {  //20p
          off_time_cap1();
          trip0 = true;
          if (nowtime > data.bom_chanle_stop) {
            off_time();
            trip0 = false;
          }
        }
        if (nowtime < data.bom_chanle_stop) {
          on_time();
        }
      }
      if (now.day() % 2 == 0) {
        if (nowtime > data.bom_chanle_start) {
          on_time();
        }
      }
    }
    if (data.mode_run == 2) {  //Moi ngày
      if (data.bom_moingay_start > data.bom_moingay_stop) {
        if (((nowtime + 1800) > data.bom_moingay_stop) && (nowtime < data.bom_moingay_start)) {
          off_time_cap1();
          trip0 = true;
          if (!maxtank && ((nowtime > data.bom_moingay_stop) && (nowtime < data.bom_moingay_start))) {
            off_time();
            trip0 = false;
          }
        }
        if ((nowtime < data.bom_moingay_stop) || (nowtime > data.bom_moingay_start) || maxtank) {
          on_time();
        }
      }
      if (data.bom_moingay_start < data.bom_moingay_stop) {
        if (((nowtime + 1800) > data.bom_moingay_stop) || (nowtime < data.bom_moingay_start)) {
          off_time_cap1();
          trip0 = true;
          if (!maxtank && ((nowtime > data.bom_moingay_stop) || (nowtime < data.bom_moingay_start))) {
            off_time();
            trip0 = false;
          }
        }
        if (((nowtime < data.bom_moingay_stop) && (nowtime > data.bom_moingay_start)) || maxtank) {
          on_time();
        }
      }
    }
  }
  */
}
//-------------------------------------------------------------------
uint16_t nhietdo_bientan[1];
bool cbWrite_nhietdo(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    temp_vdf = (nhietdo_bientan[0]);
  }
  return true;
}
uint16_t tanso[1];
bool cbWrite_tanso(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    hz = tanso[0] / 10;
  }
  return true;
}
uint16_t dongdien[2];
bool cbWrite_dongdien(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    I_vdf = float(int32_2int16(dongdien[1], dongdien[0])) / 100;
  }
  return true;
}
uint16_t apluc[2];
bool cbWrite_apluc(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    pre = float(int32_2int16(apluc[1], apluc[0])) / 1000;
  }
  return true;
}
uint16_t ref_percent_[1];
bool cbWrite_aplucset(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    ref_percent = float(int32_2int16(ref_percent_[1], ref_percent_[0])) / 100; // Áp lực tham chiếu tổng dạng %
  }
  return true;
}
uint16_t ref_blynk_[1];
bool cbWrite_ref_blynk_(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    ref_blynk = ref_blynk_[0]; // Áp lực tham chiếu nhập từ Blynk (0-16384)
  }
  return true;
}
//-------------------------
void read_modbus() {
  { // Nhiệt độ biến tần
    mb.readHreg(1, 16339, nhietdo_bientan, 1, cbWrite_nhietdo);
    while (mb.slave()) {
      mb.task();
      delay(20);
    }
  }
  // Fan
  {
    if ((temp_vdf < 35) && (status_fan == HIGH)) {
      off_fan();
    }
    if ((temp_vdf > 40) && (status_fan == LOW)) {
      on_fan();
    }
  }
  // Tần số
  {
    mb.readHreg(1, 16129, tanso, 1, cbWrite_tanso);
    while (mb.slave()) {
      mb.task();
      delay(20);
    }
  }
  // Dòng điện
  {
    mb.readHreg(1, 16139, dongdien, 2, cbWrite_dongdien);
    while (mb.slave()) { // Check if transaction is active
      mb.task();
      delay(20);
    }
  }
  // Áp lực
  {
    mb.readHreg(1, 16519, apluc, 2, cbWrite_apluc);
    while (mb.slave()) { // Check if transaction is active
      mb.task();
      delay(20);
    }
  }
  // Áp lực set
  {
    mb.readHreg(1, 16009, ref_percent_, 2, cbWrite_aplucset);
    while (mb.slave()) { // Check if transaction is active
      mb.task();
      delay(20);
    }
    float ref_bar = ref_percent / 10; // Áp lực tham chiếu tổng dạng bar
    if (ref_bar != data.pre_set) {
      if (ref_bar == 0) {
        int send_ref = int((data.pre_set * 10) / 100 * 16384);
        mb.writeHreg(1, 50009, send_ref, cbWrite);
        while (mb.slave()) { // Check if transaction is active
          mb.task();
          delay(20);
        }
        Blynk.virtualWrite(V18, data.pre_set);
      } else {
        mb.readHreg(1, 50009, ref_blynk_, 1, cbWrite_ref_blynk_);
        while (mb.slave()) {
          mb.task();
          delay(20);
        }
        float ref_blynk_percent = ref_blynk / 16384 * 100; // Áp lực tham chiếu nhập từ Blynk dạng %
        int ref_bientro_percent = ref_percent - ref_blynk_percent;
        if (ref_bientro_percent != 0) {
          if (ref_blynk != 0) {
            mb.writeHreg(1, 50009, 0, cbWrite);
            while (mb.slave()) { // Check if transaction is active
              mb.task();
              delay(20);
            }
          }
          data.pre_set = ref_bar;
          Blynk.virtualWrite(V18, data.pre_set);
        } else {
          int send_ref = int((data.pre_set * 10) / 100 * 16384);
          mb.writeHreg(1, 50009, send_ref, cbWrite);
          while (mb.slave()) { // Check if transaction is active
            mb.task();
            delay(20);
          }
          Blynk.virtualWrite(V18, data.pre_set);
        }
      }
    }
  }
}
//-------------------------
BLYNK_WRITE(V0) // Gieng
{
  if ((key) && (!trip0)) {
    if (param.asInt() == LOW) {
      off_cap1();
    } else {
      on_cap1();
    }
  } else
    Blynk.virtualWrite(V0, data.status_cap1);
}
BLYNK_WRITE(V1) // Bơm 1
{
  if ((key) && (!trip1)) {
    if (param.asInt() == LOW) {
      off_b1();
    } else {
      on_b1();
    }
  } else
    Blynk.virtualWrite(V1, data.status_b1);
}
BLYNK_WRITE(V2) // Bơm 2
{
  if ((key) && (!trip2)) {
    if (param.asInt() == LOW) {
      off_b2();
    } else {
      on_b2();
    }
  } else
    Blynk.virtualWrite(V2, data.status_b2);
}
BLYNK_WRITE(V3) // Nen Khi
{
  if ((key) && (!trip3)) {
    if (param.asInt() == LOW) {
      off_nenkhi();
    } else {
      on_nenkhi();
    }
  } else
    Blynk.virtualWrite(V3, data.status_nenkhi);
}
BLYNK_WRITE(V9) // Chon máy cài đặt bảo vệ
{
  switch (param.asInt()) {
  case 0: { // ....
    c = 0;
    Blynk.virtualWrite(V10, 0);
    Blynk.virtualWrite(V11, 0);
    break;
  }
  case 1: { // Gieng
    c = 1;
    Blynk.virtualWrite(V10, data.SetAmpemin);
    Blynk.virtualWrite(V11, data.SetAmpemax);
    break;
  }
  case 2: { // Bom 1
    c = 2;
    Blynk.virtualWrite(V10, data.SetAmpe1min);
    Blynk.virtualWrite(V11, data.SetAmpe1max);
    break;
  }
  case 3: { // Bom 2
    c = 3;
    Blynk.virtualWrite(V10, data.SetAmpe2min);
    Blynk.virtualWrite(V11, data.SetAmpe2max);
    break;
  }
  case 4: { // Nen Khi
    c = 4;
    Blynk.virtualWrite(V10, data.SetAmpe3min);
    Blynk.virtualWrite(V11, data.SetAmpe3max);
    break;
  }
  }
}
BLYNK_WRITE(V10) // min
{
  if (key) {
    if (c == 1) {
      data.SetAmpemin = param.asFloat();
    } else if (c == 2) {
      data.SetAmpe1min = param.asFloat();
    } else if (c == 3) {
      data.SetAmpe2min = param.asFloat();
    } else if (c == 4) {
      data.SetAmpe3min = param.asFloat();
    }
  } else
    Blynk.virtualWrite(V10, 0);
}
BLYNK_WRITE(V11) // max
{
  if (key) {
    if (c == 1) {
      data.SetAmpemax = param.asFloat();
    } else if (c == 2) {
      data.SetAmpe1max = param.asFloat();
    } else if (c == 3) {
      data.SetAmpe2max = param.asFloat();
    } else if (c == 4) {
      data.SetAmpe3max = param.asFloat();
    }
  } else
    Blynk.virtualWrite(V11, 0);
}
BLYNK_WRITE(V12) // String
{
  String dataS = param.asStr();
  if ((dataS == "mh") || (dataS == "MH")) {
    terminal.clear();
    key = true;
    Blynk.virtualWrite(V12, "Đơn vị vận hành: 'M.Hóa'\nKích hoạt trong 15s\n");
    timer1.setTimeout(15000, []() {
      key = false;
      terminal.clear();
    });
  } else if (dataS == "active") {
    terminal.clear();
    key = true;
    visible();
    Blynk.virtualWrite(V12, "KHÔNG sử dụng phần mềm cho đến khi thông báo này mất.\n");
  } else if (dataS == "deactive") {
    terminal.clear();
    key = false;
    hidden();
    Blynk.virtualWrite(V12, "Ok!\nNhập mã để điều khiển!\n");
  } else if (dataS == "save") {
    terminal.clear();
    savedata();
    Blynk.virtualWrite(V12, "Đã lưu cài đặt.\n");
  } else if (dataS == "reset") {
    terminal.clear();
    trip0 = false;
    trip1 = false;
    trip2 = false;
    trip3 = false;
    on_cap1();
    on_b1();
    on_b2();
    on_nenkhi();
    mb.writeHreg(1, 49999, 1212, cbWrite);
    while (mb.slave()) { // Check if transaction is active
      mb.task();
      yield();
      delay(10);
    }
    Blynk.virtualWrite(V12, "Đã RESET! \nNhập mã để điều khiển!\n");
  } else if (dataS == "rst") {
    terminal.clear();
    Blynk.virtualWrite(V12, "ESP Khởi động lại sau 3s");
    delay(3000);
    // ESP.restart();
    rst_module();
  } else if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V12, "ESP UPDATE...");
    update_fw();
  } else if (dataS == "stop") {
    terminal.clear();
    mb.writeHreg(1, 49999, 1084, cbWrite);
    while (mb.slave()) { // Check if transaction is active
      mb.task();
      yield();
      delay(10);
    }
    Blynk.virtualWrite(V12, "Đã dừng biến tần\n");
  } else if (dataS == "run") {
    terminal.clear();
    mb.writeHreg(1, 49999, 1148, cbWrite);
    while (mb.slave()) { // Check if transaction is active
      mb.task();
      yield();
      delay(10);
    }
    Blynk.virtualWrite(V12, "Chạy biến tần\n");
  } else {
    Blynk.virtualWrite(V12, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
BLYNK_WRITE(V17) // Bảo vệ
{
  if (key) {
    if (param.asInt() == LOW)
      data.keyp = false;
    else
      data.keyp = true;
    savedata();
  } else
    Blynk.virtualWrite(V17, data.keyp);
}

BLYNK_WRITE(V18) // Cai ap luc bien tan
{
  if (key) {
    data.pre_set = param.asFloat();
    savedata();
  } else
    Blynk.virtualWrite(V18, data.pre_set);
}
BLYNK_WRITE(V23) {
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
BLYNK_WRITE(V24) // Lưu lượng G1_1m3
{
  LLG1_1m3 = param.asInt();
}
BLYNK_WRITE(V27) // Rửa lọc
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
    Blynk.virtualWrite(V27, data.rualoc);
  }
}
//-------------------------------------------------------------------
void setup() {
  pinMode(D4, OUTPUT);
  pinMode(D3, INPUT);
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  //-----------------------
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  //-----------------------
  delay(10000);
  Wire.begin();
  // sensors.begin();
  S.begin(9600, SWSERIAL_8N1);
  mb.begin(&S);
  mb.master();
  delay(3000);
  //-----------------------
  rtc_module.begin();
  eeprom.initialize();
  eeprom.readBytes(address, sizeof(dataDefault), (byte *)&data);
  //-----------------------
  emon0.current(A0, 110);
  emon1.current(A0, 110);
  emon2.current(A0, 110);
  emon3.current(A0, 110);
  //-----------------------
  pcf8575_1.begin();
  pcf8575_1.pinMode(pin_G1, OUTPUT);
  pcf8575_1.digitalWrite(pin_G1, data.status_cap1);
  pcf8575_1.pinMode(pin_B1, OUTPUT);
  pcf8575_1.digitalWrite(pin_B1, data.status_b1);
  pcf8575_1.pinMode(pin_B2, OUTPUT);
  pcf8575_1.digitalWrite(pin_B2, data.status_b2);
  pcf8575_1.pinMode(pin_NK, OUTPUT);
  pcf8575_1.digitalWrite(pin_NK, data.status_nenkhi);
  pcf8575_1.pinMode(pin_rst, OUTPUT);
  pcf8575_1.digitalWrite(pin_rst, HIGH);
  pcf8575_1.pinMode(pin_Fan, OUTPUT);
  pcf8575_1.digitalWrite(pin_Fan, HIGH);

  timer1.setTimeout(5000L, []() {
    timer_I = timer.setInterval(1789, []() {
      readcurrent();
      read_timerun_b1();
      // readcurrent1();
      // readcurrent2();
      readcurrent3();
      // temperature();
      read_modbus();
      up();
      timer.restartTimer(timer_I);
    });
    /*
    timer.setInterval(5033L, []() {
      read_modbus();
      up();
      timer.restartTimer(timer_I);
    });
    */
    timer.setInterval(230L, MeasureCmForSmoothing);
    timer.setInterval(15005L, []() {
      rtctime();
      time_run_motor();
      timer.restartTimer(timer_I);
    });
    timer.setInterval(900005L, []() {
      connectionstatus();
      timer.restartTimer(timer_I);
    });
    timer.setInterval(230L, MeasureCmForSmoothing);
  });
}
void loop() {
  Blynk.run();
  timer.run();
  timer1.run();
}