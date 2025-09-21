#include <ESP8266WiFi.h>

// Thông tin WiFi của bạn
const char *ssid = "tram bom so 4";  // Thay YOUR_WIFI_NAME bằng tên WiFi của bạn
const char *password = "0943950555"; // Thay YOUR_PASSWORD bằng mật khẩu WiFi của bạn

int i = 0;
void setup() {
  Serial.begin(115200);
  pinMode(D0, OUTPUT);
  digitalWrite(D0, LOW);
  //  Kết nối WiFi
  WiFi.begin(ssid, password);

  Serial.print("Đang kết nối WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Đã kết nối WiFi thành công!");
}

void loop() {
  /*
  // Đọc cường độ tín hiệu
  long rssi = WiFi.RSSI();

  // Hiển thị kết quả
  Serial.print("Cường độ tín hiệu WiFi (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");

  // Đánh giá chất lượng tín hiệu
  if (rssi >= -50) {
    // Serial.println("Chất lượng: Rất tốt");
  } else if (rssi >= -60) {
    // Serial.println("Chất lượng: Tốt");
  } else if (rssi >= -70) {
    // Serial.println("Chất lượng: Khá");
  } else {
    // Serial.println("Chất lượng: Yếu");
  }

  */
  if (i < 51) {
    digitalWrite(D0, HIGH);
    delay(1000); // Đợi 2 giây trước khi đọc lại
    Serial.println(i++);
    digitalWrite(D0, LOW);
    delay(1000); // Đợi 2 giây trước khi đọc lại
    Serial.println(i++);
  } else {
    delay(1000);
    Serial.println(i++);
  }
}