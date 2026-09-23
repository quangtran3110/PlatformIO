/**DC7633
 *V0 - Button C2 - 1
 *V1 - Button C2 - 2
 *V2 - Button C1 - 1
 *V3 - Button C1 - 2
 *V4 - Chọn người vận hành
 *V5 - MENU motor
 *V6 - min
 *V7 - max
 *V8 - info
 *V9 - Ngày/Giờ
 *V10 - terminal key
 *V11 - Thời gian chạy Bơm
 *V12 - Hide/visible
 *V13 - Bảo vệ
 *V14 - Ap luc
 *V15 - Rửa lọc
 *V16 -
 *V17 - Thông báo
 *V18 - time input
 *V19 -

 *V20 - I0 - Cấp 1 - 1
 *V21 - I1 - Cấp 1 - 2
 *V22 - I2 - Bơm 2v
 *V23 - I3 - Bơm 2
 *V24 - Dung tich
 *V25 - Thể tích
 *V26 - Còn lại
 *V27 - Độ sâu
 *V28 - datas_volume
 *V29 - LL1m3
 *V30 - RAW AG1
 *V31 - LL24h
 *V32 - LLG1_RL
 *V33 -
 *V34 -
 *V35 -
 *V36 -
 *V37 -
 *V40 - thời gian chạy G1
 *V41 - thời gian chạy G1-24h
 *V42 - thời gian chạy G2
 *V43 - thời gian chạy G2-24h
 *V44 - thời gian chạy B1
 *V45 - thời gian chạy B1-24h
 *V46 - thời gian chạy B2
 *V47 - thời gian chạy B2-24h
 *
 */

/*
#define BLYNK_TEMPLATE_ID "TMPLbPLEi8uh"
#define BLYNK_TEMPLATE_NAME "Trạm Mộc Hóa"
#define BLYNK_AUTH_TOKEN "tKNZ99XnCSeCsoDEva3kx-O0YWw83nMn"
*/
#define BLYNK_TEMPLATE_ID "TMPL6jUnu2ld9"
#define BLYNK_TEMPLATE_NAME "TRẠM 3 BPTCopy"
#define BLYNK_AUTH_TOKEN "BvVQVlNLOnVUDldbeczLXmn0YvBNdHDK"
#define VOLUME_TOKEN "Q2KAjaqI3sWhET-Ax94VPYfIk2Fmsr36"

#define BLYNK_FIRMWARE_VERSION "260923.1"
#define BLYNK_PRINT Serial
#define APP_DEBUG
const char *ssid = "KwacoBlynk";
const char *password = "Password";
/*const char *ssid = "tram bom so 4";
const char *password = "0943950555";*/
// const char* ssid = "Wifi";
// const char* password = "Password";

#pragma region
// #include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <SimpleKalmanFilter.h>
#include <UrlEncode.h>

//--------------
#include "PCF8575.h"
PCF8575 pcf8575_2(0x21);
PCF8575 pcf8575_1(0x20);
const int btn_left = P8;
const int btn_right = P9;
const int btn_mid = P7;
const int btn_g1 = P5;
const int btn_g2 = P6;

const int pin_van1 = P0;  // Chân điều khiển Van Điện 1
const int pin_van2 = P3;  // Chân điều khiển Van Điện 2
const int pin_comp1 = P1; // Chân máy nén 1
const int pin_comp2 = P2; // Chân máy nén 2
const int pin0 = P4;
bool trip4 = false; // Cờ báo lỗi máy nén 1
bool trip5 = false; // Cờ báo lỗi máy nén 2

unsigned long int xSetAmpe4 = 0, xSetAmpe5 = 0; // Bộ đếm lỗi
unsigned long int yIrms4 = 0, yIrms5 = 0;       // Bộ đếm thời gian chạy

//--------------
#include "EmonLib.h"
EnergyMonitor emon0, emon1, emon2, emon3, emon4, emon5;
//--------------
#include "RTClib.h"
#include <TimeLib.h>
#include <WidgetRTC.h>
RTC_DS3231 rtc_module;
//--------------
#include <OneWire.h>
#include <Wire.h>
OneWire oneWire(D0);
//--------------
#include <Eeprom24C32_64.h>
#define EEPROM_ADDRESS 0x57
static Eeprom24C32_64 eeprom(EEPROM_ADDRESS);
//--------------
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
WiFiClient client;
HTTPClient http;
#define URL_fw_Bin                                                         \
  "https://raw.githubusercontent.com/quangtran3110/PlatformIO/refs/heads/" \
  "main/CN_MocHoa/Tram3BPT/.pio/build/nodemcuv2/firmware.bin"
String server_name = "http://sgp1.blynk.cloud/external/api/";
//--------------
#define filterSamples 121
const int S0pin = P3;
const int S1pin = P2;
const int S2pin = P1;
const int S3pin = P0;
const word address = 100;
const int customPin = D3; // Chân D3
                          // Biến lưu trạng thái hiện tại

// Vùng RTC user memory từ word 32 trở đi không bị eboot sử dụng.
// Cờ này cho phép khởi động lại vào chế độ OTA sạch, trước khi khởi tạo
// Blynk, I2C, EEPROM và các ngõ ra điều khiển thiết bị.
enum OtaRtcPhase : uint32_t
{
  OTA_NONE = 0,
  OTA_REQUESTED = 1,
  OTA_RUNNING = 2,
  OTA_SUCCESS = 3,
  OTA_FAILED = 4
};

struct OtaRtcState
{
  uint32_t magic;
  uint32_t phase;
  int32_t error;
  uint32_t check;
};

const uint32_t OTA_RTC_MAGIC = 0x4F544131UL;
const uint32_t OTA_RTC_CHECK_SALT = 0x5A5AA5A5UL;
const uint32_t OTA_RTC_OFFSET = 32;
const unsigned long OTA_WIFI_TIMEOUT_MS = 45000UL;
const int OTA_ERROR_WIFI_TIMEOUT = -1001;
const int OTA_ERROR_NO_UPDATE = -1002;

String lastOtaStatus = "none";
int lastOtaProgressBucket = -1;

const int dai = 750;
const int rong = 750;
const int dosau = 330;

const bool b1 = HIGH;
const bool b2 = LOW;
// long t;
long m = 60000;

char daysOfTheWeek[7][12] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
char tz[] = "Asia/Ho_Chi_Minh";
bool isManualMode = false; // Mặc định là Auto, true là Thủ công
bool key = false, keyp = true, keynoti = true;
bool key_config = false;
bool trip0 = false, trip1 = false, trip2 = false, trip3 = false;
bool temp_0 = true, temp_1 = true;
bool noti_1 = true, noti_2 = true;
bool blynk_first_connect = false;
bool pinState = LOW;
int timer_I;
int i = 0; // Biến đếm áp lực
int j = 0; // Biến đếm cấp 1 không chạy
int b, check_connect = 0, reboot_num;
int status_g1 = HIGH, status_g2 = HIGH;
int timer_1, timer_2;
int xSetAmpe = 0, xSetAmpe1 = 0, xSetAmpe2 = 0, xSetAmpe3 = 0;

int LLG1_1m3;
int timer_rualoc_1 = -1; // Hẹn giờ riêng cho Lọc 1
int timer_rualoc_2 = -1; // Hẹn giờ riêng cho Lọc 2
int op = 1;
// --- KHAI BÁO BIẾN TOÀN CỤC CHO BƠM ---

bool request_pump_swap = false;   // Cờ kích hoạt lệnh đổi bơm
bool is_in_working_hours = false; // Trạng thái ca làm việc hiện tại
bool G1_save = false, G2_save = false, B1_save = false, B2_save = false;
float Irms0, Irms1, Irms2, Irms3, Irms4, Irms5, Result1;
unsigned long int xIrms0 = 0, xIrms1 = 0, xIrms2 = 0, xIrms3 = 0;
unsigned long int yIrms0 = 0, yIrms1 = 0, yIrms2 = 0, yIrms3 = 0;
unsigned long timerun;
unsigned long last_stop_time = 0;
int volume, volume1, smoothDistance;
unsigned long G1_start, G2_start, B1_start, B2_start;
// --- BỘ LỌC KẾT HỢP: MEDIAN + KALMAN ---
const int MEDIAN_WINDOW_SIZE = 5;
int level_median_buffer[MEDIAN_WINDOW_SIZE];
int level_median_index = 0;
int pressure_median_buffer[MEDIAN_WINDOW_SIZE];
int pressure_median_index = 0;
int status_comp1 = HIGH;
int status_comp2 = HIGH;
// --- KHAI BÁO BIẾN TOÀN CỤC CHO BƠM ---
int current_pump_mode = -1;
int timer_run_main = -1;
int c = 0;
int status_v0 = LOW; // Trạng thái Bơm 1 trên App
int status_v1 = LOW; // Trạng thái Bơm 2 trên App
SimpleKalmanFilter levelKalmanFilter(2, 2, 0.01);
SimpleKalmanFilter pressureKalmanFilter(2, 2, 0.01);

// Biến toàn cục để lưu giá trị ADC đã được lọc, dùng cho việc hiệu chuẩn
float kalman_filtered_level_adc = 0;
float kalman_filtered_pressure_adc = 0;

int getMedian(int arr[], int size)
{
  for (int i = 1; i < size; i++)
  {
    int key = arr[i];
    int j = i - 1;
    while (j >= 0 && arr[j] > key)
    {
      arr[j + 1] = arr[j];
      j = j - 1;
    }
    arr[j + 1] = key;
  }
  return arr[size / 2];
}

long distance, distance1;

#define MAX_CALIB_POINTS 5

struct CalibPoint
{
  uint16_t adc;   // Giá trị ADC (0-1023)
  uint16_t value; // Giá trị quy đổi (Áp suất * 100, Mực nước cm)
};

