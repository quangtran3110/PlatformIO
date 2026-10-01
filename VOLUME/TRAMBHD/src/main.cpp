#define BLYNK_TEMPLATE_ID "TMPL6PNVY0BY7"
#define BLYNK_TEMPLATE_NAME "VOLUME"
#define BLYNK_AUTH_TOKEN "PWYW_mopMTAAnpmZOeGH3h4D4QOzZi9X"

#define BLYNK_FIRMWARE_VERSION "261001.2"
#define BLYNK_PRINT Serial
#define APP_DEBUG

#include <Arduino.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <I2C_eeprom.h>
#include <RTClib.h>
#include <SPI.h>
#include <TimeLib.h>
#include <WiFiClientSecure.h>
#include <UrlEncode.h>
#include <Wire.h>

// const char *ssid = "Cap Nuoc";
// const char *password = "0919126757";
const char *ssid = "net";
const char *password = "Password";

const char *MAIN_TOKEN = "8rYwP5-2nYyA6G1txMqXMamUNITRd-k9";
#include "ota_private.h"
#define URL_fw_Bin "https://tram-cc-private-ota.dieu-hanh-cap-nuoc.workers.dev/volume-trambhd/" VOLUME_OTA_KEY "/firmware.bin"

#define USE_RTC_DS3231
constexpr uint8_t EEPROM_I2C_ADDRESS = 0x57;
constexpr uint32_t EEPROM_SIZE = 4096;
constexpr uint32_t STATE_MAGIC = 0x42484451UL; // "BHDQ"
constexpr uint8_t FLOW_PULSE_PIN = D6;
constexpr uint8_t PULSE_ACTIVE_LEVEL = LOW;

const char *PIN_LIVE = "V32";
const char *PIN_DAILY = "V33";
const char *PIN_TERMINAL = "V0";

#include "../../shared/volume_reader_core.h"
