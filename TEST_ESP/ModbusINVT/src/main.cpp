#define BLYNK_TEMPLATE_ID "TMPL6mc-8Z360"
#define BLYNK_TEMPLATE_NAME "TEST"
#define BLYNK_AUTH_TOKEN "ytegflpR47cyAPi9JKBvRVfhVaFD8tfT"
#define BLYNK_FIRMWARE_VERSION "250711"
#define BLYNK_PRINT Serial
#define APP_DEBUG

#include "myBlynkAir.h"
#include <BlynkSimpleEsp8266.h>
#include <ModbusRTU.h>
#include <SoftwareSerial.h>

const char *ssid = "Tram Nuoc Dang Thi Manh";
const char *password = "123456789";
SoftwareSerial S(14, 12);
ModbusRTU mb;

//-----------------------------
WiFiClient client;
HTTPClient http;
#define URL_fw_Bin "https://raw.githubusercontent.com/quangtran3110/PlatformIO/main/TEST_ESP/ModbusINVT/.pio/build/nodemcuv2/firmware.bin"
String server_name = "http://sgp1.blynk.cloud/external/api/";
//-----------------------------
int reboot_num = 0;

WidgetTerminal terminal(V3);
BlynkTimer timer;
BLYNK_CONNECTED() {
}

//-------------------------
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
//----------------------------------------------------------------

int32_t int32_2int16(int int1, int int2) {
  union i32_2i16 {
    int32_t f;
    uint16_t i[2];
  };
  union i32_2i16 f_number;
  f_number.i[0] = int1;
  f_number.i[1] = int2;
  return f_number.f;
}
bool cbWrite(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
  }
  return true;
}

//-------------------------------------------------------------------
const char *status_vdf_list[] = {
    "None",            // 0 (không dùng)
    "Forward running", // 1
    "Reverse running", // 2
    "Stopped",         // 3
    "Fault",           // 4
    "PoFF",            // 5
    "Pre-exciting"     // 6
};
uint16_t status_vdf[1];
bool cbWrite_status_vdf(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    int value = status_vdf[0];
    if (value >= 1 && value <= 6) {
      Blynk.virtualWrite(V0, status_vdf_list[value]);
      Serial.println("Giá trị: " + String(status_vdf_list[value]));
    }
  }
  return true;
}
//-------------------------------------------------------------------
uint16_t fault_code[1];
const char *fault_code_list[] = {
    "None", // 0 (không dùng)
    "OUt1", "OUt2", "OUt3", "OV1", "OV2", "OV3", "OC1", "OC2", "OC3", "UV",
    "OL1", "OL2", "SPI", "SPO", "OH1", "OH2", "EF", "CE", "ItE", "tE",
    "EEP", "PIDE", "bCE", "END", "OL3", "PCE", "UPE", "DNE", "ETH1", "ETH2",
    "dEu", "STo", "LL", "OT", "E-Err", "F1-Er", "F2-Er", "C1-Er", "C2-Er",
    "E-DP", "E-NET", "E-CAN", "E-PN", "E-CAT", "E-BAC", "E-DEV", "ESCAN",
    "S-Err", "FrOST", "BLOCK", "Dr"};

bool cbWrite_fault_code(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    int value = fault_code[0];
    if (value >= 0 && value <= 51) {
      Blynk.virtualWrite(V2, fault_code_list[value]);
      Serial.println("fault code: " + String(fault_code_list[value]));
    }
  }
  return true;
}
//-------------------------
void read_modbus() {
  { // Trạng thái biến tần
    mb.readHreg(1, 8448, status_vdf, 1, cbWrite_status_vdf);
    while (mb.slave()) {
      mb.task();
      delay(20);
    }
  }
  { // Trạng thái biến tần
    mb.readHreg(1, 8450, fault_code, 1, cbWrite_fault_code);
    while (mb.slave()) {
      mb.task();
      delay(20);
    }
  }
}
BLYNK_WRITE(V1) // Resset loi
{
  if (param.asInt() == 1) {
    mb.writeHreg(1, 8192, 7, cbWrite);
    while (mb.slave()) { // Check if transaction is active
      mb.task();
      delay(20);
    }
  }
}
BLYNK_WRITE(V3) // String
{
  String dataS = param.asStr();
  if (dataS == "update") {
    terminal.clear();
    Blynk.virtualWrite(V3, "ESP UPDATE...");
    update_fw();
  } else {
    Blynk.virtualWrite(V3, "Mật mã sai.\nVui lòng nhập lại!\n");
  }
}
//-------------------------------------------------------------------
void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(300000);
  //-----------------------
  pinMode(D1, OUTPUT);
  digitalWrite(D1, HIGH);
  pinMode(D2, OUTPUT);
  digitalWrite(D2, HIGH);
  //-----------------------
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(10000);
  // Kiểm tra kỹ xem là 8N1 hay 8E1
  S.begin(9600, SWSERIAL_8N1);
  mb.begin(&S);
  mb.master();

  timer.setInterval(3000, []() {
    read_modbus();
  });
}
void loop() {
  ESP.wdtFeed();
  Blynk.run();
  timer.run();
}