struct Data
{
  byte SetAmpemax, SetAmpemin;
  byte SetAmpe1max, SetAmpe1min;
  byte SetAmpe2max, SetAmpe2min;
  byte SetAmpe3max, SetAmpe3min;
  byte SetAmpe4max, SetAmpe4min; // Cho Máy nén 1
  byte SetAmpe5max, SetAmpe5min; // Cho Máy nén 2
  byte mode_cap2;
  byte status_c1_m1;
  byte status_c1_m2;
  byte status_c2_m1;
  byte status_c2_m2;
  int start_time, stop_time;
  int save_num;
  int turn_motor = 0;
  byte check_changeday;
  byte status_btn_mid;
  byte status_btn_left;
  byte status_direct;
  byte reset_day;
  int timerun_G1, timerun_G2, timerun_B1, timerun_B2;
  int LLG1_RL;
  byte rualoc;
  // --- DỮ LIỆU HIỆU CHUẨN ĐA ĐIỂM ---
  CalibPoint pressure_points[MAX_CALIB_POINTS];
  uint8_t num_pressure_points;
  CalibPoint level_points[MAX_CALIB_POINTS];
  uint8_t num_level_points;
} data, dataCheck;
const struct Data dataDefault = {};

#pragma endregion

WidgetTerminal terminal(V10);
WidgetTerminal volume_terminal(V28);
WidgetRTC rtc;
BlynkTimer timer, timeout;

BLYNK_CONNECTED()
{
  rtc.begin();
  blynk_first_connect = true;

  Blynk.virtualWrite(V4, data.mode_cap2);
}

void savedata()
{
  if (memcmp(&data, &dataCheck, sizeof(data)) != 0)
  {
    data.save_num = data.save_num + 1;

    // Ghi an toàn từng Byte để tránh lỗi Wrap-around 32-bytes của 24C32
    byte *p = (byte *)&data;
    for (unsigned int i = 0; i < sizeof(data); i++)
    {
      eeprom.writeByte(address + i, *p++); // Hàm writeByte xử lý ghi đơn lẻ an toàn
      delay(5);                            // BẮT BUỘC: IC 24C32 cần 5ms chu kỳ Write Cycle để nạp vật lý
      ESP.wdtFeed();                       // Reset Watchdog tránh treo khi delay kéo dài
    }

    memcpy(&dataCheck, &data, sizeof(data));
    Blynk.setProperty(V10, "label", "EEPROM ", data.save_num);
  }
}
//-----------------------------
void connectionstatus()
{
  if ((WiFi.status() != WL_CONNECTED))
  {
    Serial.println("Khong ket noi WIFI");
    WiFi.begin(ssid, password);
  }
  if ((WiFi.status() == WL_CONNECTED) && (!Blynk.connected()))
  {
    reboot_num = reboot_num + 1;
    if ((reboot_num == 1) || (reboot_num == 2))
    {
      Serial.println("...");
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(ssid, password);
    }
    if (reboot_num % 5 == 0)
    {
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(ssid, password);
    }
  }
  if (Blynk.connected())
  {
    if (reboot_num != 0)
    {
      reboot_num = 0;
    }
  }
}
uint32_t otaStateCheck(const OtaRtcState &state)
{
  return state.magic ^ state.phase ^ static_cast<uint32_t>(state.error) ^
         OTA_RTC_CHECK_SALT;
}

bool readOtaRtcState(OtaRtcState &state)
{
  OtaRtcState stored = {};
  if (!ESP.rtcUserMemoryRead(OTA_RTC_OFFSET,
                             reinterpret_cast<uint32_t *>(&stored),
                             sizeof(stored)))
  {
    return false;
  }
  if (stored.magic != OTA_RTC_MAGIC || stored.check != otaStateCheck(stored))
  {
    return false;
  }
  state = stored;
  return true;
}

bool writeOtaRtcState(OtaRtcPhase phase, int32_t error = 0)
{
  OtaRtcState state = {};
  state.magic = OTA_RTC_MAGIC;
  state.phase = static_cast<uint32_t>(phase);
  state.error = error;
  state.check = otaStateCheck(state);
  return ESP.rtcUserMemoryWrite(OTA_RTC_OFFSET,
                                reinterpret_cast<uint32_t *>(&state),
                                sizeof(state));
}

void clearOtaRtcState()
{
  OtaRtcState state = {};
  ESP.rtcUserMemoryWrite(OTA_RTC_OFFSET,
                         reinterpret_cast<uint32_t *>(&state), sizeof(state));
}

void update_started()
{
  lastOtaProgressBucket = -1;
  Serial.println("CALLBACK:  HTTP update process started");
}
void update_finished()
{
  // ESPhttpUpdate có thể tự khởi động lại ngay sau callback này, vì vậy phải
  // lưu kết quả thành công trước khi callback kết thúc.
  writeOtaRtcState(OTA_SUCCESS);
  Serial.println("CALLBACK:  HTTP update process finished");
}
void update_progress(int cur, int total)
{
  ESP.wdtFeed();
  if (total <= 0)
  {
    return;
  }
  int progressBucket = static_cast<int>((static_cast<int64_t>(cur) * 10) / total);
  if (progressBucket > 10)
  {
    progressBucket = 10;
  }
  if (progressBucket != lastOtaProgressBucket)
  {
    lastOtaProgressBucket = progressBucket;
    Serial.printf("CALLBACK: HTTP update %d%%\n", progressBucket * 10);
  }
}
void update_error(int err)
{
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}
void update_fw()
{
  WiFiClientSecure client_;
  client_.setInsecure();
  Serial.print("Wait...");
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  ESPhttpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client_, URL_fw_Bin);
  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    writeOtaRtcState(OTA_FAILED, ESPhttpUpdate.getLastError());
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n",
                  ESPhttpUpdate.getLastError(),
                  ESPhttpUpdate.getLastErrorString().c_str());
    break;
  case HTTP_UPDATE_NO_UPDATES:
    writeOtaRtcState(OTA_FAILED, OTA_ERROR_NO_UPDATE);
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;
  case HTTP_UPDATE_OK:
    // Trường hợp thư viện không tự restart, vẫn bảo đảm lưu kết quả.
    writeOtaRtcState(OTA_SUCCESS);
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}

void runCleanOtaMode()
{
  // Không khởi tạo PCF8575 trong nhánh này để tránh tác động bơm/van/máy nén.
  // D3 là xung watchdog ngoài; giữ HIGH trong lúc OTA là trạng thái an toàn.
  pinMode(customPin, OUTPUT);
  digitalWrite(customPin, HIGH);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, password);

  Serial.println("Clean OTA: waiting for WiFi...");
  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < OTA_WIFI_TIMEOUT_MS)
  {
    ESP.wdtFeed();
    delay(100);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Clean OTA: WiFi timeout");
    writeOtaRtcState(OTA_FAILED, OTA_ERROR_WIFI_TIMEOUT);
    delay(250);
    ESP.restart();
    return;
  }

  Serial.printf("Clean OTA: connected, free heap=%u, max block=%u\n",
                ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
  update_fw();

  // update_fw chỉ trở về khi không cập nhật được hoặc thư viện không tự restart.
  delay(250);
  ESP.restart();
}

bool handleOtaBootState()
{
  OtaRtcState state = {};
  if (!readOtaRtcState(state))
  {
    return false;
  }

  switch (static_cast<OtaRtcPhase>(state.phase))
  {
  case OTA_REQUESTED:
    writeOtaRtcState(OTA_RUNNING);
    runCleanOtaMode();
    return true;
  case OTA_RUNNING:
    lastOtaStatus = "failed (interrupted)";
    break;
  case OTA_SUCCESS:
    lastOtaStatus = "success";
    break;
  case OTA_FAILED:
    lastOtaStatus = "failed (" + String(state.error) + ")";
    break;
  default:
    lastOtaStatus = "none";
    break;
  }

  clearOtaRtcState();
  return false;
}
//-----------------------------
void up()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client;
    // Tăng nhẹ lên 1000ms để đảm bảo quá trình bắt tay TCP hoàn tất khi mạng yếu
    client.setTimeout(1000);

    if (client.connect("sgp1.blynk.cloud", 80))
    {
      // Dùng mảng char để chống phân mảnh RAM (Heap Fragmentation)
      char url[350];
      snprintf(url, sizeof(url),
               "/external/api/batch/update?token=%s&V20=%.2f&V14=%.2f&V21=%.2f&V22=%.2f&V23=%.2f&V25=%d&V37=%.2f&V38=%.2f&V26=%d&V40=%.2f&V42=%.2f&V44=%.2f&V46=%.2f",
               BLYNK_AUTH_TOKEN, Irms0, Result1, Irms1, Irms2, Irms3, volume1, Irms4, Irms5, smoothDistance,
               (float)data.timerun_G1 / 3600000.0, (float)data.timerun_G2 / 3600000.0,
               (float)data.timerun_B1 / 3600000.0, (float)data.timerun_B2 / 3600000.0);

      client.print("GET ");
      client.print(url);
      client.print(" HTTP/1.1\r\n");
      client.print("Host: sgp1.blynk.cloud\r\n");
      client.print("Connection: close\r\n\r\n");
      // Đọc lướt qua Header (vài mili-giây) để đảm bảo server đã nhận đủ gói tin
      while (client.connected() || client.available())
      {
        if (client.available())
        {
          String line = client.readStringUntil('\n');
          if (line == "\r")
            break; // Thoát ngay khi hết Header, không chờ Body
        }
      }

      client.stop(); // Đóng kết nối an toàn
    }
  }
}
void updateTimeBlynk()
{
  if (Blynk.connected())
  { // Chỉ gửi khi blynk có kết nối
    // Tạo chuỗi định dạng: "HH:MM:SS   DD/MM/YYYY"
    String timeString = String(hour() < 10 ? "0" : "") + String(hour()) + ":" +
                        String(minute() < 10 ? "0" : "") + String(minute()) + ":" +
                        String(second() < 10 ? "0" : "") + String(second()) + "   " +
                        String(day() < 10 ? "0" : "") + String(day()) + "/" +
                        String(month() < 10 ? "0" : "") + String(month()) + "/" +
                        String(year());

    Blynk.virtualWrite(V9, timeString);
  }
}

