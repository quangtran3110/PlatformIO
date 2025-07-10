
#include <ModbusRTU.h>
#include <SoftwareSerial.h>

SoftwareSerial S(14, 12);
ModbusRTU mb;

int temp_vdf;

//-------------------------------------------------------------------

uint16_t nhietdo_bientan[1];
bool cbWrite_nhietdo(Modbus::ResultCode event, uint16_t transactionId, void *data) {
  if (event == Modbus::EX_SUCCESS) {
    temp_vdf = (nhietdo_bientan[0]);
    Serial.println("Giá trị: " + String(temp_vdf));
  }
  return true;
}
//-------------------------
void read_modbus() {
  mb.readHreg(1, 8448, nhietdo_bientan, 1, cbWrite_nhietdo);
  while (mb.slave()) {
    mb.task();
    delay(20);
  }
}

//-------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  // Kiểm tra kỹ xem là 8N1 hay 8E1
  S.begin(9600, SWSERIAL_8N1);
  mb.begin(&S);
  mb.master();
  delay(3000);
}
void loop() {
  read_modbus();
  delay(1500);
}