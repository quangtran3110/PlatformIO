/*
#define BLYNK_TEMPLATE_ID "TMPL6OXV1bmFt"
#define BLYNK_TEMPLATE_NAME "Support"
#define BLYNK_AUTH_TOKEN "7sISC1fJBO2OhzgOrlCUbspnne6I3LWr"
*/
#define BLYNK_TEMPLATE_ID "TMPL6H_Q3XkK9"
#define BLYNK_TEMPLATE_NAME "SUPPORT DEACTIVE"
#define BLYNK_AUTH_TOKEN "IXPS9jkvL6tmMMOqPhhn4YTfXimuEGu4"

#define BLYNK_FIRMWARE_VERSION "251014.T2.PRE"

#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>
#include <SimpleKalmanFilter.h>
WiFiClient client;
HTTPClient http;
String server_name = "http://sgp1.blynk.cloud/external/api/";
String Main = "BDm1LNQi_LhtaKAQU8RWUaGbiOyKIcd3";

const char *ssid = "Nha May Nuoc So 2";
const char *password = "02723841572";

WidgetTerminal terminal(V0);
BlynkTimer timer;
BLYNK_CONNECTED() {
}

// --- CẤU HÌNH EEPROM ---
#define EEPROM_SIZE 16        // Dành 16 byte cho việc lưu trữ
#define EEPROM_MAGIC_KEY 0x42 // "Chìa khóa" để xác nhận EEPROM đã có dữ liệu

//----------------------------------
// --- CÁC BIẾN TOÀN CỤC ---
float final_pressure_bar = 0;  // Áp suất cuối cùng (bar) sau khi lọc
float kalman_filtered_adc = 0; // Giá trị ADC đã lọc qua Kalman, dùng cho hiệu chuẩn

// --- CÁC THAM SỐ HIỆU CHUẨN ---
// Thay thế các giá trị hardcode bằng các biến có thể thay đổi qua Blynk
// Giả định ban đầu: 0 bar -> ADC = 197, 10 bar -> ADC = 914.6
float cal_offset_adc = 197.0f;
float cal_gain = 10.0f / (914.6f - 197.0f); // gain = pressure_range / adc_range

// --- BỘ LỌC MEDIAN + KALMAN ---
// Sử dụng cửa sổ nhỏ (3) để giảm độ trễ
#define MEDIAN_WINDOW_SIZE 3
int median_buffer[MEDIAN_WINDOW_SIZE];
int median_buffer_index = 0;

/*
  e_mea: Sai số của phép đo (Measurement Uncertainty). Tăng lên để phản ứng nhanh hơn.
  e_est: Sai số của ước tính (Estimation Uncertainty). Tăng lên để phản ứng nhanh hơn.
  q:     Nhiễu của quá trình (Process Noise).
*/
SimpleKalmanFilter pressureKalmanFilter(2.5, 2.5, 0.01);

// Hàm sắp xếp và lấy trung vị
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

void saveCalibration() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(0, EEPROM_MAGIC_KEY);       // Ghi chìa khóa ở địa chỉ 0
  EEPROM.put(1, cal_offset_adc);           // Ghi offset từ địa chỉ 1
  EEPROM.put(1 + sizeof(float), cal_gain); // Ghi gain vào ngay sau offset
  if (EEPROM.commit()) {
    terminal.println("Luu hieu chuan vao EEPROM thanh cong.");
  } else {
    terminal.println("Loi: Luu EEPROM that bai!");
  }
  EEPROM.end();
}

