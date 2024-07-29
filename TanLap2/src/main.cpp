/*
V0 - Btn Bom 1
V1 - Btn Bom 2
V2 - Btn Bom 3
V3 - MODE_CAP2
V4 - Key notification
V5 - Irms 0
V6 - Irms 1
V7 - Irms 2
V8 - Key protect
V9 - Chon máy cài đặt bảo vệ
V10- Min
V11- Max
V12- String
V13- Tan so
V14- Ap luc
V15- Con lai
V16- The tich
V17- Set ref
V18- Nhiệt độ biên tần
V19- Nhiệt độ B1
V20- Nhiệt độ B2
V21- Nhiệt độ B3
V22- Nhiệt độ tủ điện
V23- date/time
V24- INFO
V25- PRE_MIN
V26- IN_VOLT
V27- OUT_VOLT
V28- SPEED_MOTOR
V29- POWER
V30- I_vfd
V31- Mức áp thấp
V32- Menu timer
V33- Timeinput


V40- thời gian chạy B1
V41- thời gian chạy B1-24h
V42- thời gian chạy B2
V43- thời gian chạy B2 - 24h
V44- thời gian chạy B3
V45- thời gian chạy B3 - 24h
*/

#define BLYNK_TEMPLATE_ID "TMPL6XlJbectO"
#define BLYNK_TEMPLATE_NAME "TRẠM TÂN LẬP 2"
#define BLYNK_AUTH_TOKEN "4ucvqgxgGbTgGLtfIAQatiGH2JDiMMG_"
#define BLYNK_FIRMWARE_VERSION "240728"
#define BLYNK_PRINT Serial
#define APP_DEBUG
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include "PCF8575.h"
#include <WidgetRTC.h>
#include "RTClib.h"
#include <Wire.h>
#include <Eeprom24C32_64.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <ModbusRTU.h>
#include <SoftwareSerial.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include "EmonLib.h"
//-----------------------------
#define filterSamples 121
#define EEPROM_ADDRESS 0x57
#define ONE_WIRE_BUS D3
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/TanLap2/.pio/build/nodemcuv2/firmware.bin"
//-----------------------------
const char *ssid = "tram bom so 4";
const char *password = "0943950555";
//-----------------------------
PCF8575 pcf8575_1(0x20);
EnergyMonitor emon0, emon1, emon2, emon3;
RTC_DS3231 rtc_module;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClient client;
HTTPClient http;
SoftwareSerial S(14, 12); // connect RX to D5 (GPI14),TX to D6 (GPI12) - SoftwareSerial S(RX, TX);
ModbusRTU mb;
//-----------------------------
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
const word address = 0;
const int S0pin = P15;
const int S1pin = P14;
const int S2pin = P13;
const int S3pin = P12;
const int pin_B1 = P1;
const int pin_B2 = P2;
const int pin_B3 = P3;
const int pin_khuay_clo = P4;
const int pin_cham_clo = P5;
const int pin_Fan = P6;
const int pin_rst = P7;
char tz[] = "Asia/Ho_Chi_Minh";
char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
//-----------------------------
String server_name = "http://sgp1.blynk.cloud/external/api/";
byte status_b1, status_b2, status_b3, time_run = false;
byte c, j, i, x = 0, b;
byte reboot_num;

float temp[3];
float Irms0, Irms1, Irms2, Irms3, temp_vfd, I_vfd, power, pre, ref_percent;
float f2dec(float number)
{
  return round(number * 100.0) / 100.0;
}

long distance;

bool trip0 = false, trip1 = false, trip2 = false, trip3 = false;
bool key = false, blynk_first_connect = false, status_fan = HIGH;
bool B1_save = false, B2_save = false, B3_save = false;

