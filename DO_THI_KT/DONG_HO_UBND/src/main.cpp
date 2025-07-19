/*
V0 - Date/time
*/
#define BLYNK_TEMPLATE_ID "TMPL6ZpS1D0RR"
#define BLYNK_TEMPLATE_NAME "ĐỒNG HỒ UBND"
#define BLYNK_AUTH_TOKEN "oe2-Gx_w4LSfr7Ie1OsuUByCAKnpaX_Q"

#define BLYNK_FIRMWARE_VERSION "250719"
#define BLYNK_PRINT Serial
#define APP_DEBUG

const char *ssid = "PBV UB";
const char *password = "kientuong2022";

#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
//-------------------
#include "RTClib.h"
#include <WidgetRTC.h>
#include <stdlib.h> // Thêm thư viện cho hàm abs()
RTC_DS3231 rtc_module;

char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";
//-------------------
#include <I2C_eeprom.h>
#include <I2C_eeprom_cyclic_store.h>
#define MEMORY_SIZE 0x4000 // Total capacity of the EEPROM
#define PAGE_SIZE 64
//-------------------
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <SPI.h>
#include <UrlEncode.h>
#include <WiFiClientSecure.h>
WiFiClient client;
HTTPClient http;
String server_name = "http://sgp1.blynk.cloud/external/api/";

#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/DO_THI_KT/DONG_HO_UBND/.pio/build/nodemcuv2/firmware.bin"
//-------------------
#include "PCF8575.h"
PCF8575 pcf8575_1(0x20);
const int pin_motor_1 = P8;
const int pin_motor_2 = P9;
const int pin_motor_3 = P10;
const int pin_motor_4 = P11;
//-------------------
byte reboot_num, face_clock = 0, mode = 0;
volatile int dem1 = 0, dem2 = 0, dem3 = 0, dem4 = 0;
volatile bool eeprom_save_request = false; // Cờ báo hiệu cần lưu vào EEPROM
volatile bool rtc_sync_due = false;        // Cờ báo hiệu đã đến lúc đồng bộ RTC
uint32_t timestamp_now;

struct Data {
public:
  uint32_t unixtime_1, unixtime_2;
  uint32_t unixtime_3, unixtime_4;
};
Data data, dataCheck;
const struct Data dataDefault = {0, 0, 0, 0};

I2C_eeprom ee(0x57, MEMORY_SIZE); // Mạch EEPROM riêng thì là 0x50, còn mạch module DS3231 thì là 0X57
I2C_eeprom_cyclic_store<Data> cs;

WidgetTerminal terminal(V3);
WidgetRTC rtc_widget;
BlynkTimer timer;
BLYNK_CONNECTED() {
  rtc_widget.begin();  // Khởi tạo widget RTC của Blynk
  rtc_sync_due = true; // Yêu cầu đồng bộ thời gian ngay khi kết nối
}

