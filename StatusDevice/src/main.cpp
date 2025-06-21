#define BLYNK_TEMPLATE_ID "TMPL6I6ISEvF5"
#define BLYNK_TEMPLATE_NAME "SUPPORT ACTIVE"
#define BLYNK_AUTH_TOKEN "KqkJXdMncWRT3Vc8n85hzIIk2xr9N76X"
#define BLYNK_FIRMWARE_VERSION "250621"
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
#pragma region // ALL TOKEN ID
#define pin_G_main_dothi "&V99="
#define main_dothi_TOKEN "w3ZZc7F4pvOIwqozyrzYcBFVUE3XxSiW"
#define ccd_TOKEN "t9ZvFY3Syiz5C_efTM-PAS5mg0PGJgxv"
#define ubndp2_TOKEN "gvfnRXv14oMohtqMWTPQXbduFKww1zfu"
#define alb_TOKEN "5AVrDcfdNEFsPxEHeQoO-X_pHu4cI4Jc"
#define ntbinh_TOKEN "5xw1AG7yoX1e7sphhfL2jgc-dPnW9_l2"
#define boke1_TOKEN "egMiTa83bEFFC_YXyMaKNJ0a5dtSNpD0"
#define boke2_TOKEN "T86HBKpJBPvMbJeGRF8mKPEUf83Oik9A"
#define boke3_TOKEN "fjna3o_TwWwy0SMKGNTqevQGqpuCGDuQ"
#define boke4_TOKEN "pTbMkuYkt_SOcW4JWPY2kqDEvxN_XXK0"
#define dhvuong_TOKEN "5xw1AG7yoX1e7sphhfL2jgc-dPnW9_l2"
#define thpt1_TOKEN "H3VsCxfjXq67ALREdZcKOANCQ_kdFqfg"
#define thpt2_TOKEN "H04PY3egc4KE3YnBQkOFEMNAGohM_oGo"
//------------------
#define pin_G_main_denduong "&V99="
#define main_denduong_TOKEN "Ol3VH8Hv_OX2JKUWl4ENBk6Rqgh3P3MQ"
#define binhtan1_TOKEN "tCAptndMM6EXqRkWvj_6tK76_mi7gbKf"
//------------------
#define T2_G1_TOKEN "L_2oEOyv4bmrdsesIoasyKiEEOFZVgBO"
#define T2_G2_TOKEN "Hc5DgCBzl4Oi5hW_JOaNZ6oBKoGy5kFI"
#define T2_G3_TOKEN "JTnEpJjGVVJ8DM1aJx7zZT4cyNYJrhr_"
#define T2BPT_TOKEN "AdXbklpLJKTZQ5hK9Qpy7Sg5DdwgmQ8z"
#define T3BPT_TOKEN "Q2KAjaqI3sWhET-Ax94VPYfIk2Fmsr36"
#define T4_TOKEN "RyDZuYiRC4oaG5MsFI2kw4WsQpKiw2Ko"
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
static int previous_g_dothi = -1; // -1 để lần đầu tiên luôn gửi
static int previous_g_denduong = -1;
static unsigned long last_send_time = 0;
const unsigned long SEND_INTERVAL = 15 * 60 * 1000; // 15 phút tính bằng milliseconds

void makeHttpRequest(const char *token, bool &isConnected) {
  String server_path = server_name + "isHardwareConnected?token=" + token;
  http.begin(client, server_path.c_str());
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String payload = http.getString();
    payload.trim();
    isConnected = (payload.equalsIgnoreCase("true"));
  }
  http.end();
}

void sendBatchUpdate(const char *token, const char *pin, int value) {
  String server_path = server_name + "batch/update?token=" + token + pin + value;
  http.begin(client, server_path.c_str());
  http.GET();
  http.end();
}

struct DeviceInfo {
  const char *token;
  const char *name;
  unsigned long offlineTimer;
  bool notificationSent;
};

DeviceInfo devices[] = {
    {T2_G1_TOKEN, "LLuong_T2_G1", 0, false},
    {T2_G2_TOKEN, "LLuong_T2_G2", 0, false},
    {T2_G3_TOKEN, "LLuong_T2_G3", 0, false},
    {T2BPT_TOKEN, "LLuong_T2BPT", 0, false},
    {T3BPT_TOKEN, "LLuong_T3BPT", 0, false},
    {T4_TOKEN, "LLuong_T4", 0, false},
    {BHD_TOKEN, "LLuong_BHD", 0, false}};

void check_status() {
  // int dem = millis();
  String server_path;
  int g_dothi = 0;
  int g_denduong = 0;
  //---------------------------------------------------------------------------------
  const char *tokens_dothi[] = {
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
  const int moduleCount_dothi = sizeof(tokens_dothi) / sizeof(tokens_dothi[0]);
  // Kiểm tra trạng thái của từng module
  for (int i = 0; i < moduleCount_dothi; i++) {
    bool isConnected = false;
    makeHttpRequest(tokens_dothi[i], isConnected);
    bitWrite(g_dothi, i, isConnected);
  }
  //---------------------------------------------------------------------------------
  const char *tokens_denduong[] = {
      binhtan1_TOKEN // Bit 0: Bình Tân 1
  };
  // Số lượng module cần kiểm tra
  const int moduleCount_denduong = sizeof(tokens_denduong) / sizeof(tokens_denduong[0]);
  // Kiểm tra trạng thái của từng module
  for (int i = 0; i < moduleCount_denduong; i++) {
    bool isConnected = false;
    makeHttpRequest(tokens_denduong[i], isConnected);
    bitWrite(g_denduong, i, isConnected);
  }

  //---------------------------------------------------------------------------------
  // Kiểm tra Lưu lượng giếng
  const int moduleCount_LLuong = sizeof(devices) / sizeof(devices[0]);

  for (int i = 0; i < moduleCount_LLuong; i++) {
    bool isConnected = false;
    makeHttpRequest(devices[i].token, isConnected);

    if (!isConnected) {
      if (devices[i].offlineTimer == 0) {
        devices[i].offlineTimer = millis();
      } else if (!devices[i].notificationSent &&
                 (millis() - devices[i].offlineTimer >= 5 * 60 * 1000)) { // 5 phút
        String message = String(devices[i].name) + " OFFLINE";
        Blynk.logEvent("STA", message);
        devices[i].notificationSent = true;
      }
    } else {
      // Reset các biến theo dõi khi thiết bị online trở lại
      devices[i].offlineTimer = 0;
      devices[i].notificationSent = false;
    }
  }
  //---------------------------------------------------------------------------------
  if (g_dothi != previous_g_dothi) {
    sendBatchUpdate(main_dothi_TOKEN, pin_G_main_dothi, g_dothi);
    previous_g_dothi = g_dothi;
  }
  if (g_denduong || previous_g_denduong) {
    sendBatchUpdate(main_denduong_TOKEN, pin_G_main_denduong, g_denduong);
    previous_g_denduong = g_denduong;
  }
  if (millis() - last_send_time >= SEND_INTERVAL) {
    // Gửi dữ liệu
    sendBatchUpdate(main_dothi_TOKEN, pin_G_main_dothi, g_dothi);
    sendBatchUpdate(main_denduong_TOKEN, pin_G_main_denduong, g_denduong);
    // Cập nhật thời gian gửi cuối
    last_send_time = millis();
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