unsigned long int yIrms0 = 0, yIrms1 = 0, yIrms2 = 0, yIrms3 = 0;
int dai = 510;
int rong = 510;
int dosau = 260;
int hz, out_volt, in_volt;
int volume, dungtich, smoothDistance;
int xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0, xSetAmpe3 = 0;
int timer_I, timer_Measure, speed_motor;
int LLG1_1m3;
int temp_;
int B1_start, B2_start, B3_start;
int zero_pre = 194, max_pre_bar = 10;
int zeropointTank = 189, fullpointTank = 850;
int sensSmoothArray1[filterSamples];
int digitalSmooth(int rawIn, int *sensSmoothArray)
{
  int j, k, temp, top, bottom;
  long total;
  static int i;
  static int sorted[filterSamples];
  boolean done;

  i = (i + 1) % filterSamples; // increment counter and roll over if necc. -  % (modulo operator) rolls over variable
  sensSmoothArray[i] = rawIn;  // input new data into the oldest slot

  // Serial.print("raw = ");

  for (j = 0; j < filterSamples; j++)
  { // transfer data array into anther array for sorting and averaging
    sorted[j] = sensSmoothArray[j];
  }

  done = 0; // flag to know when we're done sorting
  while (done != 1)
  { // simple swap sort, sorts numbers from lowest to highest
    done = 1;
    for (j = 0; j < (filterSamples - 1); j++)
    {
      if (sorted[j] > sorted[j + 1])
      { // numbers are out of order - swap
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
  for (j = bottom; j < top; j++)
  {
    total += sorted[j]; // total remaining indices
    k++;
  }
  return total / k; // divide by number of samples
}
//-----------------------------
bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void *data)
{
  if (event == Modbus::EX_SUCCESS)
  {
  }
  return true;
}
uint16_t data_vfd[13];
bool cbWrite_data_vfd(Modbus::ResultCode event, uint16_t transactionId, void *data)
{
  if (event == Modbus::EX_SUCCESS)
  {
    hz = data_vfd[0] / 100;
    I_vfd = data_vfd[1] / 10;
    in_volt = data_vfd[2] / 10;
    out_volt = data_vfd[3] / 10;
    speed_motor = data_vfd[4];
    // pre_set = float(data_vfd[7]) / 10;
    pre = ((float(data_vfd[8]) - zero_pre) * max_pre_bar) / (1000 - zero_pre);
    power = data_vfd[9] / 10;
    temp_vfd = data_vfd[11] / 10;
  }
  return true;
}
uint16_t ref_percent_[1];
bool cbWrite_aplucset(Modbus::ResultCode event, uint16_t transactionId, void *data)
{
  if (event == Modbus::EX_SUCCESS)
  {
    ref_percent = float(ref_percent_[0]); // Áp lực tham chiếu tổng dạng %
  }
  return true;
}
//-----------------------------
struct Data
{
  byte SetAmpemax, SetAmpemin;
  byte SetAmpe1max, SetAmpe1min;
  byte SetAmpe2max, SetAmpe2min;
  byte SetAmpe3max, SetAmpe3min;
  int t1_start, t1_stop, t2_start, t2_stop, t3_start, t3_stop;
  int save_num;
  float pre_set, pre_min;
  byte mode_cap2;
  byte keyp, keynoti;
  byte reset_day;
  int timerun_B1, timerun_B2, timerun_B3;
} data, dataCheck;
const struct Data dataDefault = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//-----------------------------
WidgetRTC rtc_widget;
BlynkTimer timer, timer1;
WidgetTerminal terminal(V12);
BLYNK_CONNECTED()
{
  rtc_widget.begin();
  blynk_first_connect = true;
  Blynk.syncVirtual(V18);
}
//----------------------------------------------------------------
void savedata()
{
  if (memcmp(&data, &dataCheck, sizeof(dataDefault)) == 0)
  {
    // Serial.println("structures same no need to write to EEPROM");
  }
  else
  {
    // Serial.println("\nWrite bytes to EEPROM memory...");
    data.save_num = data.save_num + 1;
    eeprom.writeBytes(address, sizeof(dataDefault), (byte *)&data);
  }
  Blynk.setProperty(V12, "label", BLYNK_FIRMWARE_VERSION, "-EEPROM ", data.save_num);
}
//-------------------------
void connectionstatus()
{
  if ((WiFi.status() != WL_CONNECTED))
  {
    Serial.println("Khong ket noi WIFI");
    WiFi.begin(ssid, password);
  }
  if ((WiFi.status() == WL_CONNECTED) && (!Blynk.connected()))
  {
    reboot_num = reboot_num + 1;
    if ((reboot_num == 1) || (reboot_num == 2))
    {
      Serial.println("...");
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(ssid, password);
    }
    if (reboot_num % 5 == 0)
    {
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(ssid, password);
    }
  }
  if (Blynk.connected())
  {
    if (reboot_num != 0)
    {
      reboot_num = 0;
    }
  }
}
void update_started()
{
  Serial.println("CALLBACK:  HTTP update process started");
}
void update_finished()
{
  Serial.println("CALLBACK:  HTTP update process finished");
}
void update_progress(int cur, int total)
{
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}
void update_error(int err)
{
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}
void update_fw()
{
  WiFiClientSecure client_;
  client_.setInsecure();
  Serial.print("Wait...");
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client_, URL_fw_Bin);
  switch (ret)
  {
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
void up()
{
  String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN +
                       "&V5=" + Irms0 +
                       "&V6=" + Irms1 +
                       "&V7=" + Irms2 +
                       "&V13=" + hz +
                       "&V14=" + pre +
                       "&V15=" + smoothDistance +
                       "&V16=" + volume +
                       "&V18=" + temp_vfd +
                       "&V26=" + in_volt +
                       "&V27=" + out_volt +
                       "&V28=" + speed_motor +
                       "&V29=" + power +
                       "&V30=" + I_vfd;
  //+ "&V21=" + temp[0]
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}
void up_timerun_motor()
{
  String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN + "&V41=" + float(data.timerun_B1) / 1000 / 60 / 60 + "&V43=" + float(data.timerun_B2) / 1000 / 60 / 60;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}
void time_run_motor()
{
  if (blynk_first_connect)
  {
    if (data.reset_day != day())
    {
      if (Blynk.connected())
      {
        up_timerun_motor();
        data.timerun_B1 = 0;
        data.timerun_B2 = 0;
        data.reset_day = day();
        savedata();
      }
    }
  }
  if (B1_save || B2_save)
  {
    if (B1_start != 0)
    {
      data.timerun_B1 = data.timerun_B1 + (millis() - B1_start);
      B1_start = millis();
      B1_save = false;
    }
    if (B2_start != 0)
    {
      data.timerun_B2 = data.timerun_B2 + (millis() - B2_start);
      B2_start = millis();
      B2_save = false;
    }
    savedata();
  }
}
//----------------------------------------------------------------
void on_vfd()
{                                     // 0x3001
  mb.writeHreg(1, 12289, 1, cbWrite); // 0x01: FWD run
  while (mb.slave())
  {
    mb.task();
    delay(10);
  }
  mb.writeHreg(1, 12289, 1, cbWrite); // 0x01: FWD run
  while (mb.slave())
  {
    mb.task();
    delay(10);
  }
  mb.writeHreg(1, 12289, 1, cbWrite); // 0x01: FWD run
  while (mb.slave())
  {
    mb.task();
    delay(10);
  }
}
void off_vfd()
{
  mb.writeHreg(1, 12289, 5, cbWrite); // 0x05: Stop command
  while (mb.slave())
  {
    mb.task();
    delay(10);
  }
  mb.writeHreg(1, 12289, 5, cbWrite); // 0x05: Stop command
  while (mb.slave())
  {
    mb.task();
    delay(10);
  }
  mb.writeHreg(1, 12289, 5, cbWrite); // 0x05: Stop command
  while (mb.slave())
  {
    mb.task();
    delay(10);
  }
}
void on_b1()
{ // NC
  if (trip0 == false)
  {
    status_b1 = HIGH;
    pcf8575_1.digitalWrite(pin_B1, !status_b1);
    Blynk.virtualWrite(V0, status_b1);
    savedata();
  }
}
void off_b1()
{
  status_b1 = LOW;
  pcf8575_1.digitalWrite(pin_B1, !status_b1);
  Blynk.virtualWrite(V0, status_b1);
  savedata();
}
void on_b2()
{ // NO
  if (trip1 == false)
  {
    status_b2 = HIGH;
    pcf8575_1.digitalWrite(pin_B2, !status_b2);
    Blynk.virtualWrite(V1, status_b2);
    savedata();
  }
}
void off_b2()
{
  status_b2 = LOW;
  pcf8575_1.digitalWrite(pin_B2, !status_b2);
  Blynk.virtualWrite(V1, status_b2);
  savedata();
}
void on_b3()
{ // NO
  if (trip2 == false)
  {
    status_b3 = HIGH;
    pcf8575_1.digitalWrite(pin_B3, !status_b3);
    Blynk.virtualWrite(V2, status_b3);
    savedata();
  }
}
void off_b3()
{
  status_b3 = LOW;
  pcf8575_1.digitalWrite(pin_B3, !status_b3);
  Blynk.virtualWrite(V2, status_b3);
  savedata();
}
void on_fan()
{ // NC
  status_fan = HIGH;
  pcf8575_1.digitalWrite(pin_Fan, !status_fan);
}
void off_fan()
{
  status_fan = LOW;
  pcf8575_1.digitalWrite(pin_Fan, !status_fan);
}
void rst_module()
{
  pcf8575_1.digitalWrite(pin_rst, LOW);
}
void hidden()
{
  Blynk.setProperty(V32, V33, V8, V4, V9, V25, V10, V11, "isHidden", true);
  /*
  Blynk.setProperty(V4, "isHidden", true);
  Blynk.setProperty(V9, "isHidden", true);
  Blynk.setProperty(V25, "isHidden", true);
  Blynk.setProperty(V10, "isHidden", true);
  Blynk.setProperty(V11, "isHidden", true);
  */
}
void visible()
{
  Blynk.setProperty(V32, V33, V8, V4, V9, V25, V10, V11, "isHidden", false);
}
//-------------------------------------------------------------------
void MeasureCmForSmoothing() // C1
{
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float sensorValue = analogRead(A0);
  distance = (((sensorValue - zeropointTank) * 500) / (fullpointTank - zeropointTank));
  // Serial.print("sensorValue ");
  // Serial.println(distance);
  if (distance > 0)
  {
    smoothDistance = digitalSmooth(distance, sensSmoothArray1);
    volume = (dai * smoothDistance * rong) / 1000000;
    // Serial.print("\nsmoothDistance ");
    // Serial.println(smoothDistance);
  }
  // Blynk.virtualWrite(V15, smoothDistance);
  // Blynk.virtualWrite(V16, volume);
}
void temperature()
{ // Nhiệt độ
  sensors.requestTemperatures();
  for (byte i = 0; i < sensors.getDeviceCount(); i++)
  {
    temp[i] = sensors.getTempCByIndex(i);
    if (temp[i] < 0)
      temp[i] = 0;
    else
    {
      if (temp[i] >= 37)
        on_fan();
      else if (temp[i] <= 35)
        off_fan();
    }
  }
}
//-------------------------------------------------------------------
void readcurrent() // C2 - Bơm 1   - I0
{
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms0 = emon0.calcIrms(740);
  if (rms0 < 2)
  {
    Irms0 = 0;
    yIrms0 = 0;
    if (B1_start != 0)
    {
      data.timerun_B1 = data.timerun_B1 + (millis() - B1_start);
      savedata();
      B1_start = 0;
    }
  }
  else if (rms0 >= 2)
  {
    yIrms0 = yIrms0 + 1;
    Irms0 = rms0;
    if (yIrms0 > 3)
    {
      if (B1_start >= 0)
      {
        if (B1_start == 0)
          B1_start = millis();
        else if (millis() - B1_start > 60000)
        {
          B1_save = true;
        }
        else
          B1_save = false;
      }
      if ((Irms0 >= data.SetAmpemax) || (Irms0 <= data.SetAmpemin))
      {
        xSetAmpe = xSetAmpe + 1;
        if ((xSetAmpe > 3) && (data.keyp))
        {
          off_b1();
          xSetAmpe = 0;
          trip0 = true;
          Blynk.logEvent("error", String("Bơm 1 lỗi: ") + Irms0 + String(" A"));
        }
      }
      else
        xSetAmpe = 0;
    }
  }
}
void readcurrent1() // C3 - Bơm 2   - I1
{
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms1 = emon1.calcIrms(740);
  if (rms1 < 2)
  {
    Irms1 = 0;
    yIrms1 = 0;
    if (B2_start != 0)
    {
      data.timerun_B2 = data.timerun_B2 + (millis() - B2_start);
      savedata();
      B2_start = 0;
    }
  }
  else if (rms1 >= 2)
  {
    Irms1 = rms1;
    yIrms1 = yIrms1 + 1;
    if (yIrms1 > 3)
    {
      if (B2_start >= 0)
      {
        if (B2_start == 0)
          B2_start = millis();
        else if (millis() - B2_start > 60000)
        {
          B2_save = true;
        }
        else
          B2_save = false;
      }
      if ((Irms1 >= data.SetAmpe1max) || (Irms1 <= data.SetAmpe1min))
      {
        xSetAmpe1 = xSetAmpe1 + 1;
        if ((xSetAmpe1 > 3) & (data.keyp))
        {
          off_b2();
          xSetAmpe1 = 0;
          trip1 = true;
          Blynk.logEvent("error", String("Bơm 2 lỗi: ") + Irms1 + String(" A"));
        }
      }
      else
        xSetAmpe1 = 0;
    }
  }
}
void readcurrent2() // C4 - Bơm 3   - I2
{
  // Blynk.run();
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, HIGH);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms2 = emon2.calcIrms(740);
  if (rms2 < 2)
  {
    Irms2 = 0;
    yIrms2 = 0;
    if (B3_start != 0)
    {
      data.timerun_B3 = data.timerun_B3 + (millis() - B3_start);
      savedata();
      B3_start = 0;
    }
  }
  else if (rms2 >= 2)
  {
    Irms2 = rms2;
    yIrms2 = yIrms2 + 1;
    if (yIrms2 > 3)
    {
      if (B3_start >= 0)
      {
        if (B3_start == 0)
          B3_start = millis();
        else if (millis() - B3_start > 60000)
        {
          B3_save = true;
        }
        else
          B3_save = false;
      }
      if ((Irms2 >= data.SetAmpe2max) || (Irms2 <= data.SetAmpe2min))
      {
        xSetAmpe2 = xSetAmpe2 + 1;
        if ((xSetAmpe2 > 3) & (data.keyp))
        {
          off_b3();
          xSetAmpe2 = 0;
          trip2 = true;
          Blynk.logEvent("error", String("Bơm 3 lỗi: ") + Irms2 + String(" A"));
        }
      }
      else
        xSetAmpe2 = 0;
    }
  }
}
/*
void readcurrent3()  // C5 - NenKhi  - I3
{
  //Blynk.run();
  digitalWrite(S0pin, HIGH);
  digitalWrite(S1pin, LOW);
  digitalWrite(S2pin, HIGH);
  digitalWrite(S3pin, LOW);
  float rms3 = emon3.calcIrms(740);
  if (rms3 < 2) {
    Irms3 = 0;
    yIrms3 = 0;
    if (B3_start != 0) {
      data.timerun_B3 = data.timerun_B3 + (millis() - B3_start);
      savedata();
      B3_start = 0;
    }
  } else if (rms3 >= 2) {
    Irms3 = rms3;
    yIrms3 = yIrms3 + 1;
    if (yIrms3 > 3) {
      if (B3_start >= 0) {
        if (B3_start == 0) B3_start = millis();
        else if (millis() - B3_start > 60000) {
          B3_save = true;
        } else B3_save = false;
      }
      if ((Irms3 >= data.SetAmpe3max) || (Irms3 <= data.SetAmpe3min)) {
        xSetAmpe3 = xSetAmpe3 + 1;
        if ((xSetAmpe3 > 3) & (data.keyp)) {
          off_b3();
          xSetAmpe3 = 0;
          trip3 = true;
          Blynk.logEvent("error", String("Bơm 3 lỗi: ") + Irms3 + String(" A"));
        }
      } else xSetAmpe3 = 0;
    }
  }
}
*/
//-------------------------------------------------------------------
void rtctime()
{
  DateTime now = rtc_module.now();
  if (blynk_first_connect == true)
  {
    if ((now.day() != day()) || (now.hour() != hour()) || ((now.minute() - minute() > 2) || (minute() - now.minute() > 2)))
    {
      rtc_module.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
      now = rtc_module.now();
    }
  }
  Blynk.virtualWrite(V20, daysOfTheWeek[now.dayOfTheWeek()], ", ", now.day(), "/", now.month(), "/", now.year(), " - ", now.hour(), ":", now.minute(), ":", now.second());

  int nowtime = (now.hour() * 3600 + now.minute() * 60);
  if (data.mode_cap2 == 1)
  { // Chạy Tu Dong
    struct TimeInterval
    {
      int start;
      int stop;
    };
    TimeInterval timer_cap2[] = {
        {data.t1_start, data.t1_stop},
        {data.t2_start, data.t2_stop},
        {data.t3_start, data.t3_stop}};

    for (const auto &interval : timer_cap2)
    {
      if ((interval.start < interval.stop && (nowtime < interval.start || nowtime > interval.stop)) ||
          (interval.start > interval.stop && nowtime < interval.start && nowtime > interval.stop))
      {
        time_run = false;
        break; // Dừng kiểm tra nếu đã thỏa mãn điều kiện
      }
    }
    for (const auto &interval : timer_cap2)
    {
      if ((interval.start < interval.stop && nowtime > interval.start && nowtime < interval.stop) ||
          (interval.start > interval.stop && (nowtime > interval.start || nowtime < interval.stop)))
      {
        time_run = true;
        break; // Dừng kiểm tra nếu đã thỏa mãn điều kiện
      }
    }

    if (time_run == true)
    {
      if (pre > 0)
      {
        if (pre < data.pre_set)
        {
          if (status_b1 != HIGH)
          {
            on_b1();
          }
          if (hz == 0)
          {
            on_vfd();
          }
          if (pre >= data.pre_min)
          {
            j = 0;
          }
          else if ((pre < data.pre_min) && (status_b2 != HIGH) && (trip1 == false))
          {
            j++;
            if (j > 20) // 15s = 1 - 300s = 20 - 5p
            {
              if (hz > 40)
              {
                off_vfd();
              }
              timer1.setTimeout(8000, []()
                                {
                 if (hz < 15) {
                   on_b2();
                   on_vfd();
                 } });
              j = 0;
            }
          }
        }
        if ((pre > data.pre_min) && (hz < 30) && (status_b1 == HIGH) && (status_b2 == HIGH))
        {
          i++;
          if (i > 20)
          {
            off_vfd();
            timer1.setTimeout(4000, []()
                              {
               if (hz < 15) {
                 off_b2();
                 on_vfd();
               } });
            i = 0;
          }
        }
        else if (hz >= 30)
        {
          i = 0;
        }
      }
    }
    else
    {
      if (hz > 0)
        off_vfd();
      else if ((hz == 0) && ((status_b1 != LOW) || (status_b2 != LOW)))
      {
        if (status_b1 != LOW)
        {
          off_b1();
        }
        if (status_b2 != LOW)
        {
          off_b2();
        }
      }
    }
  }
}
//-------------------------------------------------------------------
BLYNK_WRITE(V0) // Bom 1
{
  if ((key) && (!trip0))
  {
    if (param.asInt() == LOW)
    {
      off_b1();
    }
    else
    {
      on_b1();
    }
  }
  else
    Blynk.virtualWrite(V0, status_b1);
}
BLYNK_WRITE(V1) // Bơm 2
{
  if ((key) && (!trip1))
  {
    if (param.asInt() == LOW)
    {
      off_b2();
    }
    else
    {
      on_b2();
    }
  }
  else
    Blynk.virtualWrite(V1, status_b2);
}
BLYNK_WRITE(V2) // Bơm 3
{
  if ((key) && (!trip2))
  {
    if (param.asInt() == LOW)
    {
      off_b3();
    }
    else
    {
      on_b3();
    }
  }
  else
    Blynk.virtualWrite(V2, status_b3);
}
BLYNK_WRITE(V3) // Chọn chế độ Cấp 2
{
  if (key)
  {
    switch (param.asInt())
    {
    case 0:
    { // Man
      data.mode_cap2 = 0;
      break;
    }
    case 1:
    { // Auto
      data.mode_cap2 = 1;
      break;
    }
    }
  }
  else
    Blynk.virtualWrite(V3, data.mode_cap2);
}
BLYNK_WRITE(V4) // Notification
{
  if (key)
  {
    if (param.asInt() == LOW)
      data.keynoti = false;
    else
      data.keynoti = true;
    savedata();
  }
  else
    Blynk.virtualWrite(V4, data.keynoti);
}
/*
BLYNK_WRITE(V3)  // Nen Khi
{
  if ((key) && (!trip3)) {
    if (param.asInt() == LOW) {
      off_nenkhi();
    } else {
      on_nenkhi();
    }
  } else Blynk.virtualWrite(V3, data.status_nenkhi);
}
*/
BLYNK_WRITE(V8) // Bảo vệ
{
  if (key)
  {
    if (param.asInt() == LOW)
      data.keyp = false;
    else
      data.keyp = true;
    savedata();
  }
  else
    Blynk.virtualWrite(V8, data.keyp);
}
BLYNK_WRITE(V9) // Chon máy cài đặt bảo vệ
{
  switch (param.asInt())
  {
  case 0:
  { // ....
    c = 0;
    Blynk.virtualWrite(V10, 0);
    Blynk.virtualWrite(V11, 0);
    break;
  }
  case 1:
  { // Bom 1
    c = 1;
    Blynk.virtualWrite(V10, data.SetAmpemin);
    Blynk.virtualWrite(V11, data.SetAmpemax);
    break;
  }
  case 2:
  { // Bom 2
    c = 2;
    Blynk.virtualWrite(V10, data.SetAmpe1min);
    Blynk.virtualWrite(V11, data.SetAmpe1max);
    break;
  }
  case 3:
  { // Bom 3
    c = 3;
    Blynk.virtualWrite(V10, data.SetAmpe2min);
    Blynk.virtualWrite(V11, data.SetAmpe2max);
    break;
  }
  case 4:
  { // Nen Khi
    c = 4;
    Blynk.virtualWrite(V10, data.SetAmpe3min);
    Blynk.virtualWrite(V11, data.SetAmpe3max);
    break;
  }
  }
}
BLYNK_WRITE(V10) // min
{
  if (key)
  {
    if (c == 1)
    {
      data.SetAmpemin = param.asFloat();
    }
    else if (c == 2)
    {
      data.SetAmpe1min = param.asFloat();
    }
    else if (c == 3)
    {
      data.SetAmpe2min = param.asFloat();
    }
    else if (c == 4)
    {
      data.SetAmpe3min = param.asFloat();
    }
  }
  else
    Blynk.virtualWrite(V10, 0);
}
BLYNK_WRITE(V11) // max
{
  if (key)
  {
    if (c == 1)
    {
      data.SetAmpemax = param.asFloat();
    }
    else if (c == 2)
    {
      data.SetAmpe1max = param.asFloat();
    }
    else if (c == 3)
    {
      data.SetAmpe2max = param.asFloat();
    }
    else if (c == 4)
    {
      data.SetAmpe3max = param.asFloat();
    }
  }
  else
    Blynk.virtualWrite(V11, 0);
}
BLYNK_WRITE(V12) // String
{
  String dataS = param.asStr();
  if (dataS == "M")
  {
    terminal.clear();
    key = true;
    Blynk.virtualWrite(V12, "N.viên vận hành: 'Quang'\nKích hoạt trong 10s\n");
    timer1.setTimeout(10000, []()
                      {
      key = false;
      terminal.clear(); });
  }
  else if (dataS == "active")
  {
    terminal.clear();
    key = true;
    visible();
    Blynk.virtualWrite(V12, "KHÔNG sử dụng phần mềm cho đến khi thông báo này mất.\n");
  }
  else if (dataS == "deactive")
  {
    terminal.clear();
    key = false;
    hidden();
    Blynk.virtualWrite(V12, "Ok!\nNhập mã để điều khiển!\n");
  }
  else if (dataS == "save")
  {
    terminal.clear();
    savedata();
    Blynk.virtualWrite(V12, "Đã lưu cài đặt.\n");
  }
  else if (dataS == "reset")
  {
    terminal.clear();
    trip0 = false;
    trip1 = false;
    trip2 = false;
    trip3 = false;
    Blynk.virtualWrite(V12, "Đã RESET! \nNhập mã để điều khiển!\n");
  }
  else if (dataS == "rst")
  {
    terminal.clear();
    Blynk.virtualWrite(V12, "ESP Khởi động lại sau 3s");
    delay(3000);
    // ESP.restart();
    rst_module();
  }
  else if (dataS == "update")
  {
    terminal.clear();
    Blynk.virtualWrite(V12, "ESP UPDATE...");
    update_fw();
  }
  else
  {
    Blynk.virtualWrite(V12, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
BLYNK_WRITE(V17) // Cai ap luc bien tan
{
  if (key)
  {
    data.pre_set = param.asFloat();
    savedata();
  }
  else
    Blynk.virtualWrite(V17, data.pre_set);
}
BLYNK_WRITE(V24) // Info
{
  if (param.asInt() == 1)
  {
    terminal.clear();
    if (data.mode_cap2 == 0)
    {
      Blynk.virtualWrite(V11, "Mode: MAN");
    }
    else if (data.mode_cap2 == 1)
    {
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
      Blynk.virtualWrite(V12, "Mode: AUTO\n- Lần 1: ", t1_start_h, ":", t1_start_m, " -> ", t1_stop_h, ":", t1_stop_m, "\n- Lần 2: ", t2_start_h, ":", t2_start_m, " -> ", t2_stop_h, ":", t2_stop_m, "\n- Lần 3: ", t3_start_h, ":", t3_start_m, " -> ", t3_stop_h, ":", t3_stop_m);
      Blynk.virtualWrite(V12, "\nMức áp cài đặt:   ", data.pre_set, "\nMức áp tối thiểu:", data.pre_min);
    }
  }
  else
  {
    terminal.clear();
  }
  timer.restartTimer(timer_I);
}
BLYNK_WRITE(V31) // Cai áp lực min
{
  if (key)
  {
    data.pre_min = param.asFloat();
    savedata();
  }
  else
    Blynk.virtualWrite(V31, data.pre_min);
}
BLYNK_WRITE(V32) // Chọn thời gian chạy Bơm
{
  switch (param.asInt())
  {
  case 0:
  { // ...
    b = 10;
    Blynk.virtualWrite(V33, 0, 0, tz);
    break;
  }
  case 1:
  { // Lần 1
    if (key)
      b = 0;
    Blynk.virtualWrite(V33, data.t1_start, data.t1_stop, tz);
    break;
  }
  case 2:
  { // Lần 2
    if (key)
      b = 1;
    Blynk.virtualWrite(V33, data.t2_start, data.t2_stop, tz);
    break;
  }
  case 3:
  { // Lần 3
    if (key)
      b = 2;
    Blynk.virtualWrite(V33, data.t3_start, data.t3_stop, tz);
    break;
  }
  }
}
BLYNK_WRITE(V33) // Time input
{
  if (key)
  {
    TimeInputParam t(param);
    if (t.hasStartTime())
    {
      if (b == 0)
      {
        data.t1_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 1)
      {
        data.t2_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
      if (b == 2)
      {
        data.t3_start = t.getStartHour() * 3600 + t.getStartMinute() * 60;
      }
    }
    if (t.hasStopTime())
    {
      if (b == 0)
      {
        data.t1_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 1)
      {
        data.t2_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
      if (b == 2)
      {
        data.t3_stop = t.getStopHour() * 3600 + t.getStopMinute() * 60;
      }
    }
  }
  else
    Blynk.virtualWrite(V33, 0);
}
//-------------------------------------------------------------------
void read_modbus()
{
  { // All
    mb.readHreg(1, 8449, data_vfd, 12, cbWrite_data_vfd);
    while (mb.slave())
    {
      mb.task();
      delay(20);
    }
  }
  // Áp lực set
  {
    mb.readHreg(1, 12296, ref_percent_, 1, cbWrite_aplucset);
    while (mb.slave())
    { // Check if transaction is active
      mb.task();
      delay(20);
    }
    float ref_bar = ((ref_percent - zero_pre) * max_pre_bar) / (1000 - zero_pre); // Áp lực tham chiếu dạng bar
    float delta_pre = f2dec(ref_bar) - f2dec(data.pre_set);
    if (f2dec(delta_pre != 0))
    {
      if (-0.01 <= f2dec(delta_pre) && f2dec(delta_pre) <= 0.01)
      {
        int send_ref = int(((data.pre_set * (1000 - zero_pre)) + (zero_pre * max_pre_bar)) / max_pre_bar) + x;
        mb.writeHreg(1, 12296, send_ref, cbWrite);
        while (mb.slave())
        { // Check if transaction is active
          mb.task();
          delay(20);
        }
        if (x < 4)
        {
          x++;
        }
        else
        {
          x = 0;
          data.pre_set = f2dec(ref_bar);
          savedata();
          Blynk.virtualWrite(V17, data.pre_set);
        }
      }
      else
      {
        int send_ref = ((data.pre_set * (1000 - zero_pre)) + (zero_pre * max_pre_bar)) / max_pre_bar;
        mb.writeHreg(1, 12296, send_ref, cbWrite);
        while (mb.slave())
        { // Check if transaction is active
          mb.task();
          delay(20);
        }
      }
    }
    else
      x = 0;
  }
}

void setup()
{
  //-----------------------
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  //-----------------------
  delay(10000);
  Wire.begin();
  sensors.begin();
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

  pcf8575_1.pinMode(S0pin, OUTPUT);
  pcf8575_1.pinMode(S1pin, OUTPUT);
  pcf8575_1.pinMode(S2pin, OUTPUT);
  pcf8575_1.pinMode(S3pin, OUTPUT);
  pcf8575_1.pinMode(pin_B1, OUTPUT);
  pcf8575_1.digitalWrite(pin_B1, HIGH);
  pcf8575_1.pinMode(pin_B2, OUTPUT);
  pcf8575_1.digitalWrite(pin_B2, HIGH);
  pcf8575_1.pinMode(pin_B3, OUTPUT);
  pcf8575_1.digitalWrite(pin_B3, HIGH);
  pcf8575_1.pinMode(pin_khuay_clo, OUTPUT);
  pcf8575_1.digitalWrite(pin_khuay_clo, HIGH);
  pcf8575_1.pinMode(pin_cham_clo, OUTPUT);
  pcf8575_1.digitalWrite(pin_cham_clo, HIGH);
  pcf8575_1.pinMode(pin_Fan, OUTPUT);
  pcf8575_1.digitalWrite(pin_Fan, HIGH);
  pcf8575_1.pinMode(pin_rst, OUTPUT);
  pcf8575_1.digitalWrite(pin_rst, HIGH);

  timer1.setTimeout(5000L, []()
                    {
    timer.setInterval(230L, MeasureCmForSmoothing);
    timer_I = timer.setInterval(1589, []() {
      readcurrent();
      readcurrent1();
      readcurrent2();
      temperature();
      read_modbus();
      up();
    });
    timer.setInterval(15005L, []() {
      rtctime();
      timer.restartTimer(timer_I);
    });
    timer.setInterval(900005L, []() {
      connectionstatus();
      timer.restartTimer(timer_I);
    }); });
}
void loop()
{
  Blynk.run();
  timer.run();
  timer1.run();
}