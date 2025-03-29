#define BLYNK_TEMPLATE_ID "TMPL6I6ISEvF5"
#define BLYNK_TEMPLATE_NAME "SUPPORT ACTIVE"
#define BLYNK_AUTH_TOKEN "KqkJXdMncWRT3Vc8n85hzIIk2xr9N76X"
#define BLYNK_FIRMWARE_VERSION "250329"
//-----------------------------
const char *ssid = "Wifi_Modem";
const char *password = "Password";
// const char* ssid = "tram bom so 4";
// const char* password = "0943950555";
//-----------------------------
#define BLYNK_PRINT Serial
#define APP_DEBUG
#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <WidgetRTC.h>
//-----------------------------
#define pin_G_main_dothi "&V99="
#pragma region // ALL TOKEN ID
#define main_dothi_TOKEN "w3ZZc7F4pvOIwqozyrzYcBFVUE3XxSiW"
#define ccd_TOKEN "jaQFoaOgdcZcKbyI_ME_oi6tThEf4FR5"
#define ubndp2_TOKEN "gvfnRXv14oMohtqMWTPQXbduFKww1zfu"
#define alb_TOKEN "1v4Fr0n4m4-GaYP26MMZ3bHbTi5k68nP"
#define ntbinh_TOKEN "5xw1AG7yoX1e7sphhfL2jgc-dPnW9_l2"
#define boke1_TOKEN "egMiTa83bEFFC_YXyMaKNJ0a5dtSNpD0"
#define boke2_TOKEN "T86HBKpJBPvMbJeGRF8mKPEUf83Oik9A"
#define boke3_TOKEN "fjna3o_TwWwy0SMKGNTqevQGqpuCGDuQ"
#define boke4_TOKEN "pTbMkuYkt_SOcW4JWPY2kqDEvxN_XXK0"
#define dhvuong_TOKEN "eBeqi9ZJhRK3r66cUzgdD1gp2xGxG7kS"
#define thpt1_TOKEN "H3VsCxfjXq67ALREdZcKOANCQ_kdFqfg"
#define thpt2_TOKEN "H04PY3egc4KE3YnBQkOFEMNAGohM_oGo"
//------------------
#define T2_G1_TOKEN "L_2oEOyv4bmrdsesIoasyKiEEOFZVgBO"
#define T2_G2_TOKEN "Hc5DgCBzl4Oi5hW_JOaNZ6oBKoGy5kFI"
#define T2_G3_TOKEN "JTnEpJjGVVJ8DM1aJx7zZT4cyNYJrhr_"
#define T2BPT_TOKEN "AdXbklpLJKTZQ5hK9Qpy7Sg5DdwgmQ8z"
#define T3BPT_TOKEN "Q2KAjaqI3sWhET-Ax94VPYfIk2Fmsr36"
#define T4_TOKEN "fQeSuHadv_EFLjXPdqE-sV_lnZ6pXWfu"
#define BHD_TOKEN "PWYW_mopMTAAnpmZOeGH3h4D4QOzZi9X"
#pragma endregion
//-----------------------------
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/StatusDevice/.pio/build/nodemcuv2/firmware.bin"
String server_name = "http://sgp1.blynk.cloud/external/api/";

WiFiClient client;
HTTPClient http;

byte reboot_num = 0;

WidgetRTC rtc_widget;
BlynkTimer timer;
BLYNK_CONNECTED() {
  rtc_widget.begin();
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
      ESP.restart();
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
//-------------------------------------------------------------------
// Biến toàn cục để lưu trạng thái cũ
// Biến toàn cục để lưu trạng thái cũ và thời gian
static int previous_g = -1; // -1 để lần đầu tiên luôn gửi
static unsigned long last_send_time = 0;
const unsigned long SEND_INTERVAL = 15 * 60 * 1000; // 30 phút tính bằng milliseconds

void check_status() {
  String server_path;
  int g = 0;
  // Mảng chứa các token của các module
  const char *tokens[] = {
      ccd_TOKEN,     // Bit 0: Cầu cửa đông
      ubndp2_TOKEN,  // Bit 1: UBND P2
      alb_TOKEN,     // Bit 2: Ao lục bình
      ntbinh_TOKEN,  // Bit 3: N.T.Bình
      boke1_TOKEN,   // Bit 4: Bờ kè 1
      boke2_TOKEN,   // Bit 5: Bờ kè 2
      boke3_TOKEN,   // Bit 6: Bờ kè 3
      boke4_TOKEN,   // Bit 7: Bờ kè 4
      dhvuong_TOKEN, // Bit 8: Đường Hùng Vương
      thpt1_TOKEN,   // Bit 9: THPT 1
      thpt2_TOKEN    // Bit 10: THPT 2
  };

  // Số lượng module cần kiểm tra
  const int moduleCount = sizeof(tokens) / sizeof(tokens[0]);
  // Kiểm tra trạng thái của từng module
  for (int i = 0; i < moduleCount; i++) {
    server_path = server_name + "isHardwareConnected?token=" + tokens[i];
    http.begin(client, server_path.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      String payload = http.getString();
      payload.trim(); // Loại bỏ khoảng trắng thừa
      // So sánh không phân biệt chữ hoa/thường
      bool isConnected = (payload.equalsIgnoreCase("true"));
      bitWrite(g, i, isConnected);
    }
    http.end();
  }
  // Kiểm tra xem có cần gửi dữ liệu không
  // Gửi dữ liệu nếu:
  // 1. Có sự thay đổi trạng thái (g khác previous_g)
  // 2. Hoặc đã đủ 30 phút kể từ lần gửi cuối
  if (g != previous_g || (millis() - last_send_time >= SEND_INTERVAL)) {
    // Gửi dữ liệu
    server_path = server_name + "batch/update?token=" + main_dothi_TOKEN + pin_G_main_dothi + g;
    http.begin(client, server_path.c_str());
    http.GET();
    http.end();

    // Cập nhật thời gian gửi cuối và trạng thái cũ
    last_send_time = millis();
    previous_g = g;
    Serial.println(g);
  }
}
//-------------------------------------------------------------------
void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(300000);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  //-----------------------
  delay(10000);

  timer.setInterval(900005L, []() {
    connectionstatus();
  });
  timer.setInterval(5000, check_status);
}
void loop() {
  ESP.wdtFeed();
  Blynk.run();
  timer.run();
}