void up_timerun_motor()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client;
    client.setTimeout(1000); // Nâng lên 1000ms

    if (client.connect("sgp1.blynk.cloud", 80))
    {
      // Dùng mảng char thay cho String
      char server_path[200];
      snprintf(server_path, sizeof(server_path),
               "/external/api/batch/update?token=%s&V41=%.2f&V43=%.2f&V45=%.2f&V47=%.2f",
               BLYNK_AUTH_TOKEN,
               (float)data.timerun_G1 / 3600000.0,
               (float)data.timerun_G2 / 3600000.0,
               (float)data.timerun_B1 / 3600000.0,
               (float)data.timerun_B2 / 3600000.0);

      client.print("GET ");
      client.print(server_path);
      client.print(" HTTP/1.1\r\n");
      client.print("Host: sgp1.blynk.cloud\r\n");
      client.print("Connection: close\r\n\r\n");
      // Chờ server nhận lệnh xong mới đóng
      while (client.connected() || client.available())
      {
        if (client.available())
        {
          String line = client.readStringUntil('\n');
          if (line == "\r")
            break;
        }
      }
      client.stop();
    }
  }
}
void time_run_motor()

{
  if (blynk_first_connect)
  {
    if (data.reset_day != day())
    {
      if (Blynk.connected())
      {
        up_timerun_motor();
        data.timerun_G1 = 0;
        data.timerun_G2 = 0;
        data.timerun_B1 = 0;
        data.timerun_B2 = 0;
        data.reset_day = day();
        savedata();
      }
    }
  }
  if (G1_save || G2_save || B1_save || B2_save)
  {
    if (G1_start != 0)
    {
      data.timerun_G1 = data.timerun_G1 + (millis() - G1_start);
      G1_start = millis();
      G1_save = false;
    }
    if (G2_start != 0)
    {
      data.timerun_G2 = data.timerun_G2 + (millis() - G2_start);
      G2_start = millis();
      G2_save = false;
    }
    if (B1_start != 0)
    {
      data.timerun_B1 = data.timerun_B1 + (millis() - B1_start);
      B1_start = millis();
      B1_save = false;
    }
    if (B2_start != 0)
    {
      data.timerun_B2 = data.timerun_B2 + (millis() - B2_start);
      B2_start = millis();
      B2_save = false;
    }
    savedata();
  }
}
void togglePin()
{
  pinState = !pinState;
  digitalWrite(customPin, pinState);
}

void on_comp1()
{
  status_comp1 = HIGH; // Xuất LOW kích Relay
  pcf8575_1.digitalWrite(pin_comp1, status_comp1);
}
void off_comp1()
{
  status_comp1 = LOW; // Xuất HIGH nhả Relay
  pcf8575_1.digitalWrite(pin_comp1, status_comp1);
}
void on_comp2()
{
  status_comp2 = HIGH;
  pcf8575_1.digitalWrite(pin_comp2, status_comp2);
}
void off_comp2()
{
  status_comp2 = LOW;
  pcf8575_1.digitalWrite(pin_comp2, status_comp2);
}
void on_G1()
{                   // Bật bơm (nhả Relay - NC đóng)
  status_g1 = HIGH; // Xuất LOW để nhả Relay
  pcf8575_1.digitalWrite(btn_g1, status_g1);
}

void off_G1()
{                  // Tắt bơm (kích Relay - NC mở)
  status_g1 = LOW; // Xuất HIGH để kích Relay
  pcf8575_1.digitalWrite(btn_g1, status_g1);
}

void on_G2()
{                   // Bật bơm
  status_g2 = HIGH; // Xuất LOW để nhả Relay
  pcf8575_1.digitalWrite(btn_g2, status_g2);
}

void off_G2()
{                  // Tắt bơm
  status_g2 = LOW; // Xuất HIGH để kích Relay
  pcf8575_1.digitalWrite(btn_g2, status_g2);
}

void hidden()
{
  Blynk.setProperty(V11, "isHidden", true);
  Blynk.setProperty(V18, "isHidden", true);
  Blynk.setProperty(V7, "isHidden", true);
  Blynk.setProperty(V6, "isHidden", true);
  Blynk.setProperty(V5, "isHidden", true);
  Blynk.setProperty(V17, "isHidden", true);
  Blynk.setProperty(V13, "isHidden", true);
}
void visible()
{
  Blynk.setProperty(V11, "isHidden", false);
  Blynk.setProperty(V18, "isHidden", false);
  Blynk.setProperty(V7, "isHidden", false);
  Blynk.setProperty(V6, "isHidden", false);
  Blynk.setProperty(V5, "isHidden", false);
  Blynk.setProperty(V17, "isHidden", false);
  Blynk.setProperty(V13, "isHidden", false);
}
void syncstatus()
{
  Blynk.virtualWrite(V4, data.mode_cap2);
  Blynk.virtualWrite(V2, status_g1);
  Blynk.virtualWrite(V3, status_g2);
}
void i2c_scaner()
{
  byte error, address;
  int nDevices;
  String stringOne;

  nDevices = 0;
  for (address = 1; address < 127; address++)
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      stringOne = String(address, HEX);
      if (address < 16)
        Blynk.virtualWrite(V10, "I2C device found at address 0x0", stringOne,
                           " !\n");
      Blynk.virtualWrite(V10, "I2C device found at address 0x", stringOne,
                         " !\n");
      nDevices++;
    }
    else if (error == 4)
    {
      stringOne = String(address, HEX);

      if (address < 16)
        Blynk.virtualWrite(V10, "Unknown error at address 0x0", stringOne,
                           " !\n");
      Blynk.virtualWrite(V10, "I2C device found at address 0x", stringOne,
                         " !\n");
    }
  }
  if (nDevices == 0)
    Blynk.virtualWrite(V10, "No I2C devices found\n");
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void readCompressor1() // Máy nén 1
{
  pcf8575_2.digitalWrite(S0pin, LOW);
  pcf8575_2.digitalWrite(S1pin, LOW);
  pcf8575_2.digitalWrite(S2pin, HIGH);
  pcf8575_2.digitalWrite(S3pin, LOW);
  float rms4 = emon4.calcIrms(740);

  if (rms4 < 2)
  {
    Irms4 = 0;
    yIrms4 = 0;
    xSetAmpe4 = 0;
  }
  else
  {
    Irms4 = rms4;
    yIrms4 = yIrms4 + 1;
    if (yIrms4 > 3)
    {
      if ((Irms4 >= data.SetAmpe4max) || (Irms4 <= data.SetAmpe4min))
      {
        xSetAmpe4 = xSetAmpe4 + 1;
        if ((xSetAmpe4 >= 3) && (keyp))
        {
          off_comp1(); // Ngắt máy nén
          trip4 = true;
          xSetAmpe4 = 0;
          if (keynoti)
          {
            Blynk.logEvent("error", String("Máy nén 1 lỗi: ") + Irms4 + String(" A"));
          }
        }
      }
      else
      {
        xSetAmpe4 = 0;
      }
    }
  }
}

void readCompressor2() // Máy nén 2
{
  pcf8575_2.digitalWrite(S0pin, HIGH);
  pcf8575_2.digitalWrite(S1pin, LOW);
  pcf8575_2.digitalWrite(S2pin, HIGH);
  pcf8575_2.digitalWrite(S3pin, LOW);
  float rms5 = emon5.calcIrms(740);

  if (rms5 < 2)
  {
    Irms5 = 0;
    yIrms5 = 0;
    xSetAmpe5 = 0;
  }
  else
  {
    Irms5 = rms5;
    yIrms5 = yIrms5 + 1;
    if (yIrms5 > 3)
    {
      if ((Irms5 >= data.SetAmpe5max) || (Irms5 <= data.SetAmpe5min))
      {
        xSetAmpe5 = xSetAmpe5 + 1;
        if ((xSetAmpe5 >= 3) && (keyp))
        {
          off_comp2(); // Ngắt máy nén
          trip5 = true;
          xSetAmpe5 = 0;
          if (keynoti)
          {
            Blynk.logEvent("error", String("Máy nén 2 lỗi: ") + Irms5 + String(" A"));
          }
        }
      }
      else
      {
        xSetAmpe5 = 0;
      }
    }
  }
}

