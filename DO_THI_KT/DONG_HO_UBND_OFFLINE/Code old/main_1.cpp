#define BLYNK_TEMPLATE_ID "TMPL6ZpS1D0RR"

#define BLYNK_TEMPLATE_NAME "ĐỒNG HỒ UBND"

#define BLYNK_AUTH_TOKEN "oe2-Gx_w4LSfr7Ie1OsuUByCAKnpaX_Q"

#define BLYNK_FIRMWARE_VERSION "250719"

#define BLYNK_PRINT Serial

#define APP_DEBUG

const char *ssid = "PBV UB";

const char *password = "kientuong2022";

// const char *ssid = "tram bom so 4";

// const char *password = "0943950555";

#include "myBlynkAir.h"

#include <BlynkSimpleEsp8266.h>

#include <ESP8266WiFi.h>

#include <Wire.h>

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

#define PAGE_SIZE 32

I2C_eeprom ee(0x57, MEMORY_SIZE);

struct Data {

  uint32_t unixtime_1, unixtime_2;

  uint32_t unixtime_3, unixtime_4;
};

I2C_eeprom_cyclic_store<Data> cs;

Data data, dataCheck;

const struct Data dataDefault = {0, 0, 0, 0};

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

const int M1_FWD_PIN = P7;

const int M1_REV_PIN = P6;

const int M2_FWD_PIN = P5;

const int M2_REV_PIN = P4;

const int M3_FWD_PIN = P3;

const int M3_REV_PIN = P2;

const int M4_FWD_PIN = P1;

const int M4_REV_PIN = P0;

const int MANUAL_ONLINE_PIN = 2; // Pin P15 trên PCF8575 để bật chế độ online thủ công

// --- Sensor Pins ---

const int SENSOR_1_PIN = D3;

const int SENSOR_2_PIN = D5;

const int SENSOR_3_PIN = D6;

const int SENSOR_4_PIN = D7;

// --- State Machine and Sync Logic ---

enum State { STATE_OFFLINE_RUNNING,

             STATE_SYNCING,

             STATE_MANUAL_ONLINE }; // Thêm trạng thái online thủ công

State currentState = STATE_OFFLINE_RUNNING;

const int SYNC_HOUR = 2;
// Giờ thực hiện đồng bộ (2 giờ sáng)

const int SYNC_MINUTE = 0;
// Phút thực hiện đồng bộ

bool dailySyncDone = false; // Cờ báo đã đồng bộ trong ngày hôm nay hay chưa

unsigned long lastAdjust = 0;

unsigned long syncStartTime = 0;
// Thời điểm bắt đầu quá trình đồng bộ

const unsigned long SYNC_TIMEOUT = 5 * 60 * 1000; // 5 phút timeout cho việc đồng bộ

bool syncCompletedSuccessfully = false;
// Cờ báo đồng bộ thành công

bool rtcSyncRequested = false;
// Cờ báo yêu cầu đồng bộ RTC

//-------------------

// Cờ báo hiệu từ ISR, phải là 'volatile'

volatile bool sensor_1_triggered = false;

volatile bool sensor_2_triggered = false;

volatile bool sensor_3_triggered = false;

volatile bool sensor_4_triggered = false;

byte reboot_num, face_clock = 0, mode = 0;

volatile int dem1 = 0, dem2 = 0, dem3 = 0, dem4 = 0;

volatile bool eeprom_save_request = false; // Cờ báo hiệu cần lưu vào EEPROM

uint32_t timestamp_now;

WidgetTerminal terminal(V3);

WidgetRTC rtc_widget;

// Khai báo trước (forward declaration) để BLYNK_CONNECTED có thể thấy hàm này

void print_clock_time(const char *clock_name, uint32_t unixtime);

BLYNK_CONNECTED() {

  Serial.println("Blynk connected.");

  rtc_widget.begin();

  rtcSyncRequested = true; // Giương cờ yêu cầu đồng bộ, không thực hiện ngay
}

ICACHE_RAM_ATTR void sensor_1() {

  if (dem1 > 0) {

    sensor_1_triggered = true;
  }
}

