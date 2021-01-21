#include <sys/time.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <TimeLib.h>	// https://github.com/PaulStoffregen/Time)
#include <WiFiUdp.h> 

extern "C" {
#include "user_interface.h"
#include "lwip/err.h"
#include "lwip/dns.h"
}

#include "gBase64.h"			// https://github.com/adamvr/arduino-base64 (I changed the name)
#include "ESP-sc-gway.h"	// This file contains configuration of GWay

int debug = 1;									// Debug level! 0 is no msgs, 1 normal, 2 is extensive

using namespace std;

char message[256];
char b64[256];
bool sx1272 = true;								// Actually we use sx1276/RFM95
byte receivedbytes;

uint32_t cp_nb_rx_rcv;
uint32_t cp_nb_rx_ok;
uint32_t cp_nb_rx_bad;
uint32_t cp_nb_rx_nocrc;
uint32_t cp_up_pkt_fwd;

enum sf_t { SF7 = 7, SF8, SF9, SF10, SF11, SF12 };

uint8_t MAC_array[6];
char MAC_char[18] = {0};


/*******************************************************************************

   Configure these values if necessary!

 *******************************************************************************/

// SX1276 - ESP8266 connections
int ssPin = DEFAULT_PIN_SS;
int dio0  = DEFAULT_PIN_DIO0;
int RST   = DEFAULT_PIN_RST;

// Set spreading factor (SF7 - SF12)
sf_t sf = SF7;

// Set center frequency. If in doubt, choose the first one, comment all others
// Each "real" gateway should support the first 3 frequencies according to LoRa spec.
uint32_t  freq = 486300000; 					// Channel 0, 486.3 MHz
//uint32_t  freq = 486500000; 					// Channel 1, 486.5 MHz
//uint32_t  freq = 486700000; 					// in Mhz! (486.7)
//uint32_t  freq = 486900000; 					// in Mhz! (486.9)
//uint32_t  freq = 487100000; 					// in Mhz! (487.1)
//uint32_t  freq = 487300000; 					// in Mhz! (487.3)
//uint32_t  freq = 487500000; 					// in Mhz! (487.5)
//uint32_t  freq = 487700000; 					// in Mhz! (487.7)

IPAddress ttnServer;							// IP Address of thethingsnetwork server

WiFiUDP Udp;
uint32_t lasttime;


// ----------------------------------------------------------------------------
// DIE is not use actively in the source code anymore.
// It is replaced by a Serial.print command so we know that we have a problem
// somewhere.
// There are at least 3 other ways to restart the ESP. Pick one if you want.
// ----------------------------------------------------------------------------
void die(const char *s)
{
  Serial.println(s);
  delay(50);
  // system_restart();						// SDK function
  // ESP.reset();
  abort();									// Within a second
}


// ----------------------------------------------------------------------------
// Convert a float to string for printing
// f is value to convert
// p is precision in decimal digits
// val is character array for results
// ----------------------------------------------------------------------------
void ftoa(float f, char *val, int p) {
  int j = 1;
  int ival, fval;
  char b[6];

  for (int i = 0; i < p; i++) {
    j = j * 10;
  }

  ival = (int) f;								// Make integer part
  fval = (int) ((f - ival) * j);					// Make fraction. Has same sign as integer part
  if (fval < 0) fval = -fval;					// So if it is negative make fraction positive again.
  // sprintf does NOT fit in memory
  strcat(val, itoa(ival, b, 10));
  strcat(val, ".");							// decimal point

  itoa(fval, b, 10);
  for (int i = 0; i < (p - strlen(b)); i++) strcat(val, "0");
  // Fraction can be anything from 0 to 10^p , so can have less digits
  strcat(val, b);
}


// ----------------------------------------------------------------------------
// GET THE DNS SERVER IP address
// ----------------------------------------------------------------------------
IPAddress getDnsIP() {
  const ip_addr_t* dns_ip = dns_getserver(0);
  IPAddress dns = IPAddress(dns_ip->addr);
  return ((IPAddress) dns);
}