ICACHE_RAM_ATTR void sensor_1() {
  if (dem1 > 0) {
    dem1 = dem1 - 1;
    data.unixtime_1 = data.unixtime_1 + 60;
    eeprom_save_request = true;
  }
  // Luôn kiểm tra sau khi giảm, nếu biến đếm về 0 thì dừng motor.
  if (dem1 <= 0) {
    pcf8575_1.digitalWrite(pin_motor_1, HIGH);
  }
}
ICACHE_RAM_ATTR void sensor_2() {
  if (dem2 > 0) {
    dem2 = dem2 - 1;
    data.unixtime_2 = data.unixtime_2 + 60;
    eeprom_save_request = true;
  }
  if (dem2 <= 0) {
    pcf8575_1.digitalWrite(pin_motor_2, HIGH);
  }
}
ICACHE_RAM_ATTR void sensor_3() {
  if (dem3 > 0) {
    dem3 = dem3 - 1;
    data.unixtime_3 = data.unixtime_3 + 60;
    eeprom_save_request = true;
  }
  if (dem3 <= 0) {
    pcf8575_1.digitalWrite(pin_motor_3, HIGH);
  }
}
ICACHE_RAM_ATTR void sensor_4() {
  // Serial.print("Triger sensor 4");
  if (dem4 > 0) {
    dem4 = dem4 - 1;
    data.unixtime_4 = data.unixtime_4 + 60;
    eeprom_save_request = true;
  }
  if (dem4 <= 0) {
    pcf8575_1.digitalWrite(pin_motor_4, HIGH);
  }
}
//-------------------------
void savedata() {
  // Chỉ ghi nếu dữ liệu hiện tại khác với dữ liệu đã được kiểm tra (đã lưu)
  if (memcmp(&data, &dataCheck, sizeof(data)) != 0) {
    // Serial.println("\nData changed, writing to EEPROM...");
    if (cs.write(data)) { // Kiểm tra xem việc ghi có thành công không
      // Cập nhật lại dataCheck sau khi ghi thành công
      memcpy(&dataCheck, &data, sizeof(data));
    } else {
      // Serial.println("EEPROM write failed!");
    }
  }
}
//-------------------------
void connectionstatus() {
  if ((WiFi.status() != WL_CONNECTED)) {
    Serial.println("Khong ket noi WIFI");
    WiFi.begin(ssid, password);
  }
  if ((WiFi.status() == WL_CONNECTED) && (!Blynk.connected())) {
    reboot_num = reboot_num + 1;
    if (reboot_num % 5 == 0) {
      Serial.println("...");
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(ssid, password);
    }
  }
  if (Blynk.connected()) {
    if (reboot_num != 0) {
      reboot_num = 0;
    }
  }
}
void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}
void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}
void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}
void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}
void update_fw() {
  WiFiClientSecure client_;
  client_.setInsecure();
  Serial.print("Wait...");
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client_, URL_fw_Bin);
  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;
  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}
//-------------------------
void check_and_adjust_clock(uint32_t current_unixtime, uint32_t clock_unixtime, volatile int &dem_counter, const int motor_pin) {
  // Chế độ 1: Cài đặt thủ công / Tạm dừng
  // Nếu người dùng bật chế độ này, dừng motor ngay lập tức và reset bộ đếm.
  if (mode == 1) {
    if (dem_counter > 0) {
      dem_counter = 0; // Reset bộ đếm
    }
    pcf8575_1.digitalWrite(motor_pin, HIGH); // Đảm bảo motor đã dừng
    return;                                  // Thoát, không thực hiện logic tự động bên dưới
  }

  // Nếu motor đang trong quá trình điều chỉnh (dem_counter > 0), thì không làm gì cả.
  // Việc giảm biến đếm và dừng motor sẽ do hàm ngắt (ISR) xử lý.
  if (dem_counter > 0) {
    return;
  }

  // Nếu thời gian của đồng hồ chưa được cài đặt (bằng 0), thì không làm gì cả
  if (clock_unixtime == 0) {
    return;
  }

  // Số giây trong một ngày (24 * 60 * 60)
  const long SECONDS_IN_A_DAY = 86400L;

  // Chỉ lấy phần thời gian trong ngày (bỏ qua ngày/tháng/năm) bằng phép toán modulo
  long time_of_day_now = current_unixtime % SECONDS_IN_A_DAY;
  long time_of_day_clock = clock_unixtime % SECONDS_IN_A_DAY;

  // Tính toán độ chênh lệch thời gian trong ngày
  long time_diff_seconds = time_of_day_now - time_of_day_clock;

  // Nếu chênh lệch lớn hơn 1 phút, tiến hành điều chỉnh
  if (abs(time_diff_seconds) >= 60) {
    int minutes_to_run = 0;
    long diff_minutes = time_diff_seconds / 60;
    if (diff_minutes > 0) { // Đồng hồ chạy chậm, cần chạy tiến
      minutes_to_run = diff_minutes;
    } else {                                // Đồng hồ chạy nhanh, cần chạy tiến gần hết 1 vòng (24h)
      minutes_to_run = 1440 + diff_minutes; // Ví dụ: 1440 + (-5 phút) = 1435 phút
    }

    // Chỉ khởi động motor nếu thực sự cần quay
    if (minutes_to_run > 0) {
      dem_counter = minutes_to_run;
      pcf8575_1.digitalWrite(motor_pin, LOW); // Bật motor
    }
  } else {
    // Nếu không có chênh lệch, đảm bảo motor đã tắt
    pcf8575_1.digitalWrite(motor_pin, HIGH); // Tắt motor
  }
}