void loadCalibration() {
  EEPROM.begin(EEPROM_SIZE);
  byte magic_key = EEPROM.read(0);

  if (magic_key == EEPROM_MAGIC_KEY) {
    // EEPROM đã có dữ liệu, tiến hành đọc
    Serial.println("Phat hien du lieu hieu chuan trong EEPROM, dang tai...");
    EEPROM.get(1, cal_offset_adc);
    EEPROM.get(1 + sizeof(float), cal_gain);
  } else {
    // Lần chạy đầu tiên hoặc EEPROM bị lỗi, sử dụng giá trị mặc định và lưu lại
    Serial.println("Khong tim thay du lieu hieu chuan, su dung gia tri mac dinh va luu vao EEPROM.");
    // Các giá trị mặc định đã được khởi tạo ở trên
    // Chỉ cần lưu chúng vào EEPROM
    saveCalibration();
  }
  EEPROM.end();
  Serial.printf("Da tai hieu chuan: Offset=%.2f, Gain=%.5f\n", cal_offset_adc, cal_gain);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void updata() {
  // Chỉ gửi khi giá trị hợp lệ
  if (final_pressure_bar >= 0) {
    String server_path = server_name + "batch/update?token=" + Main + "&V10=" + String(final_pressure_bar, 2) + "&V38=" + 1;
    http.begin(client, server_path.c_str());
    http.GET();
    http.end();
  }
}

void readAndProcessPressure() {
  // 1. Đọc giá trị ADC thô
  int raw_adc = analogRead(A0);

  // 2. Lọc nhiễu đột biến bằng Median Filter
  median_buffer[median_buffer_index] = raw_adc;
  median_buffer_index = (median_buffer_index + 1) % MEDIAN_WINDOW_SIZE;

  int sorted_buffer[MEDIAN_WINDOW_SIZE];
  memcpy(sorted_buffer, median_buffer, sizeof(median_buffer));
  int median_adc = getMedian(sorted_buffer, MEDIAN_WINDOW_SIZE);

  // 3. Làm mượt tín hiệu bằng Kalman Filter
  kalman_filtered_adc = pressureKalmanFilter.updateEstimate(median_adc);

  // 4. Áp dụng công thức hiệu chuẩn để chuyển đổi ADC sang Bar
  // Công thức: Pressure = gain * (ADC_current - ADC_offset)
  final_pressure_bar = cal_gain * (kalman_filtered_adc - cal_offset_adc);

  // Giới hạn giá trị trong khoảng hợp lý (ví dụ: -0.5 đến 12 bar)
  final_pressure_bar = constrain(final_pressure_bar, -0.5, 12.0);
}

BLYNK_WRITE(V0) {
  String cmd = param.asStr();
  cmd.trim();

  if (cmd == "pre_0") {
    cal_offset_adc = kalman_filtered_adc;
    // Khi offset thay đổi, gain cũng cần tính lại nếu điểm max đã được set
    terminal.printf("Da luu diem 0: ADC = %.1f\n", cal_offset_adc);
    saveCalibration(); // Lưu vào EEPROM
    terminal.println("Hay hieu chuan lai diem max (vd: pre_10)");
  } else if (cmd.startsWith("pre_")) {
    float p_known = cmd.substring(4).toFloat();
    if (p_known > 0) {
      float adc_at_known = kalman_filtered_adc;
      if (adc_at_known > cal_offset_adc) {
        cal_gain = p_known / (adc_at_known - cal_offset_adc);
        terminal.printf("Da hieu chuan tai %.1f bar.\n", p_known);
        saveCalibration(); // Lưu vào EEPROM
        terminal.printf("Gain moi: %.5f\n", cal_gain);
      } else {
        terminal.println("Loi: ADC hien tai phai lon hon ADC tai diem 0.");
      }
    }
  } else if (cmd == "info") {
    terminal.printf("Ap suat: %.2f bar\n", final_pressure_bar);
    terminal.printf("ADC (loc): %.1f\n", kalman_filtered_adc);
    terminal.printf("Offset (ADC): %.1f\n", cal_offset_adc);
    terminal.printf("Gain: %.5f\n", cal_gain);
  }
  terminal.flush();
}
void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);

  // Tải các giá trị hiệu chuẩn từ EEPROM khi khởi động
  loadCalibration();

  // Khởi tạo bộ đệm median với giá trị đọc đầu tiên để tránh kết quả sai lúc khởi động
  int initial_adc = analogRead(A0);
  for (int i = 0; i < MEDIAN_WINDOW_SIZE; i++) {
    median_buffer[i] = initial_adc;
  }

  timer.setInterval(1603L, updata);
  timer.setInterval(175L, readAndProcessPressure);
}
void loop() {
  Blynk.run();
  timer.run();
}