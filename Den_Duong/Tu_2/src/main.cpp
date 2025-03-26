#define BLYNK_TEMPLATE_ID "TMPLdGfzkVvi"
#define BLYNK_TEMPLATE_NAME "Đèn đường"
#define BLYNK_AUTH_TOKEN "f_sYprdrQ685jdaLTx4Pt_F13-4Ck9ru"
#define BLYNK_FIRMWARE_VERSION "250323"

#define Main_TOKEN "Ol3VH8Hv_OX2JKUWl4ENBk6Rqgh3P3MQ"
const char *ssid = "KTX A2";
const char *password = "kytucxaa";
// const char *ssid = "tram bom so 4";
// const char *password = "0943950555";
//-------------------------------------------------------------------
#include "EmonLib.h"
#include "PCF8575.h"
#include "RTClib.h"
#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <DallasTemperature.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <Eeprom24C32_64.h>
#include <OneWire.h>
#include <SPI.h>
#include <UrlEncode.h>
#include <WiFiClientSecure.h>
#include <WidgetRTC.h>
#include <Wire.h>
//-------------------
#define BLYNK_PRINT Serial
#define APP_DEBUG
#define EEPROM_ADDRESS 0x57
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/Den_Duong/Tu_2/.pio/build/nodemcuv2/firmware.bin"
String server_name = "http://sgp1.blynk.cloud/external/api/";
//-----------------------------
#define pin_dataS "&V11="
#define pin_mode "&V13="
#define pin_G "&V14="
#define pin_Irms "&V15="
String location = urlEncode(" Tủ 2\n");
//-----------------------------
EnergyMonitor emon0;
PCF8575 pcf8575_1(0x20);
RTC_DS3231 rtc_module;
OneWire oneWire(D3);
DallasTemperature sensors(&oneWire);
WiFiClient client;
HTTPClient http;
//-----------------------------
const int S0 = P15;
const int S1 = P14;
const int S2 = P13;
const int S3 = P12;
const int pin_RL1 = P1; // fan
const int pin_RL2 = P2; // reset board
const int pin_RL3 = P3; // Khoi Dong Tu
const int pin_RL4 = P4;
const int pin_RL5 = P5;
const int pin_RL6 = P6;
const int pin_RL7 = P7;
const word address = 0;

