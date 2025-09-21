
#define BLYNK_TEMPLATE_ID "TMPL6Px18Gsjk"
#define BLYNK_TEMPLATE_NAME "TRẠM SỐ 3 VFD"
#define BLYNK_AUTH_TOKEN "eXmsWQOmDdHaBMALIxHJqhbJXtzg8Gw1"
#define BLYNK_FIRMWARE_VERSION "25061050610"
//------------------

#include "PCF8575.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
//------------------
#define APP_DEBUG
#define BLYNK_PRINT Serial

PCF8575 pcf8575_1(0x20);
//-----------------------------
const char *ssid = "Nha May Nuoc Cai Cat";
const char *password = "123456789";

const word address = 0;
//------------------
BlynkTimer timer;

BLYNK_CONNECTED() {
}
//-------------------------------------------------------------------

BLYNK_WRITE(V0) {
}

int currentPin = 7;
bool isLedOn = false;

void blinkNextPin() {
  if (!isLedOn) {
    // Bật chân hiện tại
    pcf8575_1.digitalWrite(currentPin, LOW);
    isLedOn = true;
  } else {
    // Tắt chân hiện tại
    pcf8575_1.digitalWrite(currentPin, HIGH);
    // Chuyển sang chân tiếp theo
    currentPin--;
    if (currentPin < 0)
      currentPin = 7;
    isLedOn = false;
  }
}

//-------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(7000);
  //------------------------------------------------------------------

  Wire.begin();
  pcf8575_1.begin();
  for (int i = 0; i < 8; i++) {
    pcf8575_1.pinMode(i, OUTPUT);    // Đặt tất cả chân P0-P7 là OUTPUT
    pcf8575_1.digitalWrite(i, HIGH); // Tắt tất cả chân ban đầu
  }

  //------------------------------------
  timer.setTimeout(3000L, []() {
    timer.setInterval(1000L, blinkNextPin);
  });
}

void loop() {
  Blynk.run();
  timer.run();
}
