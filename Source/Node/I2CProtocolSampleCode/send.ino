#include <Wire.h>;
static float a = 230.5678;
static float c = 0.03; 
void setup()
{
  Serial.begin(115200);
  Wire.begin(11);
  Wire.onRequest(requestEvent);
}
 
void loop()
{
  delay(500);
}

void requestEvent()
{
  uint32_t b = a*100;
  uint32_t d = c*100;
  byte payload[4];
  payload[0] = highByte(b);
  payload[1] = lowByte(b);
  payload[2] = highByte(d);
  payload[3] = lowByte(d);  
  Wire.write(payload,4);
}
