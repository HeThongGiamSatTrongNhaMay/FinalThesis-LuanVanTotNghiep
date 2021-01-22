#include <Wire.h>
#include "Hshopvn_Pzem004t_V2.h"
#define RX_PZEM     8
#define TX_PZEM     11
Hshopvn_Pzem004t_V2 pzem1(TX_PZEM, RX_PZEM);
static float volt;
static float ampe;
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
}

// Function that executes whenever data is requested by master
// This function is registered as an event, see setup()
void requestEvent()
{
  // Respond with value reported from dac0
  Serial.println(volt);
  Serial.println(ampe);
  uint32_t b = volt*100;
  uint32_t d = ampe*100;
  byte payload[4];
  payload[0] = highByte(b);
  payload[1] = lowByte(b);
  payload[2] = highByte(d);
  payload[3] = lowByte(d);  
  Wire.write(payload,4);
  Serial.println(payload[0]);
  Serial.println(payload[1]);
  Serial.println(payload[2]);
  Serial.println(payload[3]);
}
