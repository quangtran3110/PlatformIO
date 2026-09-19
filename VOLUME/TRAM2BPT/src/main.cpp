#define BLYNK_TEMPLATE_ID "TMPL6swUcB_EZ"
#define BLYNK_TEMPLATE_NAME "VOLUME"
#define BLYNK_AUTH_TOKEN "AdXbklpLJKTZQ5hK9Qpy7Sg5DdwgmQ8z"

#define BLYNK_FIRMWARE_VERSION "260919.2"
#define BLYNK_PRINT Serial
#define APP_DEBUG

#include <Arduino.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <I2C_eeprom.h>
#include <RTClib.h>
#include <SPI.h>
#include <TimeLib.h>
#include <WiFiClientSecure.h>
#include <UrlEncode.h>
#include <Wire.h>

const char *ssid = "BPT2";
const char *password = "0919126757";
// const char *ssid = "tram bom so 4";
// const char *password = "0943950555";

const char *MAIN_TOKEN = "YZXkYAgH44t-kjJPKapydw5vMlR7MGAC";
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/VOLUME/TRAM2BPT/.pio/build/nodemcuv2/firmware.bin"

#define USE_RTC_DS1307
constexpr uint8_t EEPROM_I2C_ADDRESS = 0x50;
constexpr uint32_t EEPROM_SIZE = 4096;
constexpr uint32_t STATE_MAGIC = 0x32424451UL; // "2BDQ"
constexpr uint8_t FLOW_PULSE_PIN = D6;
constexpr uint8_t PULSE_ACTIVE_LEVEL = LOW;

const char *PIN_LIVE = "V24";
const char *PIN_DAILY = "V25";
const char *PIN_TERMINAL = "V0";

#include "../../shared/volume_reader_core.h"
