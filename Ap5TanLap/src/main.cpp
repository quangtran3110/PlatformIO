/*
#define BLYNK_TEMPLATE_ID "TMPL6PNR4qqpm"
#define BLYNK_TEMPLATE_NAME "Multi"
#define BLYNK_AUTH_TOKEN "LtMwtUXWi8mAX47_K17_z5rr8C3HNDjk"
*/
#define BLYNK_TEMPLATE_ID "TMPL6I6ISEvF5"
#define BLYNK_TEMPLATE_NAME "SUPPORT ACTIVE"
#define BLYNK_AUTH_TOKEN "g3GPiciujLdMuATrIxJ0zNInoUo72DiN"

#define BLYNK_FIRMWARE_VERSION "250331"
#define BLYNK_PRINT Serial
#define I2C_ADDRESS 0x40
#define APP_DEBUG

#include "INA226_WE.h"
#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <Wire.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
WiFiClient client;
HTTPClient http;
String server_name = "http://sgp1.blynk.cloud/external/api/";
String Main = "91thFYXxhfcs2ij5GDVg6NTjEgMqFBwi";
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/Ap5TanLap/.pio/build/nodemcuv2/firmware.bin"

INA226_WE ina226 = INA226_WE(I2C_ADDRESS);
OneWire oneWire(D7);
DallasTemperature sensors(&oneWire);

const char *ssid = "net";
const char *password = "Password";
// const char* ssid = "tram bom so 4";
// const char* password = "0943950555";

float shuntVoltage_mV = 0.0;
float loadVoltage_V = 0.0;
float busVoltage_V = 0.0;
float current_mA = 0.0;
float power_mW = 0.0;
float Result1;
float value14 = 0;
float temp[1], nhietdo;
float sensorValue;
byte w, key_pre = 0;
byte reboot_num;
bool p = true, key_pluse = true;
int save_num;
int time1, time2, time3;
BlynkTimer timer;

WidgetTerminal terminal(V10);
BLYNK_CONNECTED() {
}
//-------------------------
void connectionstatus() {
  if ((WiFi.status() != WL_CONNECTED)) {
    Serial.println("Khong ket noi WIFI");
    WiFi.begin(ssid, password);
    reboot_num = reboot_num + 1;
    if (reboot_num % 5) {
      key_pluse = false;
    }
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
    key_pluse = true;
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

void send_data() {
  String server_path = server_name + "batch/update?token=" + Main + "&V34=" + w + "&V26=" + 1 + "&V30=" + busVoltage_V + "&V31=" + (current_mA / 1000) + "&V33=" + Result1
                       //+ "&V35=" + save_num
                       + "&V36=" + nhietdo;
  http.begin(client, server_path.c_str());
  int httpResponseCode = http.GET();
  http.end();
}

void savedata() {
  save_num = save_num + 1;
  EEPROM.begin(512);
  delay(10);
  EEPROM.put(156, reboot_num);
  EEPROM.put(160, save_num);
  EEPROM.commit();
  EEPROM.end();
}

void updata() {
  Blynk.virtualWrite(V34, w);
  Blynk.virtualWrite(V26, 1);
  Blynk.virtualWrite(V33, Result1);
  Blynk.virtualWrite(V35, save_num);
  Blynk.virtualWrite(V36, nhietdo);
  Blynk.virtualWrite(V30, busVoltage_V);
  Blynk.virtualWrite(V31, current_mA / 1000);
}
void tem() {
  sensors.requestTemperatures();
  // Serial.println(sensors.getDeviceCount());
  for (byte i = 0; i < sensors.getDeviceCount(); i++) {
    temp[i] = sensors.getTempCByIndex(i);
    nhietdo = temp[i];
    // Blynk.virtualWrite(V36, temp[i]);
  }
  if (temp[0] > 42) {
    Blynk.logEvent("error-5", String("Nhiệt độ pin cao: ") + temp[0] + String("°C"));
  }
  w = digitalRead(D6);
}

void monitor() {
  ina226.readAndClearFlags();
  // shuntVoltage_mV = ina226.getShuntVoltage_mV();
  busVoltage_V = ina226.getBusVoltage_V();
  current_mA = ina226.getCurrent_mA();
  // power_mW = ina226.getBusPower();
  loadVoltage_V = busVoltage_V + (shuntVoltage_mV / 1000);

  if (key_pre == 1) {
    Blynk.virtualWrite(V10, "Pre_RAW: ", sensorValue, "\n");
  }
}

void pressure() {
  sensorValue = analogRead(A0);
  float Result;
  Result = (((sensorValue - 197.5) * 5) / (1005 - 197.5));

  // Serial.println(Result);
  value14 += Result;
  Result1 = value14 / 16.0;
  value14 -= Result1;
  // Serial.println(Result1);
}
void pluse_arduino() {
  if (key_pluse) {
    p = !p;
    digitalWrite(D5, p);
  }
}

BLYNK_WRITE(V10) {
  String dataS = param.asStr();
  if (dataS == "pre_raw") {
    terminal.clear();
    key_pre = !key_pre;
  } else if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V10, "ESP UPDATE...");
    update_fw();
  }
}
//-------------------------
void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(300000);
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(5000);

  pinMode(D5, OUTPUT);
  Wire.begin();
  sensors.begin();

  ina226.init();
  ina226.setAverage(AVERAGE_16);            // choose mode and uncomment for change of default
  ina226.setConversionTime(CONV_TIME_1100); // choose conversion time and uncomment for change of default
  ina226.setMeasureMode(CONTINUOUS);        // choose mode and uncomment for change of default
  ina226.setCurrentRange(MA_800);           // choose gain and uncomment for change of default
  ina226.waitUntilConversionCompleted();    // if you comment this line the first data might be zero
  delay(1000);

  EEPROM.begin(512);
  delay(10);
  EEPROM.get(156, reboot_num);
  EEPROM.get(160, save_num);
  // EEPROM.commit();
  EEPROM.end();

  timer.setTimeout(5000, []() {
    time1 = timer.setInterval(5612, []() {
      pluse_arduino();
      tem();
      monitor();
      send_data();
      // updata();
      timer.restartTimer(time1);
      timer.restartTimer(time2);
    });
    time2 = timer.setInterval(123, pressure);
    timer.setInterval(600005, []() { // 10p
      connectionstatus();
      timer.restartTimer(time1);
      timer.restartTimer(time2);
    });
  });
}
void loop() {
  ESP.wdtFeed();
  Blynk.run();
  timer.run();
}