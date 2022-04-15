/*
   RadioLib SX127x Transmit Example

   This example transmits packets using SX1272 LoRa radio module.
   Each packet contains up to 256 bytes of data, in the form of:
    - Arduino String
    - null-terminated char array (C-string)
    - arbitrary binary data (byte array)

   Other modules from SX127x/RFM9x family can also be used.

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx127xrfm9x---lora-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1272 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// RESET pin: 9
// DIO1 pin:  3
SX1272 radio = new Module(10, 2, 9, 3);

String str = "Packet nb";
int increment = 0;
float DR, meanDR = 0;

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1272 radio = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SX1272 with default settings
  Serial.println(F("-------------------------------------"));
  Serial.print(F("[SX1272] Initializing ... "));

/*
 *     "freq" in MHz. Allowed values range from 860.0 MHz to 1020.0 MHz.
 *     in EU 
 *     "bw" bandwidth in kHz. 125, 250 and 500 kHz.
 *     "sf" spreading factor. From 6 to 12.
 *     "cr" coding rate denominator. From 5 to 8.
 *     "syncWord" sync word. Can be used to distinguish different networks. Note that value 0x34 is reserved for LoRaWAN networks.
 *     "currentLimit" Trim value for OCP (over current protection) in mA. Can be set to multiplies of 5 in range 45 to 120 mA and to multiples of 10 in range 120 to 240 mA.
 *     Set to 0 to disable OCP (not recommended).
 *     "preambleLength" Length of %LoRa transmission preamble in symbols. The actual preamble length is 4.25 symbols longer than the set number.
 *     From 6 to 65535.
 *     "gain" Gain of receiver LNA (low-noise amplifier). From 1 to 6 where 1 is the highest gain and 0 is auto
 *     
 *     int16_t begin(float freq = 915.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = RADIOLIB_SX127X_SYNC_WORD, int8_t power = 10, uint16_t preambleLength = 8, uint8_t gain = 0);
 */


  int state = radio.begin(867.1, 500.0, 7, 5, RADIOLIB_SX127X_SYNC_WORD, 20, 6, 0);
  //int state = radio.begin(867.1, 500.0, 7, 5, RADIOLIB_SX127X_SYNC_WORD, 20, 6, 6);
  //int state = radio.begin(867.1, 125.0, 12, 5, RADIOLIB_SX127X_SYNC_WORD, 20, 6, 0);
  //int state = radio.begin();



  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // some modules have an external RF switch
  // controlled via two pins (RX enable, TX enable)
  // to enable automatic control of the switch,
  // call the following method
  // RX enable:   8
  // TX enable:   7
  
  radio.setRfSwitchPins(8, 7);
}

void loop() {
  Serial.print(F("[SX1272] Transmitting packet ... "));

  // you can transmit C-string or Arduino string up to
  // 256 characters long
  // NOTE: transmit() is a blocking method!
  //       See example SX127x_Transmit_Interrupt for details
  //       on non-blocking transmission method.

  int state = radio.transmit(str + (++increment));
  Serial.print(increment);

  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    int state = radio.transmit(byteArr, 8);
  */

  if (state == RADIOLIB_ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(F(" success!"));

    // print measured data rate
    Serial.print(F("[SX1272] Datarate:\t"));
    Serial.print(DR = radio.getDataRate());
    Serial.print(F(" bps "));
    Serial.print(((meanDR += DR) / increment));
    Serial.print(F(" or "));
    Serial.println((increment / meanDR * (str.length() + 3) * 1000));

  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));

  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    // timeout occurred while transmitting packet
    Serial.println(F("timeout!"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);

  }

  // wait for a second before transmitting again
  Serial.println(F("-------------------------------------"));
  delay(5000);
}
