/*
#define BLYNK_TEMPLATE_ID "TMPL6OXV1bmFt"
#define BLYNK_TEMPLATE_NAME "Support"
#define BLYNK_AUTH_TOKEN "7sISC1fJBO2OhzgOrlCUbspnne6I3LWr"
*/
#define BLYNK_TEMPLATE_ID "TMPL6H_Q3XkK9"
#define BLYNK_TEMPLATE_NAME "SUPPORT DEACTIVE"
#define BLYNK_AUTH_TOKEN "IXPS9jkvL6tmMMOqPhhn4YTfXimuEGu4"

#define BLYNK_FIRMWARE_VERSION "CC.TANK"
#define BLYNK_PRINT Serial
#define APP_DEBUG

const char *ssid = "Wifi";
const char *password = "Password";

#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>
WiFiClient client;
HTTPClient http;
String server_name = "http://sgp1.blynk.cloud/external/api/";
String Main = "vcz0jVXPSGPK6XmFP5Dqi_etQA32VNPL";

float thetich;
int xlength = 2400;
int width = 1230;
int height = 314;
byte reboot_num;
int save_num;

#define filterSamples 121
int sensSmoothArray1[filterSamples]; // array for holding raw sensor values for sensor1
int smoothDistance;                  // variables for sensor1 data
//----------------------------------------------------------------------------
int digitalSmooth(int rawIn, int *sensSmoothArray) { // "int *sensSmoothArray" passes an array to the function - the asterisk indicates the array name is a pointer
  int j, k, temp, top, bottom;
  long total;
  static int i;
  static int sorted[filterSamples];
  boolean done;

  i = (i + 1) % filterSamples; // increment counter and roll over if necc. -  % (modulo operator) rolls over variable
  sensSmoothArray[i] = rawIn;  // input new data into the oldest slot

  // Serial.print("raw = ");

  for (j = 0; j < filterSamples; j++) { // transfer data array into anther array for sorting and averaging
    sorted[j] = sensSmoothArray[j];
  }

  done = 0;           // flag to know when we're done sorting
  while (done != 1) { // simple swap sort, sorts numbers from lowest to highest
    done = 1;
    for (j = 0; j < (filterSamples - 1); j++) {
      if (sorted[j] > sorted[j + 1]) { // numbers are out of order - swap
        temp = sorted[j + 1];
        sorted[j + 1] = sorted[j];
        sorted[j] = temp;
        done = 0;
      }
    }
  }
  bottom = max(((filterSamples * 20) / 100), 1);
  top = min((((filterSamples * 80) / 100) + 1), (filterSamples - 1)); // the + 1 is to make up for asymmetry caused by integer rounding
  k = 0;
  total = 0;
  for (j = bottom; j < top; j++) {
    total += sorted[j]; // total remaining indices
    k++;
  }
  return total / k; // divide by number of samples
}
//---------------------------------------------------------------------------------
long distance, distance1; // Duration used to calculate distance

BlynkTimer timer;
BLYNK_CONNECTED() {
}

void send_data() {
  String server_path = server_name + "batch/update?token=" + Main + "&V26=" + smoothDistance + "&V25=" + thetich + "&V40=" + 1;
  http.begin(client, server_path.c_str());
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String payload = http.getString();
  }
  http.end();
}

void savedata() {
  save_num = save_num + 1;
  EEPROM.begin(512);
  delay(20);
  EEPROM.put(160, reboot_num);
  EEPROM.put(166, save_num);
  EEPROM.commit();
  EEPROM.end();
}

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
//---------------------------------------------------------
void MeasureCmForSmoothing() {
  float sensorValue = analogRead(A0);
  Serial.println(distance1);
  distance1 = (((sensorValue - 198) * 500) / (923 - 198)); // 915,74 (R=147.7)
  if (distance1 > 0) {
    smoothDistance = digitalSmooth(distance1, sensSmoothArray1);
    thetich = (xlength * smoothDistance * width) / 1000000;
  }
}
//---------------------------------------------------------------------------------
void setup() {
  ESP.wdtDisable();
  ESP.wdtEnable(300000);
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Blynk.config(BLYNK_AUTH_TOKEN);
  delay(5000);

  EEPROM.begin(512);
  delay(20);
  EEPROM.get(160, reboot_num);
  EEPROM.get(166, save_num);
  EEPROM.end();
  timer.setInterval(205, MeasureCmForSmoothing);
  timer.setInterval(5529, send_data);
  timer.setInterval(300015, connectionstatus); // 5p
}
void loop() {
  ESP.wdtFeed();
  Blynk.run();
  timer.run();
}