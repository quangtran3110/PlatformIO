#define BLYNK_TEMPLATE_ID "TMPL6xNbwEQiD"
#define BLYNK_TEMPLATE_NAME "TRAM2.G3   TRAM4"
#define BLYNK_AUTH_TOKEN "XQjby78lmTxxCG7JiC_-fyN7qEA-YrGE"

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

const char *ssid = "Hiddennet";
const char *password = "Password";

const char *MAIN_TOKEN = "BDm1LNQi_LhtaKAQU8RWUaGbiOyKIcd3";
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/VOLUME/TRAM2_G3/.pio/build/nodemcuv2/firmware.bin"

#define USE_RTC_DS3231
constexpr uint8_t EEPROM_I2C_ADDRESS = 0x57;
constexpr uint32_t EEPROM_SIZE = 4096;
constexpr uint32_t STATE_MAGIC = 0x47334451UL; // "G3DQ"
constexpr uint8_t FLOW_PULSE_PIN = D6;
constexpr uint8_t PULSE_ACTIVE_LEVEL = HIGH;

const char *PIN_LIVE = "V60";
const char *PIN_DAILY = "V63";
const char *PIN_TERMINAL = "V0";

#include "../../shared/volume_reader_core.h"
