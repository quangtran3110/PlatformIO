#define BLYNK_TEMPLATE_ID "TMPL6Px18Gsjk"
#define BLYNK_TEMPLATE_NAME "TRẠM 3 VFD"
#define BLYNK_AUTH_TOKEN "eXmsWQOmDdHaBMALIxHJqhbJXtzg8Gw1"
#define BLYNK_FIRMWARE_VERSION "250922"
//------------------
#define APP_DEBUG
#define BLYNK_PRINT Serial
// --- Helper macros to use enums with BLYNK_WRITE ---
// This correctly handles macro expansion before token pasting
#define BLYNK_PASTE_IMPL(a, b) a##b
#define BLYNK_PASTE(a, b) BLYNK_PASTE_IMPL(a, b)
#define BLYNK_TO_V(pin) BLYNK_PASTE(V, pin)
#define BLYNK_WRITE_VP(pin_enum) BLYNK_WRITE(BLYNK_TO_V(pin_enum))
#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
// const char *ssid = "tram bom so 4";
// const char *password = "0943950555";
const char *ssid = "TTTV Xay Dung";
const char *password = "0723841249";

const float PRESSURE_SENSOR_MAX_RANGE = 10.0f; // Thang đo tối đa của cảm biến áp suất (bar)
//------------------
#include "EmonLib.h"
EnergyMonitor emon0, emon1, emon2;
float Irms2;
//------------------
#include "RTClib.h"
#include <SPI.h>
#include <WidgetRTC.h>
#include <Wire.h>
RTC_DS3231 rtc_module;
WidgetRTC rtc_widget;
//------------------
#include <ModbusRTU.h>
#include <SoftwareSerial.h>
SoftwareSerial S(13, 12);
ModbusRTU mb;
//------------------
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <UrlEncode.h>
#include <WiFiClientSecure.h>
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/refs/heads/main/Tram_So_3%20_VFD/.pio/build/nodemcuv2/firmware.bin"
String server_main = "http://sgp1.blynk.cloud/external/api/";
WiFiClient client;
HTTPClient http;
//------------------
#include <I2C_eeprom.h>
#include <I2C_eeprom_cyclic_store.h>
#define MEMORY_SIZE 4096
#define PAGE_SIZE 32
I2C_eeprom ee(0x57, MEMORY_SIZE);
//------------------
#include <DallasTemperature.h>
#include <OneWire.h>
OneWire oneWire(D3);
DallasTemperature sensors(&oneWire);
float temp;
//------------------
// Sử dụng bitfield để nén các cờ vào một byte duy nhất
struct Flags {
  uint8_t mode_man_auto : 1;    // 0: MAN, 1: AUTO
  uint8_t mode_follow_tank : 1; // 0: Tắt, 1: Bật
  uint8_t key_noti : 1;         // 0: Tắt, 1: Bật
  uint8_t key_protect : 1;      // 0: Tắt, 1: Bật
  uint8_t rualoc : 1;           // 0: Tắt, 1: Bật
};

struct Data {
  uint16_t LLG1_RL;
  uint16_t t1_start, t1_stop;
  byte SetAmpemax, SetAmpemin;
  byte SetAmpe1max, SetAmpe1min;
  byte SetAmpe2max, SetAmpe2min;
  Flags flags;
  // Thay vì lưu 3 điểm hiệu chuẩn, ta tính toán và lưu 2 hệ số: offset và gain.
  // Dùng uint16_t và nhân với 100 để tiết kiệm bộ nhớ (fixed-point).
  uint16_t pressure_cal_offset_x100; // Giá trị đọc được ở điểm 0 (V * 100). Vd: 2.05V -> 205
  uint16_t pressure_cal_gain_x1000;  // Hệ số khuếch đại (gain * 1000). Vd: gain 1.25 -> 1250
  uint16_t level_cal_offset_x100;    // Giá trị đọc được ở điểm 0 (ADC * 100). Vd: ADC 194.5 -> 19450
  uint16_t level_cal_gain_x1000;     // Hệ số khuếch đại (gain * 1000). Vd: gain 0.602 -> 602
  byte pre_setpoint_x10;             // Điểm đặt áp suất (x10, vd: 2.2 bar -> 22)
  byte level_setpoint;               // Mức nước cài đặt (0-100%)
};
I2C_eeprom_cyclic_store<Data> cs;
Data edata, dataCheck;
const struct Data dataDefault = {};
//------------------
#include "PCF8575.h"
PCF8575 pcf8575_1(0x20);

const int pin_G1 = P7;
const int pin_B1 = P6;
const int pin_NK1 = P5;
const int pin_P4 = P4;
const int pin_P3 = P3;
const int pin_fan = P2;
const int pin_P1 = P1;
const int pin_P0 = P0;

const int S0pin = P15;
const int S1pin = P14;
const int S2pin = P13;
const int S3pin = P12;
//----Bế chứa--------------
#include <SimpleKalmanFilter.h>
long t;
float volume, smoothDistance;
float pi = 3.14;
float bankinh2 = 240 * 240;
const uint8_t dosau = 210;
int timer_up;
byte status_g1 = 3;
byte status_b1 = 3;
byte status_nk1 = 3;
byte status_fan = 3;
bool trip0 = false, trip1 = false, trip2 = false;
bool key = false;
bool maxtank = false, blynk_first_connect = false;
byte c, i = 0;
int xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0;
int LLG1_1m3, reboot_num = 0;
String sta_vfd_g1, sta_vfd_b1;
// Biến lưu trạng thái trước đó để tối ưu việc gửi dữ liệu lên Blynk
struct PrevState {
  String sta_vfd_g1;
  String sta_vfd_b1;
  float vfd_b1_freq = -1.0;
  float vfd_b1_current = -1.0;
  float vfd_b1_feedback = -1.0;
  float vfd_g1_freq = -1.0;
  float vfd_g1_current = -1.0;
  float smoothDistance = -1.0;
  float volume = -1.0;
  float Irms2 = -1.0;
  float temp = -127.0; // Giá trị nhiệt độ hợp lệ từ cảm biến DallasTemperature nằm trong khoảng -55 đến 125 độ C

  // Timestamps for forced updates
  unsigned long sta_vfd_g1_ts = 0;
  unsigned long sta_vfd_b1_ts = 0;
  unsigned long vfd_b1_freq_ts = 0;
  unsigned long vfd_b1_current_ts = 0;
  unsigned long vfd_b1_feedback_ts = 0;
  unsigned long vfd_g1_freq_ts = 0;
  unsigned long vfd_g1_current_ts = 0;
  unsigned long smoothDistance_ts = 0;
  unsigned long volume_ts = 0;
  unsigned long Irms2_ts = 0;
  unsigned long temp_ts = 0;
} prevState;
unsigned long int xIrms0 = 0, xIrms1 = 0, xIrms2 = 0;
unsigned long int yIrms0 = 0, yIrms1 = 0, yIrms2 = 0;
//---Chức năng chân ảo Blynk---

// --- PD Controller for Water Level ---
const float PD_KP = 2.0f;                        // Tỷ lệ: Tăng Kp để phản ứng mạnh hơn với sai lệch mực nước.
const float PD_KD = 5.0f;                        // Vi phân: Tăng Kd để phản ứng mạnh hơn với tốc độ thay đổi mực nước.
const float MAX_FREQ_CHANNEL_B = 25.0f;          // Tần số tối đa cho kênh B (Hz) theo đề xuất
const float PD_DEADBAND_PERCENT = 2.0f;          // Khoảng an toàn (%), bộ điều khiển sẽ không hoạt động trong khoảng này.
const unsigned long PD_CONTROL_INTERVAL = 10000; // Chạy logic điều khiển mỗi 10 giây

