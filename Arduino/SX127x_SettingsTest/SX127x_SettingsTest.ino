/*
   RadioLib SX127x Settings Example

   This example shows how to change all the properties of LoRa transmission.
   RadioLib currently supports the following settings:
    - pins (SPI slave select, digital IO 0, digital IO 1)
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word
    - output power during transmission

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

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1276 radio3 = RadioShield.ModuleB;

void setup() {
  Serial.begin(9600);

  // initialize SX1272 with default settings
  Serial.print(F("[SX1272] Initializing ... "));
  int state = radio.begin();
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

  // you can also change the settings at runtime
  // and check if the configuration was changed successfully

  // set carrier frequency to 433.5 MHz
  if (radio.setFrequency(868.0) == RADIOLIB_ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected frequency is invalid for this module!"));
    while (true);
  }
  Serial.println("Frequency change success");

  // set bandwidth to 250 kHz
  if (radio.setBandwidth(500.0) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
    Serial.println(F("Selected bandwidth is invalid for this module!"));
    while (true);
  }
  Serial.println("Brandwith change success");

  // set spreading factor to 10
  if (radio.setSpreadingFactor(7) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
    Serial.println(F("Selected spreading factor is invalid for this module!"));
    while (true);
  }
  Serial.println("Spreading factor change success");

  // set coding rate to 6
  if (radio.setCodingRate(5) == RADIOLIB_ERR_INVALID_CODING_RATE) {
    Serial.println(F("Selected coding rate is invalid for this module!"));
    while (true);
  }
  Serial.println("Coding rate change success");

  // set LoRa sync word to 0x14
  // NOTE: value 0x34 is reserved for LoRaWAN networks and should not be used
  if (radio.setSyncWord(0x12) != RADIOLIB_ERR_NONE) {
    Serial.println(F("Unable to set sync word!"));
    while (true);
  }
  Serial.println("Sync word change success");

  // set output power to 10 dBm (accepted range is -3 - 17 dBm)
  // NOTE: 20 dBm value allows high power operation, but transmission
  //       duty cycle MUST NOT exceed 1%
  if (radio.setOutputPower(2) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    Serial.println(F("Selected output power is invalid for this module!"));
    while (true);
  }
  Serial.println("Out power change success");

  // set over current protection limit to 80 mA (accepted range is 45 - 240 mA)
  // NOTE: set value to 0 to disable overcurrent protection
  if (radio.setCurrentLimit(80) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
    Serial.println(F("Selected current limit is invalid for this module!"));
    while (true);
  }
  Serial.println("Current limit change success");

  // set LoRa preamble length to 15 symbols (accepted range is 6 - 65535)
  if (radio.setPreambleLength(6) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
    Serial.println(F("Selected preamble length is invalid for this module!"));
    while (true);
  }
  Serial.println("Preamble change success");

  // set amplifier gain to 1 (accepted range is 1 - 6, where 1 is maximum gain)
  // NOTE: set value to 0 to enable automatic gain control
  //       leave at 0 unless you know what you're doing
  if (radio.setGain(1) == RADIOLIB_ERR_INVALID_GAIN) {
    Serial.println(F("Selected gain is invalid for this module!"));
    while (true);
  }
  Serial.println("Frequency change success");

  Serial.println(F("All settings successfully changed!"));
}

void loop() {
  // nothing here
}
