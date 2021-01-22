#include <Wire.h>
#include "Hshopvn_Pzem004t_V2.h"
#define RX_PZEM     8
#define TX_PZEM     11
Hshopvn_Pzem004t_V2 pzem1(TX_PZEM, RX_PZEM);
static float volt, PF;
static float ampe, power;
void setup()
{
  // Join i2c bus with address #10
  Serial.begin(115200); 
  Wire.begin(7);      
  Wire.onRequest(requestEvent);      // register event

  // init module
  pzem1.begin();
  pzem1.setTimeout(100);
}

void loop()
{
  delay(500);
  pzem_info pzemData = pzem1.getData();
  volt = pzemData.volt;
  ampe = pzemData.ampe; 
  PF = pzemData.powerFactor;
  power = pzemData.power;
}

// Function that executes whenever data is requested by master
// This function is registered as an event, see setup()
void requestEvent()
{
  // Respond with value reported from dac0
  Serial.println(volt);Serial.println(PF);
  Serial.println(ampe);Serial.println(power);
  uint32_t b = volt*100;uint32_t pf = PF*100;
  uint32_t d = ampe*100;uint32_t p = power*100;
  byte payload[8];
  payload[0] = highByte(b);payload[4] = highByte(pf);
  payload[1] = lowByte(b);payload[5] = lowByte(pf);
  payload[2] = highByte(d);payload[6] = highByte(p);
  payload[3] = lowByte(d);payload[7] = highByte(p);
  Wire.write(payload,8);
  Serial.println(payload[0]);
  Serial.println(payload[1]);
  Serial.println(payload[2]);
  Serial.println(payload[3]);
  Serial.println(payload[4]);
  Serial.println(payload[5]);
  Serial.println(payload[6]);
  Serial.println(payload[7]);
}
