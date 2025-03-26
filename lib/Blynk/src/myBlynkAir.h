#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
void reboot() {
#if defined(ARDUINO_ARCH_MEGAAVR)
  wdt_enable(WDT_PERIOD_8CLK_gc);
#elif defined(__AVR__)
  wdt_enable(WDTO_15MS);
#elif defined(__arm__)
  NVIC_SystemReset();
#elif defined(ESP8266) || defined(ESP32)
  ESP.restart();
#else
#error "MCU reset procedure not implemented"
#endif
  for (;;) {
  }
}

//________________________________________________________________
String overTheAirURL = "";
BLYNK_WRITE(InternalPinOTA) {
  overTheAirURL = param.asString();
  Blynk.disconnect();

  HTTPClient http;
  http.begin(overTheAirURL);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Blynk.connect();
    return;
  }
  int contentLength = http.getSize();
  if (contentLength <= 0) {
    Blynk.connect();
    return;
  }
  bool canBegin = Update.begin(contentLength);
  if (!canBegin) {
    Blynk.connect();
    return;
  }
  Client &client = http.getStream();
  int written = Update.writeStream(client);
  if (written != contentLength) {
    Blynk.connect();
    return;
  }
  if (!Update.end()) {
    Blynk.connect();
    return;
  }
  if (!Update.isFinished()) {
    Blynk.connect();
    return;
  }
  reboot();
}

//__________________________________________________________
