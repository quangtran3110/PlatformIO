
#include <BlynkSimpleEsp8266.h>

BlynkTimer timer;

ICACHE_RAM_ATTR void buttonPressed() {
  if (key_pulse) {
    key_pulse = false;
    data.pulse++;
    timer.setTimeout(5000, []() {
      key_pulse = true;
    });
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(D0, INPUT_PULLUP); // Button pin
  attachInterrupt(D6, buttonPressed, RISING);
}

void loop() {
  timer.run();
}