unsigned long last_pd_control_time = 0;
float previous_level_percent = -1.0f; // Khởi tạo với giá trị không hợp lệ
uint16_t last_written_freq_b = 0;     // Giá trị tần số kênh B cuối cùng đã ghi
//------------------
uint16_t current_vfd_b1_freq_channel_b = 0; // Biến RAM để lưu tần số kênh B, không lưu vào EEPROM

#define V_BTN_G1 0            // Nút bật/tắt giếng 1
#define V_BTN_B1 1            // Nút bật/tắt bơm 1
#define V_BTN_NK1 2           // Nút bật/tắt nén khí
#define V_STA_VFD_G1 3        // Trạng thái biến tần giếng 1
#define V_STA_VFD_B1 4        // Trạng thái biến tần bơm 1
#define V_VFD_B1_CURRENT 5    // Dòng biến tần bơm 1
#define V_VFD_B1_FREQ 6       // Tần số biến tần bơm 1
#define V_VFD_B1_FEEDBACK 7   // Tín hiệu hồi tiếp biến tần bơm 1
#define V_VFD_G1_CURRENT 8    // Dòng biến tần giếng 1
#define V_VFD_G1_FREQ 9       // Tần số biến tần giếng 1
#define V_INFO 10             // Info
#define V_TERMINAL 11         // Terminal
#define V_CONLAI 12           // Nước còn lại
#define V_MODE_MAN_AUTO 13    // Chế độ vận hành cáp 2
#define V_MODE_FOLLOW_TANK 14 // Chế độ chạy theo bể
#define V_MENU_MOTOR 15       // Menu chọn máy bảo vệ
#define V_INPUT_MIN_A 16      // Cài đặt dòng min A
#define V_INPUT_MAX_A 17      // Cài đặt dòng max A
#define V_TIMEINPUT 18        // Time input
#define V_PROTECT 19          // Bảo vệ
#define V_THONGBAO 20         // Thông báo
#define V_RUALOC 21           // Rửa lọc
#define V_LL_RUALOC 22        // LL rửa lọc
#define V_LLG1_1M3 23         // LLG1_1m3
#define V_LLG1_24H 24         // LLG1_24
#define V_THETICH 25          // Thể tích
#define V_NENKHI1_CURRENT 26  // Dòng điện máy nén khí
#define V_PRE_SETPOINT 27     // Setpoint áp suất
#define V_LEVEL_SETPOINT 28   // Setpoint mực nước
#define V_WIFI_SIGNAL 29      // Wifi signal
#define V_TEMP 30             // Nhiệt độ
#define V_TEMINAL_VOLUME 31   // Terminal volume

//------------------
WidgetTerminal terminal(V_TERMINAL);
BlynkTimer timer;
BLYNK_CONNECTED() {
  rtc_widget.begin();
  blynk_first_connect = true;
  Blynk.virtualWrite(V_PRE_SETPOINT, edata.pre_setpoint_x10 / 10.0f); // Cập nhật giá trị setpoint lên Blynk (đơn vị bar)
}
// -- -Modbus VFD Communication-- -
// Hằng số chung

const uint16_t VFD_STATUS_REG = 0x2100;
const uint16_t VFD_PARAMS_REG_START = 0x3000;
const uint16_t VFD_STATUS_REG_COUNT = 3; // Đọc 3 thanh ghi: 0x2100 (Trạng thái), 0x2101 (Bỏ qua), 0x2102 (Mã lỗi)
const uint16_t VFD_FREQ_CHANNEL_B = 0x2001;
const uint16_t VFD_SETPOINT_REG = 0x2002;
const uint16_t VFD_FREQ_SETPOINT_COUNT = 2; // Đọc 2 thanh ghi: 0x2001 (Tần số B), 0x2002 (Setpoint)

// Cấu hình và bộ đệm cho từng biến tần
const uint16_t VFD_G1_SLAVE_ID = 1;
uint16_t vfd_g1_data_status[VFD_STATUS_REG_COUNT];

const uint16_t VFD_B1_SLAVE_ID = 2;
uint16_t vfd_b1_data_status[VFD_STATUS_REG_COUNT];
uint16_t vfd_b1_data_pid[VFD_FREQ_SETPOINT_COUNT];

const uint16_t VFD_G1_PARAMS_TO_READ = 5;
uint16_t vfd_g1_params_data[VFD_G1_PARAMS_TO_READ];

const uint16_t VFD_B1_PARAMS_TO_READ = 13;
uint16_t vfd_b1_params_data[VFD_B1_PARAMS_TO_READ];

// Biến lưu trữ giá trị thực tế của các thông số
float vfd_g1_freq = 0.0, vfd_g1_current = 0.0;
float vfd_b1_freq = 0.0, vfd_b1_current = 0.0, vfd_b1_feedback = 0.0;
uint16_t vfd_b1_setpoint = 0;
// Danh sách mã lỗi và trạng thái (dùng chung cho cả 2 biến tần)
const char *fault_code_list[] = {
    "None", // 0 (không dùng)
    "OUt1", "OUt2", "OUt3", "OV1", "OV2", "OV3", "OC1", "OC2", "OC3", "UV",
    "OL1", "OL2", "SPI", "SPO", "OH1", "OH2", "EF", "CE", "ItE", "tE",
    "EEP", "PIDE", "bCE", "END", "OL3", "PCE", "UPE", "DNE", "ETH1", "ETH2",
    "dEu", "STo", "LL", "OT", "E-Err", "F1-Er", "F2-Er", "C1-Er", "C2-Er",
    "E-DP", "E-NET", "E-CAN", "E-PN", "E-CAT", "E-BAC", "E-DEV", "ESCAN",
    "S-Err", "FrOST", "BLOCK", "Dr"};
const char *status_vdf_list[] = {
    "None",            // 0 (không dùng)
    "Đang chạy",       // 1
    "Reverse running", // 2
    "Dừng",            // 3
    "Fault",           // 4
    "PoFF",            // 5
    "Pre-exciting"     // 6
};

/**
 * @brief Hàm chung để xử lý dữ liệu từ biến tần và cập nhật chuỗi trạng thái.
 * @param data_buffer Con trỏ tới bộ đệm chứa dữ liệu Modbus.
 * @param status_string Tham chiếu đến chuỗi trạng thái sẽ được cập nhật.
 */
void processVfdData(uint16_t *data_buffer, String &status_string) {
  uint16_t status_code = data_buffer[0]; // Trạng thái nằm ở thanh ghi đầu tiên (0x2100)
  uint16_t fault_code = data_buffer[2];  // Mã lỗi nằm ở thanh ghi thứ ba (0x2102)

  // Kiểm tra nếu biến tần báo lỗi
  if (status_code == 4) { // 4 = "Fault"
    size_t fault_list_size = sizeof(fault_code_list) / sizeof(fault_code_list[0]);
    if (fault_code > 0 && fault_code < fault_list_size) {
      status_string = "Lỗi: " + String(fault_code_list[fault_code]);
    } else {
      status_string = "Lỗi không xác định";
    }
  } else { // Biến tần hoạt động bình thường
    size_t status_list_size = sizeof(status_vdf_list) / sizeof(status_vdf_list[0]);
    if (status_code > 0 && status_code < status_list_size) {
      status_string = status_vdf_list[status_code];
    } else {
      status_string = "Trạng thái không xác định";
    }
  }
  // Serial.printf("VFD Status Code: %d, Fault Code: %d, Status: %s\n", status_code, fault_code, status_string.c_str());
}