char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";
char s_day[50] = "";
char B[50] = "";
float temp;
float Irms0, prev_Irms0 = 0;
unsigned long int xIrms0 = 0;
unsigned long int yIrms0 = 0;
byte reboot_num, prev_mode = 0;
String num_KDT = "KDT1";
String s_timer_van_1, s_temp;
String s_weekday;
bool key = false, blynk_first_connect = false, dayOfTheWeek_ = false;
bool sta_rl1 = LOW, sta_rl3 = LOW, prev_sta_rl3 = LOW;
bool trip0 = false;
int hour_start_rl3 = 0, minute_start_rl3 = 0, hour_stop_rl3 = 0, minute_stop_rl3 = 0;
int timer_I;
int dayadjustment = -1;
int xSetAmpe = 0;
//-----------------------------
struct Data {
  byte mode;
  byte reboot_num;
  byte save_num;
  uint32_t rl3_r, rl3_s;
  byte MonWeekDay, TuesWeekDay, WedWeekDay, ThuWeekDay, FriWeekDay, SatWeekend, SunWeekend;
  byte SetAmpemax, SetAmpemin;
} data, dataCheck;
const struct Data dataDefault = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//-----------------------------
WidgetTerminal DATAS(V0);
WidgetRTC rtc_widget;
//-------------------------------------------------------------------
BlynkTimer timer;
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V1);
  rtc_widget.begin();
  blynk_first_connect = true;
}
//-------------------------------------------------------------------
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
void savedata() {
  if (memcmp(&data, &dataCheck, sizeof(dataDefault)) == 0) {
    // Serial.println("structures same no need to write to EEPROM");
  } else {
    // Serial.println("\nWrite bytes to EEPROM memory...");
    data.save_num = data.save_num + 1;
    eeprom.writeBytes(address, sizeof(dataDefault), (byte *)&data);
    // Blynk.setProperty(V0, "label", BLYNK_FIRMWARE_VERSION, "-EEPROM ", data.save_num);
  }
}
//-----------------------------
void weekday_() {
  //---------------------Day
  int A[7] = {data.MonWeekDay, data.TuesWeekDay, data.WedWeekDay, data.ThuWeekDay, data.FriWeekDay, data.SatWeekend, data.SunWeekend};
  memset(s_day, '\0', sizeof(s_day));
  strcat(s_day, "DAY: ");
  memset(B, '\0', sizeof(B));
  for (int i = 0; i < 7; i++) {
    // Nếu ngày i được chọn
    if (A[i] == 1) {
      // Thêm giá trị i vào mảng A
      strcat(B, String(i + 1).c_str());
      strcat(B, ",");
      if (i == 6) {
        strcat(s_day, "CN");
        strcat(s_day, ",");
      } else {
        strcat(s_day, "T");
        strcat(s_day, String(i + 2).c_str());
        strcat(s_day, ",");
      }
    }
  }
  B[strlen(B) - 1] = '\0'; // Xóa ký tự cuối cùng là dấu phẩy
  s_day[strlen(s_day) - 1] = '\0';
  strcat(s_day, "\n"); // Xuống dòng cuối câu
  s_weekday = urlEncode(s_day);
  //---------------------Time RL 1
  if ((hour_start_rl3 == 0) && (minute_start_rl3 == 0) && (hour_stop_rl3 == 0) && (minute_stop_rl3 == 0)) {
    hour_start_rl3 = data.rl3_r / 3600;
    minute_start_rl3 = (data.rl3_r - (hour_start_rl3 * 3600)) / 60;
    hour_stop_rl3 = data.rl3_s / 3600;
    minute_stop_rl3 = (data.rl3_s - (hour_stop_rl3 * 3600)) / 60;
  }
  char s_timer_van_1_[30]; // Tạo một mảng ký tự để lưu trữ chuỗi định dạng
  sprintf(s_timer_van_1_, "KĐT 1: %02d:%02d - %02d:%02d\n", hour_start_rl3, minute_start_rl3, hour_stop_rl3, minute_stop_rl3);
  s_timer_van_1 = urlEncode(s_timer_van_1_);
}
void print_terminal() {
  String s_ampe = "Ampe: " + String(data.SetAmpemin) + "A - " + String(data.SetAmpemax) + "A\n";

  String server_path = server_name + "batch/update?token=" + Main_TOKEN + pin_dataS + location + pin_dataS + s_weekday + pin_dataS + s_timer_van_1 + pin_dataS + urlEncode(s_ampe) + pin_dataS + urlEncode(s_temp);
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
  // Serial.println(server_path);
}
void print_terminal_main() {
  String server_path = server_name + "batch/update?token=" + Main_TOKEN + "&V0=" + location + "&V0=" + s_weekday + "&V0=" + s_timer_van_1;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}

void check_and_update() {
  if (data.mode != prev_mode || sta_rl3 != prev_sta_rl3 || abs(Irms0 - prev_Irms0) >= 0.1) {
    // Có sự thay đổi, thực hiện gửi dữ liệu
    byte g;
    bitWrite(g, 0, data.mode);
    bitWrite(g, 1, sta_rl3);
    String server_path = server_name + "batch/update?token=" + Main_TOKEN + pin_G + g + pin_Irms + Irms0;
    http.begin(client, server_path.c_str());
    http.GET();
    http.end();

    // Cập nhật giá trị trước đó
    prev_mode = data.mode;
    prev_sta_rl3 = sta_rl3;
    prev_Irms0 = Irms0;
  }
}
//-------------------------
void on_KDT1() {
  sta_rl3 = HIGH;
  pcf8575_1.digitalWrite(pin_RL3, !sta_rl3);
}
void off_KDT1() {
  sta_rl3 = LOW;
  pcf8575_1.digitalWrite(pin_RL3, !sta_rl3);
}
void on_fan() {
  sta_rl1 = HIGH;
  pcf8575_1.digitalWrite(pin_RL1, !sta_rl1);
}
void off_fan() {
  sta_rl1 = LOW;
  pcf8575_1.digitalWrite(pin_RL1, !sta_rl1);
}
void reset_board() {
  pcf8575_1.digitalWrite(pin_RL2, LOW);
}

