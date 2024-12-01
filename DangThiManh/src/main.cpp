#define BLYNK_TEMPLATE_ID "TMPL67lOs7dLq"
#define BLYNK_TEMPLATE_NAME "TRẠM SỐ 4"
#define BLYNK_AUTH_TOKEN "ra1gZtR0irrwiTH1L-L_nhXI6TMRH7M9"

#include <BlynkSimpleEsp8266.h>
#include <SPI.h>
//-------------------
#include "PCF8575.h"
PCF8575 pcf8575_1(0x20);
PCF8575 pcf8575_2(0x21);
const int pin_read_p0 = P0;
const int pin_read_p1 = P1;
const int pin_read_p2 = P2;
const int pin_read_p3 = P3;
const int pin_read_p4 = P4;
const int pin_read_p5 = P5;

const int van_loc_1 = P0;
const int van_nguon_loc_1 = P1;
const int van_khi_loc_1 = P2;
const int van_loc_2 = P3;
const int van_nguon_loc_2 = P4;
const int van_khi_loc_2 = P5;
const int van_xa_be = P6;
const int thoi_khi = P7;
const int bom_loc = P8;
const int nguon_220v = P9;

const int S0pin = P15;
const int S1pin = P14;
const int S2pin = P13;
const int S3pin = P12;
//-------------------
#include <OneWire.h>
#include <Wire.h>
//-------------------
#include <Eeprom24C32_64.h>
#define EEPROM_ADDRESS 0x57
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
const word address = 0;
//-------------------
int thoi_gian_rua_loc_1, thoi_gian_rua_loc_2;
int timer_nguon_220;
byte status_rualoc1 = HIGH;
byte status_rualoc2 = HIGH;
struct Data {
  byte status_rualoc1123;

} data, dataCheck;
const struct Data dataDefault = {0};

#pragma endregion
BlynkTimer timer, timer1;
//-------------------------------------------------------------------
void nguon_220VAC() {
  timer.deleteTimer(timer_nguon_220);
  pcf8575_1.digitalWrite(nguon_220v, LOW);
  timer_nguon_220 = timer.setTimeout(60 * 1000, []() {
    pcf8575_1.digitalWrite(nguon_220v, HIGH);
  });
}
void loc1() {
  pcf8575_1.digitalWrite(van_loc_1, !status_rualoc1);
  pcf8575_1.digitalWrite(van_khi_loc_1, !status_rualoc1);
  pcf8575_1.digitalWrite(van_xa_be, !status_rualoc1);
}
void loc2() {
  pcf8575_1.digitalWrite(van_loc_2, !status_rualoc2);
  pcf8575_1.digitalWrite(van_khi_loc_2, !status_rualoc2);
  pcf8575_1.digitalWrite(van_xa_be, !status_rualoc2);
}
void rualoc() {
  if ((pcf8575_2.digitalRead(pin_read_p1) == HIGH) || (pcf8575_2.digitalRead(pin_read_p2) == HIGH)) {
    if ((pcf8575_2.digitalRead(pin_read_p1) == HIGH) && (pcf8575_2.digitalRead(pin_read_p2) == HIGH)) {
      pcf8575_1.digitalWrite(van_nguon_loc_1, LOW);
      pcf8575_1.digitalWrite(van_nguon_loc_2, LOW);
    } else if (pcf8575_2.digitalRead(pin_read_p1) == HIGH) {
      pcf8575_1.digitalWrite(van_nguon_loc_1, HIGH);
      pcf8575_1.digitalWrite(van_nguon_loc_2, LOW);
    } else if (pcf8575_2.digitalRead(pin_read_p2) == HIGH) {
      pcf8575_1.digitalWrite(van_nguon_loc_1, LOW);
      pcf8575_1.digitalWrite(van_nguon_loc_2, HIGH);
    }
  }
  if (pcf8575_2.digitalRead(pin_read_p0) == HIGH) { // Nếu ở chế độ rửa lọc
    if ((pcf8575_2.digitalRead(pin_read_p1) == HIGH) && (pcf8575_2.digitalRead(pin_read_p2) == HIGH)) {
      status_rualoc1 = true;
      status_rualoc2 = true;
      loc1();
      loc2();
      timer.setTimeout(10 * 1000, []() {
        pcf8575_1.digitalWrite(thoi_khi, !status_rualoc1);
        pcf8575_1.digitalWrite(bom_loc, !status_rualoc1);
      });
    } else if (pcf8575_2.digitalRead(pin_read_p1) == HIGH) {
      status_rualoc1 = true;
      status_rualoc2 = false;
      loc1();
      loc2();
      timer.setTimeout(10 * 1000, []() {
        pcf8575_1.digitalWrite(thoi_khi, !status_rualoc1);
        pcf8575_1.digitalWrite(bom_loc, !status_rualoc1);
      });
    } else if (pcf8575_2.digitalRead(pin_read_p2) == HIGH) {
      status_rualoc1 = false;
      status_rualoc2 = true;
      loc1();
      loc2();
      timer.setTimeout(10 * 1000, []() {
        pcf8575_1.digitalWrite(thoi_khi, !status_rualoc2);
        pcf8575_1.digitalWrite(bom_loc, !status_rualoc2);
      });
    }
  } else {
    status_rualoc1 = false;
    status_rualoc2 = false;
    loc1();
    loc2();
    pcf8575_1.digitalWrite(thoi_khi, !status_rualoc1);
    pcf8575_1.digitalWrite(bom_loc, !status_rualoc1);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  eeprom.initialize();
  eeprom.readBytes(address, sizeof(dataDefault), (byte *)&data);

  pcf8575_1.begin();
  pcf8575_2.begin();
  pcf8575_2.pinMode(pin_read_p0, INPUT);
  pcf8575_2.pinMode(pin_read_p1, INPUT);
  pcf8575_2.pinMode(pin_read_p2, INPUT);
  pcf8575_2.pinMode(pin_read_p3, INPUT);
  pcf8575_2.pinMode(pin_read_p4, INPUT);
  pcf8575_2.pinMode(pin_read_p5, INPUT);

  timer.setTimeout(5000L, []() {
    timer.setInterval(3000, []() {
      Serial.print("P0: ");
      Serial.println(pcf8575_2.digitalRead(pin_read_p0));
      Serial.print("P1: ");
      Serial.println(pcf8575_2.digitalRead(pin_read_p1));
      Serial.print("P2: ");
      Serial.println(pcf8575_2.digitalRead(pin_read_p2));
      Serial.print("P3: ");
      Serial.println(pcf8575_2.digitalRead(pin_read_p3));
      Serial.print("P4: ");
      Serial.println(pcf8575_2.digitalRead(pin_read_p4));
      Serial.print("P5: ");
      Serial.println(pcf8575_2.digitalRead(pin_read_p5));
      Serial.println(" ");
    });
  });
}
void loop() {
  timer.run();
}