// Phản hồi 1 - Đọc trạng thái biến tần 1 (Giếng)
bool cbReadVfdg1Data(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    processVfdData(vfd_g1_data_status, sta_vfd_g1);
  }
  i++;
  return true;
}
// Phản hồi 2 - Đọc thông số biến tần 1 (Giếng)
bool cbReadVfdg1Params(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    // 0x3000 - Tần số: index 0, scale 0.01
    vfd_g1_freq = vfd_g1_params_data[0] / 100.0f;
    // 0x3004 - Dòng điện: index 4, scale 0.1
    vfd_g1_current = vfd_g1_params_data[4] / 10.0f;

    // Serial.printf("VFD Giếng 1 Params: Freq=%.2f Hz, Current=%.1f A\n", vfd_g1_freq, vfd_g1_current);
  }
  i++;
  return true;
}
// Phản hồi 3 - Đọc trạng thái biến tần 2 (Bơm)
bool cbReadVfdb1Data(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    processVfdData(vfd_b1_data_status, sta_vfd_b1);
  }
  // Lệnh 4 - Sau khi đọc trạng thái xong thì đọc thông số biến tần 2 (Bơm)
  i++;
  return true;
}
// Phản hồi 4 - Đọc thông số biến tần 2 (Bơm)
bool cbReadVfdb1Params(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    // 0x3000 - Tần số: index 0, scale 0.01
    vfd_b1_freq = vfd_b1_params_data[0] / 100.0f;
    // 0x3004 - Dòng điện: index 4, scale 0.1
    vfd_b1_current = vfd_b1_params_data[4] / 10.0f;
    // 0x300C - Phản hồi PID (cảm biến áp suất): index 12, scale 0.01
    // Giá trị đọc từ thanh ghi 0x300C là giá trị điện áp (0-1000) tương ứng 0-10V.
    float voltage = vfd_b1_params_data[12] / 100.0f;

    // --- ÁP DỤNG CÔNG THỨC HIỆU CHUẨN TỪ OFFSET VÀ GAIN ---
    // Công thức: Pressure = gain * (V_current - offset)
    float gain = edata.pressure_cal_gain_x1000 / 1000.0f;
    float offset = edata.pressure_cal_offset_x100 / 100.0f;
    if (gain > 0) {
      float pressure = gain * (voltage - offset);
      vfd_b1_feedback = constrain(pressure, -1.0f, PRESSURE_SENSOR_MAX_RANGE * 1.1); // Cho phép giá trị âm nhỏ để gỡ lỗi
    } else {
      vfd_b1_feedback = 0.0f;
    }
    // Serial.printf("VFD Bơm 1 Params: Freq=%.2f Hz, Current=%.1f A, Feedback=%.2f\n", vfd_b1_freq, vfd_b1_current, vfd_b1_feedback);
  }
  // Lệnh 5 - Sau khi đọc xong thông số biến tần 2 thì đọc tần số kênh B và setpoint
  i++;
  return true;
}
// Phản hồi 5 - Đọc tần số kênh B và setpoint biến tần 2 (Bơm)
bool cbWritevfdb1pid(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
  }
  i++;
  return true;
}
// Generic callback for Modbus write operations
bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event != Modbus::EX_SUCCESS) {
    Serial.printf("Modbus write failed, event: 0x%02X\n", event);
  } else {
    // Serial.println("Modbus write successful.");
  }
  i = 0; // Reset để bắt đầu chu kỳ mới
  return true;
}
//-------------------------------------------------------------------
void connectionstatus() {
  if ((WiFi.status() != WL_CONNECTED)) {
    Serial.println("Khong ket noi WIFI");
    WiFi.begin(ssid, password);
  }
  if ((WiFi.status() == WL_CONNECTED) && (!Blynk.connected())) {
    reboot_num = reboot_num + 1;
    if ((reboot_num == 1) || (reboot_num == 2)) {
      Serial.println("...");
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(ssid, password);
    }
    if (reboot_num % 5 == 0) {
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
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur,
                total);
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
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n",
                  ESPhttpUpdate.getLastError(),
                  ESPhttpUpdate.getLastErrorString().c_str());
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;
  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}
//-------------------------------------------------------------------

void savedata() {
  if (memcmp(&edata, &dataCheck, sizeof(edata)) != 0) {
    Serial.println("\nData changed, writing to EEPROM...");
    if (cs.write(edata)) {
      memcpy(&dataCheck, &edata, sizeof(edata));
      Serial.println("EEPROM write successful.");
    } else {
      Serial.println("EEPROM write failed!");
    }
  }
}
void on_cap1() {
  if ((status_g1 != HIGH) && (trip0 == false)) {
    status_g1 = HIGH;
    Blynk.virtualWrite(V_BTN_G1, status_g1);
    pcf8575_1.digitalWrite(pin_G1, status_g1);
  }
}
void off_cap1() {
  if (status_g1 != LOW) {
    status_g1 = LOW;
    Blynk.virtualWrite(V_BTN_G1, status_g1);
  }
  pcf8575_1.digitalWrite(pin_G1, status_g1);
}
void on_bom() {
  if ((status_b1 != HIGH) && (trip1 == false)) {
    status_b1 = HIGH;
    Blynk.virtualWrite(V_BTN_B1, status_b1);
    pcf8575_1.digitalWrite(pin_B1, status_b1);
  }
}
void off_bom() {
  if (status_b1 != LOW) {
    status_b1 = LOW;
    Blynk.virtualWrite(V_BTN_B1, status_b1);
  }
  pcf8575_1.digitalWrite(pin_B1, status_b1);
}
void on_nenkhi() {
  if ((status_nk1 != HIGH) && (trip2 == false)) {
    status_nk1 = HIGH;
    savedata();
    Blynk.virtualWrite(V_BTN_NK1, status_nk1);
    pcf8575_1.digitalWrite(pin_NK1, status_nk1);
  }
}
void off_nenkhi() {
  if (status_nk1 != LOW) {
    status_nk1 = LOW;
    savedata();
    Blynk.virtualWrite(V_BTN_NK1, status_nk1);
  }
  pcf8575_1.digitalWrite(pin_NK1, status_nk1);
}
void on_fan() {
  if (status_fan != HIGH) {
    status_fan = HIGH;
    pcf8575_1.digitalWrite(pin_fan, !status_fan);
  }
}
void off_fan() {
  if (status_fan != LOW) {
    status_fan = LOW;
    pcf8575_1.digitalWrite(pin_fan, !status_fan);
  }
}
void readPower() { // Gieng
  if (vfd_g1_current >= 1) {
    yIrms0 = yIrms0 + 1;
    if (yIrms0 > 3) {
      if ((vfd_g1_current >= edata.SetAmpemax) || (vfd_g1_current <= edata.SetAmpemin)) {
        xSetAmpe = xSetAmpe + 1;
        if ((xSetAmpe > 3) && (edata.flags.key_protect)) {
          off_cap1();
          xSetAmpe = 0;
          trip0 = true;
          if (edata.flags.key_noti) {
            Blynk.logEvent("error", String("Cấp 1 lỗi: ") + vfd_g1_current + String(" A"));
          }
        }
      } else {
        xSetAmpe = 0;
      }
    }
  }
}
void readPower1() { // Bom
  if (vfd_b1_current >= 1) {
    yIrms1 = yIrms1 + 1;
    if (yIrms1 > 3) {
      if ((vfd_b1_current >= edata.SetAmpe1max) || (vfd_b1_current <= edata.SetAmpe1min)) {
        xSetAmpe1 = xSetAmpe1 + 1;
        if ((xSetAmpe1 > 3) && (edata.flags.key_protect)) {
          off_bom();
          xSetAmpe1 = 0;
          trip1 = true;
          if (edata.flags.key_noti) {
            Blynk.logEvent("error", String("Cấp 2 lỗi: ") + vfd_b1_current + String(" A"));
          }
        }
      } else {
        xSetAmpe1 = 0;
      }
    }
  }
}
void readPower2() { // Nen khi- I2 - C2
  pcf8575_1.digitalWrite(S0pin, LOW);
  pcf8575_1.digitalWrite(S1pin, HIGH);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);
  float rms2 = emon2.calcIrms(740);
  if (rms2 < 1.5) {
    Irms2 = 0;
    yIrms2 = 0;
  } else if (rms2 >= 1.5) {
    Irms2 = rms2;
    yIrms2 = yIrms2 + 1;
    if ((yIrms2 > 3) &&
        ((Irms2 >= edata.SetAmpe2max) || (Irms2 <= edata.SetAmpe2min))) {
      xSetAmpe2 = xSetAmpe2 + 1;
      if ((xSetAmpe2 > 3) && (edata.flags.key_protect)) {
        off_nenkhi();
        xSetAmpe2 = 0;
        trip2 = true;
        if (edata.flags.key_noti) {
          Blynk.logEvent("error", String("Máy nén khí lỗi: ") + Irms2 + String(" A"));
        }
      }
    } else {
      xSetAmpe2 = 0;
    }
  }
}
void temperature() { // Nhiệt độ
  sensors.requestTemperatures();
  if (sensors.getDeviceCount() > 0) {
    float newTemp = sensors.getTempCByIndex(0);

    // Kiểm tra xem giá trị đọc được có phải là mã lỗi không.
    // Thư viện DallasTemperature trả về -127 (DEVICE_DISCONNECTED_C) khi đọc lỗi.
    if (newTemp != DEVICE_DISCONNECTED_C) {
      // Đã đọc được giá trị hợp lệ, gán nó cho biến toàn cục 'temp'.
      temp = newTemp;
    } else {
      // Lỗi đọc cảm biến, không cập nhật 'temp'.
      // 'temp' sẽ giữ giá trị hợp lệ cuối cùng.
      // Serial.println("Lỗi: Không thể đọc giá trị từ cảm biến nhiệt độ.");
    }

    // Logic điều khiển quạt và in giá trị sẽ dùng biến 'temp' (giá trị mới hoặc giá trị hợp lệ cuối cùng).
    if (temp > 39)
      on_fan();
    else if (temp < 37)
      off_fan();
    // Serial.println("Temp C: " + String(temp, 2)); // In với 2 chữ số thập phân
  }
}
//-------------------------------------------------------------------
/*
  SimpleKalmanFilter(e_mea, e_est, q);
  e_mea: Measurement Uncertainty - Sai số của phép đo.
         Cảm biến của bạn nhiễu đến mức nào? Giá trị lớn hơn nếu cảm biến nhiễu nhiều.
  e_est: Estimation Uncertainty - Sai số của ước tính.
         Bạn tin vào giá trị ước tính ban đầu đến mức nào? Thường có thể bắt đầu bằng giá trị giống e_mea.
  q:     Process Noise - Nhiễu của quá trình.
         Mực nước thực tế có thể thay đổi nhanh đến mức nào giữa các lần đo?
         Nếu nước tĩnh, q rất nhỏ. Nếu đang bơm/xả, q lớn hơn.
*/
// Bạn cần "tune" 3 giá trị này để có kết quả tốt nhất. Hãy bắt đầu với các giá trị này.
SimpleKalmanFilter pressureKalmanFilter(2, 2, 0.01);