// ----------------------------------------------------------------------------
// Read a package
//
// ----------------------------------------------------------------------------
void readUdp(int packetSize)
{
  uint8_t receiveBuffer[64]; //buffer to hold incoming packet
  Udp.read(receiveBuffer, packetSize);
  receiveBuffer[packetSize] = 0;
  IPAddress remoteIpNo = Udp.remoteIP();
  unsigned int remotePortNo = Udp.remotePort();
  if (debug >= 1) {
    Serial.print(F(" Received packet of size "));
    Serial.print(packetSize);
    Serial.print(F(" From "));
    Serial.print(remoteIpNo);
    Serial.print(F(", port "));
    Serial.print(remotePortNo);
    Serial.print(F(", Contents: 0x"));
    for (int i = 0; i < packetSize; i++) {
      Serial.printf("%02X:", receiveBuffer[i]);
    }
    Serial.println();
  }
}



// ----------------------------------------------------------------------------
// Send an UDP/DGRAM message to the MQTT server
// If we send to more than one host (not sure why) then we need to set sockaddr
// before sending.
// ----------------------------------------------------------------------------
void sendUdp(char *msg, int length) {
  int l;
  bool err = true ;
  //send the update
  Udp.beginPacket(ttnServer, (int) _TTN_PORT);

  if ((l = Udp.write((char *)msg, length)) != length) {
    Serial.println(F("sendUdp:: Error write"));
  } else {
    err = false;
    
    Serial.printf("sendUdp: sent %d bytes", l);
   
  }

  yield();
  Udp.endPacket();
}





// =================================================================================
// LORA GATEWAY FUNCTIONS
// The LoRa supporting functions are in the section below

// ----------------------------------------------------------------------------
// The SS (Chip select) pin is used to make sure the RFM95 is selected
// ----------------------------------------------------------------------------
inline void selectreceiver()
{
  digitalWrite(ssPin, LOW);
}

// ----------------------------------------------------------------------------
// ... or unselected
// ----------------------------------------------------------------------------
inline void unselectreceiver()
{
  digitalWrite(ssPin, HIGH);
}

// ----------------------------------------------------------------------------
// Read one byte value, par addr is address
// Returns the value of register(addr)
// ----------------------------------------------------------------------------
byte readRegister(byte addr)
{
  selectreceiver();
  SPI.beginTransaction(SPISettings(50000, MSBFIRST, SPI_MODE0));
  SPI.transfer(addr & 0x7F);
  uint8_t res = SPI.transfer(0x00);
  SPI.endTransaction();
  unselectreceiver();
  return res;
}

// ----------------------------------------------------------------------------
// Write value to a register with address addr.
// Function writes one byte at a time.
// ----------------------------------------------------------------------------
void writeRegister(byte addr, byte value)
{
  unsigned char spibuf[2];

  spibuf[0] = addr | 0x80;
  spibuf[1] = value;
  selectreceiver();
  SPI.beginTransaction(SPISettings(50000, MSBFIRST, SPI_MODE0));
  SPI.transfer(spibuf[0]);
  SPI.transfer(spibuf[1]);
  SPI.endTransaction();
  unselectreceiver();
}

// ----------------------------------------------------------------------------
// This LoRa function reads a message from the LoRa transceiver
// returns true when message correctly received or fails on error
// (CRC error for example)
// ----------------------------------------------------------------------------
bool receivePkt(char *payload)
{
  // clear rxDone
  writeRegister(REG_IRQ_FLAGS, 0x40);

  int irqflags = readRegister(REG_IRQ_FLAGS);

  cp_nb_rx_rcv++;											// Receive statistics counter

  //  payload crc: 0x20
  if ((irqflags & 0x20) == 0x20)
  {
    Serial.println(F("CRC error"));
    writeRegister(REG_IRQ_FLAGS, 0x20);
    return false;
  } else {

    cp_nb_rx_ok++;										// Receive OK statistics counter

    byte currentAddr = readRegister(REG_FIFO_RX_CURRENT_ADDR);
    byte receivedCount = readRegister(REG_RX_NB_BYTES);
    receivedbytes = receivedCount;

    writeRegister(REG_FIFO_ADDR_PTR, currentAddr);

    for (int i = 0; i < receivedCount; i++)
    {
      payload[i] = (char)readRegister(REG_FIFO);
    }
  }
  return true;
}

