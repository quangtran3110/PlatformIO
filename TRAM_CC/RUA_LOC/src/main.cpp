#define BLYNK_TEMPLATE_ID "TMPL6I6ISEvF5"
#define BLYNK_TEMPLATE_NAME "SUPPORT ACTIVE"
#define BLYNK_AUTH_TOKEN "mAEloc4FYavbw8Jh8KPbhJSjUGWyxKqn"
#define BLYNK_PRINT Serial
#define APP_DEBUG

const char *ssid = "Wifi";
const char *password = "Password";

#include "EmonLib.h"
#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <UrlEncode.h>

EnergyMonitor emon0, emon1;

//-----------------------------
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include <WiFiClientSecure.h>
WiFiClient client;
HTTPClient http;
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/TRAM_CC/RUA_LOC/.pio/build/nodemcuv2/firmware.bin"
String server_name = "http://sgp1.blynk.cloud/external/api/";
String Main = "vcz0jVXPSGPK6XmFP5Dqi_etQA32VNPL";

const int S0 = D0;
const int S1 = D1;
const int S2 = D2;
const int S3 = D3;
const int RL1 = D4;
const int RL2 = D5;
const int RL3 = D6;
const int RL4 = D7;

bool statusRualoc1 = LOW, statusRualoc2 = LOW, status_NK1 = HIGH, status_NK2 = HIGH;
bool trip1 = false, trip2 = false, key = false;
int xSetAmpe = 0, xSetAmpe1 = 0;
int timer_I;
unsigned long int yIrms0 = 0, yIrms1 = 0;
float Irms0, Irms1;

WidgetTerminal keyterminal(V5);

BlynkTimer timer;
BLYNK_CONNECTED() {
}

int SetAmpemax = 12, SetAmpe1max = 12;
int SetAmpemin = 3, SetAmpe1min = 3;
int reboot_num;

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
void send_Main(String token, int virtual_pin, float value_to_send) {
  String server_path = server_name + "update?token=" + token + "&pin=v" + String(virtual_pin) + "&value=" + float(value_to_send);
  http.begin(client, server_path.c_str());
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String payload = http.getString();
  }
  http.end();
}

void send_data_main() {
  String server_path = server_name + "batch/update?token=" + Main +
                       "&V41=" + float(Irms0) +
                       "&V42=" + float(Irms1) +
                       "&V22=" + 1;
  http.begin(client, server_path.c_str());
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String payload = http.getString();
  }
  http.end();
}

void on_NK1() {
  digitalWrite(RL3, LOW);
}
void off_NK1() {
  digitalWrite(RL3, HIGH);
}
void on_NK2() {
  digitalWrite(RL4, LOW);
}
void off_NK2() {
  digitalWrite(RL4, HIGH);
}
void loc1() {
  if (statusRualoc1 == HIGH) {
    digitalWrite(RL1, LOW);
    timer.setTimeout((7 * 60 * 1000), []() {
      digitalWrite(RL1, HIGH);
    });
  }
}
void loc2() {
  if (statusRualoc2 == HIGH) {
    digitalWrite(RL2, LOW);
    timer.setTimeout((7 * 60 * 1000), []() {
      digitalWrite(RL2, HIGH);
    });
  }
}

void readcurrent() // C0 - NK1
{
  digitalWrite(S0, LOW);
  digitalWrite(S1, LOW);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  float rms0 = emon0.calcIrms(1480);
  if (rms0 < 1) {
    Irms0 = 0;
    yIrms0 = 0;
  } else if (rms0 > 1) {
    Irms0 = rms0;
    yIrms0 = yIrms0 + 1;
    if ((yIrms0 > 2) && ((Irms0 > SetAmpemax) || (Irms0 < SetAmpemin))) {
      xSetAmpe = xSetAmpe + 1;
      if (xSetAmpe >= 3) {
        trip1 = true;
        off_NK1();
        // Blynk.notify("Cái Cát - NK 1 lỗi: {Irms0}A!");
        xSetAmpe = 0;
      }
    } else {
      xSetAmpe = 0;
    }
  }
  // Serial.println(Irms0);
}
void readcurrent1() // C1 - NK 2
{
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  float rms1 = emon1.calcIrms(1480);
  if (rms1 < 1) {
    Irms1 = 0;
    yIrms1 = 0;
  } else if (rms1 > 1) {
    Irms1 = rms1;
    yIrms1 = yIrms1 + 1;
    if ((yIrms1 > 2) && ((Irms1 > SetAmpe1max) || (Irms1 < SetAmpe1min))) {
      xSetAmpe1 = xSetAmpe1 + 1;
      if (xSetAmpe1 >= 3) {
        trip2 = true;
        off_NK2();
        // Blynk.notify("Cái Cát - NK 2 lỗi: {Irms1}A!");
        xSetAmpe1 = 0;
      }
    } else {
      xSetAmpe1 = 0;
    }
  }
}
BLYNK_WRITE(V1) // data string
{
  int dataS = param.asInt();
  if (dataS == 1) {
    trip1 = false;
    trip2 = false;
    digitalWrite(RL3, LOW);
    digitalWrite(RL4, LOW);
    String server_path = server_name + "update?token=" + Main + "&pin=v5" + "&value=" + String("Reset NK!");
    http.begin(client, server_path.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String payload = http.getString();
    }
    http.end();
  } else if (dataS == 2 && trip1) {
    status_NK1 = HIGH;
    on_NK1();
  } else if (dataS == 3) {
    status_NK1 = LOW;
    off_NK1();
  } else if (dataS == 4 && trip2) {
    status_NK2 = HIGH;
    on_NK2();
  } else if (dataS == 5) {
    status_NK2 = LOW;
    off_NK2();
  } else if (dataS == 6) {
    String server_path = server_name + "batch/update?token=" + Main + "&V43=" + byte(status_NK1) + "&V44=" + byte(status_NK2);
    http.begin(client, server_path.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String payload = http.getString();
    }
    http.end();
  } else if (dataS == 7) {
    update_fw();
  }
}

BLYNK_WRITE(V2) {
  if (param.asInt() == 0) {
    statusRualoc1 = LOW;
    statusRualoc2 = LOW;
  } else if (param.asInt() == 1) {
    statusRualoc1 = HIGH;
    statusRualoc2 = LOW;
  } else if (param.asInt() == 2) {
    statusRualoc1 = LOW;
    statusRualoc2 = HIGH;
  } else if (param.asInt() == 3) {
    statusRualoc1 = HIGH;
    statusRualoc2 = HIGH;
  }
}

void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(300000);
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  pinMode(RL1, OUTPUT);
  digitalWrite(RL1, HIGH);
  pinMode(RL2, OUTPUT);
  digitalWrite(RL2, HIGH);
  pinMode(RL3, OUTPUT);
  digitalWrite(RL3, LOW);
  pinMode(RL4, OUTPUT);
  digitalWrite(RL4, LOW);

  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(5000);

  emon0.current(A0, 59);
  emon1.current(A0, 111);

  timer.setInterval((10 * 50 * 1000), loc1);
  timer.setInterval((10 * 51 * 1000), loc2);
  timer.setInterval(1503, send_data_main);
  timer_I = timer.setInterval(689, []() {
    readcurrent();
    readcurrent1();
  });
  timer.setInterval(900005L, []() {
    connectionstatus();
    timer.restartTimer(timer_I);
  });
}

void loop() {
  ESP.wdtFeed();
  Blynk.run();
  timer.run();
}