// Biến toàn cục để lưu giá trị ADC đã được lọc, dùng cho việc hiệu chuẩn
float kalman_filtered_adc_value = 0;

// --- BỘ LỌC KẾT HỢP: MEDIAN + KALMAN ---
const int MEDIAN_WINDOW_SIZE = 5;
int median_buffer[MEDIAN_WINDOW_SIZE];
int median_buffer_index = 0;

// Hàm sắp xếp và lấy trung vị (giống như trong đề xuất trước)
int getMedian(int arr[], int size) {
  for (int i = 1; i < size; i++) {
    int key = arr[i];
    int j = i - 1;
    while (j >= 0 && arr[j] > key) {
      arr[j + 1] = arr[j];
      j = j - 1;
    }
    arr[j + 1] = key;
  }
  return arr[size / 2];
}

/**
 * @brief Đọc và xử lý giá trị từ cảm biến bằng bộ lọc Median + Kalman.
 * @return Giá trị mực nước (cm) đã được xử lý.
 */
float readPressureWithMedianAndKalman() {
  // 1. Chọn kênh analog
  pcf8575_1.digitalWrite(S0pin, HIGH);
  pcf8575_1.digitalWrite(S1pin, LOW);
  pcf8575_1.digitalWrite(S2pin, LOW);
  pcf8575_1.digitalWrite(S3pin, LOW);

  // 2. Đọc giá trị thô từ ADC
  int raw_value = analogRead(A0);

  // 3. Áp dụng Median Filter để loại bỏ nhiễu đột biến
  median_buffer[median_buffer_index] = raw_value;
  median_buffer_index = (median_buffer_index + 1) % MEDIAN_WINDOW_SIZE;

  int sorted_buffer[MEDIAN_WINDOW_SIZE];
  memcpy(sorted_buffer, median_buffer, sizeof(median_buffer));
  int median_value = getMedian(sorted_buffer, MEDIAN_WINDOW_SIZE);

  // 4. Đưa giá trị đã qua bộ lọc trung vị vào bộ lọc Kalman
  kalman_filtered_adc_value = pressureKalmanFilter.updateEstimate(median_value);

  // 5. Chuyển đổi giá trị ADC đã làm mịn sang đơn vị đo thực tế (cm)
  // --- ÁP DỤNG CÔNG THỨC HIỆU CHUẨN TỪ OFFSET VÀ GAIN ---
  // Công thức: Level = gain * (ADC_current - offset)
  float gain = edata.level_cal_gain_x1000 / 1000.0f;
  float offset = edata.level_cal_offset_x100 / 100.0f;
  float result;

  if (gain > 0) {
    // Chuyển đổi giá trị ADC sang mực nước (cm) đo được bởi cảm biến.
    result = gain * (kalman_filtered_adc_value - offset);
  } else {
    result = 0.0f;
  }

  // Cảm biến cách đáy 25cm, nên cộng thêm 25cm vào kết quả đo được để có mực nước thực tế từ đáy bể.
  result += 25.0f;

  // Giới hạn giá trị trong khoảng hợp lý
  // Serial.println("mực nước: " + String(result) + " cm");
  return constrain(result, 0.0, dosau * 1.4); // Cho phép vượt 40%
}
void MeasureAndProcessWaterLevel() {
  // Đọc và lọc giá trị mực nước
  smoothDistance = readPressureWithMedianAndKalman();
  // Serial.println("mực nước1: " + String(smoothDistance) + " cm");
  //  Tính toán và gửi dữ liệu lên Blynk
  if (smoothDistance >= 0) {
    // Công thức tính thể tích cho bể tròn: V = pi * r^2 * h
    // pi: 3.14
    // bankinh2: bán kính bình phương (cm^2)
    // smoothDistance: chiều cao mực nước (cm)
    // Chia cho 1,000,000 để đổi từ cm^3 sang m^3
    volume = (pi * bankinh2 * smoothDistance) / 1000000; // m3

    // Logic cảnh báo mực nước cao
    if (smoothDistance - dosau >= 20) {
      maxtank = true;
    } else if (smoothDistance < (dosau - 50)) {
      maxtank = false;
    }
  }
}