// ----------------------------------------------------------------------------
// Setup the LoRa environment on the connected transceiver.
// - Determine the correct transceiver type (sx1272/RFM92 or sx1276/RFM95)
// - Set the frequency to listen to (1-channel remember)
// - Set Spreading Factor (standard SF7)
// The reset RST pin might not be necessary for at least the RGM95 transceiver
// ----------------------------------------------------------------------------
void SetupLoRa()
{
  Serial.println(F("Trying to Setup LoRa Module with"));
  Serial.printf("CS=GPIO%d  DIO0=GPIO%d  Reset=", ssPin, dio0 );

  if (RST == NOT_A_PIN ) {
    Serial.println(F("Unused"));
  } else {
    Serial.printf("GPIO%d\r\n", RST);
    digitalWrite(RST, HIGH);
    delay(100);
    digitalWrite(RST, LOW);
    delay(100);
  }

  byte version = readRegister(REG_VERSION);					// Read the LoRa chip version id
  if (version == 0x22) {
    // sx1272
    Serial.println(F("SX1272 detected, starting."));
    sx1272 = true;
  } else {
    // sx1276?
    if (RST != NOT_A_PIN ) {
      digitalWrite(RST, LOW);
      delay(100);
      digitalWrite(RST, HIGH);
      delay(100);
    }

    version = readRegister(REG_VERSION);
    if (version == 0x12) {
      // sx1276
      Serial.println(F("SX1276 detected, starting."));
      sx1272 = false;
    } else {
      Serial.print(F("Unrecognized transceiver, version: 0x"));
      Serial.printf("%02X", version);
      die("");
    }
  }

  writeRegister(REG_OPMODE, SX72_MODE_SLEEP);

  // set frequency
  uint64_t frf = ((uint64_t)freq << 19) / 32000000;
  writeRegister(REG_FRF_MSB, (uint8_t)(frf >> 16) );
  writeRegister(REG_FRF_MID, (uint8_t)(frf >> 8) );
  writeRegister(REG_FRF_LSB, (uint8_t)(frf >> 0) );

  writeRegister(REG_SYNC_WORD, 0x34); // LoRaWAN public sync word

  // Set spreading Factor
  if (sx1272) {
    if (sf == SF11 || sf == SF12) {
      writeRegister(REG_MODEM_CONFIG, 0x0B);
    } else {
      writeRegister(REG_MODEM_CONFIG, 0x0A);
    }
    writeRegister(REG_MODEM_CONFIG2, (sf << 4) | 0x04);
  } else {
    if (sf == SF11 || sf == SF12) {
      writeRegister(REG_MODEM_CONFIG3, 0x0C);
    } else {
      writeRegister(REG_MODEM_CONFIG3, 0x04);
    }
    writeRegister(REG_MODEM_CONFIG, 0x72);
    writeRegister(REG_MODEM_CONFIG2, (sf << 4) | 0x04);
  }

  if (sf == SF10 || sf == SF11 || sf == SF12) {
    writeRegister(REG_SYMB_TIMEOUT_LSB, 0x05);
  } else {
    writeRegister(REG_SYMB_TIMEOUT_LSB, 0x08);
  }
  writeRegister(REG_MAX_PAYLOAD_LENGTH, 0x80);
  writeRegister(REG_PAYLOAD_LENGTH, PAYLOAD_LENGTH);
  writeRegister(REG_HOP_PERIOD, 0xFF);
  writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_BASE_AD));

  // Set Continous Receive Mode
  writeRegister(REG_LNA, LNA_MAX_GAIN);  // max lna gain
  writeRegister(REG_OPMODE, SX72_MODE_RX_CONTINUOS);
}