ICACHE_RAM_ATTR void sensor_2() {

  if (dem2 > 0) {

    sensor_2_triggered = true;
  }
}

ICACHE_RAM_ATTR void sensor_3() {

  if (dem3 > 0) {

    sensor_3_triggered = true;
  }
}

ICACHE_RAM_ATTR void sensor_4() {

  if (dem4 > 0) {

    sensor_4_triggered = true;
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

void update_started() {

  Serial.println("CALLBACK:  HTTP update process started");

  terminal.println("OTA Update: Process started...");

  terminal.flush();
}

void update_finished() {

  Serial.println("CALLBACK:  HTTP update process finished");

  terminal.println("OTA Update: Finished. Rebooting...");

  terminal.flush();
}

void update_progress(int cur, int total) {

  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);

  // Tránh làm ngập terminal, thay vào đó cập nhật tiến trình lên V0

  static int last_percent = -1;

  int percent = (cur * 100) / total;

  if (percent > last_percent && percent % 5 == 0) { // Cập nhật mỗi 5%

    char buffer[30];

    sprintf(buffer, "Updating: %d%%", percent);

    Blynk.virtualWrite(V0, buffer);

    last_percent = percent;
  }
}

void update_error(int err) {

  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);

  char buffer[50];

  sprintf(buffer, "OTA Update Error: %d", err);

  terminal.println(buffer);

  terminal.flush();
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

void disconnectWifiAndBlynk() {

  Blynk.disconnect();

  WiFi.disconnect(true);

  WiFi.mode(WIFI_OFF);

  Serial.println("WiFi and Blynk disconnected.");
}

void processRtcSync() {

  // Chỉ thực hiện khi có yêu cầu và năm từ Blynk đã hợp lệ (tránh giá trị rác khi chưa đồng bộ xong)

  if (rtcSyncRequested && year() > 2024) {

    Serial.println("Processing RTC sync request...");

    // 1. Lấy thời gian hiện tại từ server Blynk.

    DateTime blynkLocalTime(year(), month(), day(), hour(), minute(), second());

    uint32_t new_unixtime = blynkLocalTime.unixtime();

    // 2. Kiểm tra xem thời gian mới có hợp lệ so với thời gian đã lưu của các mặt đồng hồ không.

    bool isTimeValid = true;

    char reason[128]; // Buffer for the reason

    if (data.unixtime_1 != 0 && new_unixtime <= data.unixtime_1) {

      isTimeValid = false;

      snprintf(reason, sizeof(reason), "Sync time (%lu) <= Clock 1 time (%lu)", new_unixtime, data.unixtime_1);

    } else if (data.unixtime_2 != 0 && new_unixtime <= data.unixtime_2) {

      isTimeValid = false;

      snprintf(reason, sizeof(reason), "Sync time (%lu) <= Clock 2 time (%lu)", new_unixtime, data.unixtime_2);

    } else if (data.unixtime_3 != 0 && new_unixtime <= data.unixtime_3) {

      isTimeValid = false;

      snprintf(reason, sizeof(reason), "Sync time (%lu) <= Clock 3 time (%lu)", new_unixtime, data.unixtime_3);

    } else if (data.unixtime_4 != 0 && new_unixtime <= data.unixtime_4) {

      isTimeValid = false;

      snprintf(reason, sizeof(reason), "Sync time (%lu) <= Clock 4 time (%lu)", new_unixtime, data.unixtime_4);
    }

    if (isTimeValid) {

      // 3. Cập nhật RTC trực tiếp với thời gian đã nhận được

      rtc_module.adjust(blynkLocalTime);

      Serial.println("RTC time synced with Blynk server.");

      print_clock_time("Thời gian đã đồng bộ: ", new_unixtime);

      terminal.println("RTC sync successful.");

      terminal.flush();

      // Cập nhật cờ cho máy trạng thái

      if (currentState == STATE_SYNCING) {

        syncCompletedSuccessfully = true; // Đánh dấu đồng bộ thành công

      } else if (currentState == STATE_MANUAL_ONLINE) {

        terminal.println("--------------------");

        terminal.println("MANUAL ONLINE MODE ACTIVE");

        terminal.flush();
      }

    } else {

      // Thời gian không hợp lệ, hủy đồng bộ

      Serial.println("RTC sync aborted. Reason:");

      Serial.println(reason);

      terminal.println("RTC sync aborted: Time is not greater than a stored clock time.");

      terminal.flush();

      if (currentState == STATE_SYNCING) {

        // Coi như đồng bộ thất bại, quay về offline để tránh treo

        Serial.println("Sync process failed (invalid time). Returning to OFFLINE mode.");

        disconnectWifiAndBlynk();

        dailySyncDone = true; // Đánh dấu đã thử đồng bộ hôm nay để không thử lại ngay

        currentState = STATE_OFFLINE_RUNNING;
      }
    }

    // Hạ cờ yêu cầu trong mọi trường hợp (thành công hay thất bại)

    rtcSyncRequested = false;
  }
}

void handleManualOnlineMode() {

  // 1. Kết nối WiFi nếu chưa kết nối

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("Manual Online Mode: Connecting to WiFi & Blynk...");

    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);

    Blynk.config(BLYNK_AUTH_TOKEN);

    // Chờ một chút để WiFi bắt đầu kết nối

    unsigned long startConnectTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startConnectTime < 20000) { // Chờ tối đa 20s

      delay(500);

      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {

      Blynk.connect();

      Serial.println("\nWiFi connected for Manual Mode.");

    } else {

      Serial.println("\nWiFi connection failed. Will retry...");
    }
  }

  // 2. Chạy Blynk để nhận lệnh từ app

  Blynk.run();

  processRtcSync(); // Kiểm tra và thực hiện đồng bộ nếu dữ liệu thời gian đã sẵn sàng

  // Lưu ý: Không chạy logic điều khiển đồng hồ ở chế độ này.
}

