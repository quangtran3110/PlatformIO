#define BLYNK_TEMPLATE_ID "TMPL6VP9MY4gS"
#define BLYNK_TEMPLATE_NAME "VOLUME"
#define BLYNK_AUTH_TOKEN "jaQFoaOgdcZcKbyI_ME_oi6tThEf4FR5"

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

const char *ssid = "NHA MAY NUOC CAI CAT";
const char *password = "12345678";

const char *MAIN_TOKEN = "vcz0jVXPSGPK6XmFP5Dqi_etQA32VNPL";
#include "ota_private.h"
#define URL_fw_Bin "https://tram-cc-private-ota.dieu-hanh-cap-nuoc.workers.dev/volume-tram-cc-g1/" VOLUME_OTA_KEY "/firmware.bin"

#define USE_RTC_DS3231
constexpr uint8_t EEPROM_I2C_ADDRESS = 0x57;
constexpr uint32_t EEPROM_SIZE = 4096;
constexpr uint32_t STATE_MAGIC = 0x43314451UL; // "C1DQ"
constexpr uint8_t FLOW_PULSE_PIN = D6;
constexpr uint8_t PULSE_ACTIVE_LEVEL = HIGH;

const char *PIN_LIVE = "V51";
const char *PIN_DAILY = "V52";
const char *PIN_TERMINAL = "V0";

#include "../../shared/volume_reader_core.h"