// ----------------------------------------------------------------------------
// Send periodic status message to server even when we do not receive any
// data.
// Parameter is socketr to TX to
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Receive a LoRa package
//
// Receive a LoRa message and fill the buff_up char buffer.
// returns values:
// - returns the length of string returned in buff_up
// - returns -1 when no message arrived.
// ----------------------------------------------------------------------------
int receivepacket(char *buff_up) {

  long int SNR;
  int rssicorr;
  char cfreq[12] = {0};			  // Character array to hold freq in MHz
  if (digitalRead(dio0) == 1)	// READY?
  { 
    if (receivePkt(message)) {
      byte value = readRegister(REG_PKT_SNR_VALUE);

      if ( value & 0x80 ) { // The SNR sign bit is 1
        // Invert and divide by 4
        value = ( ( ~value + 1 ) & 0xFF ) >> 2;
        SNR = -value;
      } else {
        // Divide by 4
        SNR = ( value & 0xFF ) >> 2;
      }

      if (sx1272) {
        rssicorr = 139;
      } else {											// Probably SX1276 or RFM95
        rssicorr = 157;
      }

      Serial.print(F("Packet RSSI: "));
      Serial.print(readRegister(0x1A) - rssicorr);
      Serial.print(F(" RSSI: "));
      Serial.print(readRegister(0x1B) - rssicorr);
      Serial.print(F(" SNR: "));
      Serial.print(SNR);
      Serial.print(F(" Length: "));
      Serial.print((int)receivedbytes);
      Serial.println();
      yield();
      

      int j;

      // XXX Base64 library is nopad. So we may have to add padding characters until
      // 	length is multiple of 4!
      int encodedLen = base64_enc_len(receivedbytes);		// max 341
      base64_encode(b64, message, receivedbytes);			// max 341

      //j = bin_to_b64((uint8_t *)message, receivedbytes, (char *)(b64), 341);
      //fwrite(b64, sizeof(char), j, stdout);

      int buff_index = 0;

      // pre-fill the data buffer with fixed fields
      buff_up[0] = PROTOCOL_VERSION;
      buff_up[3] = PKT_PUSH_DATA;

      // XXX READ MAC ADDRESS OF ESP8266
      buff_up[4]  = MAC_array[0];
      buff_up[5]  = MAC_array[1];
      buff_up[6]  = MAC_array[2];
      buff_up[7]  = 0x05;
      buff_up[8]  = 0x05;
      buff_up[9]  = MAC_array[3];
      buff_up[10] = MAC_array[4];
      buff_up[11] = MAC_array[5];

      // start composing datagram with the header
      uint8_t token_h = (uint8_t)rand(); 					// random token
      uint8_t token_l = (uint8_t)rand(); 					// random token
      buff_up[1] = token_h;
      buff_up[2] = token_l;
      buff_index = 12; /* 12-byte header */

      // TODO: tmst can jump if time is (re)set, not good.
      struct timeval now;
      gettimeofday(&now, NULL);
      uint32_t tmst = (uint32_t)(now.tv_sec * 1000000 + now.tv_usec);

      // start of JSON structure that will make payload
      memcpy((void *)(buff_up + buff_index), (void *)"{\"rxpk\":[", 9);
      buff_index += 9;
      buff_up[buff_index] = '{';
      ++buff_index;
      j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE - buff_index, "\"tmst\":%u", tmst);
      buff_index += j;
      ftoa((double)freq / 1000000, cfreq, 6);						// XXX This can be done better
      j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE - buff_index, ",\"chan\":%1u,\"rfch\":%1u,\"freq\":%s", 0, 0, cfreq);
      buff_index += j;
      memcpy((void *)(buff_up + buff_index), (void *)",\"stat\":1", 9);
      buff_index += 9;
      memcpy((void *)(buff_up + buff_index), (void *)",\"modu\":\"LORA\"", 14);
      buff_index += 14;
      /* Lora datarate & bandwidth, 16-19 useful chars */
      switch (sf) {
        case SF7:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF7", 12);
          buff_index += 12;
          break;
        case SF8:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF8", 12);
          buff_index += 12;
          break;
        case SF9:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF9", 12);
          buff_index += 12;
          break;
        case SF10:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF10", 13);
          buff_index += 13;
          break;
        case SF11:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF11", 13);
          buff_index += 13;
          break;
        case SF12:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF12", 13);
          buff_index += 13;
          break;
        default:
          memcpy((void *)(buff_up + buff_index), (void *)",\"datr\":\"SF?", 12);
          buff_index += 12;
      }
      memcpy((void *)(buff_up + buff_index), (void *)"BW125\"", 6);
      buff_index += 6;
      memcpy((void *)(buff_up + buff_index), (void *)",\"codr\":\"4/5\"", 13);
      buff_index += 13;
      j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE - buff_index, ",\"lsnr\":%li", SNR);
      buff_index += j;
      j = snprintf((char *)(buff_up + buff_index), TX_BUFF_SIZE - buff_index, ",\"rssi\":%d,\"size\":%u", readRegister(0x1A) - rssicorr, receivedbytes);
      buff_index += j;
      memcpy((void *)(buff_up + buff_index), (void *)",\"data\":\"", 9);
      buff_index += 9;

      // Use gBase64 library
      encodedLen = base64_enc_len(receivedbytes);		// max 341
      j = base64_encode((char *)(buff_up + buff_index), message, receivedbytes);

      buff_index += j;
      buff_up[buff_index++] = '"';

      // End of packet serialization
      buff_up[buff_index++] = '}';
      buff_up[buff_index++] = ']';
      // end of JSON datagram payload */
      buff_up[buff_index++] = '}';
      buff_up[buff_index] = 0; 						// add string terminator, for safety

      
      Serial.print(F("rxpk update: "));
      Serial.println((char *)(buff_up + 12));		// DEBUG: display JSON payload
      
      return (buff_index);

    } // received a message
  } // dio0=1
  return (-1);
}



