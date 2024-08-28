
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

int timer_run = 1, timer_I;
BlynkTimer timer;
BLYNK_CONNECTED() {
}

void run() {
  timer_run = timer_run + 1;
  Serial.println(timer_run);
  timer_I = timer.setTimeout(timer_run * 1000, []() {
    run();
  });
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(7000);

  timer_I = timer.setTimeout(timer_run * 1000, []() {
    run();
  });
}

void loop() {
  Blynk.run();
  timer.run();
}