BLYNK_WRITE(V0) {
  String dataS = param.asStr();
  if (dataS == "update") {
    update_fw();
  } else if (dataS == "rst") {
    ESP.restart();
  } else if (dataS == "m") { // man
    data.mode = 0;
    savedata();
  } else if (dataS == "a") { // auto
    data.mode = 1;
    savedata();
  } else if (dataS == "info") { // mode?
    print_terminal();
  } else if (dataS == "KDT1") { // mode?
    num_KDT = "KDT1";
    print_terminal_main();
  } else if (dataS == "KDT1_on") { // RL1 on
    if (data.mode == 0)
      on_KDT1();
  } else if (dataS == "KDT1_off") { // RL1 off
    if (data.mode == 0)
      off_KDT1();
  } else if (dataS.substring(0, 3) == "max") {
    String numStr = dataS.substring(3);
    byte a = numStr.toInt();
    data.SetAmpemax = a;
    savedata();
  } else if (dataS.substring(0, 3) == "min") {
    String numStr = dataS.substring(3);
    byte a = numStr.toInt();
    data.SetAmpemin = a;
    savedata();
  }
}
BLYNK_WRITE(V1) {
  TimeInputParam t(param);
  //-------------------------
  data.MonWeekDay = t.isWeekdaySelected(1);
  data.TuesWeekDay = t.isWeekdaySelected(2);
  data.WedWeekDay = t.isWeekdaySelected(3);
  data.ThuWeekDay = t.isWeekdaySelected(4);
  data.FriWeekDay = t.isWeekdaySelected(5);
  data.SatWeekend = t.isWeekdaySelected(6);
  data.SunWeekend = t.isWeekdaySelected(7);
  if (memcmp(&data, &dataCheck, sizeof(dataDefault)) != 0) {
    dataCheck.MonWeekDay = data.MonWeekDay;
    dataCheck.TuesWeekDay = data.TuesWeekDay;
    dataCheck.WedWeekDay = data.WedWeekDay;
    dataCheck.ThuWeekDay = data.ThuWeekDay;
    dataCheck.FriWeekDay = data.FriWeekDay;
    dataCheck.SatWeekend = data.SatWeekend;
    dataCheck.SunWeekend = data.SunWeekend;
  }
  //-------------------------
  if (num_KDT == "KDT1") {
    if (t.hasStartTime()) {
      hour_start_rl3 = t.getStartHour();
      minute_start_rl3 = t.getStartMinute();
      data.rl3_r = hour_start_rl3 * 3600 + minute_start_rl3 * 60;
    }
    if (t.hasStopTime()) {
      hour_stop_rl3 = t.getStopHour();
      minute_stop_rl3 = t.getStopMinute();
      data.rl3_s = hour_stop_rl3 * 3600 + minute_stop_rl3 * 60;
    }
  }
  //-------------------------
  savedata();
  weekday_();
  print_terminal_main();
}
//-------------------------
void readcurrent() // C2
{
  pcf8575_1.digitalWrite(S0, LOW);
  pcf8575_1.digitalWrite(S1, HIGH);
  pcf8575_1.digitalWrite(S2, LOW);
  pcf8575_1.digitalWrite(S3, LOW);
  float rms0 = emon0.calcIrms(1480);
  if (rms0 < 2) {
    Irms0 = 0;
    yIrms0 = 0;
  } else if (rms0 >= 2) {
    Irms0 = rms0;
    yIrms0 = yIrms0 + 1;
    if ((yIrms0 > 3) && ((Irms0 >= data.SetAmpemax) || (Irms0 <= data.SetAmpemin))) {
      xSetAmpe = xSetAmpe + 1;
      if (xSetAmpe >= 4) {
        off_KDT1();
        xSetAmpe = 0;
        trip0 = true;
        String dataS = "Tủ 1 - KĐT1 lỗi! " + String(Irms0) + "A";
        String server_path = server_name + "batch/update?token=" + Main_TOKEN + "&V100=" + urlEncode(dataS);
        http.begin(client, server_path.c_str());
        http.GET();
        http.end();
      }
    }
  }
}
void temperature() { // Nhiệt độ
  sensors.begin();
  sensors.requestTemperatures();
  // Serial.println(sensors.getDeviceCount());
  if (sensors.getDeviceCount() > 0) {
    temp = sensors.getTempCByIndex(0);
    s_temp = "Temp: " + String(temp) + "°C\n";
    if (temp > 37 && sta_rl1 == LOW)
      on_fan();
    else if (temp < 35 && sta_rl1 == HIGH)
      off_fan();
  }
  // Serial.println(temp);
}
//-------------------------
void rtctime() {
  DateTime now = rtc_module.now();
  //-------------------------
  if (blynk_first_connect == true) {
    if ((now.day() != day()) || (now.hour() != hour()) || ((now.minute() - minute() > 2) || (minute() - now.minute() > 2))) {
      rtc_module.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
    }
  }
  float nowtime = (now.hour() * 3600 + now.minute() * 60);

  if (weekday() == 1) {
    dayadjustment = 6; // needed for Sunday, Time library is day 1 and Blynk is day 7
  }
  if ((((weekday() + dayadjustment) == 1) && (data.MonWeekDay)) || (((weekday() + dayadjustment) == 2) && (data.TuesWeekDay)) || (((weekday() + dayadjustment) == 3) && (data.WedWeekDay)) || (((weekday() + dayadjustment) == 4) && (data.ThuWeekDay)) || (((weekday() + dayadjustment) == 5) && (data.FriWeekDay)) || (((weekday() + dayadjustment) == 6) && (data.SatWeekend)) || (((weekday() + dayadjustment) == 7) && (data.SunWeekend))) {
    dayOfTheWeek_ = true;
  } else
    dayOfTheWeek_ = false;
  if (data.mode == 1) { // Auto
    if (dayOfTheWeek_) {
      if (data.rl3_r > data.rl3_s) {
        if ((nowtime > data.rl3_s) && (nowtime < data.rl3_r)) {
          off_KDT1();
        }
        if ((nowtime < data.rl3_s) || (nowtime > data.rl3_r)) {
          if (!trip0)
            on_KDT1();
        }
      }
      if (data.rl3_r < data.rl3_s) {
        if ((nowtime > data.rl3_s) || (nowtime < data.rl3_r)) {
          off_KDT1();
        }
        if ((nowtime < data.rl3_s) && (nowtime > data.rl3_r)) {
          if (!trip0) {
            on_KDT1();
          }
        }
      }
    } else {
      if (sta_rl3 == HIGH) {
        off_KDT1();
      }
    }
  }
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
    }
    if (reboot_num % 5 == 0) {
      reset_board();
    }
  }
  if (Blynk.connected()) {
    if (reboot_num != 0) {
      reboot_num = 0;
    }
  }
}