// ----------------------------------------------------------------------------
// Output the 4-byte IP address for easy printing
// ----------------------------------------------------------------------------
String printIP(IPAddress ipa) {
  String response;
  response += (String)ipa[0]; response += ".";
  response += (String)ipa[1]; response += ".";
  response += (String)ipa[2]; response += ".";
  response += (String)ipa[3];
  return (response);
}


// ========================================================================
// MAIN PROGRAM (SETUP AND LOOP)

// ----------------------------------------------------------------------------
// Setup code (one time)
// ----------------------------------------------------------------------------
void setup () {

  Serial.begin(_BAUDRATE);	// As fast as possible for bus

  // Wi-Fi Connection
  WiFi.mode(WIFI_STA);          // dang ky cho esp8266 lam mot diem truy cap wifi vao router local = access point
  WiFi.begin(STASSID, STAPSK);  // ket noi wifi nha de truy cap vao internet
  Serial.println();
  Serial.println("Connect to wifi");
  while(WiFi.status()!=WL_CONNECTED){
    Serial.print('.');  
    delay(500);
  }
  Serial.print("Connected! IP address: "); // IP cua esp8266 trong mang wifi local
  Serial.println(WiFi.localIP());
  
  Serial.print("IP server: ");    // IP cua server China The Things Network
  WiFi.hostByName(_TTN_SERVER, ttnServer);  // tra ve dia chi ip cua server cua ban China router.cn.thethings.network
                                            // minh dung router cua ban China vi minh muon dang ky tan so 470 MHz

  //UDP Connection 
  bool conectadoUDP = (Udp.begin((int)_TTN_PORT) == 1);  // establish UDP connection and listening at this port _TTN_PORT
                                                             //, ==1 de kiem tra da connect chua 
  if (conectadoUDP){
    Serial.print("Conectado ao servidor: ");   // connect to server ttnServer to listen incoming UDP packets from this port 
    Serial.println(ttnServer);            // in ra dia chi ip cua server router.cn.thethings.network, sau khi da convert o line 37
  }                                       // y la UDP se lang nghe incoming packets o port _TTN_PORT=1700
  
  char MAC_char[18] = {0};
  WiFi.macAddress(MAC_array);           // get MAC address of ESP8266 device, return 6 byte array 
  for (int i = 0; i<sizeof(MAC_array);++i){
    sprintf(MAC_char, "%s%02x:", MAC_char, MAC_array[i]);
  }
  Serial.print(F("MAC: "));
  Serial.println(MAC_char);



  // Configure IO Pin
  pinMode(ssPin, OUTPUT);
  pinMode(dio0, INPUT);
  pinMode(RST, OUTPUT);

  SPI.begin();
  delay(100);
  SetupLoRa();
  delay(100);

  
}

