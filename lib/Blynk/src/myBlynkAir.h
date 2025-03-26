#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>

// Hàm kiểm tra và hiển thị thông tin bộ nhớ
void printMemoryInfo() {
  Serial.println("\n--- Memory Information ---");
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Heap Fragmentation: %d%%\n", ESP.getHeapFragmentation());
  Serial.printf("Maximum Free Block: %d bytes\n", ESP.getMaxFreeBlockSize());
  Serial.printf("Sketch Size: %d bytes\n", ESP.getSketchSize());
  Serial.printf("Free Sketch Space: %d bytes\n", ESP.getFreeSketchSpace());
  Serial.printf("Flash Chip Size: %d bytes\n", ESP.getFlashChipSize());
  Serial.printf("Flash Chip Real Size: %d bytes\n", ESP.getFlashChipRealSize());
  Serial.println("------------------------\n");
}

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

  Serial.println("Starting OTA update...");
  Serial.println("URL: " + overTheAirURL);

  // In thông tin bộ nhớ trước khi thực hiện OTA
  printMemoryInfo();

  WiFiClient client;
  HTTPClient http;

  // Sử dụng API mới với WiFiClient
  http.begin(client, overTheAirURL);

  int httpCode = http.GET();
  Serial.println("HTTP Code: " + String(httpCode));

  if (httpCode != HTTP_CODE_OK) {
    Serial.println("HTTP GET failed");
    http.end();
    Blynk.connect();
    return;
  }

  int contentLength = http.getSize();
  Serial.println("Content Length: " + String(contentLength));
  Serial.printf("Required memory for OTA: %d bytes\n", contentLength);
  Serial.printf("Available memory: %d bytes\n", ESP.getFreeHeap());

  if (contentLength <= 0) {
    Serial.println("Content length invalid");
    http.end();
    Blynk.connect();
    return;
  }

  Serial.println("Starting update...");
  bool canBegin = Update.begin(contentLength);

  if (!canBegin) {
    Serial.println("Not enough space for OTA");
    http.end();
    Blynk.connect();
    return;
  }

  WiFiClient &stream = http.getStream();
  int written = Update.writeStream(stream);

  if (written != contentLength) {
    Serial.println("Write failed: " + String(written) + "/" + String(contentLength));
    http.end();
    Blynk.connect();
    return;
  }

  Serial.println("Update written successfully");

  if (!Update.end()) {
    Serial.println("Update end failed: " + String(Update.getError()));
    http.end();
    Blynk.connect();
    return;
  }

  if (!Update.isFinished()) {
    Serial.println("Update not finished");
    http.end();
    Blynk.connect();
    return;
  }

  Serial.println("Update finished successfully!");
  http.end();
  reboot();
}
//__________________________________________________________