void rtc_time() {
  // Kiểm tra xem có yêu cầu đồng bộ RTC và có kết nối không
  if (rtc_sync_due && Blynk.connected()) {
    rtc_module.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
    Serial.println("RTC time synced with Blynk server.");
    rtc_sync_due = false; // Hạ cờ sau khi đã đồng bộ thành công
  }

  DateTime now = rtc_module.now();
  timestamp_now = now.unixtime();
  Blynk.virtualWrite(V0, String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()));

  check_and_adjust_clock(timestamp_now, data.unixtime_1, dem1, pin_motor_1);
  check_and_adjust_clock(timestamp_now, data.unixtime_2, dem2, pin_motor_2);
  check_and_adjust_clock(timestamp_now, data.unixtime_3, dem3, pin_motor_3);
  check_and_adjust_clock(timestamp_now, data.unixtime_4, dem4, pin_motor_4);
}

BLYNK_WRITE(V1) {
  TimeInputParam t(param);

  // Lấy thời gian hiện tại từ module RTC để có thông tin ngày/tháng/năm
  DateTime now = rtc_module.now();
  if (t.hasStartTime()) {
    DateTime startTime(now.year(), now.month(), now.day(),
                       t.getStartHour(), t.getStartMinute(), t.getStartSecond());
    if (mode == 1) {
      if (face_clock == 1) {
        data.unixtime_1 = startTime.unixtime();
      } else if (face_clock == 2) {
        data.unixtime_2 = startTime.unixtime();
      } else if (face_clock == 3) {
        data.unixtime_3 = startTime.unixtime();
      } else if (face_clock == 4) {
        data.unixtime_4 = startTime.unixtime();
      }
      savedata();
      Serial.print("Start Time Unixtime: ");
      Serial.println(startTime.unixtime());
    }
  }
}
BLYNK_WRITE(V2) {
  switch (param.asInt()) {
  case 0: { // Emty
    face_clock = 0;
    break;
  }
  case 1: { // Mặt số 1
    face_clock = 1;
    break;
  }
  case 2: { // Mặt số 2
    face_clock = 2;
    break;
  }
  case 3: { // Mặt số 3
    face_clock = 3;
    break;
  }
  case 4: { // Mặt số 4
    face_clock = 4;
    break;
  }
  }
}
BLYNK_WRITE(V3) {
  String dataS = param.asStr();
  if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V0, "UPDATE FIRMWARE...");
    update_fw();
  } else if (dataS == "1") {
    terminal.clear();
    String info = "--- THÔNG TIN HỆ THỐNG ---\n";
    info += "Chế độ: " + String(mode == 0 ? "Tự động" : "Cài đặt thủ công") + "\n\n";

    // Đồng hồ 1
    info += "Mặt 1: ";
    if (data.unixtime_1 == 0) {
      info += "Chưa cài đặt.";
    } else {
      DateTime dt(data.unixtime_1);
      char buffer[9];
      sprintf(buffer, "%02d:%02d:%02d", dt.hour(), dt.minute(), dt.second());
      info += buffer;
    }
    info += " | Trạng thái: " + String(dem1 > 0 ? "Đang chạy" : "Đã dừng");
    info += " | Cần quay: " + String(dem1) + " phút\n";

    // Đồng hồ 2
    info += "Mặt 2: ";
    if (data.unixtime_2 == 0) {
      info += "Chưa cài đặt.";
    } else {
      DateTime dt(data.unixtime_2);
      char buffer[9];
      sprintf(buffer, "%02d:%02d:%02d", dt.hour(), dt.minute(), dt.second());
      info += buffer;
    }
    info += " | Trạng thái: " + String(dem2 > 0 ? "Đang chạy" : "Đã dừng");
    info += " | Cần quay: " + String(dem2) + " phút\n";

    // Đồng hồ 3
    info += "Mặt 3: ";
    if (data.unixtime_3 == 0) {
      info += "Chưa cài đặt.";
    } else {
      DateTime dt(data.unixtime_3);
      char buffer[9];
      sprintf(buffer, "%02d:%02d:%02d", dt.hour(), dt.minute(), dt.second());
      info += buffer;
    }
    info += " | Trạng thái: " + String(dem3 > 0 ? "Đang chạy" : "Đã dừng");
    info += " | Cần quay: " + String(dem3) + " phút\n";

    // Đồng hồ 4
    info += "Mặt 4: ";
    if (data.unixtime_4 == 0) {
      info += "Chưa cài đặt.";
    } else {
      DateTime dt(data.unixtime_4);
      char buffer[9];
      sprintf(buffer, "%02d:%02d:%02d", dt.hour(), dt.minute(), dt.second());
      info += buffer;
    }
    info += " | Trạng thái: " + String(dem4 > 0 ? "Đang chạy" : "Đã dừng");
    info += " | Cần quay: " + String(dem4) + " phút\n";

    Blynk.virtualWrite(V3, info);
  } else if (dataS == "wifi") {
    long rssi = WiFi.RSSI();
    String wifi_rssi = "WiFi: " + String(rssi) + " dBm\n";
    Blynk.virtualWrite(V3, wifi_rssi);
  }
}
BLYNK_WRITE(V4) {
  if (param.asInt() == 0) {
    mode = 0;
  } else
    mode = 1;
}
//---------------------------------------------//-----------------------------------//------------------
// Hàm trợ giúp để in thời gian từ unixtime ra định dạng ngày/tháng/năm giờ:phút:giây
void print_clock_time(const char *clock_name, uint32_t unixtime) {
  Serial.print(clock_name);
  if (unixtime == 0) {
    Serial.println("Chưa cài đặt.");
    return;
  }
  DateTime dt(unixtime);
  char buffer[25];
  // Định dạng chuỗi thành "dd/mm/yyyy hh:mm:ss"
  sprintf(buffer, "%02d/%02d/%d %02d:%02d:%02d",
          dt.day(), dt.month(), dt.year(), dt.hour(), dt.minute(), dt.second());
  Serial.println(buffer);
}
//-------------------------
//-------------------------
void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(300000);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);

  timer.setTimeout(8000L, []() {
    Wire.begin();
    rtc_module.begin();
    ee.begin();
    cs.begin(ee, PAGE_SIZE, MEMORY_SIZE / PAGE_SIZE);
    // Đọc dữ liệu từ EEPROM và đồng bộ vào cả data và dataCheck
    cs.read(data);
    memcpy(&dataCheck, &data, sizeof(data));
    Serial.println("--- Thời gian đã lưu trong EEPROM khi khởi động ---");
    print_clock_time("Đồng hồ 1: ", data.unixtime_1);
    print_clock_time("Đồng hồ 2: ", data.unixtime_2);
    print_clock_time("Đồng hồ 3: ", data.unixtime_3);
    print_clock_time("Đồng hồ 4: ", data.unixtime_4);
    Serial.println("-------------------------------------------------");
    pcf8575_1.begin();

    pcf8575_1.pinMode(pin_motor_1, OUTPUT);
    pcf8575_1.digitalWrite(pin_motor_1, HIGH);
    pcf8575_1.pinMode(pin_motor_2, OUTPUT);
    pcf8575_1.digitalWrite(pin_motor_2, HIGH);
    pcf8575_1.pinMode(pin_motor_3, OUTPUT);
    pcf8575_1.digitalWrite(pin_motor_3, HIGH);
    pcf8575_1.pinMode(pin_motor_4, OUTPUT);
    pcf8575_1.digitalWrite(pin_motor_4, HIGH);

    attachInterrupt(D3, sensor_1, FALLING);
    attachInterrupt(D5, sensor_2, FALLING);
    attachInterrupt(D6, sensor_3, FALLING);
    attachInterrupt(D7, sensor_4, FALLING);

    timer.setInterval(61005, connectionstatus);
    timer.setInterval(5003, rtc_time);
    // Timer này chỉ giương cờ báo hiệu, không thực hiện đồng bộ trực tiếp
    timer.setInterval(86400000L, []() { rtc_sync_due = true; }); // 24 giờ
  });
}

void loop() {
  ESP.wdtFeed();
  Blynk.run();
  timer.run();
  // Nếu có yêu cầu lưu từ ISR, thực hiện việc lưu ở đây
  if (eeprom_save_request) {
    eeprom_save_request = false; // Reset cờ ngay lập tức
    savedata();
  }
}