// ----------------------------------------------------------------------------
// LOOP
// This is the main program that is executed time and time again.
// We need to geive way to the bacjend WiFi processing that
// takes place somewhere in the ESP8266 firmware and therefore
// we include yield() statements at important points.
//
// Note: If we spend too much time in user processing functions
//	and the backend system cannot do its housekeeping, the watchdog
// function will be executed which means effectively that the
// program crashes.
// ----------------------------------------------------------------------------
void loop ()
{
  yield();
  
  int buff_index;
  char buff_up[TX_BUFF_SIZE]; 						// buffer to compose the upstream packet

  // Receive Lora messages
  if ((buff_index = receivepacket(buff_up)) >= 0) {	// read is successful
    yield();
    sendUdp(buff_up, buff_index);					// We can send to multiple sockets if necessary
  }


  // Receive WiFi messages. This is important since the TTN broker will return confirmation
  // messages on UDP for every message sent by the gateway.
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    readUdp(packetSize);
  }

  uint32_t nowseconds = (uint32_t) millis() / 1000;
  if (nowseconds - lasttime >= 30) {					// Send status every 30 seconds
    enviaDados();
    lasttime = nowseconds;
  }
}

// ham gui len server
void enviaDados(){
  // message kieu json gui tu esp8266 len server TTN
  char msg[] = "000000000000"
  "{"
  "\"rxpk\":[{"
  "\"tmst\":13202941,"
  "\"chan\":0,"
  "\"rfch\":0,"
//  "\"freq\":915.299987,"
  "\"freq\":486.299987,"
  "\"stat\":1,"
  "\"modu\":\"LORA\","
  "\"datr\":\"SF7BW125\","
  "\"codr\":\"4/5\","
  "\"lsnr\":0,"
  "\"rssi\":-60,"
  "\"size\":23,"
  "\"data\":\"QJkUASYAAAABodsMSXtu+qxauVfYT/U=\""
  "}]"
  "}";

  int bytesEnviar = sizeof(msg)-1;

  msg[0] = 1; // PROTOCOL VERSION
  msg[1] = (uint8_t)rand();
  msg[2] = (uint8_t)rand();
  msg[3] = 0; // PKT_PUSH_DATA, always 0x00
  msg[4] = MAC_array[0]; // from 4-11 is gateway EUI
  msg[5] = MAC_array[1];
  msg[6] = MAC_array[2];
  msg[7] = 0x05;
  msg[8] = 0x05;
  msg[9] = MAC_array[3];
  msg[10] = MAC_array[4];
  msg[11] = MAC_array[5];

  Serial.println();
  for(int i = 0; i < 12; i++){
    Serial.printf("%02X:", msg[i]);
  }
  for(int i = 12; i < bytesEnviar ; i++){
    Serial.write(msg[i]);
  }
  Serial.println();
  sendUdp(msg, bytesEnviar);  
}