void runClockAdjustmentLogic(); // Khai báo trước hàm

//-------------------------

void check_and_adjust_clock(uint32_t current_unixtime, uint32_t clock_unixtime,

                            volatile int &dem_counter,

                            const int fwd_pin, const int rev_pin) {

  // Chế độ 1: Cài đặt thủ công / Tạm dừng

  if (mode == 1) {

    dem_counter = 0;

    // Tắt cả hai chiều motor

    pcf8575_1.digitalWrite(fwd_pin, HIGH);

    pcf8575_1.digitalWrite(rev_pin, HIGH);

    return;
  }

  if (dem_counter > 0 || clock_unixtime == 0)

    return;

  const long SECONDS_IN_A_DAY = 86400L;

  long now_sec = current_unixtime % SECONDS_IN_A_DAY;

  long clk_sec = clock_unixtime % SECONDS_IN_A_DAY;

  long diff = now_sec - clk_sec;

  if (diff == 0) {

    // Đúng giờ, tắt cả hai chiều motor

    pcf8575_1.digitalWrite(fwd_pin, HIGH);

    pcf8575_1.digitalWrite(rev_pin, HIGH);

    return;
  }

  // Tính số phút cần quay tiến và quay lùi

  int forward_minutes = (diff > 0) ? diff / 60 : (SECONDS_IN_A_DAY + diff) / 60;

  int backward_minutes = (diff > 0) ? (SECONDS_IN_A_DAY - diff) / 60 : (-diff) / 60;

  // Nếu lệch nhỏ hơn 1 phút thì không cần chỉnh

  if (min(forward_minutes, backward_minutes) < 1) {

    pcf8575_1.digitalWrite(fwd_pin, HIGH);

    pcf8575_1.digitalWrite(rev_pin, HIGH);

    return;
  }

  // Tắt tất cả các chân điều khiển trước khi đổi chiều

  pcf8575_1.digitalWrite(fwd_pin, HIGH);

  pcf8575_1.digitalWrite(rev_pin, HIGH);

  delay(100); // Đợi 100ms để đảm bảo relay/mosfet đã nhả

  if (forward_minutes <= backward_minutes) {

    // Quay tiến là nhanh nhất

    dem_counter = forward_minutes;

    pcf8575_1.digitalWrite(rev_pin, HIGH);

    pcf8575_1.digitalWrite(fwd_pin, LOW); // Quay tiến

  } else {

    // Quay lùi là nhanh nhất

    dem_counter = backward_minutes;

    pcf8575_1.digitalWrite(fwd_pin, HIGH);

    pcf8575_1.digitalWrite(rev_pin, LOW); // Quay lùi
  }
}

