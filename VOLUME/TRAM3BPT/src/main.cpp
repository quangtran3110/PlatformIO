#define BLYNK_TEMPLATE_ID "TMPL6swUcB_EZ"
#define BLYNK_TEMPLATE_NAME "VOLUME"
#define BLYNK_AUTH_TOKEN "Q2KAjaqI3sWhET-Ax94VPYfIk2Fmsr36"

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

const char *ssid = "KwacoBlynk";
const char *password = "Password";
//const char *ssid = "tram bom so 4";
//const char *password = "0943950555";

const char *MAIN_TOKEN = "Xd_XI0fm9nIsXBvvMZ6pjEtRd0irLLR2";
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/VOLUME/TRAM3BPT/.pio/build/nodemcuv2/firmware.bin"

#define USE_RTC_DS1307
constexpr uint8_t EEPROM_I2C_ADDRESS = 0x50;
constexpr uint32_t EEPROM_SIZE = 4096;
constexpr uint32_t STATE_MAGIC = 0x33424451UL; // "3BDQ"
constexpr uint8_t FLOW_PULSE_PIN = D6;
constexpr uint8_t PULSE_ACTIVE_LEVEL = LOW;

const char *PIN_LIVE = "V29";
const char *PIN_DAILY = "V31";
const char *PIN_TERMINAL = "V0";

#include "../../shared/volume_reader_core.h"