void readPower0() // C0 Cấp 1 - 1 - I0
{
  pcf8575_2.digitalWrite(S0pin, LOW);
  pcf8575_2.digitalWrite(S1pin, LOW);
  pcf8575_2.digitalWrite(S2pin, LOW);
  pcf8575_2.digitalWrite(S3pin, LOW);
  float rms0 = emon0.calcIrms(740);
  if (rms0 < 3)
  {
    Irms0 = 0;
    yIrms0 = 0;
    if (G1_start != 0)
    {
      data.timerun_G1 = data.timerun_G1 + (millis() - G1_start);
      savedata();
      G1_start = 0;
    }
  }
  else if (rms0 >= 3)
  {
    Irms0 = rms0;
    yIrms0 = yIrms0 + 1;
    if (yIrms0 > 3)
    {
      if (G1_start >= 0)
      {
        if (G1_start == 0)
          G1_start = millis();
        else if (millis() - G1_start > 60000)
        {
          G1_save = true;
        }
        else
          G1_save = false;
      }
      if ((Irms0 >= data.SetAmpemax) || (Irms0 <= data.SetAmpemin))
      {
        xSetAmpe = xSetAmpe + 1;
        if ((xSetAmpe >= 4) && (keyp))
        {
          off_G1();
          xSetAmpe = 0;
          trip0 = true;
          if (keynoti)
          {
            Blynk.logEvent("error",
                           String("Giếng 1 lỗi: ") + Irms0 + String(" A"));
          }
        }
      }
      else
        xSetAmpe = 0;
    }
  }
}
void readPower1() // C1 - Cấp 1 - 2 - I1
{
  pcf8575_2.digitalWrite(S0pin, HIGH);
  pcf8575_2.digitalWrite(S1pin, LOW);
  pcf8575_2.digitalWrite(S2pin, LOW);
  pcf8575_2.digitalWrite(S3pin, LOW);
  float rms1 = emon1.calcIrms(740);
  if (rms1 < 3)
  {
    Irms1 = 0;
    yIrms1 = 0;
    if (G2_start != 0)
    {
      data.timerun_G2 = data.timerun_G2 + (millis() - G2_start);
      savedata();
      G2_start = 0;
    }
  }
  else if (rms1 >= 3)
  {
    Irms1 = rms1;
    yIrms1 = yIrms1 + 1;
    if (yIrms1 > 3)
    {
      if (G2_start >= 0)
      {
        if (G2_start == 0)
        {
          G2_start = millis();
        }
        else if (millis() - G2_start > 60000)
        {
          G2_save = true;
        }
        else
          G2_save = false;
      }
      if ((Irms1 >= data.SetAmpe1max) || (Irms1 <= data.SetAmpe1min))
      {
        xSetAmpe1 = xSetAmpe1 + 1;
        if ((xSetAmpe1 >= 3) && (keyp))
        {
          off_G2();
          trip1 = true;
          xSetAmpe1 = 0;
          if (keynoti)
          {
            Blynk.logEvent("error",
                           String("Giếng 2 lỗi: ") + Irms1 + String(" A"));
          }
        }
      }
      else
        xSetAmpe1 = 0;
    }
  }
}
void readPower2() // C2- Bơm 1  - I2
{
  pcf8575_2.digitalWrite(S0pin, LOW);
  pcf8575_2.digitalWrite(S1pin, HIGH);
  pcf8575_2.digitalWrite(S2pin, LOW);
  pcf8575_2.digitalWrite(S3pin, LOW);
  float rms2 = emon2.calcIrms(740);
  if (rms2 < 3)
  {
    Irms2 = 0;
    yIrms2 = 0;
    if (B1_start != 0)
    {
      data.timerun_B1 = data.timerun_B1 + (millis() - B1_start);
      savedata();
      B1_start = 0;
    }
  }
  else if (rms2 >= 3)
  {
    Irms2 = rms2;
    yIrms2 = yIrms2 + 1;
    if (yIrms2 > 15)
    {
      if (B1_start >= 0)
      {
        if (B1_start == 0)
          B1_start = millis();
        else if (millis() - B1_start > 60000)
        {
          B1_save = true;
        }
        else
          B1_save = false;
      }
      if ((Irms2 >= data.SetAmpe2max) || ((Irms2 <= data.SetAmpe2min) && (Result1 < 2.4)))
      {
        xSetAmpe2 = xSetAmpe2 + 1;
        if ((xSetAmpe2 >= 2) && (keyp))
        {
          xSetAmpe2 = 0;
          trip2 = true; // Lưu cờ lỗi Bơm 1
          if (keynoti)
            Blynk.logEvent("error", String("Bơm 1 lỗi: ") + Irms2 + String(" A"));
        }
      }
      else
        xSetAmpe2 = 0;
    }
  }
}
void readPower3() // C3 - Bơm 2  - I3
{
  pcf8575_2.digitalWrite(S0pin, HIGH);
  pcf8575_2.digitalWrite(S1pin, HIGH);
  pcf8575_2.digitalWrite(S2pin, LOW);
  pcf8575_2.digitalWrite(S3pin, LOW);
  float rms3 = emon3.calcIrms(740);
  if (rms3 < 3)
  {
    Irms3 = 0;
    yIrms3 = 0;
    if (B2_start != 0)
    {
      data.timerun_B2 = data.timerun_B2 + (millis() - B2_start);
      savedata();
      B2_start = 0;
    }
  }
  else if (rms3 >= 3)
  {
    Irms3 = rms3;
    yIrms3 = yIrms3 + 1;
    if (yIrms3 > 3)
    {
      if (B2_start >= 0)
      {
        if (B2_start == 0)
          B2_start = millis();
        else if (millis() - B2_start > 60000)
        {
          B2_save = true;
        }
        else
          B2_save = false;
      }
      if ((Irms3 >= data.SetAmpe3max) || ((Irms3 <= data.SetAmpe3min) && (Result1 < 2.4)))
      {
        xSetAmpe3 = xSetAmpe3 + 1;
        if ((xSetAmpe3 >= 15) && (keyp))
        {
          xSetAmpe3 = 0;
          trip3 = true; // Lưu cờ lỗi Bơm 2
          if (keynoti)
            Blynk.logEvent("error", String("Bơm 2 lỗi: ") + Irms3 + String(" A"));
        }
      }
      else
        xSetAmpe3 = 0;
    }
  }
}
//-------------------------------------------------------------------
// --- CÁC HÀM HỖ TRỢ HIỆU CHUẨN ĐA ĐIỂM ---
void sortCalibPoints(CalibPoint points[], uint8_t num_points)
{
  for (uint8_t i = 1; i < num_points; i++)
  {
    CalibPoint key = points[i];
    int8_t j = i - 1;
    while (j >= 0 && points[j].adc > key.adc)
    {
      points[j + 1] = points[j];
      j--;
    }
    points[j + 1] = key;
  }
}

void addOrUpdateCalibPoint(CalibPoint new_point, CalibPoint points[], uint8_t &num_points)
{
  if (num_points < MAX_CALIB_POINTS)
  {
    points[num_points] = new_point;
    num_points++;
  }
  else
  {
    int8_t closest_idx = -1;
    uint16_t min_diff = 65535;
    for (uint8_t i = 0; i < num_points; i++)
    {
      uint16_t diff = abs((int)points[i].value - (int)new_point.value);
      if (closest_idx == -1 || diff < min_diff)
      {
        min_diff = diff;
        closest_idx = i;
      }
    }
    if (closest_idx != -1)
      points[closest_idx] = new_point;
  }
  sortCalibPoints(points, num_points);
}

float interpolate(float current_adc, const CalibPoint points[], uint8_t num_points)
{
  if (num_points < 2)
    return (num_points == 1) ? (float)points[0].value : 0.0f;
  const CalibPoint *p1, *p2;
  if (current_adc <= points[0].adc)
  {
    p1 = &points[0];
    p2 = &points[1];
  }
  else if (current_adc >= points[num_points - 1].adc)
  {
    p1 = &points[num_points - 2];
    p2 = &points[num_points - 1];
  }
  else
  {
    uint8_t i = 0;
    while (i < num_points - 1 && current_adc > points[i + 1].adc)
      i++;
    p1 = &points[i];
    p2 = &points[i + 1];
  }
  float x = current_adc;
  float x1 = p1->adc, y1 = p1->value;
  float x2 = p2->adc, y2 = p2->value;
  if (abs(x2 - x1) < 0.001)
    return y1;
  return y1 + (x - x1) * (y2 - y1) / (x2 - x1);
}

void readPressure() // C14 - Ap Luc
{
  pcf8575_2.digitalWrite(S0pin, LOW);
  pcf8575_2.digitalWrite(S1pin, HIGH);
  pcf8575_2.digitalWrite(S2pin, HIGH);
  pcf8575_2.digitalWrite(S3pin, HIGH);
  int raw_value = analogRead(A0);

  // 1. Áp dụng Median Filter
  pressure_median_buffer[pressure_median_index] = raw_value;
  pressure_median_index = (pressure_median_index + 1) % MEDIAN_WINDOW_SIZE;

  int sorted_buffer[MEDIAN_WINDOW_SIZE];
  memcpy(sorted_buffer, pressure_median_buffer, sizeof(pressure_median_buffer));
  int median_value = getMedian(sorted_buffer, MEDIAN_WINDOW_SIZE);

  // 2. Đưa vào bộ lọc Kalman
  kalman_filtered_pressure_adc =
      pressureKalmanFilter.updateEstimate(median_value);

  // 3. Áp dụng công thức hiệu chuẩn (Nội suy đa điểm)
  float val = interpolate(kalman_filtered_pressure_adc, data.pressure_points, data.num_pressure_points);
  Result1 = val / 100.0f;
  Result1 = constrain(Result1, 0.0f, 15.0f); // Giới hạn an toàn (bar)
}
void MeasureCmForSmoothing() // c15
{
  pcf8575_2.digitalWrite(S0pin, HIGH);
  pcf8575_2.digitalWrite(S1pin, HIGH);
  pcf8575_2.digitalWrite(S2pin, HIGH);
  pcf8575_2.digitalWrite(S3pin, HIGH);
  int raw_value = analogRead(A0);

  // 1. Áp dụng Median Filter
  level_median_buffer[level_median_index] = raw_value;
  level_median_index = (level_median_index + 1) % MEDIAN_WINDOW_SIZE;

  int sorted_buffer[MEDIAN_WINDOW_SIZE];
  memcpy(sorted_buffer, level_median_buffer, sizeof(level_median_buffer));
  int median_value = getMedian(sorted_buffer, MEDIAN_WINDOW_SIZE);

  // 2. Đưa vào bộ lọc Kalman
  kalman_filtered_level_adc = levelKalmanFilter.updateEstimate(median_value);

  // 3. Áp dụng công thức hiệu chuẩn (Nội suy đa điểm)
  smoothDistance = interpolate(kalman_filtered_level_adc, data.level_points, data.num_level_points);
  smoothDistance = constrain(smoothDistance, 0.0, dosau * 1.5);
  volume1 = (dai * (long)smoothDistance * rong) / 1000000;
}
//-------------------------------------------------------------------
// Hàm giám sát Biến Tần (Chân P7) cố định theo ca
void process_cycle_lọc_1()
{
  if (timer_rualoc_1 != -1)
  {
    timer.deleteTimer(timer_rualoc_1);
    timer_rualoc_1 = -1;
  }

  // Nếu trạng thái EEPROM ghi nhận Lọc 1 đang tắt hoặc đã chuyển sang chế độ khác, không chạy tiếp
  if (data.rualoc != 1 && data.rualoc != 3)
  { // Giả định trạng thái kết hợp
    pcf8575_1.digitalWrite(pin_van1, LOW);
    return;
  }

  // PHA 1: Mở Van 1 trong 2 phút
  pcf8575_1.digitalWrite(pin_van1, LOW);

  // Hẹn giờ sau 2 phút (120000 ms) chuyển sang PHA 2
  timer_rualoc_1 = timer.setTimeout(120000L, []()
                                    {
    // PHA 2: Đóng Van 1 trong 8 phút
    pcf8575_1.digitalWrite(pin_van1, HIGH);

    // Hẹn giờ sau 8 phút lặp lại chu kỳ
    timer_rualoc_1 = timer.setTimeout(300000L, process_cycle_lọc_1); });
}