void runClockAdjustmentLogic() {

  DateTime now = rtc_module.now();

  timestamp_now = now.unixtime();

  check_and_adjust_clock(timestamp_now, data.unixtime_1, dem1, M1_FWD_PIN, M1_REV_PIN);

  check_and_adjust_clock(timestamp_now, data.unixtime_2, dem2, M2_FWD_PIN, M2_REV_PIN);

  check_and_adjust_clock(timestamp_now, data.unixtime_3, dem3, M3_FWD_PIN, M3_REV_PIN);

  check_and_adjust_clock(timestamp_now, data.unixtime_4, dem4, M4_FWD_PIN, M4_REV_PIN);
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

  // Using a char buffer and snprintf is more memory-efficient on ESP8266

  // than concatenating String objects repeatedly.

  char infoBuffer[512];

  int offset = 0;

  String dataS = param.asStr();

  if (dataS == "update") {

    terminal.clear();

    terminal.println("Received 'update' command.");

    terminal.println("Starting OTA Firmware Update...");

    terminal.flush();

    // Thông báo trên V0 rằng quá trình cập nhật đang diễn ra

    Blynk.virtualWrite(V0, "Updating FW...");

    update_fw();

  } else if (dataS == "1") {

    terminal.clear();

    offset = snprintf(infoBuffer, sizeof(infoBuffer), "--- THÔNG TIN ĐỒNG HỒ ---\n\n");

    // Đồng hồ 1

    if (data.unixtime_1 == 0) {

      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 1: Chưa cài đặt.");

    } else {

      DateTime dt(data.unixtime_1);

      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 1: %02d:%02d", dt.hour(), dt.minute());
    }

    offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, " | Cần quay: %d phút\n", dem1);

    // Đồng hồ 2

    if (data.unixtime_2 == 0) {

      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 2: Chưa cài đặt.");

    } else {

      DateTime dt(data.unixtime_2);

      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 2: %02d:%02d", dt.hour(), dt.minute());
    }

    offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, " | Cần quay: %d phút\n", dem2);

    // Đồng hồ 3

    if (data.unixtime_3 == 0) {

      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 3: Chưa cài đặt.");

    } else {

      DateTime dt(data.unixtime_3);

      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 3: %02d:%02d", dt.hour(), dt.minute());
    }

    offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, " | Cần quay: %d phút\n", dem3);

    // Đồng hồ 4

    if (data.unixtime_4 == 0) {

      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 4: Chưa cài đặt.");

    } else {

      DateTime dt(data.unixtime_4);

      offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, "Mặt 4: %02d:%02d", dt.hour(), dt.minute());
    }

    offset += snprintf(infoBuffer + offset, sizeof(infoBuffer) - offset, " | Cần quay: %d phút\n", dem4);

    Blynk.virtualWrite(V3, infoBuffer);

  } else if (dataS == "wifi") {

    long rssi = WiFi.RSSI();

    snprintf(infoBuffer, sizeof(infoBuffer), "WiFi: %ld dBm\n", rssi);

    Blynk.virtualWrite(V3, infoBuffer);
  }
}

BLYNK_WRITE(V4) {

  if (param.asInt() == 0) {

    mode = 0;

  } else

    mode = 1;
}

// Hàm kiểm tra kết nối I2C tới module DS3231

bool isRtcConnected() {

  // Địa chỉ I2C mặc định của DS3231 là 0x68

  Wire.beginTransmission(0x68);

  byte error = Wire.endTransmission();

  if (error == 0) {

    return true; // Tìm thấy thiết bị
  }

  // Lỗi 2: Address NACK - không tìm thấy thiết bị

  // Lỗi 3: Data NACK

  // Lỗi 4: Lỗi không xác định khác

  Serial.printf("RTC I2C connection error: %d\n", error);

  return false; // Không tìm thấy thiết bị
}