//-------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(10000);
  //-----------------------
  emon0.current(A0, 105);
  //-----------------------
  rtc_module.begin();
  eeprom.initialize();
  eeprom.readBytes(address, sizeof(dataDefault), (byte *)&data);
  //-----------------------
  Wire.begin();
  sensors.begin();
  pcf8575_1.begin();

  pcf8575_1.pinMode(S0, OUTPUT);
  pcf8575_1.pinMode(S1, OUTPUT);
  pcf8575_1.pinMode(S2, OUTPUT);
  pcf8575_1.pinMode(S3, OUTPUT);
  pcf8575_1.pinMode(pin_RL1, OUTPUT);
  pcf8575_1.digitalWrite(pin_RL1, HIGH);
  pcf8575_1.pinMode(pin_RL2, OUTPUT);
  pcf8575_1.digitalWrite(pin_RL2, HIGH);
  pcf8575_1.pinMode(pin_RL3, OUTPUT);
  pcf8575_1.digitalWrite(pin_RL3, HIGH);
  pcf8575_1.pinMode(pin_RL4, OUTPUT);
  pcf8575_1.digitalWrite(pin_RL4, HIGH);
  pcf8575_1.pinMode(pin_RL5, OUTPUT);
  pcf8575_1.digitalWrite(pin_RL5, HIGH);
  pcf8575_1.pinMode(pin_RL6, OUTPUT);
  pcf8575_1.digitalWrite(pin_RL6, HIGH);
  pcf8575_1.pinMode(pin_RL7, OUTPUT);
  pcf8575_1.digitalWrite(pin_RL7, HIGH);

  timer.setTimeout(5000L, []() {
    weekday_();
    timer_I = timer.setInterval(589, []() {
      readcurrent();
    });
    timer.setInterval(2589, []() {
      check_and_update();
      temperature();
      timer.restartTimer(timer_I);
    });
    timer.setInterval(15005L, []() {
      rtctime();
      timer.restartTimer(timer_I);
    });
    timer.setInterval(120005L, []() {
      connectionstatus();
      timer.restartTimer(timer_I);
    });
  });
}
void loop() {
  Blynk.run();
  timer.run();
}
