#define BLYNK_TEMPLATE_ID "TMPL6ekKXp4iu"
#define BLYNK_TEMPLATE_NAME "VOLUME"
#define BLYNK_AUTH_TOKEN "JTnEpJjGVVJ8DM1aJx7zZT4cyNYJrhr_"

#define BLYNK_FIRMWARE_VERSION "250620"
#define BLYNK_PRINT Serial
#define APP_DEBUG

const char *ssid = "Hiddennet";
const char *password = "Password";

// const char *ssid = "tram bom so 4";
// const char *password = "0943950555";

#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>

#include "RTClib.h"
#include <WidgetRTC.h>

#include <I2C_eeprom.h>
#include <I2C_eeprom_cyclic_store.h>
#define MEMORY_SIZE 0x4000 // Total capacity of the EEPROM
#define PAGE_SIZE 64

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <SPI.h>
#include <UrlEncode.h>
#include <WiFiClientSecure.h>
WiFiClient client;
HTTPClient http;
String server_name = "http://sgp1.blynk.cloud/external/api/";
String Main_TOKEN = "BDm1LNQi_LhtaKAQU8RWUaGbiOyKIcd3";
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/VOLUME/TRAM2_G3/.pio/build/nodemcuv2/firmware.bin"

String LL24h = "&V63=";
String LL1m3 = "&V60=";
String terminal_main = "&V50=";
String text;

bool blynk_first_connect = false, key_i2c = false, key_pulse = true;
int var_10m3;
byte reboot_num;

struct Data {
public:
  uint32_t pulse, save_num;
  byte save_day;
};
Data data, dataCheck;
const struct Data dataDefault = {0, 0, 0};

I2C_eeprom ee(0x50, MEMORY_SIZE);
I2C_eeprom_cyclic_store<Data> cs;

WidgetTerminal terminal(V0);
WidgetRTC rtc_widget;
BlynkTimer timer;
BLYNK_CONNECTED() {
  rtc_widget.begin();
  blynk_first_connect = true;
}

ICACHE_RAM_ATTR void buttonPressed() {
  if (key_pulse) {
    key_pulse = false;
    data.pulse++;
    timer.setTimeout(10000, []() {
      key_pulse = true;
    });
  }
}

//-------------------------
void savedata() {
  if (memcmp(&data, &dataCheck, sizeof(dataDefault)) == 0) {
    // Serial.println("structures same no need to write to EEPROM");
  } else {
    // Serial.println("\nWrite bytes to EEPROM memory...");
    data.save_num = data.save_num + 1;
    cs.write(data);
  }
  // Blynk.virtualWrite(V1, BLYNK_FIRMWARE_VERSION, "-EEPROM ", data.save_num);
}
//-------------------------
void connectionstatus() {
  if ((WiFi.status() != WL_CONNECTED)) {
    Serial.println("Khong ket noi WIFI");
    WiFi.begin(ssid, password);
  }
  if ((WiFi.status() == WL_CONNECTED) && (!Blynk.connected())) {
    reboot_num = reboot_num + 1;
    if (reboot_num % 5 == 0) {
      ESP.restart();
    } else {
      Serial.println("...");
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
//-------------------------
void send_LL_24h() {
  String server_path = server_name + "batch/update?token=" + Main_TOKEN +
                       LL24h + data.pulse;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}
void send_LL_1m3() {
  String server_path = server_name + "batch/update?token=" + Main_TOKEN +
                       LL1m3 + data.pulse;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}
void rtc_time() {
  send_LL_1m3();
  if (blynk_first_connect) {
    // Serial.println(data.save_day);
    if (data.save_day != day()) {
      if (Blynk.connected()) {
        send_LL_24h();
        data.pulse = 0;
        data.save_day = day();
        savedata();
      }
    }
  }
  if (((data.pulse % 10) == 0) && (data.pulse != var_10m3)) {
    var_10m3 = data.pulse;
    savedata();
  }
}
//-------------------------
void print_terminal_main() {
  String server_path = server_name + "batch/update?token=" + Main_TOKEN +
                       terminal_main + "clr";
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
  server_path = server_name + "batch/update?token=" + Main_TOKEN +
                terminal_main + urlEncode(text);
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}
BLYNK_WRITE(V0) // String
{
  String dataS = param.asStr();
  if (dataS == "rst_G3") {
    text = "ESP khởi động lại sau 3s";
    print_terminal_main();
    delay(3000);
    ESP.restart();
  } else if (dataS == "update_G3") {
    text = "UPDATE FIRMWARE...";
    print_terminal_main();
    update_fw();
  } else if (dataS == "rst_vl_G3") {
    text = "Đã reset Volume";
    print_terminal_main();
    data.pulse = 0;
    savedata();
  } else if (dataS == "i2c_G3") {
    key_i2c = !key_i2c;
  } else if (dataS == "savedata_G3") {
    text = "DATA_SAVE... ok!";
    print_terminal_main();
    savedata();
  }
}
//-------------------------
void scanI2C() {
  if (key_i2c) {
    long rssi = WiFi.RSSI();
    String wifi_rssi = "WiFi: " + String(rssi) + " dBm\n";
    String server_path = server_name + "batch/update?token=" + Main_TOKEN +
                         terminal_main + "clr";
    http.begin(client, server_path.c_str());
    http.GET();
    http.end();
    byte error, address;
    int nDevices;
    char result[2];
    nDevices = 0;
    for (address = 1; address < 127; address++) {
      // The i2c_scanner uses the return value of
      // the Write.endTransmisstion to see if
      // a device did acknowledge to the address.
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
      if (error == 0) {
        // Blynk.virtualWrite(V0, "I2C device address 0x");
        if (address < 16) {
          // Blynk.virtualWrite(V0, "0");
        }
        String stringOne = String(address, HEX) + "\n";
        String server_path = server_name + "batch/update?token=" + Main_TOKEN +
                             terminal_main + urlEncode("I2C device address 0x") + urlEncode(stringOne) + urlEncode(wifi_rssi);
        http.begin(client, server_path.c_str());
        http.GET();
        http.end();
        nDevices++;
      } else if (error == 4) {
        // Blynk.virtualWrite(V0, "Unknown error at address 0x");
        if (address < 16) {
          // Blynk.virtualWrite(V0, "0");
        }
        String stringOne = String(address, HEX) + "\n";
        // Blynk.virtualWrite(V0, stringOne);
        String server_path = server_name + "batch/update?token=" + Main_TOKEN +
                             terminal_main + urlEncode("Unknown error at address 0x") + urlEncode(stringOne) + urlEncode(wifi_rssi);
        http.begin(client, server_path.c_str());
        http.GET();
        http.end();
        nDevices++;
      }
    }
    if (nDevices == 0) {
      // Blynk.virtualWrite(V0, "No I2C devices found\n");
      String server_path = server_name + "batch/update?token=" + Main_TOKEN +
                           terminal_main + urlEncode("No I2C devices found\n") + urlEncode(wifi_rssi);
      http.begin(client, server_path.c_str());
      http.GET();
      http.end();
    } else {
      Blynk.virtualWrite(V0, "Done\n");
    }
  }
}
//-------------------------
void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(300000);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(5000);

  Wire.begin();
  ee.begin();
  cs.begin(ee, PAGE_SIZE, MEMORY_SIZE / PAGE_SIZE);
  cs.read(data);

  attachInterrupt(D6, buttonPressed, RISING);

  timer.setInterval(61005, connectionstatus);
  timer.setInterval(15003, rtc_time);
  timer.setInterval(5013, scanI2C);
}

void loop() {
  ESP.wdtFeed();
  Blynk.run();
  timer.run();
}
