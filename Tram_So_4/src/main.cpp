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
 *V32 -
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
#define VOLUME_TOKEN "RyDZuYiRC4oaG5MsFI2kw4WsQpKiw2Ko"

#define BLYNK_FIRMWARE_VERSION "251106"

const char *ssid = "tram bom so 4";
const char *password = "0943950555";
#define APP_DEBUG

#pragma region
#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <SimpleKalmanFilter.h>
#include <UrlEncode.h>
//-------------------
const char *host = "script.google.com";
const int httpsPort = 443;
String LOG_ID = "AKfycby_KPSb6ZSU_koFXzexJBHGEl3ajALoW8ANagHukK-ZZinLy2vuOYXqKVUMhln3IVI-";
WiFiClientSecure client_secure;
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
#include <I2C_eeprom.h>
#include <I2C_eeprom_cyclic_store.h>
#define MEMORY_SIZE 4096
#define PAGE_SIZE 32
I2C_eeprom ee(0x57, MEMORY_SIZE);
//-------------------
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
WiFiClient client;
HTTPClient http;
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/refs/heads/main/Tram_So_4/.pio/build/nodemcuv2/firmware.bin"
String server_name = "http://sgp1.blynk.cloud/external/api/";
//-------------------

long m = 60 * 1000;
bool key = false, keyp = true, keytank = true;
bool timer_updata_status, timer_I_status;
bool time_run1 = false, time_run2 = false;
bool noti_1 = true, noti_2 = true, noti_3 = true, noti_4 = true, noti_5 = true, noti_6 = true;
bool blynk_first_connect = false, pre_raw = false, tank_raw = false;
float Result1, smoothDistance;
uint32_t timestamp;
int a, c, b, f = 0;
int timer_2, timer_1, timer_3, timer_4, timer_5;
int time_cycle, timer_cycle;
int LLG1_1m3, reboot_num;
int time_run_nenkhi = 5 * 60;
int time_stop_nenkhi = 10 * 60;
byte status_b1 = LOW, status_b2 = LOW, status_g1 = LOW;
int G1_start, B1_start, B2_start;
bool G1_save = false, B1_save = false, B2_save = false;
//-------------------
int startup_cycles = 8; // Bỏ qua 5 chu kỳ đọc đầu tiên để cảm biến ổn định
//-------------------
int dai = 800;
int rong = 800;
int dosau = 330;
int volume1;
long t;
//-------------------

struct Data {
  struct Flags {
    uint8_t man : 1;
    uint8_t mode_cap2 : 1;
    uint8_t statusRualoc : 1;
    uint8_t key_noti : 1;
    uint8_t reserved : 4; // Dự phòng dành cho các cờ sau này
  } flags;

  uint8_t SetAmpemax, SetAmpemin;
  uint8_t SetAmpe1max, SetAmpe1min;
  uint8_t SetAmpe2max, SetAmpe2min;
  uint8_t SetAmpe3max, SetAmpe3min;
  uint8_t SetAmpe4max, SetAmpe4min;
  uint16_t b1_1_start, b1_1_stop, b1_2_start, b1_2_stop, b1_3_start, b1_3_stop, b1_4_start, b1_4_stop;
  uint16_t b2_1_start, b2_1_stop, b2_2_start, b2_2_stop, b2_3_start, b2_3_stop, b2_4_start, b2_4_stop;
  uint32_t save_num;
  uint8_t clo;
  uint32_t time_clo;
  int LLG1_RL;
  uint8_t reset_day;
  uint32_t timerun_G1, timerun_B1, timerun_B2;

  // Calibration data
  uint16_t pressure_cal_offset;     // ADC value at 0 bar
  uint16_t pressure_cal_gain_x1000; // Gain * 1000
  uint16_t level_cal_offset;        // ADC value at 0 cm
  uint16_t level_cal_gain_x1000;    // Gain * 1000
} data, dataCheck;
I2C_eeprom_cyclic_store<Data> cs;

struct PrevState {
  float Result1 = -1.0, volume1 = -1.0, smoothDistance = -1.0, Irms0 = -1.0, Irms1 = -1.0, Irms2 = -1.0, Irms3 = -1.0, Irms4 = -1.0;
  float timerun_G1 = -1.0, timerun_B1 = -1.0, timerun_B2 = -1.0;
  unsigned long Result1_ts = 0, volume1_ts = 0, smoothDistance_ts = 0, Irms0_ts = 0, Irms1_ts = 0, Irms2_ts = 0, Irms3_ts = 0, Irms4_ts = 0;
  unsigned long timerun_G1_ts = 0, timerun_B1_ts = 0, timerun_B2_ts = 0;
} prevState;
#pragma endregion

// --- Kalman Filters ---
// Áp suất: Nhạy hơn để phát hiện sự cố nhanh
// e_mea: Sai số phép đo (ADC noise) - để giá trị nhỏ vì ADC của ESP8266 khá ổn định.
// e_est: Sai số ước tính ban đầu - để bằng e_mea.
// q: Nhiễu quá trình - để giá trị nhỏ vì áp suất không nên thay đổi quá nhanh.
SimpleKalmanFilter pressureKalmanFilter(1.0, 1.0, 0.01);

// Mực nước: Mượt hơn để tính toán thể tích
// e_mea: Sai số phép đo - lớn hơn một chút do cảm biến siêu âm có thể bị ảnh hưởng bởi môi trường.
// e_est: Sai số ước tính ban đầu.
// q: Nhiễu quá trình - nhỏ vì mực nước trong bể lớn thay đổi chậm.
SimpleKalmanFilter levelKalmanFilter(2.0, 2.0, 0.01);