// Hàm kiểm tra kết nối I2C tới module EEPROM

bool isEepromConnected() {

  // Địa chỉ I2C mặc định của EEPROM là 0x50

  Wire.beginTransmission(0x57);

  byte error = Wire.endTransmission();

  if (error == 0) {

    return true; // Tìm thấy thiết bị
  }

  Serial.printf("EEPROM I2C connection error: %d\n", error);

  return false; // Không tìm thấy thiết bị
}

// Hàm kiểm tra kết nối I2C tới module PCF8575

bool isPcfConnected(uint8_t address) {

  Wire.beginTransmission(address);

  byte error = Wire.endTransmission();

  if (error == 0) {

    return true; // Tìm thấy thiết bị
  }

  return false; // Không tìm thấy thiết bị
}

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

void handleEepromSave() {

  // Nếu có yêu cầu lưu từ ISR, thực hiện việc lưu ở đây

  if (eeprom_save_request) {

    savedata();

    eeprom_save_request = false; // Reset cờ ngay lập tức
  }
}

void handleOfflineMode() {

  // Kiểm tra kết nối RTC trước khi thực hiện bất kỳ logic nào liên quan đến thời gian

  if (!isRtcConnected()) {

    return;
  }

  // 1. Chạy logic điều khiển đồng hồ

  runClockAdjustmentLogic();

  // 2. Kiểm tra xem đã đến giờ đồng bộ chưa

  DateTime now = rtc_module.now();

  if (now.hour() == SYNC_HOUR && now.minute() >= SYNC_MINUTE && !dailySyncDone) {

    Serial.println("It's time to sync. Entering SYNCING state.");

    currentState = STATE_SYNCING;

    syncStartTime = millis();
    // Bắt đầu đếm giờ cho timeout

    syncCompletedSuccessfully = false; // Reset cờ trạng thái
  }

  // 3. Reset cờ dailySyncDone khi đã qua giờ đồng bộ, chuẩn bị cho ngày hôm sau

  if (now.hour() != SYNC_HOUR && dailySyncDone) {

    dailySyncDone = false;
  }
}

void handleSyncMode() {

  // 1. Kiểm tra timeout

  if (millis() - syncStartTime > SYNC_TIMEOUT) {

    Serial.println("Sync process timed out. Returning to OFFLINE mode.");

    disconnectWifiAndBlynk();

    dailySyncDone = true; // Đánh dấu đã thử đồng bộ hôm nay (dù thất bại) để không thử lại ngay

    currentState = STATE_OFFLINE_RUNNING;

    return;
  }

  // 2. Kết nối WiFi nếu chưa kết nối

  if (WiFi.status() != WL_CONNECTED) {

    Serial.print("Connecting to WiFi...");

    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);

    Blynk.config(BLYNK_AUTH_TOKEN); // Cấu hình Blynk nhưng chưa kết nối

    // Chờ một chút để WiFi bắt đầu kết nối

    unsigned long startConnectTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startConnectTime < 20000) { // Chờ tối đa 20s

      delay(500);

      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {

      Serial.println("\nWiFi connected.");

    } else {

      Serial.println("\nWiFi connection failed.");

      // Sẽ bị timeout ở lần lặp tiếp theo nếu vẫn không kết nối được

      return;
    }
  }

  // 3. Chạy Blynk (sẽ tự động kết nối nếu WiFi đã sẵn sàng)

  Blynk.run();

  processRtcSync(); // Kiểm tra và thực hiện đồng bộ nếu dữ liệu thời gian đã sẵn sàng

  // 4. Nếu đồng bộ thành công (cờ được bật trong BLYNK_CONNECTED)

  if (syncCompletedSuccessfully) {

    Serial.println("Sync successful. Disconnecting and returning to OFFLINE mode.");

    terminal.println("Sync process complete.");

    terminal.println("Returning to offline mode.");

    terminal.flush();

    Blynk.virtualWrite(V0, "Synced OK"); // Gửi thông báo lên App

    // Tăng nhẹ delay để đảm bảo tất cả các message trên terminal đã được gửi đi

    delay(1500);

    disconnectWifiAndBlynk();

    dailySyncDone = true; // Đánh dấu đã đồng bộ xong cho ngày hôm nay

    currentState = STATE_OFFLINE_RUNNING;

    // Chạy lại logic điều chỉnh đồng hồ một lần nữa với thời gian mới nhất

    runClockAdjustmentLogic();
  }
}

