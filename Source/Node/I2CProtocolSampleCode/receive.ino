#include <Wire.h>;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
    byte payload[4];
    Wire.requestFrom(11, 4);   
    payload[0] = Wire.read();
    payload[1] = Wire.read();
    payload[2] = Wire.read();
    payload[3] = Wire.read();
    uint32_t a = (payload[0] << 8) | payload[1];
    uint32_t b = (payload[2] << 8) | payload[3];
    Serial.println((float)a/100);
    Serial.println((float)b/100);
    delay(5000);
}
