
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6mc-8Z360"
#define BLYNK_TEMPLATE_NAME "TEST"
#define BLYNK_AUTH_TOKEN "dgSlGPc-gQsyUpqtZuMGFg33w-Q3ot-r"

#define APP_DEBUG

#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
const char *ssid = "tram bom so 4";
const char *password = "0943950555";
#include "PCF8575.h"
PCF8575 pcf8575_1(0x20);
const int pin_7 = P7;
const int pin_6 = P6;
const int pin_5 = P5;
const int pin_4 = P4;
const int pin_3 = P3;
const int pin_2 = P2;
const int pin_1 = P1;

int timer_run = 1, timer_I;
BlynkTimer timer;
BLYNK_CONNECTED() {
}

void setup() {
  Serial.begin(115200);
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);
  // Blynk.config(BLYNK_AUTH_TOKEN);
  // delay(7000);

  pcf8575_1.begin();

  pcf8575_1.pinMode(pin_1, OUTPUT);
  pcf8575_1.pinMode(pin_2, OUTPUT);
  pcf8575_1.pinMode(pin_3, OUTPUT);
  pcf8575_1.pinMode(pin_4, OUTPUT);
  pcf8575_1.pinMode(pin_5, OUTPUT);
  pcf8575_1.pinMode(pin_6, OUTPUT);
  pcf8575_1.pinMode(pin_7, OUTPUT);
}

void loop() {
  // Blynk.run();
  // timer.run();
  pcf8575_1.digitalWrite(pin_1, LOW);
  delay(500);
  pcf8575_1.digitalWrite(pin_1, HIGH);
  delay(500);
  pcf8575_1.digitalWrite(pin_2, LOW);
  delay(500);
  pcf8575_1.digitalWrite(pin_2, HIGH);
  delay(500);
  pcf8575_1.digitalWrite(pin_3, LOW);
  delay(500);
  pcf8575_1.digitalWrite(pin_3, HIGH);
  delay(500);
  pcf8575_1.digitalWrite(pin_4, LOW);
  delay(500);
  pcf8575_1.digitalWrite(pin_4, HIGH);
  delay(500);
  pcf8575_1.digitalWrite(pin_5, LOW);
  delay(500);
  pcf8575_1.digitalWrite(pin_5, HIGH);
  delay(500);
  pcf8575_1.digitalWrite(pin_6, LOW);
  delay(500);
  pcf8575_1.digitalWrite(pin_6, HIGH);
  delay(500);
  pcf8575_1.digitalWrite(pin_7, LOW);
  delay(500);
  pcf8575_1.digitalWrite(pin_7, HIGH);
}

