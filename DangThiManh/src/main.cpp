#define BLYNK_TEMPLATE_ID "TMPL67lOs7dLq"
#define BLYNK_TEMPLATE_NAME "TRẠM SỐ 4"
#define BLYNK_AUTH_TOKEN "ra1gZtR0irrwiTH1L-L_nhXI6TMRH7M9"

#include <BlynkSimpleEsp8266.h>
#include <SPI.h>
//-------------------
#include "PCF8575.h"
PCF8575 pcf8575_1(0x21);
PCF8575 pcf8575_2(0x20);
const int IN_MODE_RUALOC = P0;
const int IN_LOC_1 = P1;
const int IN_LOC_2 = P2;
const int IN_TIMER_LOC_1 = P3;
const int IN_TIMER_LOC_2 = P4;

const int nguon_220v = P0;
const int van_loc_1 = P1;
const int van_khi_loc_1 = P2;
const int van_nguon_loc_1 = P3;
const int van_loc_2 = P4;
const int van_khi_loc_2 = P5;
const int van_nguon_loc_2 = P6;
const int van_xa_be = P7;
const int bom_loc = P8;
const int thoi_khi = P9;
const int thoi_gio = P10;

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
int timer_loc12, timer_loc1, timer_loc2;
byte status_rualoc1 = HIGH;
byte status_rualoc2 = HIGH;
byte pulse = LOW;
bool key_loc12 = true, key_loc1 = true, key_loc2 = true;