// --- Chu kỳ cho Rửa Lọc 2 (Van 2) ---
void process_cycle_lọc_2()
{
  if (timer_rualoc_2 != -1)
  {
    timer.deleteTimer(timer_rualoc_2);
    timer_rualoc_2 = -1;
  }

  if (data.rualoc != 2 && data.rualoc != 3)
  {
    pcf8575_1.digitalWrite(pin_van2, LOW);
    return;
  }

  // PHA 1: Mở Van 2 trong 2 phút
  pcf8575_1.digitalWrite(pin_van2, LOW);

  // Hẹn giờ sau 2 phút (120000 ms) chuyển sang PHA 2
  timer_rualoc_2 = timer.setTimeout(120000L, []()
                                    {
    // PHA 2: Đóng Van 2 trong 8 phút
    pcf8575_1.digitalWrite(pin_van2, HIGH);

    // Hẹn giờ sau 8 phút lặp lại chu kỳ
    timer_rualoc_2 = timer.setTimeout(300000L, process_cycle_lọc_2); });
}

void rtctime()
{
  timerun = millis();
  DateTime now = rtc_module.now();

  // SỬA LỖI 1: Chỉ lấy giờ từ Blynk đè lên mạch RTC khi Blynk đã kết nối và có giờ thực tế (Năm > 2023).
  // Tránh việc hệ thống lấy năm mặc định (1970) khi vừa khởi động gây nhảy ca làm việc.
  if (blynk_first_connect == true && year() > 2023)
  {
    if ((now.day() != day()) || (now.hour() != hour()) ||
        ((now.minute() - minute() > 2) || (minute() - now.minute() > 2)))
    {
      rtc_module.adjust(
          DateTime(year(), month(), day(), hour(), minute(), second()));
      now = rtc_module.now(); // Cập nhật lại mốc thời gian now sau khi đã đồng bộ
    }
  }

  int nowtime = (now.hour() * 3600 + now.minute() * 60);

  if (data.mode_cap2 == 2)
  { // HỆ THỐNG ĐANG Ở CHẾ ĐỘ AUTO
    bool is_in_working_hours = false;

    // Xác định dải "Giờ chạy 2 bơm"
    if (data.start_time < data.stop_time)
    {
      if ((nowtime >= data.start_time) && (nowtime < data.stop_time))
        is_in_working_hours = true;
    }
    else if (data.start_time > data.stop_time)
    {
      if ((nowtime >= data.start_time) || (nowtime < data.stop_time))
        is_in_working_hours = true;
    }

    {
      // =================================================================
      // MỐC 1: BƯỚC VÀO GIỜ CHẠY 2 BƠM -> KHÔNG ĐỔI BƠM, GIỮ NGUYÊN BIẾN TẦN
      // =================================================================
      if (is_in_working_hours && data.check_changeday == 0)
      {
        data.check_changeday = 1;
        savedata();
      }
      // =================================================================
      // MỐC 2: KẾT THÚC GIỜ CHẠY 2 BƠM -> ĐỔI BƠM BIẾN TẦN CHO CA SAU
      // =================================================================
      else if (!is_in_working_hours && data.check_changeday == 1)
      {
        data.check_changeday = 0;
        request_pump_swap = true; // Kích hoạt lệnh đổi bơm DUY NHẤT khi hết giờ
        savedata();
      }
    }

    // XỬ LÝ QUY TRÌNH ĐẢO BƠM AN TOÀN (CHỈ CHẠY 1 LẦN KHI HẾT GIỜ LÀM VIỆC)
    if (request_pump_swap)
    {
      request_pump_swap = false;

      // 1. Ngắt toàn bộ lệnh chạy bơm để rơ-le nhả an toàn
      pcf8575_1.digitalWrite(btn_left, HIGH);  // Tắt lệnh Biến tần (P8 NC)
      pcf8575_1.digitalWrite(btn_right, HIGH); // Tắt lệnh Trực tiếp (P9 NC)
      current_pump_mode = 0;

      // 2. Tiến hành đảo rơ-le chuyển mạch P7 sang bơm còn lại
      data.status_btn_mid = (data.status_btn_mid == HIGH) ? LOW : HIGH;
      savedata();

      // 3. Đợi đúng 10 giây cho biến tần xả hết điện rồi mới đóng rơ-le khởi động lại
      if (timer_run_main != -1)
        timeout.deleteTimer(timer_run_main);
      timer_run_main = timeout.setTimeout(10000L, []()
                                          {
         
         pcf8575_1.digitalWrite(btn_mid, data.status_btn_mid); // Đóng rơ-le chuyển mạch P7
         
         timeout.setTimeout(1000L, []() { // Trễ 1s ổn định tiếp điểm rơ-le trước khi cho chạy
           pcf8575_1.digitalWrite(btn_right, HIGH); // Khóa cứng ngõ ra trực tiếp P9 bên ngoài ca
           
           // Khởi động bơm biến tần mới được chuyển giao để chạy duy trì ngoài ca
           if (data.status_btn_mid == HIGH) {
             if (!trip2) pcf8575_1.digitalWrite(btn_left, LOW); // Bật Bơm 1 chạy Biến tần
             current_pump_mode = 4;
           } else {
             if (!trip3) pcf8575_1.digitalWrite(btn_left, LOW); // Bật Bơm 2 chạy Biến tần
             current_pump_mode = 3;
           }
           timer_run_main = -1;
         }); });
    }

    // VÒNG LẶP DUY TRÌ VÀ GIÁM SÁT TRẠNG THÁI ỔN ĐỊNH (Khi không trong quá trình đảo ca)
    else if (timer_run_main == -1)
    {
      if (is_in_working_hours)
      {
        // TRONG CA: Tự động gọi cả 2 bơm chạy đồng thời (Bơm biến tần ổn định từ trước, không cần ngắt quãng)
        if (data.status_btn_mid == HIGH && current_pump_mode != 1)
        {
          pcf8575_1.digitalWrite(btn_mid, HIGH);
          if (!trip2)
            pcf8575_1.digitalWrite(btn_left, LOW); // Bơm 1 Biến tần duy trì
          if (!trip3)
            pcf8575_1.digitalWrite(btn_right, LOW); // Bơm 2 gọi thêm chạy Trực tiếp
          current_pump_mode = 1;
        }
        else if (data.status_btn_mid == LOW && current_pump_mode != 2)
        {
          pcf8575_1.digitalWrite(btn_mid, LOW);
          if (!trip3)
            pcf8575_1.digitalWrite(btn_left, LOW); // Bơm 2 Biến tần duy trì
          if (!trip2)
            pcf8575_1.digitalWrite(btn_right, LOW); // Bơm 1 gọi thêm chạy Trực tiếp
          current_pump_mode = 2;
        }
      }
      else
      {
        // NGOÀI CA: Đảm bảo chỉ có 1 bơm biến tần hoạt động, bơm trực tiếp luôn tắt
        if (data.status_btn_mid == HIGH && current_pump_mode != 4)
        {
          pcf8575_1.digitalWrite(btn_mid, HIGH);
          pcf8575_1.digitalWrite(btn_right, HIGH);
          if (!trip2)
            pcf8575_1.digitalWrite(btn_left, LOW);
          current_pump_mode = 4;
        }
        else if (data.status_btn_mid == LOW && current_pump_mode != 3)
        {
          pcf8575_1.digitalWrite(btn_mid, LOW);
          pcf8575_1.digitalWrite(btn_right, HIGH);
          if (!trip3)
            pcf8575_1.digitalWrite(btn_left, LOW);
          current_pump_mode = 3;
        }
      }
    }
    Blynk.virtualWrite(V0, (current_pump_mode == 1 || current_pump_mode == 4) ? 1 : 0);
    Blynk.virtualWrite(V1, (current_pump_mode == 2 || current_pump_mode == 3) ? 1 : 0);
  }
}
BLYNK_WRITE(V0) // NÚT NHẤN BƠM 1
{
  if ((key) && (data.mode_cap2 == 1))
  {
    if (param.asInt() == 1)
    {
      status_v0 = HIGH;
      // --- LỆNH BẬT BƠM 1 ---
      if (data.status_btn_mid == HIGH)
      {
        pcf8575_1.digitalWrite(btn_left, LOW); // Bơm 1 đang nối Biến Tần -> Kích chạy Biến Tần
      }
      else
      {
        pcf8575_1.digitalWrite(btn_right, LOW); // Bơm 1 đang nối Trực Tiếp -> Kích chạy Trực Tiếp
      }
    }
    else
    {
      status_v0 = LOW;
      // --- LỆNH TẮT BƠM 1 ---
      if (data.status_btn_mid == HIGH) // Nếu Bơm 1 đang chạy Biến Tần mà bị chủ động tắt
      {
        // 1. Ngắt lệnh chạy biến tần và lệnh trực tiếp ngay lập tức để đảm bảo an toàn cơ điện
        pcf8575_1.digitalWrite(btn_left, HIGH);
        pcf8575_1.digitalWrite(btn_right, HIGH);

        // Chờ 10s rồi mới kiểm tra xem người dùng có tắt Bơm 2 không
        timeout.setTimeout(10000L, []()
                           {
          if (status_v1 == HIGH) 
          {
            // 2. Thay đổi trạng thái bộ nhớ: chuyển quyền chạy Biến tần sang cho Bơm 2
            data.status_btn_mid = LOW;
            savedata();

            // Đảo rơ-le chọn bơm sang Bơm 2 (lúc này biến tần đã xả điện tử 10s trước)
            pcf8575_1.digitalWrite(btn_mid, data.status_btn_mid); 
            
            // Trễ thêm 1 giây cho tiếp điểm rơ-le ổn định hoàn toàn rồi mới kích chạy biến tần cho Bơm 2
            timeout.setTimeout(1000L, []() {
              if (!trip3) { // Điều kiện: Bơm 2 không bị lỗi quá dòng/sự cố trước đó
                pcf8575_1.digitalWrite(btn_left, LOW); // Kích chạy biến tần cho Bơm 2
              }
            }); 
          }
        });
      }
      else // Nếu Bơm 1 đang chạy Trực Tiếp thì chỉ ngắt trực tiếp thông thường
      {
        pcf8575_1.digitalWrite(btn_right, HIGH);
      }
    }
  }
  else
  {
    // Nếu chưa nhập mã bảo vệ (mh), trả lại trạng thái nút nhấn cũ trên App
    Blynk.virtualWrite(V0, status_v0);
  }
}