void handleSensorTriggers() {

  if (sensor_1_triggered) {

    sensor_1_triggered = false; // Hạ cờ ngay lập tức

    if (dem1 > 0) {

      dem1--;

      data.unixtime_1 += 60; // Tăng 60 giây (1 phút) cho đồng hồ

      eeprom_save_request = true;
    }

    if (dem1 <= 0) {

      // Dừng motor 1 (tắt cả hai chiều)

      pcf8575_1.digitalWrite(M1_FWD_PIN, HIGH);

      pcf8575_1.digitalWrite(M1_REV_PIN, HIGH);
    }
  }

  if (sensor_2_triggered) {

    sensor_2_triggered = false;

    if (dem2 > 0) {

      dem2--;

      data.unixtime_2 += 60; // Tăng 60 giây (1 phút) cho đồng hồ

      eeprom_save_request = true;
    }

    if (dem2 <= 0) {

      // Dừng motor 2 (tắt cả hai chiều)

      pcf8575_1.digitalWrite(M2_FWD_PIN, HIGH);

      pcf8575_1.digitalWrite(M2_REV_PIN, HIGH);
    }
  }

  if (sensor_3_triggered) {

    sensor_3_triggered = false;

    if (dem3 > 0) {

      dem3--;

      data.unixtime_3 += 60; // Tăng 60 giây (1 phút) cho đồng hồ

      eeprom_save_request = true;
    }

    if (dem3 <= 0) {

      // Dừng motor 3 (tắt cả hai chiều)

      pcf8575_1.digitalWrite(M3_FWD_PIN, HIGH);

      pcf8575_1.digitalWrite(M3_REV_PIN, HIGH);
    }
  }

  if (sensor_4_triggered) {

    sensor_4_triggered = false;

    if (dem4 > 0) {

      dem4--;

      data.unixtime_4 += 60; // Tăng 60 giây (1 phút) cho đồng hồ

      eeprom_save_request = true;
    }

    if (dem4 <= 0) {

      // Dừng motor 4 (tắt cả hai chiều)

      pcf8575_1.digitalWrite(M4_FWD_PIN, HIGH);

      pcf8575_1.digitalWrite(M4_REV_PIN, HIGH);
    }
  }
}

//-------------------------

//-------------------------

