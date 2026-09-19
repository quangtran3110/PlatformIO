#define BLYNK_TEMPLATE_ID "TMPL6WyEmVeSK"
#define BLYNK_TEMPLATE_NAME "VOLUME"
#define BLYNK_AUTH_TOKEN "eBeqi9ZJhRK3r66cUzgdD1gp2xGxG7kS"

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

const char *ssid = "TTTV Xay Dung";
const char *password = "0723841249";
//const char *ssid = "Phong Tai Vu";
//const char *password = "0974040699";
//const char *ssid = "tram bom so 4";
//const char *password = "0943950555";

const char *MAIN_TOKEN = "eXmsWQOmDdHaBMALIxHJqhbJXtzg8Gw1";
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/refs/heads/main/VOLUME/TRAM3/.pio/build/nodemcuv2/firmware.bin"

#define USE_RTC_DS1307
constexpr uint8_t EEPROM_I2C_ADDRESS = 0x50;
constexpr uint32_t EEPROM_SIZE = 4096;
constexpr uint32_t STATE_MAGIC = 0x54334451UL; // "T3DQ"
constexpr uint8_t FLOW_PULSE_PIN = D6;
constexpr uint8_t PULSE_ACTIVE_LEVEL = LOW;

const char *PIN_LIVE = "V23";
const char *PIN_DAILY = "V24";
const char *PIN_TERMINAL = "V0";

#include "../../shared/volume_reader_core.h"