// --- Median Filter for Water Level ---
const int MEDIAN_WINDOW_SIZE = 5;
int median_buffer[MEDIAN_WINDOW_SIZE];
int median_buffer_index = 0;
float filtered_adc_pressure, filtered_adc_level;

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
  const unsigned long FORCE_UPDATE_INTERVAL = 45000; // 45 giây
  unsigned long current_millis = millis();
  String params_to_update = "";

  if (Result1 != prevState.Result1 || (current_millis - prevState.Result1_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V14=" + String(Result1);
    prevState.Result1 = Result1;
    prevState.Result1_ts = current_millis;
  }
  if (volume1 != prevState.volume1 || (current_millis - prevState.volume1_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V19=" + String(volume1);
    prevState.volume1 = volume1;
    prevState.volume1_ts = current_millis;
  }
  if (smoothDistance != prevState.smoothDistance || (current_millis - prevState.smoothDistance_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V20=" + String(smoothDistance);
    prevState.smoothDistance = smoothDistance;
    prevState.smoothDistance_ts = current_millis;
  }
  if (Irms0 != prevState.Irms0 || (current_millis - prevState.Irms0_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V27=" + String(Irms0);
    prevState.Irms0 = Irms0;
    prevState.Irms0_ts = current_millis;
  }
  if (Irms1 != prevState.Irms1 || (current_millis - prevState.Irms1_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V26=" + String(Irms1);
    prevState.Irms1 = Irms1;
    prevState.Irms1_ts = current_millis;
  }
  if (Irms2 != prevState.Irms2 || (current_millis - prevState.Irms2_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V24=" + String(Irms2);
    prevState.Irms2 = Irms2;
    prevState.Irms2_ts = current_millis;
  }
  if (Irms3 != prevState.Irms3 || (current_millis - prevState.Irms3_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V30=" + String(Irms3);
    prevState.Irms3 = Irms3;
    prevState.Irms3_ts = current_millis;
  }
  if (Irms4 != prevState.Irms4 || (current_millis - prevState.Irms4_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V25=" + String(Irms4);
    prevState.Irms4 = Irms4;
    prevState.Irms4_ts = current_millis;
  }
  float timerun_G1_h = float(data.timerun_G1) / 1000 / 60 / 60;
  if (timerun_G1_h != prevState.timerun_G1 || (current_millis - prevState.timerun_G1_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V39=" + String(timerun_G1_h);
    prevState.timerun_G1 = timerun_G1_h;
    prevState.timerun_G1_ts = current_millis;
  }
  float timerun_B1_h = float(data.timerun_B1) / 1000 / 60 / 60;
  if (timerun_B1_h != prevState.timerun_B1 || (current_millis - prevState.timerun_B1_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V41=" + String(timerun_B1_h);
    prevState.timerun_B1 = timerun_B1_h;
    prevState.timerun_B1_ts = current_millis;
  }
  float timerun_B2_h = float(data.timerun_B2) / 1000 / 60 / 60;
  if (timerun_B2_h != prevState.timerun_B2 || (current_millis - prevState.timerun_B2_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V43=" + String(timerun_B2_h);
    prevState.timerun_B2 = timerun_B2_h;
    prevState.timerun_B2_ts = current_millis;
  }

  if (params_to_update.length() > 0) {
    String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN + params_to_update;
    http.begin(client, server_path.c_str());
    http.GET();
    http.end();
  }
}
void up_timerun_motor() {
  String server_path = server_name + "batch/update?token=" + BLYNK_AUTH_TOKEN + "&V40=" + float(data.timerun_G1) / 1000 / 60 / 60 + "&V42=" + float(data.timerun_B1) / 1000 / 60 / 60 + "&V44=" + float(data.timerun_B2) / 1000 / 60 / 60;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}
void upData() {
  Serial.print("connecting to ");
  Serial.println(host);

  if (!client_secure.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  // Create a URL for sending or writing data to Google Sheets.
  String Send_Data_URL = "sts=write";
  Send_Data_URL += "&AL=" + String(Result1, 2); // Ap luc (làm tròn 2 chữ số thập phân)
  Send_Data_URL += "&AG1=" + String(Irms0, 2);  // Dòng điện Bơm Giếng (làm tròn 2 chữ số thập phân)

  String url = "/macros/s/" + LOG_ID + "/exec?" + Send_Data_URL;
  Serial.println(url);

  // Gửi yêu cầu GET bằng đối tượng client_secure
  client_secure.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "User-Agent: BuildFailureDetectorESP8266\r\n" + "Connection: close\r\n\r\n");
  Serial.println("OK");
}
String dataForm(float value, int leng, int decimal) {
  String str = String(value, decimal);
  if (str.length() < leng) {
    int space = leng - str.length();
    for (int i = 0; i < space; ++i) {
      str = " " + str;
    }
  }
  return str;
}
//-------------------------------------------------------------------
void savedata() {
  if (memcmp(&data, &dataCheck, sizeof(data)) != 0) {
    Serial.println("\nData changed, writing to EEPROM...");
    data.save_num = data.save_num + 1;
    if (cs.write(data)) {
      memcpy(&dataCheck, &data, sizeof(data));
      Blynk.setProperty(V10, "label", data.save_num);
      Serial.println("EEPROM write successful.");
    } else {
      Serial.println("EEPROM write failed!");
    }
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
  if (data.flags.statusRualoc == HIGH) {
    pcf8575_1.digitalWrite(pin_Vandien, !data.flags.statusRualoc);
    timer1.setTimeout(long(time_run_nenkhi * 1000), []() { pcf8575_1.digitalWrite(pin_Vandien, !LOW); });
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

  float rms0;
  if (status_g1 == HIGH) {
    // Nếu có lệnh bật động cơ, đo chi tiết luôn
    rms0 = emon0.calcIrms(1480);
  } else {
    // Nếu động cơ đang tắt, đo nhanh để kiểm tra
    rms0 = emon0.calcIrms(370);
  }

  // Nếu đang trong chu kỳ khởi động, chỉ đọc để ổn định và thoát.
  if (startup_cycles > 0) {
    return;
  }

  if (rms0 < 2) {
    Irms0 = 0;
    yIrms0 = 0;
    if (status_g1 == HIGH) {
      // Nếu có lệnh BẬT nhưng không có dòng, bắt đầu đếm lỗi
      xIrms0++;
      if (xIrms0 > 3) {
        // Kiểm tra xem có phải do bể đầy nên phao ngắt bơm không
        if (smoothDistance >= (dosau - 45)) {
          // Bể đầy, phao đã ngắt bơm. Đây là hoạt động bình thường.
          // Chỉ cần cập nhật lại trạng thái, không báo lỗi.
          status_g1 = LOW; // Cập nhật trạng thái để logic bảo vệ hoạt động
          xIrms0 = 0;      // Reset bộ đếm lỗi
        } else {
          // Bể chưa đầy nhưng bơm không chạy -> Đây mới là lỗi thực sự.
          offcap1(); // Tắt bơm (cập nhật status_g1 = LOW)
          trip0 = true;
          if (data.flags.key_noti)
            Blynk.logEvent("error", String("Bơm GIẾNG lỗi không đo được DÒNG ĐIỆN!"));
        }
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
      if (status_g1 == LOW) {
        // Lệnh đang là TẮT nhưng vẫn đo được dòng điện ổn định -> Contactor kẹt?
        status_g1 = HIGH; // Cập nhật trạng thái để logic bảo vệ hoạt động
        Blynk.virtualWrite(V2, status_g1);
      }
      if (G1_start >= 0) {
        if (G1_start == 0)
          G1_start = millis();
        else if (millis() - G1_start > 60000) {
          G1_save = true;
        } else
          G1_save = false;
      }
      if ((Irms0 >= data.SetAmpemax) || (Irms0 <= data.SetAmpemin && data.SetAmpemin > 0)) {
        xSetAmpe = xSetAmpe + 1;
        if ((xSetAmpe >= 4) && (keyp)) {
          offcap1();
          xSetAmpe = 0;
          trip0 = true;
          if (data.flags.key_noti)
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

  float rms1;
  if (status_b1 == HIGH) {
    // Nếu có lệnh bật động cơ, đo chi tiết luôn
    rms1 = emon1.calcIrms(1480);
  } else {
    // Nếu động cơ đang tắt, đo nhanh để kiểm tra
    rms1 = emon1.calcIrms(370);
  }

  // Nếu đang trong chu kỳ khởi động, chỉ đọc để ổn định và thoát.
  if (startup_cycles > 0) {
    return;
  }

  if (rms1 < 2) {
    Irms1 = 0;
    yIrms1 = 0;
    if (status_b1 == HIGH) {
      // Nếu có lệnh BẬT nhưng không có dòng, bắt đầu đếm lỗi
      xIrms1++;
      Serial.println("xIrms1: " + String(xIrms1));
      if (xIrms1 > 3) {
        // Lệnh đang là BẬT nhưng không đo được dòng điện -> Động cơ lỗi không chạy
        offbom1(); // Hàm này đã bao gồm việc đặt status_b1 = LOW
        trip1 = true;
        if (data.flags.key_noti)
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
      if (status_b1 == LOW) {
        // Lệnh đang là TẮT nhưng vẫn đo được dòng điện ổn định -> Contactor kẹt?
        status_b1 = HIGH; // Cập nhật trạng thái để logic bảo vệ hoạt động
        Blynk.virtualWrite(V0, status_b1);
      }
      if (B1_start >= 0) {
        if (B1_start == 0)
          B1_start = millis();
        else if (millis() - B1_start > 60000) {
          B1_save = true;
        } else
          B1_save = false;
      }
      if ((Irms1 >= data.SetAmpe1max) || (Irms1 <= data.SetAmpe1min && data.SetAmpe1min > 0)) {
        xSetAmpe1 = xSetAmpe1 + 1;
        if ((xSetAmpe1 >= 2) && (keyp)) {
          offbom1();
          xSetAmpe1 = 0;
          trip1 = true;
          if (data.flags.key_noti)
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

  float rms2;
  if (status_b2 == HIGH) {
    // Nếu có lệnh bật động cơ, đo chi tiết luôn
    rms2 = emon2.calcIrms(1480);
  } else {
    // Nếu động cơ đang tắt, đo nhanh để kiểm tra
    rms2 = emon2.calcIrms(370);
  }

  // Nếu đang trong chu kỳ khởi động, chỉ đọc để ổn định và thoát.
  if (startup_cycles > 0) {
    return;
  }

  if (rms2 < 2) {
    Irms2 = 0;
    yIrms2 = 0;
    if (status_b2 == HIGH) {
      // Nếu có lệnh BẬT nhưng không có dòng, bắt đầu đếm lỗi
      xIrms2++;
      if (xIrms2 > 3) {
        // Lệnh đang là BẬT nhưng không đo được dòng điện -> Động cơ lỗi không chạy
        offbom2(); // Hàm này đã bao gồm việc đặt status_b2 = LOW
        trip2 = true;
        if (data.flags.key_noti)
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
      if (status_b2 == LOW) {
        // Lệnh đang là TẮT nhưng vẫn đo được dòng điện ổn định -> Contactor kẹt?
        status_b2 = HIGH; // Cập nhật trạng thái để logic bảo vệ hoạt động
        Blynk.virtualWrite(V1, status_b2);
      }
      if (B2_start >= 0) {
        if (B2_start == 0)
          B2_start = millis();
        else if (millis() - B2_start > 60000) {
          B2_save = true;
        } else
          B2_save = false;
      }
      if ((Irms2 >= data.SetAmpe2max) || (Irms2 <= data.SetAmpe2min && data.SetAmpe2min > 0)) {
        xSetAmpe2 = xSetAmpe2 + 1;
        if ((xSetAmpe2 >= 2) && (keyp)) {
          offbom2();
          xSetAmpe2 = 0;
          trip2 = true;
          if (data.flags.key_noti)
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

  float rms3;
  // Nén khí chạy chung nguồn với Bơm Giếng, nên dựa vào status_g1
  if (status_g1 == HIGH) {
    // Nếu bơm giếng đang chạy, đo chi tiết cho nén khí
    rms3 = emon3.calcIrms(740);
  } else {
    // Nếu bơm giếng tắt, đo nhanh
    rms3 = emon3.calcIrms(370);
  }

  // Nếu đang trong chu kỳ khởi động, chỉ đọc để ổn định và thoát.
  if (startup_cycles > 0) {
    return;
  }

  if (rms3 < 1) {
    Irms3 = 0;
    yIrms3 = 0;
  } else if (rms3 >= 1) {
    Irms3 = rms3;
    yIrms3 = yIrms3 + 1;
    if ((yIrms3 > 3) && ((Irms3 >= data.SetAmpe3max && data.SetAmpe3max > 0) || (Irms3 <= data.SetAmpe3min && data.SetAmpe3min > 0))) {
      xSetAmpe3 = xSetAmpe3 + 1;
      if ((xSetAmpe3 >= 3) && (keyp)) {
        // Lỗi nén khí -> Tắt nguồn chung (Bơm Giếng)
        offcap1();
        trip3 = true;
        xSetAmpe3 = 0;
        if (data.flags.key_noti)
          Blynk.logEvent("error", String("Máy NÉN KHÍ lỗi: ") + Irms3 + String(" A"));
      }
    } else {
      xSetAmpe3 = 0;
    }
  }
}
void readPower4() // C6 - Van điện - I4
{
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, HIGH);
  pcf8575_1.digitalWrite(S3pin, LOW);

  float rms4;
  // Van điện được điều khiển bởi `data.flags.statusRualoc`
  if (data.flags.statusRualoc == HIGH) {
    // Nếu có lệnh bật van, đo chi tiết
    rms4 = emon4.calcIrms(740);
  } else {
    // Nếu không có lệnh, đo nhanh
    rms4 = emon4.calcIrms(370);
  }

  // Nếu đang trong chu kỳ khởi động, chỉ đọc để ổn định và thoát.
  if (startup_cycles > 0) {
    return;
  }

  if (rms4 < 1) {
    Irms4 = 0;
    yIrms4 = 0;
    if (data.flags.statusRualoc == HIGH) {
      // Có lệnh bật nhưng không có dòng -> Lỗi?
      // Có thể thêm logic đếm lỗi và cảnh báo ở đây nếu cần
    }
  } else if (rms4 >= 1) {
    Irms4 = rms4;
    yIrms4 = yIrms4 + 1;
    if ((yIrms4 > 3) && ((Irms4 >= data.SetAmpe4max && data.SetAmpe4max > 0) || (Irms4 <= data.SetAmpe4min && data.SetAmpe4min > 0))) {
      xSetAmpe4 = xSetAmpe4 + 1;
      if ((xSetAmpe4 >= 3) && (keyp)) {
        data.flags.statusRualoc = LOW;
        savedata();
        pcf8575_1.digitalWrite(pin_Vandien, !data.flags.statusRualoc);
        xSetAmpe4 = 0;
        trip4 = true;
        if (data.flags.key_noti)
          Blynk.logEvent("error", String("Van điện lỗi: ") + Irms4 + String(" A"));
      }
    } else {
      xSetAmpe4 = 0;
    }
  }
}
void temperature() { // Nhiệt độ
  sensors.requestTemperatures();
  // Serial.println(sensors.getDeviceCount());
  for (byte i = 0; i < sensors.getDeviceCount(); i++)
    temp[i] = sensors.getTempCByIndex(i);
  // Blynk.virtualWrite(V15, temp[1]);
  // Blynk.virtualWrite(V23, temp[0]);
}
// Hàm sắp xếp và lấy trung vị
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
void readPressure() { // C0 - Ap Luc
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  int raw_adc = analogRead(A0);

  // Đưa giá trị ADC thô qua bộ lọc Kalman
  filtered_adc_pressure = pressureKalmanFilter.updateEstimate(raw_adc);

  // Áp dụng công thức hiệu chuẩn
  // Pressure = gain * (ADC_current - ADC_zero)
  float gain = data.pressure_cal_gain_x1000 / 1000.0f;
  if (gain > 0) {
    Result1 = gain * (filtered_adc_pressure - data.pressure_cal_offset);
  } else {
    Result1 = 0.0f; // Tránh chia cho 0 nếu chưa hiệu chuẩn
  }
  Result1 = constrain(Result1, 0.0, 10.0); // Giới hạn trong thang đo 0-10 bar
}

void readWaterLevel() { // C1 - Muc Nuoc
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  int raw_adc = analogRead(A0);

  // 1. Áp dụng Median Filter để loại bỏ nhiễu đột biến
  median_buffer[median_buffer_index] = raw_adc;
  median_buffer_index = (median_buffer_index + 1) % MEDIAN_WINDOW_SIZE;

  int sorted_buffer[MEDIAN_WINDOW_SIZE];
  memcpy(sorted_buffer, median_buffer, sizeof(median_buffer));
  int median_value = getMedian(sorted_buffer, MEDIAN_WINDOW_SIZE);

  // 2. Đưa giá trị đã qua bộ lọc trung vị vào bộ lọc Kalman
  filtered_adc_level = levelKalmanFilter.updateEstimate(median_value);

  // 3. Áp dụng công thức hiệu chuẩn
  // Level = gain * (ADC_current - ADC_zero)
  float gain = data.level_cal_gain_x1000 / 1000.0f;
  if (gain > 0) {
    smoothDistance = gain * (filtered_adc_level - data.level_cal_offset);
  } else {
    smoothDistance = 0.0f; // Tránh chia cho 0 nếu chưa hiệu chuẩn
  }
  smoothDistance = constrain(smoothDistance, 0.0, dosau * 1.2); // Giới hạn giá trị

  // 4. Tính toán thể tích và kiểm tra logic
  volume1 = (dai * smoothDistance * rong) / 1000000;
  if ((smoothDistance < (dosau / 2)) && (Irms0 == 0) && !trip0 && data.flags.key_noti && keytank) {
    Blynk.logEvent("info", String("Mực nước thấp nhưng bơm giếng không chạy: ") + smoothDistance + String(" cm"));
    keytank = false;
    timer1.setTimeout(10 * m, []() { keytank = true; });
  } else if ((smoothDistance - dosau >= 20) && (data.flags.key_noti) && (keytank)) {
    Blynk.logEvent("info", String("Nước trong bể cao vượt mức ") + (smoothDistance - dosau) + String(" cm"));
    keytank = false;
    timer1.setTimeout(15 * m, []() { keytank = true; });
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
  uint16_t nowtime = now.hour() * 360 + now.minute() * 6; // Chuyển sang đơn vị mới
  if (data.flags.mode_cap2 == 1) {
    if ((nowtime > data.b1_1_start && nowtime < data.b1_1_stop) || (nowtime > data.b1_2_start && nowtime < data.b1_2_stop) || (nowtime > data.b1_3_start && nowtime < data.b1_3_stop) || (nowtime > data.b1_4_start && nowtime < data.b1_4_stop)) { // Chạy bơm 1
      if (Irms1 == 0 && !trip1) {
        if ((Irms2 == 0 && !time_run2) || (time_run2)) {
          onbom1(); // Chạy bơm 1
        }
        if (time_run1 && noti_3) {
          noti_3 = false;
          if (data.flags.key_noti)
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
          if (data.flags.key_noti)
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
          if (data.flags.key_noti)
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
          if (data.flags.key_noti)
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
          if (data.flags.key_noti) Blynk.logEvent("error", String("Lỗi lịch chạy!\nBơm 2 đang chạy: ") + Irms0 + String(" A"));
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
            if ((f == 3) && (data.flags.key_noti))
              Blynk.logEvent("error", String("Bơm 1 bị lỗi không chạy kìa.\nKiểm tra lẹ."));
          }
        }
      }
      if (now.day() % 2 != 0) {
        if (((nowtime >= data.bom_chanle_start) && (trip2 == false)) && (Irms2 == 0)) {
          onbom1();
          if (Irms2 == 0) {
            f = f + 1;
            if ((f == 3) && (data.flags.key_noti))
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
            if ((f == 3) && (data.flags.key_noti))
              Blynk.logEvent("error", String("Bơm 1 lỗi không thể chạy.\nXin hãy kiểm tra. "));
          }
        }
      }
      if (now.day() % 2 == 0) {
        if (((nowtime >= data.bom_chanle_start) && (trip2 == false)) && (Irms2 == 0)) {
          onbom1();
          if (Irms2 == 0) {
            f = f + 1;
            if ((f == 3) && (data.flags.key_noti))
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
          if (data.flags.key_noti) Blynk.logEvent("error", String("Lỗi lịch chạy!\nBơm 1 đang chạy: ") + Irms2 + String(" A"));
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
            if ((f == 3) && (data.flags.key_noti))
              Blynk.logEvent("error", String("Bơm 2 bị lỗi không chạy kìa.\nKiểm tra lẹ."));
          }
        }
      }
      if (now.day() % 2 != 0) {
        if (((nowtime >= data.bom_chanle_start) && (trip0 == false)) && (Irms0 == 0)) {
          onbom2();
          if (Irms0 == 0) {
            f = f + 1;
            if ((f == 3) && (data.flags.key_noti))
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
            if ((f == 3) && (data.flags.key_noti))
              Blynk.logEvent("error", String("Bơm 2 lỗi không thể chạy.\nXin hãy kiểm tra. "));
          }
        }
      }
      if (now.day() % 2 == 0) {
        if (((nowtime >= data.bom_chanle_start) && (trip0 == false)) && (Irms0 == 0)) {
          onbom2();
          if (Irms0 == 0) {
            f = f + 1;
            if ((f == 3) && (data.flags.key_noti))
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
    case 0: { // Thủ công
      data.flags.mode_cap2 = 0;
      visible_man();
      break;
    }
    case 1: { // Tự động
      data.flags.mode_cap2 = 1;
      hidden_auto();
      break;
    }
    }
  } else
    Blynk.virtualWrite(V3, data.flags.mode_cap2);
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
  case 3: { // NK
    c = 3;
    Blynk.virtualWrite(V6, data.SetAmpe3min);
    Blynk.virtualWrite(V7, data.SetAmpe3max);
    break;
  }
  case 4: { // Van điện
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
  if (dataS == "help") {
    terminal.clear();
    Blynk.virtualWrite(V10,
                       "--- DANH SÁCH LỆNH ---\n"
                       "M       : Kich hoat che do cai dat (15s)\n"
                       "active    : Kich hoat che do cai dat (vo han)\n"
                       "deactive  : Thoat che do cai dat\n"
                       "save      : Luu tat ca cai dat\n"
                       "reset     : Xoa trang thai loi cua may bom\n"
                       "rst       : Khoi dong lai thiet bi\n"
                       "update    : Cap nhat firmware (OTA)\n"
                       "pre_0     : Hieu chuan diem 0 bar (ap suat)\n"
                       "pre_X.X   : Hieu chuan tai ap suat X.X bar\n"
                       "i2c       : Quet cac thiet bi I2C\n"
                       "level_0   : Hieu chuan muc nuoc 0 cm\n"
                       "level_YYY : Hieu chuan tai muc nuoc YYY cm\n");
  } else if (dataS == "M") {
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
  } else if (dataS == "clr") {
    terminal.clear();
  } else if (dataS == "i2c") {
    terminal.clear();
    byte error, address;
    int nDevices;
    terminal.println("Scanning for I2C devices...");
    nDevices = 0;
    for (address = 1; address < 127; address++) {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
      if (error == 0) {
        terminal.print("I2C device found at address 0x");
        if (address < 16) {
          terminal.print("0");
        }
        terminal.println(address, HEX);
        nDevices++;
      } else if (error == 4) {
        terminal.print("Unknown error at address 0x");
        terminal.println(address, HEX);
      }
    }
    if (nDevices == 0)
      terminal.println("No I2C devices found.");
    // terminal.clear();
  } else {
    bool handled = false;
    if (key) {
      if (dataS == "pre_0") {
        data.pressure_cal_offset = filtered_adc_pressure;
        savedata();
        terminal.printf("Đã lưu offset áp suất: ADC=%.0f\n", filtered_adc_pressure);
        handled = true;
      } else if (dataS.startsWith("pre_")) {
        float p_known = dataS.substring(4).toFloat();
        if (p_known > 0 && filtered_adc_pressure > data.pressure_cal_offset) {
          float gain = p_known / (filtered_adc_pressure - data.pressure_cal_offset);
          data.pressure_cal_gain_x1000 = gain * 1000;
          savedata();
          terminal.printf("Đã tính gain áp suất: %.4f\n", gain);
        } else {
          terminal.print("Lỗi: Giá trị ADC hoặc áp suất không hợp lệ.\n");
        }
        handled = true;
      } else if (dataS == "level_0") {
        data.level_cal_offset = filtered_adc_level;
        savedata();
        terminal.printf("Đã lưu offset mực nước: ADC=%.0f\n", filtered_adc_level);
        handled = true;
      } else if (dataS.startsWith("level_")) {
        float l_known = dataS.substring(6).toFloat();
        if (l_known > 0 && filtered_adc_level > data.level_cal_offset) {
          float gain = l_known / (filtered_adc_level - data.level_cal_offset);
          data.level_cal_gain_x1000 = gain * 1000;
          savedata();
          terminal.printf("Đã tính gain mực nước: %.4f\n", gain);
        } else {
          terminal.print("Lỗi: Giá trị ADC hoặc mực nước không hợp lệ.\n");
        }
        handled = true;
      }
    }
    if (!handled) {
      Blynk.virtualWrite(V10, "Mật mã sai.\nVui lòng nhập lại!\n");
    }
  }
}
BLYNK_WRITE(V11) // Chọn thời gian chạy 2 Bơm
{
  if (data.flags.mode_cap2 == 1) {
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
      // Chuyển đổi ngược từ đơn vị tùy chỉnh sang giây để hiển thị đúng trên app
      Blynk.virtualWrite(V18, (data.b1_1_start / 360) * 3600 + ((data.b1_1_start % 360) / 6) * 60,
                         (data.b1_1_stop / 360) * 3600 + ((data.b1_1_stop % 360) / 6) * 60, tz);
      break;
    }
    case 1: { // Bơm 2 - Lần 1
      if (key)
        b = 1;
      Blynk.virtualWrite(V18, (data.b2_1_start / 360) * 3600 + ((data.b2_1_start % 360) / 6) * 60,
                         (data.b2_1_stop / 360) * 3600 + ((data.b2_1_stop % 360) / 6) * 60, tz);
      break;
    }
    case 2: { // Bơm 1 - Lần 2
      if (key)
        b = 2;
      Blynk.virtualWrite(V18, (data.b1_2_start / 360) * 3600 + ((data.b1_2_start % 360) / 6) * 60,
                         (data.b1_2_stop / 360) * 3600 + ((data.b1_2_stop % 360) / 6) * 60, tz);
      break;
    }
    case 3: { // Bơm 2 - Lần 2
      if (key)
        b = 3;
      Blynk.virtualWrite(V18, (data.b2_2_start / 360) * 3600 + ((data.b2_2_start % 360) / 6) * 60,
                         (data.b2_2_stop / 360) * 3600 + ((data.b2_2_stop % 360) / 6) * 60, tz);
      break;
    }
    case 4: { // Bơm 1 - Lần 3
      if (key)
        b = 4;
      Blynk.virtualWrite(V18, (data.b1_3_start / 360) * 3600 + ((data.b1_3_start % 360) / 6) * 60,
                         (data.b1_3_stop / 360) * 3600 + ((data.b1_3_stop % 360) / 6) * 60, tz);
      break;
    }
    case 5: { // Bơm 2 - Lần 3
      if (key)
        b = 5;
      Blynk.virtualWrite(V18, (data.b2_3_start / 360) * 3600 + ((data.b2_3_start % 360) / 6) * 60,
                         (data.b2_3_stop / 360) * 3600 + ((data.b2_3_stop % 360) / 6) * 60, tz);
      break;
    }
    case 6: { // Bơm 1 - Lần 4
      if (key)
        b = 6;
      Blynk.virtualWrite(V18, (data.b1_4_start / 360) * 3600 + ((data.b1_4_start % 360) / 6) * 60,
                         (data.b1_4_stop / 360) * 3600 + ((data.b1_4_stop % 360) / 6) * 60, tz);
      break;
    }
    case 7: { // Bơm 2 - Lần 4
      if (key)
        b = 7;
      Blynk.virtualWrite(V18, (data.b2_4_start / 360) * 3600 + ((data.b2_4_start % 360) / 6) * 60,
                         (data.b2_4_stop / 360) * 3600 + ((data.b2_4_stop % 360) / 6) * 60, tz);
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
      data.flags.key_noti = false;
    else
      data.flags.key_noti = true;
    savedata();
  } else
    Blynk.virtualWrite(V16, data.flags.key_noti);
}
BLYNK_WRITE(V17) // Chế độ rửa lọc
{
  if (key) {
    if (param.asInt() == LOW) {
      data.flags.statusRualoc = LOW;
      pcf8575_1.digitalWrite(pin_Vandien, !data.flags.statusRualoc);
      if (data.LLG1_RL != 0) {
        Blynk.virtualWrite(V38, LLG1_1m3 - data.LLG1_RL);
        data.LLG1_RL = 0;
        savedata();
      }
    } else {
      data.flags.statusRualoc = HIGH;
      if (data.LLG1_RL == 0) {
        data.LLG1_RL = LLG1_1m3;
        savedata();
      }
    }
  } else {
    Blynk.virtualWrite(V17, data.flags.statusRualoc);
  }
}
BLYNK_WRITE(V18) // Time input
{
  if (key) {
    TimeInputParam t(param);
    // Chuyển đổi từ giây trong ngày sang đơn vị (giờ * 360 + phút * 6)
    if (t.hasStartTime()) {
      if (b == 0)
        data.b1_1_start = t.getStartHour() * 360 + t.getStartMinute() * 6;
      else if (b == 1)
        data.b2_1_start = t.getStartHour() * 360 + t.getStartMinute() * 6;
      else if (b == 2)
        data.b1_2_start = t.getStartHour() * 360 + t.getStartMinute() * 6;
      else if (b == 3)
        data.b2_2_start = t.getStartHour() * 360 + t.getStartMinute() * 6;
      else if (b == 4)
        data.b1_3_start = t.getStartHour() * 360 + t.getStartMinute() * 6;
      else if (b == 5)
        data.b2_3_start = t.getStartHour() * 360 + t.getStartMinute() * 6;
      else if (b == 6)
        data.b1_4_start = t.getStartHour() * 360 + t.getStartMinute() * 6;
      else if (b == 7)
        data.b2_4_start = t.getStartHour() * 360 + t.getStartMinute() * 6;
    }
    if (t.hasStopTime()) {
      if (b == 0)
        data.b1_1_stop = t.getStopHour() * 360 + t.getStopMinute() * 6;
      else if (b == 1)
        data.b2_1_stop = t.getStopHour() * 360 + t.getStopMinute() * 6;
      else if (b == 2)
        data.b1_2_stop = t.getStopHour() * 360 + t.getStopMinute() * 6;
      else if (b == 3)
        data.b2_2_stop = t.getStopHour() * 360 + t.getStopMinute() * 6;
      else if (b == 4)
        data.b1_3_stop = t.getStopHour() * 360 + t.getStopMinute() * 6;
      else if (b == 5)
        data.b2_3_stop = t.getStopHour() * 360 + t.getStopMinute() * 6;
      else if (b == 6)
        data.b1_4_stop = t.getStopHour() * 360 + t.getStopMinute() * 6;
      else if (b == 7)
        data.b2_4_stop = t.getStopHour() * 360 + t.getStopMinute() * 6;
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
    if (data.flags.mode_cap2 == 0) {
      Blynk.virtualWrite(V10, "Chế độ bơm: Vận hành THỦ CÔNG");
    } else if (data.flags.mode_cap2 == 1) {
      int hour_start_b1_1 = data.b1_1_start / 360;
      int minute_start_b1_1 = (data.b1_1_start - (hour_start_b1_1 * 360)) / 6;
      int hour_stop_b1_1 = data.b1_1_stop / 360;
      int minute_stop_b1_1 = (data.b1_1_stop - (hour_stop_b1_1 * 360)) / 6;

      int hour_start_b2_1 = data.b2_1_start / 360;
      int minute_start_b2_1 = (data.b2_1_start - (hour_start_b2_1 * 360)) / 6;
      int hour_stop_b2_1 = data.b2_1_stop / 360;
      int minute_stop_b2_1 = (data.b2_1_stop - (hour_stop_b2_1 * 360)) / 6;

      int hour_start_b1_2 = data.b1_2_start / 360;
      int minute_start_b1_2 = (data.b1_2_start - (hour_start_b1_2 * 360)) / 6;
      int hour_stop_b1_2 = data.b1_2_stop / 360;
      int minute_stop_b1_2 = (data.b1_2_stop - (hour_stop_b1_2 * 360)) / 6;

      int hour_start_b2_2 = data.b2_2_start / 360;
      int minute_start_b2_2 = (data.b2_2_start - (hour_start_b2_2 * 360)) / 6;
      int hour_stop_b2_2 = data.b2_2_stop / 360;
      int minute_stop_b2_2 = (data.b2_2_stop - (hour_stop_b2_2 * 360)) / 6;

      int hour_start_b1_3 = data.b1_3_start / 360;
      int minute_start_b1_3 = (data.b1_3_start - (hour_start_b1_3 * 360)) / 6;
      int hour_stop_b1_3 = data.b1_3_stop / 360;
      int minute_stop_b1_3 = (data.b1_3_stop - (hour_stop_b1_3 * 360)) / 6;

      int hour_start_b2_3 = data.b2_3_start / 360;
      int minute_start_b2_3 = (data.b2_3_start - (hour_start_b2_3 * 360)) / 6;
      int hour_stop_b2_3 = data.b2_3_stop / 360;
      int minute_stop_b2_3 = (data.b2_3_stop - (hour_stop_b2_3 * 360)) / 6;

      int hour_start_b1_4 = data.b1_4_start / 360;
      int minute_start_b1_4 = (data.b1_4_start - (hour_start_b1_4 * 360)) / 6;
      int hour_stop_b1_4 = data.b1_4_stop / 360;
      int minute_stop_b1_4 = (data.b1_4_stop - (hour_stop_b1_4 * 360)) / 6;

      int hour_start_b2_4 = data.b2_4_start / 360;
      int minute_start_b2_4 = (data.b2_4_start - (hour_start_b2_4 * 360)) / 6;
      int hour_stop_b2_4 = data.b2_4_stop / 360;
      int minute_stop_b2_4 = (data.b2_4_stop - (hour_stop_b2_4 * 360)) / 6;

      Blynk.virtualWrite(V10, "Mode: Auto\nPump 1: ", hour_start_b1_1, "h", minute_start_b1_1, " -> ", hour_stop_b1_1, "h", minute_stop_b1_1, "\nPump 2: ", hour_start_b2_1, "h", minute_start_b2_1, " -> ", hour_stop_b2_1, "h", minute_stop_b2_1, "\nPump 1: ", hour_start_b1_2, "h", minute_start_b1_2, " -> ", hour_stop_b1_2, "h", minute_stop_b1_2, "\nPump 2: ", hour_start_b2_2, "h", minute_start_b2_2, " -> ", hour_stop_b2_2, "h", minute_stop_b2_2);
      Blynk.virtualWrite(V10, "\nPump 1: ", hour_start_b1_3, "h", minute_start_b1_3, " -> ", hour_stop_b1_3, "h", minute_stop_b1_3, "\nPump 2: ", hour_start_b2_3, "h", minute_start_b2_3, " -> ", hour_stop_b2_3, "h", minute_stop_b2_3, "\nPump 1: ", hour_start_b1_4, "h", minute_start_b1_4, " -> ", hour_stop_b1_4, "h", minute_stop_b1_4, "\nPump 2: ", hour_start_b2_4, "h", minute_start_b2_4, " -> ", hour_stop_b2_4, "h", minute_stop_b2_4);
    } /* else if ((data.flags.mode_cap2 == 1) || (data.flags.mode_cap2 == 2)) {
      int hour_start = data.bom_chanle_start / 3600;
      int minute_start = (data.bom_chanle_start - (hour_start * 3600)) / 60;
      int hour_stop = data.bom_chanle_stop / 3600;
      int minute_stop = (data.bom_chanle_stop - (hour_stop * 3600)) / 60;
      if (data.flags.mode_cap2 == 1) {
        if (data.cap2_chanle == 0)
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 1 tự động\nTắt máy vào: NGÀY CHẴN\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop);
        else if (data.cap2_chanle == 1)
          Blynk.virtualWrite(V10, "Chế độ bơm: Bơm 1 tự động\nTắt máy vào: NGÀY LẺ\nThời gian: ", hour_start, ":", minute_start, " - ", hour_stop, ":", minute_stop);
      } else if (data.flags.mode_cap2 == 2) {
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
  Serial.begin(115200);
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

  client_secure.setInsecure();
  Wire.begin();
  sensors.begin(); // DS18B20 start
  rtc_module.begin();
  ee.begin();
  cs.begin(ee, PAGE_SIZE, MEMORY_SIZE / PAGE_SIZE);
  cs.read(data);
  memcpy(&dataCheck, &data, sizeof(data));

  // Khởi tạo giá trị hiệu chuẩn mặc định nếu chưa có
  if (data.pressure_cal_gain_x1000 == 0) {
    Serial.println("Initializing default calibration for pressure sensor.");
    // Giả định: 0 bar -> ADC 186, 6 bar -> ADC 805
    data.pressure_cal_offset = 186;
    data.pressure_cal_gain_x1000 = (6.0f / (805.0f - 186.0f)) * 1000; // gain ~ 0.00969
  }
  if (data.level_cal_gain_x1000 == 0) {
    Serial.println("Initializing default calibration for water level sensor.");
    // Giả định: 0cm -> ADC 196, 500cm -> ADC 750
    data.level_cal_offset = 196;
    data.level_cal_gain_x1000 = (500.0f / (750.0f - 196.0f)) * 1000; // gain ~ 0.9025
  }
  savedata(); // Lưu lại nếu có thay đổi

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
    timer_1 = timer.setInterval(1103L, []() {
      if (startup_cycles > 0) {
        Serial.printf("Startup stabilization... %d cycles left.\n", startup_cycles);
        startup_cycles--; // Giảm biến đếm
      }
      // Luôn gọi các hàm đọc. Logic bên trong hàm sẽ quyết định có xử lý kết quả hay không.
      readPower();
      readPower1();
      readPower2();
      readPower3();
      readPower4();
      up();

    });
    timer_5 = timer.setInterval(15006L, []() {
      rtctime();
      upData();
      time_run_motor();
    });
    timer.setInterval(900005L, []() {
      connectionstatus();
    });
    timer.setInterval(long(((time_run_nenkhi + time_stop_nenkhi) * 1000) + 500), rualoc);
    terminal.clear(); });
  timer.setTimeout(10000L, []() { timer.setInterval(250L, []() { // Tăng tần suất đọc cảm biến
                                    readPressure();
                                    readWaterLevel();
                                  }); });
}
void loop() {
  ESP.wdtFeed();
  Blynk.run();
  timer.run();
  timer1.run();
}