void setup() {

  Wire.begin();

  pcf8575_1.begin();

  pcf8575_1.pinMode(P0, OUTPUT);

  pcf8575_1.digitalWrite(P0, HIGH);

  pcf8575_1.pinMode(P1, OUTPUT);

  pcf8575_1.digitalWrite(P1, HIGH);

  pcf8575_1.pinMode(P2, OUTPUT);

  pcf8575_1.digitalWrite(P2, HIGH);

  pcf8575_1.pinMode(P3, OUTPUT);

  pcf8575_1.digitalWrite(P3, HIGH);

  pcf8575_1.pinMode(P4, OUTPUT);

  pcf8575_1.digitalWrite(P4, HIGH);

  pcf8575_1.pinMode(P5, OUTPUT);

  pcf8575_1.digitalWrite(P5, HIGH);

  pcf8575_1.pinMode(P6, OUTPUT);

  pcf8575_1.digitalWrite(P6, HIGH);

  pcf8575_1.pinMode(P7, OUTPUT);

  pcf8575_1.digitalWrite(P7, HIGH);

  pcf8575_1.pinMode(P8, OUTPUT);

  pcf8575_1.digitalWrite(P8, HIGH);

  pcf8575_1.pinMode(P9, OUTPUT);

  pcf8575_1.digitalWrite(P9, HIGH);

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

  delay(5000);

  ESP.wdtDisable();

  ESP.wdtEnable(300000);

  Serial.begin(115200);

  rtc_module.begin();

  ee.begin();

  cs.begin(ee, PAGE_SIZE, MEMORY_SIZE / PAGE_SIZE);

  // Đọc dữ liệu từ EEPROM và đồng bộ vào cả data và dataCheck

  cs.read(data);

  memcpy(&dataCheck, &data, sizeof(data));

  Serial.println("-----------------------------------");

  Serial.println("\n--- Check Module I2C ---");

  while (!isRtcConnected()) {

    Serial.println("DS3231 not found! Please check wiring. Retrying in 5s...");

    delay(5000);
  }

  Serial.println("DS3231 OK!");

  // Kiểm tra kết nối I2C với EEPROM

  while (!isEepromConnected()) {

    Serial.println("EEPROM not found! Please check wiring. Retrying in 5s...");

    delay(5000);
  }

  Serial.println("EEPROM OK!");

  // Kiểm tra kết nối I2C với PCF8575

  // Thay thế pcf8575_1.isConnected() bằng hàm kiểm tra thủ công với địa chỉ 0x20

  if (!isPcfConnected(0x20)) {

    Serial.println("PCF8575 not found! Please check wiring. Halting.");

    while (1)

      delay(1000); // Dừng chương trình nếu không có I/O expander
  }

  Serial.println("PCF8575 OK!");

  // Kiểm tra kết nối I2C với RTC DS3231

  DateTime now = rtc_module.now(); // Lấy thời gian hiện tại từ module RTC

  Serial.println("--- Last saved time from EEPROM ---");

  print_clock_time("Clock 1: ", data.unixtime_1);

  print_clock_time("Clock 2: ", data.unixtime_2);

  print_clock_time("Clock 3: ", data.unixtime_3);

  print_clock_time("Clock 4: ", data.unixtime_4);

  print_clock_time("Thời gian RTC hiện tại: ", now.unixtime());

  // Cấu hình chân P15 làm input với điện trở kéo lên. Kéo xuống GND để kích hoạt.

  pinMode(MANUAL_ONLINE_PIN, INPUT_PULLUP);

  pinMode(D0, OUTPUT);

  digitalWrite(D0, HIGH);

  attachInterrupt(digitalPinToInterrupt(SENSOR_1_PIN), sensor_1, RISING);

  attachInterrupt(digitalPinToInterrupt(SENSOR_2_PIN), sensor_2, RISING);

  attachInterrupt(digitalPinToInterrupt(SENSOR_3_PIN), sensor_3, RISING);

  attachInterrupt(digitalPinToInterrupt(SENSOR_4_PIN), sensor_4, RISING);

  currentState = STATE_OFFLINE_RUNNING;

  Serial.println("Initialization complete. Entering OFFLINE mode.");

  Serial.println("-----------------------------------");
}