void controlPumpByWaterLevel() {
  // Chỉ thực hiện khi chế độ chạy theo bể được bật
  if (edata.flags.mode_follow_tank == 0) {
    // Nếu chế độ tắt, đảm bảo tần số kênh B được đặt về 0
    if (last_written_freq_b != 0) {
      Serial.println("Water level control disabled. Setting Channel B freq to 0.");
      mb.writeHreg(VFD_B1_SLAVE_ID, VFD_FREQ_CHANNEL_B, 0, cbWrite);
      last_written_freq_b = 0;
      current_vfd_b1_freq_channel_b = 0; // Cập nhật giá trị trong RAM
    }
    return;
  }

  // Chạy logic điều khiển theo chu kỳ PD_CONTROL_INTERVAL
  if (millis() - last_pd_control_time < PD_CONTROL_INTERVAL) {
    return;
  }
  unsigned long current_time = millis();
  float dt = (current_time - last_pd_control_time) / 1000.0f;
  last_pd_control_time = current_time;

  // Lấy mực nước hiện tại (đã được lọc bởi MeasureAndProcessWaterLevel)
  if (smoothDistance < 0 || dosau <= 0)
    return; // Dữ liệu chưa hợp lệ

  float current_level_percent = (smoothDistance / dosau) * 100.0;
  current_level_percent = constrain(current_level_percent, 0.0, 100.0);

  // Khởi tạo giá trị trước đó trong lần chạy đầu tiên
  if (previous_level_percent < 0) {
    previous_level_percent = current_level_percent;
    return;
  }

  // 1. Tính toán sai lệch (Proportional - P)
  float error = edata.level_setpoint - current_level_percent;

  // 2. Tính toán tốc độ thay đổi (Derivative - D)
  float derivative = (current_level_percent - previous_level_percent) / dt; // Đơn vị: %/giây
  previous_level_percent = current_level_percent;

  uint16_t freq_to_write = 0;

  // Chỉ can thiệp (tăng tần số kênh B) khi mực nước thấp hơn điểm đặt một khoảng an toàn (deadband)
  if (error > PD_DEADBAND_PERCENT) {
    // 3. Tính toán sai lệch đã điều chỉnh để có phản ứng mượt mà khi bắt đầu vào vùng điều khiển
    float adjusted_error = error - PD_DEADBAND_PERCENT;

    // 4. Áp dụng công thức PD để tính toán tần số kênh B
    float freq_b_output = (PD_KP * adjusted_error) - (PD_KD * derivative);

    // 5. Giới hạn giá trị an toàn
    freq_b_output = constrain(freq_b_output, 0.0f, MAX_FREQ_CHANNEL_B);

    // 6. Chuyển đổi sang định dạng cho Modbus (0.01 Hz resolution)
    freq_to_write = (uint16_t)(freq_b_output * 100);
  }

  // 7. Gửi lệnh tới biến tần nếu giá trị thay đổi
  if (freq_to_write != last_written_freq_b) {
    // Serial.printf("Water Level Control: Level=%.1f%%, Setpoint=%d%%, Error=%.1f, Derivative=%.2f -> FreqB=%u (%.2fHz)\n", current_level_percent, edata.level_setpoint, error, derivative, freq_to_write, freq_to_write / 100.0f);

    mb.writeHreg(VFD_B1_SLAVE_ID, VFD_FREQ_CHANNEL_B, freq_to_write, cbWrite);
    last_written_freq_b = freq_to_write;
    current_vfd_b1_freq_channel_b = freq_to_write; // Cập nhật giá trị vào RAM
  }
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void rtctime() {
  DateTime now = rtc_module.now();
  if (blynk_first_connect == true) {
    if ((now.day() != day()) || (now.hour() != hour()) ||
        ((now.minute() - minute() > 2) || (minute() - now.minute() > 2))) {
      rtc_module.adjust(
          DateTime(year(), month(), day(), hour(), minute(), second()));
    }
  }
  uint16_t nowtime = (now.hour() * 360 + now.minute() * 6);

  if (edata.flags.mode_man_auto == 1) {
    if (edata.t1_start > edata.t1_stop) {
      if (!maxtank && ((nowtime >= edata.t1_stop) && (nowtime < edata.t1_start))) {
        off_cap1();
        off_bom();
      }
      if ((nowtime < edata.t1_stop) || (nowtime > edata.t1_start) || maxtank) {
        if (maxtank) {
          off_cap1();
          on_bom();
        } else {
          on_cap1();
          on_bom();
        }
      }
    } else if (edata.t1_start < edata.t1_stop) {
      if (!maxtank && ((nowtime > edata.t1_stop) || (nowtime < edata.t1_start))) {
        off_cap1();
        off_bom();
      }
      if (((nowtime < edata.t1_stop) && (nowtime > edata.t1_start)) || maxtank) {
        if (maxtank) {
          off_cap1();
          on_bom();
        } else {
          on_cap1();
          on_bom();
        }
      }
    }
  }
}
//-------------------------------------------------------------------
void i2c_scaner() {
  byte error, address;
  int nDevices;
  String stringOne;

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      stringOne = String(address, HEX);
      if (address < 16)
        Blynk.virtualWrite(V_TERMINAL, "I2C device found at address 0x0", stringOne, " !\n");
      Blynk.virtualWrite(V_TERMINAL, "I2C device found at address 0x", stringOne, " !\n");
      nDevices++;
    } else if (error == 4) {
      stringOne = String(address, HEX);

      if (address < 16)
        Blynk.virtualWrite(V_TERMINAL, "Unknown error at address 0x0", stringOne, " !\n");
      Blynk.virtualWrite(V_TERMINAL, "I2C device found at address 0x", stringOne, " !\n");
    }
  }
  if (nDevices == 0)
    Blynk.virtualWrite(V_TERMINAL, "No I2C devices found\n");
}
//-------------------------------------------------------------------
BLYNK_WRITE_VP(V_BTN_G1) // Gieng
{
  if ((key) && (!trip0)) {
    if (param.asInt() == LOW) {
      off_cap1();
    } else {
      on_cap1();
    }
  }
  Blynk.virtualWrite(V_BTN_G1, status_g1);
}
BLYNK_WRITE_VP(V_BTN_B1) // Bơm 1
{
  if ((key) && (!trip1)) {
    if (param.asInt() == LOW) {
      off_bom();
    } else {
      on_bom();
    }
  }
  Blynk.virtualWrite(V_BTN_B1, status_b1);
}
BLYNK_WRITE_VP(V_BTN_NK1) // Nén khí 1
{
  if ((key) && (!trip2)) {
    if (param.asInt() == LOW) {
      off_nenkhi();
    } else {
      on_nenkhi();
    }
  }
  Blynk.virtualWrite(V_BTN_NK1, status_nk1);
}
BLYNK_WRITE_VP(V_MODE_MAN_AUTO) // Chọn chế độ Cấp 2
{
  if (key) {
    switch (param.asInt()) {
    case 0: { // Man
      edata.flags.mode_man_auto = 0;
      break;
    }
    case 1: { // Auto
      edata.flags.mode_man_auto = 1;
      break;
    }
    }
  } else
    Blynk.virtualWrite(V_MODE_MAN_AUTO, edata.flags.mode_man_auto);
}
BLYNK_WRITE_VP(V_MENU_MOTOR) // Chon máy cài đặt bảo vệ
{
  switch (param.asInt()) {
  case 0: { // ....
    c = 0;
    Blynk.virtualWrite(V_INPUT_MIN_A, 0);
    Blynk.virtualWrite(V_INPUT_MAX_A, 0);
    break;
  }
  case 1: { // Gieng
    c = 1;
    Blynk.virtualWrite(V_INPUT_MIN_A, edata.SetAmpemin);
    Blynk.virtualWrite(V_INPUT_MAX_A, edata.SetAmpemax);
    break;
  }
  case 2: { // Bom
    c = 2;
    Blynk.virtualWrite(V_INPUT_MIN_A, edata.SetAmpe1min);
    Blynk.virtualWrite(V_INPUT_MAX_A, edata.SetAmpe1max);
    break;
  }
  case 3: { // Nén khí
    c = 3;
    Blynk.virtualWrite(V_INPUT_MIN_A, edata.SetAmpe2min);
    Blynk.virtualWrite(V_INPUT_MAX_A, edata.SetAmpe2max);
    break;
  }
  }
}
BLYNK_WRITE_VP(V_INPUT_MIN_A) // min
{
  if (key) {
    if (c == 1) {
      edata.SetAmpemin = param.asInt();
    } else if (c == 2) {
      edata.SetAmpe1min = param.asInt();
    } else if (c == 3) {
      edata.SetAmpe2min = param.asInt();
    }
  } else {
    Blynk.virtualWrite(V_INPUT_MIN_A, 0);
  }
}
BLYNK_WRITE_VP(V_INPUT_MAX_A) // max
{
  if (key) {
    if (c == 1) {
      edata.SetAmpemax = param.asInt();
    } else if (c == 2) {
      edata.SetAmpe1max = param.asInt();
    } else if (c == 3) {
      edata.SetAmpe2max = param.asInt();
    }
  } else {
    Blynk.virtualWrite(V_INPUT_MAX_A, 0);
  }
}
BLYNK_WRITE_VP(V_TERMINAL) // String
{
  String dataS = param.asStr();
  dataS.trim(); // Xóa khoảng trắng thừa

  if (dataS == "help") {
    terminal.clear();
    Blynk.virtualWrite(V_TERMINAL,
                       "--- DANH SÁCH LỆNH ---\n"
                       "ts3       : Kich hoat che do cai dat (15s)\n"
                       "active    : Kich hoat che do cai dat (vo han)\n"
                       "deactive  : Thoat che do cai dat\n"
                       "save      : Luu tat ca cai dat\n"
                       "reset     : Xoa trang thai loi cua may bom\n"
                       "rst       : Khoi dong lai thiet bi\n"
                       "update    : Cap nhat firmware (OTA)\n"
                       "i2c       : Quet cac thiet bi I2C\n"
                       "pre_0     : Hieu chuan diem 0 bar (ap suat)\n"
                       "pre_<so>  : Hieu chuan tai ap suat <so> bar\n"
                       "level_0   : Hieu chuan muc nuoc 0 cm\n"
                       "level_<so>: Hieu chuan tai muc nuoc <so> cm\n");
    char ram_buffer[50];
    sprintf(ram_buffer, "Free RAM: %u bytes\n", ESP.getFreeHeap());
    Blynk.virtualWrite(V_TERMINAL, ram_buffer);
  } else if (dataS == "ts3") {
    terminal.clear();
    key = true;
    Blynk.virtualWrite(V_TERMINAL, "Người vận hành: 'V.Tài'\nKích hoạt trong 15s\n");
    timer.setTimeout(15000, []() {
      key = false;
      terminal.clear();
    });
  } else if (dataS == "active") {
    terminal.clear();
    key = true;
    Blynk.virtualWrite(V_TERMINAL, "KHÔNG sử dụng phần mềm cho đến khi thông báo này mất.\n");
  } else if (dataS == "deactive") {
    terminal.clear();
    key = false;
    Blynk.virtualWrite(V_TERMINAL, "Ok!\nNhập mã để điều khiển!\n");
  } else if (dataS == "save") {
    terminal.clear();
    savedata();
    Blynk.virtualWrite(V_TERMINAL, "Đã lưu cài đặt.\n");
  } else if (dataS == "reset") {
    terminal.clear();
    trip1 = false;
    trip0 = false;
    trip2 = false;
    maxtank = false;
    Blynk.virtualWrite(V_TERMINAL, "Đã RESET! \nNhập mã để điều khiển!\n");
  } else if (dataS == "rst") {
    terminal.clear();
    Blynk.virtualWrite(V_TERMINAL, "ESP khởi động lại sau 3s");
    delay(3000);
    ESP.restart();
  } else if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V_TERMINAL, "UPDATE FIRMWARE...");
    update_fw();
  } else if (dataS == "i2c") {
    terminal.clear();
    i2c_scaner();
  } else if (dataS == "pre_0") { // Lệnh hiệu chuẩn điểm 0 bar
    if (key) {
      float v_zero = vfd_b1_params_data[12] / 100.0f;
      edata.pressure_cal_offset_x100 = v_zero * 100;
      savedata();
      char buffer[50];
      sprintf(buffer, "Đã lưu offset áp suất: %.2fV\n", v_zero);
      Blynk.virtualWrite(V_TERMINAL, buffer);
    }
  } else if (dataS.startsWith("pre_")) { // Lệnh hiệu chuẩn điểm áp suất đã biết, ví dụ: pre_3.5
    if (key) {
      String numStr = dataS.substring(4);
      float p_known = numStr.toFloat();
      float v_at_known_p = vfd_b1_params_data[12] / 100.0f;
      float v_zero = edata.pressure_cal_offset_x100 / 100.0f;

      if (v_at_known_p > v_zero) {
        float gain = p_known / (v_at_known_p - v_zero);
        edata.pressure_cal_gain_x1000 = gain * 1000;
        savedata();
        char buffer[60];
        sprintf(buffer, "Đã tính gain áp suất: %.3f\n", gain);
        Blynk.virtualWrite(V_TERMINAL, buffer);
      } else {
        Blynk.virtualWrite(V_TERMINAL, "Lỗi: V đọc được phải > V tại điểm 0\n");
      }
    }
  } else if (dataS == "level_0") { // Lệnh hiệu chuẩn điểm 0 cm cho mực nước
    if (key) {
      edata.level_cal_offset_x100 = kalman_filtered_adc_value * 100;
      savedata();
      char buffer[60];
      sprintf(buffer, "Đã lưu offset mực nước: %.0f\n", kalman_filtered_adc_value);
      Blynk.virtualWrite(V_TERMINAL, buffer);
    }
  } else if (dataS.startsWith("level_")) { // Lệnh hiệu chuẩn điểm mực nước đã biết, ví dụ: level_150
    if (key) {
      String numStr = dataS.substring(6);
      float level_known = numStr.toFloat();
      float adc_at_known = kalman_filtered_adc_value;
      float adc_zero = edata.level_cal_offset_x100 / 100.0f;

      if (adc_at_known > adc_zero) {
        float gain = level_known / (adc_at_known - adc_zero);
        edata.level_cal_gain_x1000 = gain * 1000;
        savedata();
        char buffer[80];
        sprintf(buffer, "Đã tính gain mực nước: %.3f\n", gain);
        Blynk.virtualWrite(V_TERMINAL, buffer);
      } else {
        Blynk.virtualWrite(V_TERMINAL, "Lỗi: ADC đọc được phải > ADC tại điểm 0\n");
      }
    }
  } else {
    Blynk.virtualWrite(V_TERMINAL, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
BLYNK_WRITE_VP(V_TIMEINPUT) // Time input
{
  if (key) {
    TimeInputParam t(param);
    if (t.hasStartTime()) {
      edata.t1_start = t.getStartHour() * 360 + t.getStartMinute() * 6;
    }
    if (t.hasStopTime()) {
      edata.t1_stop = t.getStopHour() * 360 + t.getStopMinute() * 6;
    }
  } else
    Blynk.virtualWrite(V_TIMEINPUT, 0);
}
BLYNK_WRITE_VP(V_PROTECT) // Bảo vệ
{
  if (key) {
    int data = param.asInt();
    if (data == LOW) {
      edata.flags.key_protect = false;
    } else {
      edata.flags.key_protect = true;
    }
    savedata();
  } else
    Blynk.virtualWrite(V_PROTECT, edata.flags.key_protect);
}
BLYNK_WRITE_VP(V_THONGBAO) // Thông báo
{
  if (key) {
    if (param.asInt() == LOW) {
      edata.flags.key_noti = false;
    } else {
      edata.flags.key_noti = true;
    }
    savedata();
  } else
    Blynk.virtualWrite(V_THONGBAO, edata.flags.key_noti);
}
BLYNK_WRITE_VP(V_RUALOC) // Rửa lọc
{
  if (key) {
    switch (param.asInt()) {
    case 0: { // Tắt
      edata.flags.rualoc = 0;
      if (edata.LLG1_RL != 0) {
        Blynk.virtualWrite(V_LL_RUALOC, LLG1_1m3 - edata.LLG1_RL);
        edata.LLG1_RL = 0;
        savedata();
      }
      break;
    }
    case 1: { // RL 1
      edata.flags.rualoc = 1;
      if (edata.LLG1_RL == 0) {
        edata.LLG1_RL = LLG1_1m3;
      }
      break;
    }
    }
    savedata();
  } else {
    Blynk.virtualWrite(V_RUALOC, edata.flags.rualoc);
  }
}
BLYNK_WRITE_VP(V_INFO) // Info
{
  if (param.asInt() == 1) {
    terminal.clear();
    if (edata.flags.mode_man_auto == 0) {
      Blynk.virtualWrite(V_TERMINAL, "Mode: MAN");
    } else if (edata.flags.mode_man_auto == 1) {
      int moingay_start_h = edata.t1_start / 360;
      int moingay_start_m =
          (edata.t1_start - (moingay_start_h * 360)) / 6;
      int moingay_stop_h = edata.t1_stop / 360;
      int moingay_stop_m = (edata.t1_stop - (moingay_stop_h * 360)) / 6;
      Blynk.virtualWrite(V_TERMINAL, "Mode: AUTO - Mỗi ngày\nThời gian nghỉ: ",
                         moingay_stop_h, " : ", moingay_stop_m, " -> ",
                         moingay_start_h, " : ", moingay_start_m);
    }
  } else {
    terminal.clear();
  }
}
BLYNK_WRITE_VP(V_PRE_SETPOINT) // Setpoint áp suất
{
  if (key) {
    // Người dùng nhập 2.2 (bar), lưu thành 22 (byte)
    edata.pre_setpoint_x10 = param.asFloat() * 10;
    // Chuyển đổi giá trị để ghi vào VFD (22 -> 220)
    uint16_t vfd_setpoint = edata.pre_setpoint_x10 * 10;
    mb.writeHreg(VFD_B1_SLAVE_ID, VFD_SETPOINT_REG, vfd_setpoint, cbWrite);
    savedata();
  } else
    Blynk.virtualWrite(V_PRE_SETPOINT, edata.pre_setpoint_x10 / 10.0f);
}
BLYNK_WRITE_VP(V_LEVEL_SETPOINT) // Setpoint mực nước
{
  if (key) {
    edata.level_setpoint = param.asInt();
    savedata();
  } else
    Blynk.virtualWrite(V_LEVEL_SETPOINT, edata.level_setpoint);
}
BLYNK_WRITE_VP(V_MODE_FOLLOW_TANK) // Chế độ theo dõi bồn
{
  if (key) {
    switch (param.asInt()) {
    case 0: { // Tắt
      edata.flags.mode_follow_tank = 0;
      savedata();
      break;
    }
    case 1: { // Bật
      edata.flags.mode_follow_tank = 1;
      savedata();
      break;
    }
    }
    savedata();
  } else
    Blynk.virtualWrite(V_MODE_FOLLOW_TANK, edata.flags.mode_follow_tank);
}
//-------------------------
BLYNK_WRITE_VP(V_LLG1_1M3) // Lưu lượng G1_1m3
{
  LLG1_1m3 = param.asInt();
}
//-------------------------------------------------------------------
void up() {
  const unsigned long FORCE_UPDATE_INTERVAL = 45000; // 45 giây
  unsigned long current_millis = millis();
  String params_to_update = "";

  // So sánh từng giá trị với trạng thái trước đó
  // và chỉ thêm vào chuỗi nếu có thay đổi HOẶC đã quá 45 giây.

  if (sta_vfd_g1 != prevState.sta_vfd_g1 || (current_millis - prevState.sta_vfd_g1_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V" + String(V_STA_VFD_G1) + "=" + urlEncode(sta_vfd_g1);
    prevState.sta_vfd_g1 = sta_vfd_g1;
    prevState.sta_vfd_g1_ts = current_millis;
  }
  if (sta_vfd_b1 != prevState.sta_vfd_b1 || (current_millis - prevState.sta_vfd_b1_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V" + String(V_STA_VFD_B1) + "=" + urlEncode(sta_vfd_b1);
    prevState.sta_vfd_b1 = sta_vfd_b1;
    prevState.sta_vfd_b1_ts = current_millis;
  }
  if (vfd_b1_freq != prevState.vfd_b1_freq || (current_millis - prevState.vfd_b1_freq_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V" + String(V_VFD_B1_FREQ) + "=" + String(vfd_b1_freq);
    prevState.vfd_b1_freq = vfd_b1_freq;
    prevState.vfd_b1_freq_ts = current_millis;
  }
  if (vfd_b1_current != prevState.vfd_b1_current || (current_millis - prevState.vfd_b1_current_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V" + String(V_VFD_B1_CURRENT) + "=" + String(vfd_b1_current);
    prevState.vfd_b1_current = vfd_b1_current;
    prevState.vfd_b1_current_ts = current_millis;
  }
  if (vfd_b1_feedback != prevState.vfd_b1_feedback || (current_millis - prevState.vfd_b1_feedback_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V" + String(V_VFD_B1_FEEDBACK) + "=" + String(vfd_b1_feedback);
    prevState.vfd_b1_feedback = vfd_b1_feedback;
    prevState.vfd_b1_feedback_ts = current_millis;
  }
  if (vfd_g1_freq != prevState.vfd_g1_freq || (current_millis - prevState.vfd_g1_freq_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V" + String(V_VFD_G1_FREQ) + "=" + String(vfd_g1_freq);
    prevState.vfd_g1_freq = vfd_g1_freq;
    prevState.vfd_g1_freq_ts = current_millis;
  }
  if (vfd_g1_current != prevState.vfd_g1_current || (current_millis - prevState.vfd_g1_current_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V" + String(V_VFD_G1_CURRENT) + "=" + String(vfd_g1_current);
    prevState.vfd_g1_current = vfd_g1_current;
    prevState.vfd_g1_current_ts = current_millis;
  }
  if (smoothDistance != prevState.smoothDistance || (current_millis - prevState.smoothDistance_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V" + String(V_CONLAI) + "=" + String(smoothDistance);
    prevState.smoothDistance = smoothDistance;
    prevState.smoothDistance_ts = current_millis;
  }
  if (volume != prevState.volume || (current_millis - prevState.volume_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V" + String(V_THETICH) + "=" + String(volume);
    prevState.volume = volume;
    prevState.volume_ts = current_millis;
  }
  if (Irms2 != prevState.Irms2 || (current_millis - prevState.Irms2_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V" + String(V_NENKHI1_CURRENT) + "=" + String(Irms2);
    prevState.Irms2 = Irms2;
    prevState.Irms2_ts = current_millis;
  }
  if (temp != prevState.temp || (current_millis - prevState.temp_ts > FORCE_UPDATE_INTERVAL)) {
    params_to_update += "&V" + String(V_TEMP) + "=" + String(temp);
    prevState.temp = temp;
    prevState.temp_ts = current_millis;
  }

  // Chỉ gửi yêu cầu HTTP nếu có ít nhất một giá trị đã thay đổi
  if (params_to_update.length() > 0) {
    String server_path = server_main + "batch/update?token=" + BLYNK_AUTH_TOKEN + params_to_update;
    http.begin(client, server_path.c_str());
    http.GET();
    http.end();
  }
}
//-------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(7000);
  //------------------------------------------------------------------
  emon0.current(A0, 105);
  emon1.current(A0, 105);
  emon2.current(A0, 105);

  Wire.begin();
  sensors.begin();
  rtc_module.begin();
  ee.begin();
  cs.begin(ee, PAGE_SIZE, MEMORY_SIZE / PAGE_SIZE);
  cs.read(edata);
  memcpy(&dataCheck, &edata, sizeof(edata));
  // Khởi tạo giá trị hiệu chuẩn mặc định nếu chưa có (lần chạy đầu tiên)
  if (edata.pressure_cal_gain_x1000 == 0) {
    Serial.println("Initializing default calibration values for pressure sensor.");
    // Giả định: 0 bar -> 2.0V, 10 bar -> 10.0V
    edata.pressure_cal_offset_x100 = 2.0f * 100;
    edata.pressure_cal_gain_x1000 = (10.0f / (10.0f - 2.0f)) * 1000; // gain = 1.25
  }
  if (edata.level_cal_gain_x1000 == 0) {
    Serial.println("Initializing default calibration values for water level sensor.");
    // Giả định: 0cm -> ADC 194, 500cm -> ADC 1024
    edata.level_cal_offset_x100 = 194.0f * 100;
    edata.level_cal_gain_x1000 = (500.0f / (1024.0f - 194.0f)) * 1000; // gain ~ 0.602
  }
  // Luôn lưu lại sau khi kiểm tra để đảm bảo các giá trị mặc định được ghi trong lần chạy đầu tiên
  // Hàm savedata() có cơ chế chống ghi thừa nên không ảnh hưởng hiệu suất.
  savedata();

  pcf8575_1.begin();
  pcf8575_1.pinMode(S0pin, OUTPUT);
  pcf8575_1.pinMode(S1pin, OUTPUT);
  pcf8575_1.pinMode(S2pin, OUTPUT);
  pcf8575_1.pinMode(S3pin, OUTPUT);

  pcf8575_1.pinMode(pin_G1, OUTPUT);
  pcf8575_1.digitalWrite(pin_G1, HIGH);
  pcf8575_1.pinMode(pin_B1, OUTPUT);
  pcf8575_1.digitalWrite(pin_B1, HIGH);
  pcf8575_1.pinMode(pin_NK1, OUTPUT);
  pcf8575_1.digitalWrite(pin_NK1, HIGH);
  pcf8575_1.pinMode(pin_fan, OUTPUT);
  pcf8575_1.digitalWrite(pin_fan, HIGH);
  pcf8575_1.pinMode(pin_P0, OUTPUT);
  pcf8575_1.digitalWrite(pin_P0, HIGH);
  pcf8575_1.pinMode(pin_P1, OUTPUT);
  pcf8575_1.digitalWrite(pin_P1, HIGH);
  pcf8575_1.pinMode(pin_P3, OUTPUT);
  pcf8575_1.digitalWrite(pin_P3, HIGH);
  pcf8575_1.pinMode(pin_P4, OUTPUT);
  pcf8575_1.digitalWrite(pin_P4, HIGH);

  S.begin(9600, SWSERIAL_8N1);
  mb.begin(&S);
  mb.master();
  //------------------------------------
  timer.setTimeout(5000L, []() {
    // timer.setInterval(3000L, read_modbus1);
    timer_up = timer.setInterval(1483L, []() {
      readPower();                   // Gieng
      readPower1();                  // Bom
      readPower2();                  // Nen khi
      MeasureAndProcessWaterLevel(); // Đọc và xử lý mực nước
      up();
      timer.restartTimer(timer_up);
    });
    timer.setInterval(15006L, []() {
      rtctime();
      Blynk.virtualWrite(V_WIFI_SIGNAL, WiFi.RSSI());
      temperature();
      timer.restartTimer(timer_up);
    });
    timer.setInterval(900005L, []() {
      connectionstatus();
      timer.restartTimer(timer_up);
    });
    terminal.clear();
  });
}

void loop() {
  Blynk.run();
  timer.run();
  mb.task(); // Xử lý các tác vụ Modbus ở chế độ nền
  controlPumpByWaterLevel();

  static unsigned long lastCheck = 0;
  if (i != 0) {
    switch (i) {
    case 1:
      if (!mb.slave()) {
        mb.readHreg(VFD_G1_SLAVE_ID, VFD_PARAMS_REG_START, vfd_g1_params_data, VFD_G1_PARAMS_TO_READ, cbReadVfdg1Params);
      }
      break;
    case 2:
      if (!mb.slave()) {
        mb.readHreg(VFD_B1_SLAVE_ID, VFD_STATUS_REG, vfd_b1_data_status, VFD_STATUS_REG_COUNT, cbReadVfdb1Data);
      }
      break;
    case 3:
      if (!mb.slave()) {
        mb.readHreg(VFD_B1_SLAVE_ID, VFD_PARAMS_REG_START, vfd_b1_params_data, VFD_B1_PARAMS_TO_READ, cbReadVfdb1Params);
      }
      break;
    case 4:
      if (!mb.slave()) {
        mb.readHreg(VFD_B1_SLAVE_ID, VFD_FREQ_CHANNEL_B, vfd_b1_data_pid, VFD_FREQ_SETPOINT_COUNT, cbWritevfdb1pid);
      }
      break;
    case 5:
      if (!mb.slave()) {
        if (vfd_b1_data_pid[0] != current_vfd_b1_freq_channel_b) {
          // Giá trị đọc được khác với giá trị đã lưu, tiến hành ghi lại
          Serial.println("Channel B Freq mismatch. Writing saved value to VFD.");
          mb.writeHreg(VFD_B1_SLAVE_ID, VFD_FREQ_CHANNEL_B, current_vfd_b1_freq_channel_b, cbWrite);
        }
        // Chỉ kiểm tra và ghi setpoint nếu tần số đã đúng, để tránh gửi 2 lệnh ghi cùng lúc
        else if (vfd_b1_data_pid[1] != (uint16_t)(edata.pre_setpoint_x10 * 10)) {
          // Giá trị đọc được khác với giá trị đã lưu, tiến hành ghi lại
          uint16_t vfd_setpoint = edata.pre_setpoint_x10 * 10; // Chuyển đổi từ 22 (2.2 bar) -> 220 (cho VFD)
          Serial.print(vfd_b1_data_pid[1]);
          Serial.print(" != ");
          Serial.println(vfd_setpoint);
          Serial.println("Setpoint mismatch. Writing saved value to VFD.");
          mb.writeHreg(VFD_B1_SLAVE_ID, VFD_SETPOINT_REG, vfd_setpoint, cbWrite);
        }
      }
      // Serial.println("Modbus sync check complete.");
      i++;
      break;
    }
  }
  // Bắt đầu chu kỳ đọc Modbus mới sau mỗi khoảng thời gian,
  // nhưng chỉ sau khi board đã chạy được ít nhất 10 giây để đảm bảo ổn định.
  if (millis() > 10000 && millis() - lastCheck > 1157) {
    lastCheck = millis();
    if (!mb.slave()) {
      i = 0;
      mb.readHreg(VFD_G1_SLAVE_ID, VFD_STATUS_REG, vfd_g1_data_status, VFD_STATUS_REG_COUNT, cbReadVfdg1Data);
    }
  }
}