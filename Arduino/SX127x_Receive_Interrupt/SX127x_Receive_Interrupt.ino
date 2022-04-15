/*
   RadioLib SX127x Receive with Interrupts Example

   This example listens for LoRa transmissions and tries to
   receive them. Once a packet is received, an interrupt is
   triggered. To successfully receive data, the following
   settings have to be the same on both transmitter
   and receiver:
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word

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

int state, increment = 0, rssi, meanRSSI = 0, rxLength;
float snr, meanSNR = 0;
String str;

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

  int state = radio.begin(868, 125.0, 12, 8, RADIOLIB_SX127X_SYNC_WORD, 2, 6, 6);
  //int state = radio.begin(866.4, 500.0, 7, 5, RADIOLIB_SX127X_SYNC_WORD, 2, 6, 6); // max bw
  //int state = radio.begin(866.4, 125.0, 12, 5, RADIOLIB_SX127X_SYNC_WORD, 20, 8, 1); // max dist
  
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

  // set the function that will be called
  // when new packet is received
  radio.setDio0Action(setFlag);

  // start listening for LoRa packets
  Serial.print(F("[SX1272] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // if needed, 'listen' mode can be disabled by calling
  // any of the following methods:
  //
  // radio.standby()
  // radio.sleep()
  // radio.transmit();
  // radio.receive();
  // radio.readData();
  // radio.scanChannel();
}

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if(!enableInterrupt) {
    return;
  }

  // we got a packet, set the flag
  receivedFlag = true;
}

void printMsg(String data, int *dataLength, bool caret = true, bool hexad = true) {
    if(hexad) {
        char *hexa = "0123456789ABCDEF";
        for(int i = 0; i < *dataLength; i++) {
            if (data[i] > 4095) Serial.print(hexa[data[i] / 4096]);
            if (data[i] > 255) Serial.print(hexa[data[i] / 256 % 16]);     // write ASCII value of hexadecimal high nibble
            Serial.print(hexa[data[i] / 16 % 16]);     // write ASCII value of hexadecimal high nibble
            Serial.print(hexa[data[i] % 16]);     // write ASCII value of hexadecimal low nibble
            Serial.print(" ");
        }
    } else {
        for(int i = 0; i < *dataLength; i++) {
            Serial.print(data[i]);
            Serial.print(" ");
        }
    }
    if(caret) Serial.print("\r\n");
}

void loop() {
  // check if the flag is set
  if(receivedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    state = radio.readData(str);

    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int state = radio.readData(byteArr, 8);
    */

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[SX1272] Received packet!"));

      // print data of the packet
      Serial.print(F("[SX1272] Data:\t"));
      //Serial.println(str);
      rxLength = radio.getPacketLength();
      printMsg(str, &rxLength);

      // print RSSI (Received Signal Strength Indicator)
      Serial.print(F("[SX1272] RSSI:\t"));
      Serial.print(rssi = ((int) radio.getRSSI()));
      Serial.print(F(" dBm "));
      Serial.print(((meanRSSI += rssi) / ++increment));
      Serial.println();

      // print SNR (Signal-to-Noise Ratio)
      Serial.print(F("[SX1272] SNR:\t"));
      Serial.print(snr = radio.getSNR());
      Serial.print(F(" dB "));
      Serial.print(((meanSNR += snr) / increment));
      Serial.println();

      // print frequency error
      Serial.print(F("[SX1272] Frq er:\t"));
      Serial.print(radio.getFrequencyError(true));
      Serial.println(F(" Hz"));

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("[SX1272] CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("[SX1272] Failed, code "));
      Serial.println(state);

    }
    
    Serial.println(F("-------------------------------------"));

    // put module back to listen mode
    radio.startReceive();

    // we're ready to receive more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }

}