String previousState = ""; // Biến để lưu trạng thái trước đó

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
}
void loc2() {
  pcf8575_1.digitalWrite(van_loc_2, !status_rualoc2);
  pcf8575_1.digitalWrite(van_khi_loc_2, !status_rualoc2);
}
void rualoc() {
  byte in_loc_1 = pcf8575_2.digitalRead(IN_LOC_1);
  byte in_loc_2 = pcf8575_2.digitalRead(IN_LOC_2);
  byte in_mode_RL = pcf8575_2.digitalRead(IN_MODE_RUALOC);
  byte in_timer_loc_1 = pcf8575_2.digitalRead(IN_TIMER_LOC_1);
  byte in_timer_loc_2 = pcf8575_2.digitalRead(IN_TIMER_LOC_2);
  String currentState = ""; // Biến để lưu trạng thái hiện tại

  // Hàm trợ giúp để in trạng thái khi có thay đổi
  auto printStateWhenChanged = [&](const String &newState) {
    if (newState != previousState) {
      Serial.println("--------------------");
      Serial.print("Trang thai cac cong tac: LOC1=");
      Serial.print(in_loc_1);
      Serial.print(" | LOC2=");
      Serial.print(in_loc_2);
      Serial.print(" | RUA LOC=");
      Serial.print(in_mode_RL);
      Serial.print(" | TIMER1=");
      Serial.print(in_timer_loc_1);
      Serial.print(" | TIMER2=");
      Serial.println(in_timer_loc_2);
      Serial.println(newState);
      previousState = newState;
    }
  };

  if ((in_loc_1 == HIGH) && (in_loc_2 == HIGH)) { // Nếu công tắc cơ lọc 1 và lọc 2 đều mở
    currentState = "Che do: Ca hai loc deu bat.\n";
    if ((in_timer_loc_1 == HIGH) || (in_timer_loc_2 == HIGH)) {                // Nếu timer rửa lọc 1 hoặc lọc 2 mở
      if ((in_timer_loc_1 == HIGH) && (in_timer_loc_2 == HIGH) && key_loc12) { // Nếu timer rửa lọc 1 và lọc 2 mở
        currentState += "Hanh dong: Rua ca hai loc (theo timer).";
        printStateWhenChanged(currentState);
        pcf8575_1.digitalWrite(van_xa_be, LOW);
        pcf8575_1.digitalWrite(van_nguon_loc_1, HIGH);
        pcf8575_1.digitalWrite(van_nguon_loc_2, HIGH);
        key_loc12 = false;
        status_rualoc1 = true;
        status_rualoc2 = true;
        loc1();
        loc2();
        timer.setTimeout(1500, []() {
          pcf8575_1.digitalWrite(thoi_khi, LOW);
        });
        timer_loc12 = timer.setTimeout(10 * 1000, []() {
          pcf8575_1.digitalWrite(bom_loc, LOW);
          key_loc12 = true;
        });
      } else if (in_timer_loc_1 == HIGH && key_loc1) { // Nếu timer rửa lọc 1 mở
        currentState += "Hanh dong: Chi rua loc 1 (theo timer).";
        printStateWhenChanged(currentState);
        pcf8575_1.digitalWrite(van_xa_be, LOW);
        pcf8575_1.digitalWrite(van_nguon_loc_1, HIGH);
        pcf8575_1.digitalWrite(van_nguon_loc_2, LOW);
        key_loc1 = false;
        status_rualoc1 = true;
        status_rualoc2 = false;
        loc1();
        loc2();
        timer.setTimeout(1500, []() {
          pcf8575_1.digitalWrite(thoi_khi, LOW);
        });
        timer_loc1 = timer.setTimeout(10 * 1000, []() {
          pcf8575_1.digitalWrite(bom_loc, LOW);
          key_loc1 = true;
        });
      } else if (in_timer_loc_2 == HIGH && key_loc2) { // Nếu timer rửa lọc 2 mở
        currentState += "Hanh dong: Chi rua loc 2 (theo timer).";
        printStateWhenChanged(currentState);
        pcf8575_1.digitalWrite(van_xa_be, LOW);
        pcf8575_1.digitalWrite(van_nguon_loc_1, LOW);
        pcf8575_1.digitalWrite(van_nguon_loc_2, HIGH);
        key_loc2 = false;
        status_rualoc1 = false;
        status_rualoc2 = true;
        loc1();
        loc2();
        timer.setTimeout(1500, []() {
          pcf8575_1.digitalWrite(thoi_khi, LOW);
        });
        timer_loc2 = timer.setTimeout(10 * 1000, []() {
          pcf8575_1.digitalWrite(bom_loc, LOW);
          key_loc2 = true;
        });
      }
    } else if (in_mode_RL == HIGH && key_loc12) { // Nếu công tắc cơ ở chế độ rửa lọc
      currentState += "Hanh dong: Rua ca hai loc (che do bang tay).";
      printStateWhenChanged(currentState);
      pcf8575_1.digitalWrite(van_xa_be, LOW);
      pcf8575_1.digitalWrite(van_nguon_loc_1, HIGH);
      pcf8575_1.digitalWrite(van_nguon_loc_2, HIGH);
      key_loc12 = false;
      status_rualoc1 = true;
      status_rualoc2 = true;
      loc1();
      loc2();
      timer.setTimeout(1500, []() {
        pcf8575_1.digitalWrite(thoi_khi, LOW);
      });
      timer_loc12 = timer.setTimeout(10 * 1000, []() {
        pcf8575_1.digitalWrite(bom_loc, LOW);
        key_loc12 = true;
      });
    } else if ((in_timer_loc_1 == LOW) && (in_timer_loc_2 == LOW) && (in_mode_RL == LOW)) {
      currentState += "Hanh dong: Loc binh thuong (ca hai loc).";
      printStateWhenChanged(currentState);
      status_rualoc1 = false;
      status_rualoc2 = false;
      key_loc1 = true;
      key_loc2 = true;
      loc1();
      loc2();
      pcf8575_1.digitalWrite(van_nguon_loc_1, HIGH);
      pcf8575_1.digitalWrite(van_nguon_loc_2, HIGH);
      pcf8575_1.digitalWrite(thoi_khi, HIGH);
      pcf8575_1.digitalWrite(van_xa_be, HIGH);
      pcf8575_1.digitalWrite(bom_loc, HIGH);
      timer.deleteTimer(timer_loc12);
      timer.deleteTimer(timer_loc1);
      timer.deleteTimer(timer_loc2);
    }

  } else if (in_loc_1 == HIGH) {
    currentState = "Che do: Chi loc 1 bat.\n";
    pcf8575_1.digitalWrite(van_nguon_loc_1, HIGH);
    pcf8575_1.digitalWrite(van_nguon_loc_2, LOW);
    if (((in_mode_RL == HIGH) || (in_timer_loc_1 == HIGH)) && key_loc1) {
      currentState += "Hanh dong: Rua loc 1 (bang tay hoac timer).";
      printStateWhenChanged(currentState);
      pcf8575_1.digitalWrite(van_xa_be, LOW);
      key_loc1 = false;
      status_rualoc1 = true;
      loc1();
      timer.setTimeout(1500, []() {
        pcf8575_1.digitalWrite(thoi_khi, LOW);
      });
      timer_loc1 = timer.setTimeout(10 * 1000, []() {
        pcf8575_1.digitalWrite(bom_loc, LOW);
        key_loc1 = true;
      });
    } else if ((in_timer_loc_1 == LOW) && (in_mode_RL == LOW)) {
      currentState += "Hanh dong: Loc binh thuong (chi loc 1).";
      printStateWhenChanged(currentState);
      status_rualoc1 = false;
      key_loc1 = true;
      loc1();
      pcf8575_1.digitalWrite(van_nguon_loc_1, HIGH);
      pcf8575_1.digitalWrite(thoi_khi, HIGH);
      pcf8575_1.digitalWrite(van_xa_be, HIGH);
      pcf8575_1.digitalWrite(bom_loc, HIGH);
      timer.deleteTimer(timer_loc1);
    }
  } else if (in_loc_2 == HIGH) {
    currentState = "Che do: Chi loc 2 bat.\n";
    pcf8575_1.digitalWrite(van_nguon_loc_1, LOW);
    pcf8575_1.digitalWrite(van_nguon_loc_2, HIGH);
    if (((in_mode_RL == HIGH) || (in_timer_loc_2 == HIGH)) && key_loc2) {
      currentState += "Hanh dong: Rua loc 2 (bang tay hoac timer).";
      printStateWhenChanged(currentState);
      pcf8575_1.digitalWrite(van_xa_be, LOW);
      key_loc2 = false;
      status_rualoc2 = true;
      loc2();
      timer.setTimeout(1500, []() {
        pcf8575_1.digitalWrite(thoi_khi, LOW);
      });
      timer_loc2 = timer.setTimeout(10 * 1000, []() {
        pcf8575_1.digitalWrite(bom_loc, LOW);
        key_loc2 = true;
      });
    } else if ((in_timer_loc_2 == LOW) && (in_mode_RL == LOW)) {
      currentState += "Hanh dong: Loc binh thuong (chi loc 2).";
      printStateWhenChanged(currentState);
      status_rualoc2 = false;
      key_loc2 = true;
      loc2();
      pcf8575_1.digitalWrite(van_nguon_loc_2, HIGH);
      pcf8575_1.digitalWrite(thoi_khi, HIGH);
      pcf8575_1.digitalWrite(van_xa_be, HIGH);
      pcf8575_1.digitalWrite(bom_loc, HIGH);
      timer.deleteTimer(timer_loc2);
    }
  } else if ((in_loc_1 == LOW) && (in_loc_2 == LOW)) {
    currentState = "Che do: TAT HE THONG.";
    printStateWhenChanged(currentState);
    pcf8575_1.digitalWrite(van_nguon_loc_1, LOW);
    pcf8575_1.digitalWrite(van_nguon_loc_2, LOW);
    pcf8575_1.digitalWrite(thoi_khi, HIGH);
    pcf8575_1.digitalWrite(van_xa_be, HIGH);
    pcf8575_1.digitalWrite(bom_loc, HIGH);
    status_rualoc1 = false;
    status_rualoc2 = false;
    loc1();
    loc2();
    key_loc1 = true;
    key_loc2 = true;
    key_loc12 = true;
    timer.deleteTimer(timer_loc12);
    timer.deleteTimer(timer_loc1);
    timer.deleteTimer(timer_loc2);
  }
}
void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");
  Wire.begin();
  eeprom.initialize();
  eeprom.readBytes(address, sizeof(dataDefault), (byte *)&data);

  pinMode(D3, OUTPUT);
  digitalWrite(D3, HIGH);
  pcf8575_1.begin();
  pcf8575_1.pinMode(van_loc_1, OUTPUT);
  pcf8575_1.digitalWrite(van_loc_1, HIGH);
  pcf8575_1.pinMode(van_loc_2, OUTPUT);
  pcf8575_1.digitalWrite(van_loc_2, HIGH);
  pcf8575_1.pinMode(van_nguon_loc_1, OUTPUT);
  pcf8575_1.digitalWrite(van_nguon_loc_1, HIGH);
  pcf8575_1.pinMode(van_khi_loc_1, OUTPUT);
  pcf8575_1.digitalWrite(van_khi_loc_1, HIGH);
  pcf8575_1.pinMode(van_nguon_loc_2, OUTPUT);
  pcf8575_1.digitalWrite(van_nguon_loc_2, HIGH);
  pcf8575_1.pinMode(van_khi_loc_2, OUTPUT);
  pcf8575_1.digitalWrite(van_khi_loc_2, HIGH);
  pcf8575_1.pinMode(van_xa_be, OUTPUT);
  pcf8575_1.digitalWrite(van_xa_be, HIGH);
  pcf8575_1.pinMode(thoi_khi, OUTPUT);
  pcf8575_1.digitalWrite(thoi_khi, HIGH);
  pcf8575_1.pinMode(bom_loc, OUTPUT);
  pcf8575_1.digitalWrite(bom_loc, HIGH);
  pcf8575_1.pinMode(nguon_220v, OUTPUT);
  pcf8575_1.digitalWrite(nguon_220v, LOW);
  pcf8575_1.pinMode(P10, OUTPUT);
  pcf8575_1.digitalWrite(P10, HIGH);
  pcf8575_1.pinMode(P11, OUTPUT);
  pcf8575_1.digitalWrite(P11, HIGH);
  pcf8575_1.pinMode(P12, OUTPUT);
  pcf8575_1.digitalWrite(P12, HIGH);
  pcf8575_1.pinMode(P13, OUTPUT);
  pcf8575_1.digitalWrite(P13, HIGH);
  pcf8575_1.pinMode(P14, OUTPUT);
  pcf8575_1.digitalWrite(P14, HIGH);
  pcf8575_1.pinMode(P15, OUTPUT);
  pcf8575_1.digitalWrite(P15, HIGH);

  pcf8575_2.begin();
  pcf8575_2.pinMode(IN_MODE_RUALOC, INPUT_PULLUP);
  pcf8575_2.pinMode(IN_LOC_1, INPUT_PULLUP);
  pcf8575_2.pinMode(IN_LOC_2, INPUT_PULLUP);
  pcf8575_2.pinMode(IN_TIMER_LOC_1, INPUT_PULLUP);
  pcf8575_2.pinMode(IN_TIMER_LOC_2, INPUT_PULLUP);

  timer.setTimeout(1000, []() {
    timer.setInterval(1000, []() {
      pulse = !pulse;
      digitalWrite(D3, pulse); // Chớp LED D3 để biết hàm rualoc() đang chạy
      rualoc();
    });
  });
}
void loop() {
  timer.run();
}