BLYNK_WRITE(V1) // NÚT NHẤN BƠM 2
{
  if ((key) && (data.mode_cap2 == 1))
  {
    if (param.asInt() == 1)
    {
      status_v1 = HIGH;
      // --- LỆNH BẬT BƠM 2 ---
      if (data.status_btn_mid == HIGH)
      {
        pcf8575_1.digitalWrite(btn_right, LOW); // Bơm 2 đang nối Trực Tiếp -> Kích chạy Trực Tiếp
      }
      else
      {
        pcf8575_1.digitalWrite(btn_left, LOW); // Bơm 2 đang nối Biến Tần -> Kích chạy Biến Tần
      }
    }
    else
    {
      status_v1 = LOW;
      // --- LỆNH TẮT BƠM 2 ---
      if (data.status_btn_mid == LOW) // Nếu Bơm 2 đang chạy Biến Tần mà bị chủ động tắt
      {
        // 1. Ngắt lệnh chạy biến tần và lệnh trực tiếp ngay lập tức để bảo vệ hệ thống
        pcf8575_1.digitalWrite(btn_left, HIGH);
        pcf8575_1.digitalWrite(btn_right, HIGH);

        // Chờ 10s rồi mới kiểm tra xem người dùng có tắt Bơm 1 không
        timeout.setTimeout(10000L, []()
                           {
          if (status_v0 == HIGH) 
          {
            // 2. Thay đổi trạng thái bộ nhớ: chuyển quyền chạy Biến tần về lại cho Bơm 1
            data.status_btn_mid = HIGH;
            savedata();

            // Đảo rơ-le chọn bơm sang Bơm 1
            pcf8575_1.digitalWrite(btn_mid, data.status_btn_mid); 
            
            // Trễ thêm 1 giây ổn định cơ khí tiếp điểm rơ-le rồi mới kích chạy biến tần cho Bơm 1
            timeout.setTimeout(1000L, []() {
              if (!trip2) { // Điều kiện: Bơm 1 không bị lỗi quá dòng/sự cố trước đó
                pcf8575_1.digitalWrite(btn_left, LOW); // Kích chạy biến tần cho Bơm 1
              }
            }); 
          }
        });
      }
      else // Nếu Bơm 2 đang chạy Trực Tiếp thì chỉ ngắt trực tiếp thông thường
      {
        pcf8575_1.digitalWrite(btn_right, HIGH);
      }
    }
  }
  else
  {
    // Nếu chưa nhập mã bảo vệ (mh), trả lại trạng thái nút nhấn cũ trên App
    Blynk.virtualWrite(V1, status_v1);
  }
}
BLYNK_WRITE(V2)
{
  if (key)
  {
    if ((param.asInt() == HIGH) && (!trip0))
    {
      on_G1();
    }
    else
      off_G1();
  }
  else
  {
    Blynk.virtualWrite(V2, status_g1);
  }
}
// Điều khiển Bơm Cấp 1 - Máy 2
BLYNK_WRITE(V3)
{
  if (key)
  {
    if ((param.asInt() == HIGH) && (!trip1))
    {
      on_G2();
    }
    else
      off_G2();
  }
  else
  {
    Blynk.virtualWrite(V3, status_g2);
  }
}
BLYNK_WRITE(V4) // Chọn chế độ (Man / Auto / Off)
{
  if (key)
  {
    switch (param.asInt())
    {
    case 0:
    { // TẮT TOÀN BỘ TRẠM
      data.mode_cap2 = 0;
      pcf8575_1.digitalWrite(btn_left, HIGH);  // Tắt Biến tần (P8 NC)
      pcf8575_1.digitalWrite(btn_right, HIGH); // Tắt Trực tiếp (P9 NC)
      current_pump_mode = 0;
      break;
    }
    case 1:
    { // Chế độ MAN
      data.mode_cap2 = 1;
      Blynk.setProperty(V0, "isHidden", false);
      Blynk.setProperty(V1, "isHidden", false);
      break;
    }
    case 2:
    { // Chế độ AUTO
      data.mode_cap2 = 2;
      Blynk.setProperty(V0, "isHidden", true);
      Blynk.setProperty(V1, "isHidden", true);
      break;
    }
    }
  }
  else
  {
    Blynk.virtualWrite(V4, data.mode_cap2);
  }
}
BLYNK_WRITE(V5) // Chọn thiết bị cài Ampe
{
  int motor_choice = param.asInt();
  c = motor_choice;

  switch (motor_choice)
  {
  case 0:
    Blynk.virtualWrite(V6, 0);
    Blynk.virtualWrite(V7, 0);
    break;
  case 1:
    Blynk.virtualWrite(V6, data.SetAmpemin);
    Blynk.virtualWrite(V7, data.SetAmpemax);
    break;
  case 2:
    Blynk.virtualWrite(V6, data.SetAmpe1min);
    Blynk.virtualWrite(V7, data.SetAmpe1max);
    break;
  case 3:
    Blynk.virtualWrite(V6, data.SetAmpe2min);
    Blynk.virtualWrite(V7, data.SetAmpe2max);
    break;
  case 4:
    Blynk.virtualWrite(V6, data.SetAmpe3min);
    Blynk.virtualWrite(V7, data.SetAmpe3max);
    break;
  case 5:
    Blynk.virtualWrite(V6, data.SetAmpe4min);
    Blynk.virtualWrite(V7, data.SetAmpe4max);
    break;
  case 6:
    Blynk.virtualWrite(V6, data.SetAmpe5min);
    Blynk.virtualWrite(V7, data.SetAmpe5max);
    break;
  }
}
BLYNK_WRITE(V6) // Cài đặt ampe MIN
{
  if (key_config) // Yêu cầu phải ở chế độ 'active'
  {
    if (c == 1)
    {
      data.SetAmpemin = param.asInt();
    }
    else if (c == 2)
    {
      data.SetAmpe1min = param.asInt();
    }
    else if (c == 3)
    {
      data.SetAmpe2min = param.asInt();
    }
    else if (c == 4)
    {
      data.SetAmpe3min = param.asInt();
    }
    else if (c == 5)
    {
      data.SetAmpe4min = param.asInt();
    }
    else if (c == 6)
    {
      data.SetAmpe5min = param.asInt();
    }
    savedata();
  }
  else
  {
    // Cập nhật lại UI nếu cố tình kéo thanh trượt mà chưa gõ 'active'
    if (c == 1)
    {
      Blynk.virtualWrite(V6, data.SetAmpemin);
    }
    else if (c == 2)
    {
      Blynk.virtualWrite(V6, data.SetAmpe1min);
    }
    else if (c == 3)
    {
      Blynk.virtualWrite(V6, data.SetAmpe2min);
    }
    else if (c == 4)
    {
      Blynk.virtualWrite(V6, data.SetAmpe3min);
    }
    else if (c == 5)
    {
      Blynk.virtualWrite(V6, data.SetAmpe4min);
    }
    else if (c == 6)
    {
      Blynk.virtualWrite(V6, data.SetAmpe5min);
    }
    else
    {
      Blynk.virtualWrite(V6, 0);
    }
  }
}