void loop() {

  ESP.wdtDisable();

  ESP.wdtEnable(300000);

  // Chế độ online được kích hoạt khi chân D4 để hở (HIGH)

  static unsigned long lastManualCheck = 0;

  if (millis() - lastManualCheck > 1000) { // Kiểm tra mỗi 1 giây

    lastManualCheck = millis();

    bool manualModeActive = (digitalRead(MANUAL_ONLINE_PIN) == LOW);

    // Chuyển từ các chế độ khác sang MANUAL_ONLINE nếu D4 LOW

    if (manualModeActive && currentState != STATE_MANUAL_ONLINE) {

      Serial.println("D4 LOW: Kích hoạt chế độ ONLINE thủ công.");

      // Dừng ngay lập tức tất cả các motor

      pcf8575_1.digitalWrite(M1_FWD_PIN, HIGH);

      pcf8575_1.digitalWrite(M1_REV_PIN, HIGH);

      pcf8575_1.digitalWrite(M2_FWD_PIN, HIGH);

      pcf8575_1.digitalWrite(M2_REV_PIN, HIGH);

      pcf8575_1.digitalWrite(M3_FWD_PIN, HIGH);

      pcf8575_1.digitalWrite(M3_REV_PIN, HIGH);

      pcf8575_1.digitalWrite(M4_FWD_PIN, HIGH);

      pcf8575_1.digitalWrite(M4_REV_PIN, HIGH);

      // Reset bộ đếm để không chạy lại khi thoát chế độ

      dem1 = 0;

      dem2 = 0;

      dem3 = 0;

      dem4 = 0;

      currentState = STATE_MANUAL_ONLINE;

    }

    // Chuyển từ MANUAL_ONLINE về OFFLINE nếu D4 HIGH

    else if (!manualModeActive && currentState == STATE_MANUAL_ONLINE) { // Nếu D4 HIGH và đang ở MANUAL_ONLINE

      Serial.println("D4 LOW: Thoát chế độ ONLINE.");

      disconnectWifiAndBlynk(); // Ngắt kết nối WiFi để tiết kiệm năng lượng

      currentState = STATE_OFFLINE_RUNNING;

      // Chạy lại logic điều chỉnh một lần để đảm bảo đồng hồ đúng vị trí sau khi cài đặt

      runClockAdjustmentLogic();
    }
  }

  // --- Thực thi logic dựa trên trạng thái hiện tại ---

  switch (currentState) {

  case STATE_OFFLINE_RUNNING:

    if (millis() - lastAdjust > 2000) {

      handleOfflineMode();

      lastAdjust = millis();
    }

    break;

  case STATE_SYNCING:

    handleSyncMode();

    break;

  case STATE_MANUAL_ONLINE:

    handleManualOnlineMode();

    break;
  }

  // Xử lý các sự kiện từ cảm biến một cách an toàn

  handleSensorTriggers();

  // Chỉ xử lý lưu EEPROM khi ở chế độ offline để tránh ghi không cần thiết

  if (currentState == STATE_OFFLINE_RUNNING) {

    handleEepromSave();
  }

  /*

 pcf8575_1.digitalWrite(M1_FWD_PIN, HIGH);

 pcf8575_1.digitalWrite(M1_REV_PIN, HIGH);

 pcf8575_1.digitalWrite(M2_FWD_PIN, HIGH);

 pcf8575_1.digitalWrite(M2_REV_PIN, HIGH);

 pcf8575_1.digitalWrite(M3_FWD_PIN, HIGH);

 pcf8575_1.digitalWrite(M3_REV_PIN, HIGH);

 pcf8575_1.digitalWrite(M4_FWD_PIN, HIGH);

 pcf8575_1.digitalWrite(M4_REV_PIN, HIGH);

 delay(3000);



 pcf8575_1.digitalWrite(M4_REV_PIN, LOW);

 delay(3000);

 pcf8575_1.digitalWrite(M4_REV_PIN, HIGH);

 delay(3000);

 pcf8575_1.digitalWrite(M4_FWD_PIN, LOW);

 delay(3000);

 pcf8575_1.digitalWrite(M4_FWD_PIN, HIGH);

 delay(3000);

 pcf8575_1.digitalWrite(M3_REV_PIN, LOW);

 delay(3000);

 pcf8575_1.digitalWrite(M3_REV_PIN, HIGH);

 delay(3000);

 pcf8575_1.digitalWrite(M3_FWD_PIN, LOW);

 delay(3000);

 pcf8575_1.digitalWrite(M3_FWD_PIN, HIGH);

 delay(3000);

 pcf8575_1.digitalWrite(M2_REV_PIN, LOW);

 delay(3000);

 pcf8575_1.digitalWrite(M2_REV_PIN, HIGH);

 delay(3000);

 pcf8575_1.digitalWrite(M2_FWD_PIN, LOW);

 delay(3000);

 pcf8575_1.digitalWrite(M2_FWD_PIN, HIGH);

 delay(3000);

 pcf8575_1.digitalWrite(M1_REV_PIN, LOW);

 delay(3000);

 pcf8575_1.digitalWrite(M1_REV_PIN, HIGH);

 delay(3000);

 pcf8575_1.digitalWrite(M1_FWD_PIN, LOW);

 delay(3000);

 pcf8575_1.digitalWrite(M1_FWD_PIN, HIGH);

 delay(3000);

 */
}