BLYNK_WRITE(V7) // Cài đặt ampe MAX
{
  if (key_config)
  {
    if (c == 1)
    {
      data.SetAmpemax = param.asInt();
    }
    else if (c == 2)
    {
      data.SetAmpe1max = param.asInt();
    }
    else if (c == 3)
    {
      data.SetAmpe2max = param.asInt();
    }
    else if (c == 4)
    {
      data.SetAmpe3max = param.asInt();
    }
    else if (c == 5)
    {
      data.SetAmpe4max = param.asInt();
    }
    else if (c == 6)
    {
      data.SetAmpe5max = param.asInt();
    }
    savedata(); // ---> GHI NGAY VÀO EEPROM KHI CÓ THAY ĐỔI
  }
  else
  {
    // Nếu chưa mở khóa, giữ nguyên hiển thị giá trị thực tế của máy hiện tại
    if (c == 1)
    {
      Blynk.virtualWrite(V7, data.SetAmpemax);
    }
    else if (c == 2)
    {
      Blynk.virtualWrite(V7, data.SetAmpe1max);
    }
    else if (c == 3)
    {
      Blynk.virtualWrite(V7, data.SetAmpe2max);
    }
    else if (c == 4)
    {
      Blynk.virtualWrite(V7, data.SetAmpe3max);
    }
    else if (c == 5)
    {
      Blynk.virtualWrite(V7, data.SetAmpe4max);
    }
    else if (c == 6)
    {
      Blynk.virtualWrite(V7, data.SetAmpe5max);
    }
    else
    {
      Blynk.virtualWrite(V7, 0);
    }
  }
}
BLYNK_WRITE(V8) // info
{
  if (param.asInt() == 1)
  {
    terminal.clear();
    if (data.mode_cap2 == 1)
    {
      Blynk.virtualWrite(V10, "MODE: MAN");
    }
    else if (data.mode_cap2 == 2)
    {
      int hour_start = data.start_time / 3600;
      int minute_start = (data.start_time - (hour_start * 3600)) / 60;
      int hour_stop = data.stop_time / 3600;
      int minute_stop = (data.stop_time - (hour_stop * 3600)) / 60;
      Blynk.virtualWrite(V10, "MODE: AUTO\nThời gian chạy 2 bơm: ", hour_start,
                         ":", minute_start, " -> ", hour_stop, " : ",
                         minute_stop);
    }
  }
  else
  {
    terminal.clear();
  }
  timer.restartTimer(timer_1);
  timer.restartTimer(timer_2);
}
BLYNK_WRITE(V10) // String
{
  String dataS = param.asStr();
  if (dataS == "mh" || dataS == "MH")
  {
    terminal.clear();
    key = true;
    Blynk.virtualWrite(V10,
                       "Đơn vị vận hành: CN-Mộc Hóa\nKích hoạt trong 15s\n");
    timeout.setTimeout(15000, []()
                       {
      key = false;
      savedata(); 
      Blynk.virtualWrite(V10, "Đã khóa và Tự động lưu cấu hình!\n");
      terminal.clear(); });
  }
  else if (dataS == "active")
  {
    terminal.clear();
    c = 0;
    visible();
    key = true;
    key_config = true; // MỞ KHÓA quyền thay đổi cấu hình
  }
  else if (dataS == "deactive")
  {
    terminal.clear();
    c = 0;
    key = false;
    key_config = false; // ĐÓNG KHÓA quyền thay đổi cấu hình
    hidden();
    Blynk.virtualWrite(V10, "Ok!\nNhập mã để điều khiển!\n");
  }
  else if (dataS == "save")
  {
    terminal.clear();
    savedata();
    Blynk.virtualWrite(V10, "Đã lưu cài đặt.\n");
  }
  else if (dataS == "reset")
  {
    terminal.clear();
    trip0 = false;
    trip1 = false;
    trip2 = false;
    trip3 = false;
    trip4 = false;
    trip5 = false;
    on_G1();
    on_G2();
    Blynk.virtualWrite(V10, "Đã RESET! \nNhập mã để điều khiển!\n");
  }
  else if (dataS == "save_num")
  {
    terminal.clear();
    Blynk.virtualWrite(V10, "Số lần ghi EEPROM: ", data.save_num);
  }
  else if (dataS == "rst")
  {
    terminal.clear();
    Blynk.virtualWrite(V10, "ESP khởi động lại sau 3s");
    ESP.restart();
  }
  else if (dataS == "update")
  {
    terminal.clear();
    savedata();
    if (!writeOtaRtcState(OTA_REQUESTED))
    {
      Blynk.virtualWrite(V10, "Không thể tạo yêu cầu OTA. Thiết bị chưa khởi động lại.\n");
      return;
    }
    Blynk.virtualWrite(V10,
                       "Đã nhận yêu cầu OTA. Thiết bị sẽ khởi động lại vào chế độ cập nhật sạch...\n");
    Blynk.run();
    delay(300);
    ESP.restart();
  }
  else if (dataS == "i2c")
  {
    Blynk.virtualWrite(V10, "Firmware: ", BLYNK_FIRMWARE_VERSION,
                       "\nLast OTA: ", lastOtaStatus,
                       "\nFree heap: ", ESP.getFreeHeap(),
                       "\nMax block: ", ESP.getMaxFreeBlockSize(), "\n");
    i2c_scaner();
  }
  else if (dataS == "calib")
  {
    terminal.clear();
    Blynk.virtualWrite(V10, "--- THÔNG TIN HIỆU CHUẨN ---\n");
    Blynk.virtualWrite(V10, "[CẢM BIẾN ÁP SUẤT]\n");
    char buff[100];
    snprintf(buff, sizeof(buff), " - Số điểm: %d/%d\n", data.num_pressure_points, MAX_CALIB_POINTS);
    Blynk.virtualWrite(V10, buff);
    for (uint8_t i = 0; i < data.num_pressure_points; i++)
    {
      snprintf(buff, sizeof(buff), " #%d: ADC=%d -> %.2f bar\n", i + 1, data.pressure_points[i].adc, data.pressure_points[i].value / 100.0f);
      Blynk.virtualWrite(V10, buff);
    }
    snprintf(buff, sizeof(buff), " - ADC hiện tại: %.2f\n", kalman_filtered_pressure_adc);
    Blynk.virtualWrite(V10, buff);

    Blynk.virtualWrite(V10, "[CẢM BIẾN MỰC NƯỚC]\n");
    snprintf(buff, sizeof(buff), " - Số điểm: %d/%d\n", data.num_level_points, MAX_CALIB_POINTS);
    Blynk.virtualWrite(V10, buff);
    for (uint8_t i = 0; i < data.num_level_points; i++)
    {
      snprintf(buff, sizeof(buff), " #%d: ADC=%d -> %d cm\n", i + 1, data.level_points[i].adc, data.level_points[i].value);
      Blynk.virtualWrite(V10, buff);
    }
    snprintf(buff, sizeof(buff), " - ADC hiện tại: %.2f\n", kalman_filtered_level_adc);
    Blynk.virtualWrite(V10, buff);
  }
  else if (dataS == "pre_clear")
  {
    if (key)
    {
      data.num_pressure_points = 0;
      savedata();
      Blynk.virtualWrite(V10, "Đã xóa calib áp suất.\n");
    }
  }
  else if (dataS.startsWith("pre_"))
  {
    if (key)
    {
      float p_known = dataS.substring(4).toFloat();
      CalibPoint pt;
      pt.adc = (uint16_t)round(kalman_filtered_pressure_adc);
      pt.value = (uint16_t)(p_known * 100);
      addOrUpdateCalibPoint(pt, data.pressure_points, data.num_pressure_points);
      savedata();
      char buff[64];
      snprintf(buff, sizeof(buff), "Đã lưu điểm áp suất: ADC=%d -> %.2f bar\n", pt.adc, p_known);
      Blynk.virtualWrite(V10, buff);
    }
  }
  else if (dataS == "level_clear")
  {
    if (key)
    {
      data.num_level_points = 0;
      savedata();
      Blynk.virtualWrite(V10, "Đã xóa calib mực nước.\n");
    }
  }
  else if (dataS.startsWith("level_"))
  {
    if (key)
    {
      float l_known = dataS.substring(6).toFloat();
      CalibPoint pt;
      pt.adc = (uint16_t)round(kalman_filtered_level_adc);
      pt.value = (uint16_t)l_known;
      addOrUpdateCalibPoint(pt, data.level_points, data.num_level_points);
      savedata();
      char buff[64];
      snprintf(buff, sizeof(buff), "Đã lưu điểm mực nước: ADC=%d -> %d cm\n", pt.adc, pt.value);
      Blynk.virtualWrite(V10, buff);
    }
  }
  else if (dataS == "help")
  {
    terminal.clear();
    Blynk.virtualWrite(V10, "Danh sách lệnh:\n"
                            "- mh: Bật quyền điều khiển 15s\n"
                            "- active / deactive / save / rst / update\n"
                            "- calib: Xem thông tin calib\n"
                            "- pre_[val]: Calib áp suất tại [val] bar\n"
                            "- pre_clear: Xóa calib áp suất\n"
                            "- level_[val]: Calib mực nước tại [val] cm\n"
                            "- level_clear: Xóa calib mực nước\n");
  }
  else
  {
    Blynk.virtualWrite(V10, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
BLYNK_WRITE(V11) // Chọn thời gian chạy 2 Bơm
{
  switch (param.asInt())
  {
  case 0:
  { // ...
    b = 0;
    Blynk.virtualWrite(V18, 0);
    break;
  }
  case 1:
  {
    b = 1;
    Blynk.virtualWrite(V18, data.start_time, data.stop_time, tz);
    break;
  }
  }
}
BLYNK_WRITE(V13) // Bảo vệ
{
  if (key)
  {
    int data13 = param.asInt();
    if (data13 == LOW)
    {
      keyp = false;
    }
    else
    {
      keyp = true;
    }
  }
  else
    Blynk.virtualWrite(V13, keyp);
}
// ============================================================================
// Hàm tự động đảo trạng thái Van: Mở 2 phút (120000ms), Đóng 8 phút (480000ms)

BLYNK_WRITE(V15)
{
  if (key)
  {
    int btn_state = param.asInt();

    if (btn_state == HIGH)
    {
      // Cập nhật trạng thái bit/bộ nhớ cho rualoc
      if (data.rualoc == 2)
        data.rualoc = 3; // 3 nghĩa là bật cả hai
      else if (data.rualoc == 0)
        data.rualoc = 1;

      if (data.LLG1_RL == 0)
      {
        data.LLG1_RL = LLG1_1m3;
      }
      process_cycle_lọc_1(); // Chạy chu kỳ riêng cho van 1
    }
    else
    {
      if (data.rualoc == 3)
        data.rualoc = 2;
      else
        data.rualoc = 0;

      if (timer_rualoc_1 != -1)
      {
        timer.deleteTimer(timer_rualoc_1);
        timer_rualoc_1 = -1;
      }
      pcf8575_1.digitalWrite(pin_van1, HIGH); // Tắt van 1 ngay lập tức

      if (data.rualoc == 0 && data.LLG1_RL != 0)
      {
        Blynk.virtualWrite(V31, LLG1_1m3 - data.LLG1_RL);
        data.LLG1_RL = 0;
      }
    }
    savedata();
  }
  else
  {
    Blynk.virtualWrite(V15, (data.rualoc == 1 || data.rualoc == 3) ? HIGH : LOW);
  }
}

// --- XỬ LÝ NÚT NHẤN RỬA LỌC 2 (V16) ---
BLYNK_WRITE(V16)

{
  if (key)
  {
    int btn_state = param.asInt();

    if (btn_state == HIGH)
    {
      if (data.rualoc == 1)
        data.rualoc = 3; // 3 nghĩa là bật cả hai
      else if (data.rualoc == 0)
        data.rualoc = 2;

      if (data.LLG1_RL == 0)
      {
        data.LLG1_RL = LLG1_1m3;
      }
      process_cycle_lọc_2(); // Chạy chu kỳ riêng cho van 2
    }
    else
    {
      if (data.rualoc == 3)
        data.rualoc = 1;
      else
        data.rualoc = 0;

      if (timer_rualoc_2 != -1)
      {
        timer.deleteTimer(timer_rualoc_2);
        timer_rualoc_2 = -1;
      }
      pcf8575_1.digitalWrite(pin_van2, HIGH); // Tắt van 2 ngay lập tức

      if (data.rualoc == 0 && data.LLG1_RL != 0)
      {
        Blynk.virtualWrite(V31, LLG1_1m3 - data.LLG1_RL);
        data.LLG1_RL = 0;
      }
    }
    savedata();
  }
  else
  {
    Blynk.virtualWrite(V16, (data.rualoc == 2 || data.rualoc == 3) ? HIGH : LOW);
  }
}
BLYNK_WRITE(V39) // Điều khiển Máy nén 1
{
  if (key)
  { // Yêu cầu mở khóa bảo mật (nhập mã 'mh') mới cho phép thao tác
    if ((param.asInt() == HIGH) && (!trip4))
    {
      on_comp1();
    }
    else
    {
      off_comp1();
    }
  }
  else
  {
    // Trả lại trạng thái cũ trên app nếu chưa mở khóa bảo vệ
    Blynk.virtualWrite(V39, status_comp1 == LOW ? HIGH : LOW);
  }
}

BLYNK_WRITE(V48) // Điều khiển Máy nén 2
{
  if (key)
  {
    if ((param.asInt() == HIGH) && (!trip5))
    {
      on_comp2();
    }
    else
    {
      off_comp2();
    }
  }
  else
  {
    Blynk.virtualWrite(V48, status_comp2 == LOW ? HIGH : LOW);
  }
}
BLYNK_WRITE(V17) // Thông báo
{
  if (key)
  {
    int data17 = param.asInt();
    if (data17 == LOW)
    {
      keynoti = false;
    }
    else
    {
      keynoti = true;
    }
  }
  else
    Blynk.virtualWrite(V17, keynoti);
}

BLYNK_WRITE(V18) // Nhận thời gian từ Widget Time Input trên Blynk
{
  if (key_config)
  { // Hệ thống yêu cầu phải nhập mã (mh) để mở khóa an toàn
    TimeInputParam t(param);

    // 1. Nhận và quy đổi Giờ Bắt Đầu (Start Time) ra số giây
    if (t.hasStartTime())
    {
      data.start_time = t.getStartHour() * 3600 + t.getStartMinute() * 60;
    }

    // 2. Nhận và quy đổi Giờ Kết Thúc (Stop Time) ra số giây
    if (t.hasStopTime())
    {
      data.stop_time = t.getStopHour() * 3600 + t.getStopMinute() * 60;
    }

    // 3. QUAN TRỌNG: Lưu cấu hình vào EEPROM
    savedata();

    // 4. In thông báo ra Terminal để người vận hành biết đã cài thành công
    char buff[128];
    snprintf(buff, sizeof(buff), "Đã lưu giờ chạy :\nBắt đầu: %02d:%02d\nKết thúc: %02d:%02d\n",
             t.getStartHour(), t.getStartMinute(), t.getStopHour(), t.getStopMinute());
    Blynk.virtualWrite(V10, buff);

    // 5. CẬP NHẬT: Xóa sạch Terminal sau 5 giây bằng lệnh "clr"
    timeout.setTimeout(5000L, []()
                       { Blynk.virtualWrite(V10, "clr"); });
  }
  else
  {
    // Nếu chưa mở khóa, đẩy cảnh báo và reset lại giao diện V18
    Blynk.virtualWrite(V10, "LỖI: Vui lòng nhập mã để thay đổi giờ!\n");
    Blynk.virtualWrite(V18, 0);

    // Tự động xóa dòng cảnh báo lỗi này sau 5s
    timeout.setTimeout(5000L, []()
                       { Blynk.virtualWrite(V10, "clr"); });
  }
}
BLYNK_WRITE(V29) // Lưu lượng G1_1m3
{
  LLG1_1m3 = param.asInt();
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void setup()
{
  Serial.begin(9600);

  // Đưa watchdog ngoài về trạng thái an toàn ngay từ đầu. Nhánh OTA phải được
  // xử lý trước mọi khởi tạo Blynk/I2C/EEPROM và ngõ ra điều khiển.
  pinMode(customPin, OUTPUT);
  digitalWrite(customPin, HIGH);
  ESP.wdtDisable();
  ESP.wdtEnable(300000);

  if (handleOtaBootState())
  {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);

  delay(5000);
  //---------------------------------------------------------------------------------
  
  pcf8575_2.begin();
  pcf8575_2.pinMode(S0pin, OUTPUT);
  pcf8575_2.pinMode(S1pin, OUTPUT);
  pcf8575_2.pinMode(S2pin, OUTPUT);
  pcf8575_2.pinMode(S3pin, OUTPUT);
  emon0.current(A0, 112);
  emon1.current(A0, 112);
  emon2.current(A0, 112);
  emon3.current(A0, 112);
  emon4.current(A0, 112); // Chú ý hiệu chuẩn (112) tùy theo CT bạn dùng
  emon5.current(A0, 112);
  eeprom.initialize();
  byte *p = (byte *)&data;
  for (unsigned int i = 0; i < sizeof(data); i++)
  {
    *p++ = eeprom.readByte(address + i);
  }

  memcpy(&dataCheck, &data, sizeof(data));
  // Khởi tạo giá trị hiệu chuẩn mặc định nếu chưa có (Dựa trên thông số cũ)
  if (data.num_pressure_points == 0 || data.num_pressure_points > MAX_CALIB_POINTS)
  {
    data.pressure_points[0] = {193, 0};    // 0 bar tại ADC 193
    data.pressure_points[1] = {925, 1000}; // 10 bar tại ADC 925
    data.num_pressure_points = 2;
  }
  if (data.num_level_points == 0 || data.num_level_points > MAX_CALIB_POINTS)
  {
    data.level_points[0] = {190, 0};   // 0 cm tại ADC 190
    data.level_points[1] = {890, 800}; // 800 cm tại ADC 890
    data.num_level_points = 2;
  }
  savedata();
  rtc_module.begin();

  pcf8575_1.begin();
  pcf8575_1.pinMode(btn_left, OUTPUT);
  pcf8575_1.digitalWrite(btn_left, !data.status_btn_left);
  pcf8575_1.pinMode(btn_mid, OUTPUT);
  pcf8575_1.digitalWrite(btn_mid, data.status_btn_mid);
  pcf8575_1.pinMode(btn_right, OUTPUT);
  pcf8575_1.digitalWrite(btn_right, !data.status_direct);
  pcf8575_1.pinMode(btn_g1, OUTPUT);
  pcf8575_1.digitalWrite(btn_g1, HIGH);
  pcf8575_1.pinMode(btn_g2, OUTPUT);
  pcf8575_1.digitalWrite(btn_g2, HIGH);
  pcf8575_1.pinMode(P2, OUTPUT);
  pcf8575_1.pinMode(pin_van1, OUTPUT);
  pcf8575_1.digitalWrite(pin_van1, HIGH);
  pcf8575_1.pinMode(pin_van2, OUTPUT);
  pcf8575_1.digitalWrite(pin_van2, HIGH);
  pcf8575_1.pinMode(pin_comp1, OUTPUT);
  pcf8575_1.digitalWrite(pin_comp1, HIGH); // Mặc định tắt
  pcf8575_1.pinMode(pin_comp2, OUTPUT);
  pcf8575_1.digitalWrite(pin_comp2, HIGH); // Mặc định tắt
  pcf8575_1.pinMode(pin0, OUTPUT);
  pcf8575_1.digitalWrite(pin0, HIGH);
  pinMode(customPin, OUTPUT);
  terminal.clear();
  //---------------------------------------------------------------
  timeout.setTimeout(5000, []()
                     {timer.setInterval(1000L, updateTimeBlynk);
   timer_1 = timer.setInterval(1183L, []() {
      // readPower();
      readPower0();
      readPower1();
      readPower2();
      readPower3();
      readCompressor1();
      readCompressor2(); 
      
    
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
     
      
    });
    timer.setInterval(3000L, []() {
      up(); 
  });
    timer_2 = timer.setInterval(251L, []() {
      readPressure();
      MeasureCmForSmoothing();
    });
    
    //-------------------------------
    timer.setInterval(30016L, []() {
      rtctime();
      time_run_motor();
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
    });
    timer.setInterval(900005L, []() {
      connectionstatus();
      timer.restartTimer(timer_1);
      timer.restartTimer(timer_2);
      
    }); });

  data.rualoc = 0;
  timer.setInterval(1000L, togglePin);
  savedata();
}

void loop()
{

  ESP.wdtFeed();
  Blynk.run();
  timeout.run();
  timer.run();
}
