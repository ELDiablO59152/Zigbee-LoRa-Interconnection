/*
    RadioLib SX127x Ping-Pong Example

    For default module settings, see the wiki page
    https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx127xrfm9x---lora-modem

    For full API reference, see the GitHub Pages
    https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

#define TRANSMIT_HEADER  "T"   // Header tag for serial transmit message

#define HEADER_0_POS 0
#define HEADER_0 0x4E

#define HEADER_1_POS 1
#define HEADER_1 0xAD

#define DEST_ID_POS 2
#define SOURCE_ID_POS 3
#define HEI_ID 0x01
#define ISEN_ID 0x02
#define MY_ID HEI_ID

#define COMMAND_POS 4
#define DISCOVER 0x01
#define ASKING 0x02
#define ABLE_MEASURE 0x03
#define DISABLE_MEASURE 0x04
#define ACK 0x05
#define NACK 0x06
#define TIMEOUT 0x42
#define LED_ON 0x66
#define LED_OFF 0x67
#define PING 0x17

#define DATA_LONG_POS 5
#define DATA_LONG 0x01
#define NUL 0x00

#define DISCOVER_LONG 6
#define ACK_LONG 5
#define TRANSMIT_LONG (DATA_LONG + 4)
#define DISABLE_LONG 5

// SX1272 has the following connections:
// NSS pin:   10 
// DIO0 pin:  2 or 0 on rasp
// NRST pin:  9 or 7 on rasp
// DIO1 pin:  3
SX1272 radio = new Module(10, 2, 9, 3);

// save transmission states between loops
int state = RADIOLIB_ERR_NONE;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

void isSOk(void) {
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
    }
}

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
    // check if the interrupt is enabled
    if (!enableInterrupt) {
        return;
    }

    // we sent or received packet, set the flag
    operationDone = true;
}

void setup() {
    Serial.begin(38400);
    pinMode(4, OUTPUT);

    // initialize SX1272 with default settings
    Serial.println(F("\n-------------------------------------"));
    Serial.print(F("[SX1272] Initializing ... "));
    
    state = radio.begin(868.0, 500.0, 12, 5, RADIOLIB_SX127X_SYNC_WORD, 2, 8, 6); // max bw
    //state = radio.begin(866.4, 500.0, 7, 5, RADIOLIB_SX127X_SYNC_WORD, 2, 6, 6); // max bw
    //int state = radio.begin(866.4, 125.0, 12, 5, RADIOLIB_SX127X_SYNC_WORD, 20, 8, 1); // max dist

    isSOk();

    // some modules have an external RF switch
    // controlled via two pins (RX enable, TX enable)
    // to enable automatic control of the switch,
    // call the following method
    // RX enable:   8 or 3 on rasp
    // TX enable:   7 or 2 on rasp

    radio.setRfSwitchPins(8, 7);

    // set the function that will be called
    // when new packet is received
    radio.setDio0Action(setFlag);

    //radio.setEncoding()  // chiffrage
}  // end of setup

void printMsg(byte *data, int *dataLength, bool caret = true, bool hexad = true) {
    if(hexad) {
        char *hexa = "0123456789ABCDEF";
        for(int i = 0; i < dataLength; i++) {
            if (data[i] > 4095) Serial.print(hexa[data[i] / 4096]);
            if (data[i] > 255) Serial.print(hexa[data[i] / 256 % 16]);     // write ASCII value of hexadecimal high nibble
            Serial.print(hexa[data[i] / 16 % 16]);     // write ASCII value of hexadecimal high nibble
            Serial.print(hexa[data[i] % 16]);     // write ASCII value of hexadecimal low nibble
            Serial.print(" ");
        }
    } else {
        for(int i = 0; i < dataLength; i++) {
            Serial.print(data[i]);
            Serial.print(" ");
        }
    }
    if(caret) Serial.print("\r\n");
}

void printHex(byte data, bool caret = true, bool hexad = true) {
    if(hexad) {
        char *hexa = "0123456789ABCDEF";
        if (data > 4095) Serial.print(hexa[data / 4096]);
        if (data > 255) Serial.print(hexa[data / 256 % 16]);     // write ASCII value of hexadecimal high nibble
        Serial.print(hexa[data / 16 % 16]);     // write ASCII value of hexadecimal high nibble
        Serial.print(hexa[data % 16]);     // write ASCII value of hexadecimal low nibble
        Serial.print(" ");
    } else {
        Serial.print(data);
        Serial.print(" ");
    }
    if(caret) Serial.print("\r\n");
}

/*
 * byte byteArr[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
 * int state = radio.transmit(byteArr, 8);
 */

/*
 * RC5 MOSI 15
 * RC4 MISO 14
 * RC3 Clock 13
 * RC2 CS LoRa 16
 * RC1 CS Lcd
 * RC0 CS Capteur
 * RB4 RX Pic 5
 * RB3 TX Pic 4
 * RB2 Reset 12
 * 
 * #define HEADER_0_POS 0
 * #define HEADER_0 0x4E
 * 
 * #define HEADER_1_POS 1
 * #define HEADER_1 0xAD
 * 
 * #define NETWORK_ID_POS 2
 * #define NETWORK_ID 0x01
 * 
 * #define TARGET_ID_POS 3
 * #define TARGET_ID 0x01
 * 
 * #define COMMAND_POS 4
 * #define DISCOVER 0x01
 * #define ASKING 0x02
 * #define ABLE_MEASURE 0x03
 * #define DISABLE_MEASURE 0x04
 * #define ACK 0x05
 * #define NACK 0x06
 * #define TIMEOUT 0x42
 * #define LED_ON 0x66
 * #define LED_OFF 0x67
 * 
 * #define TYPE_CAPT_POS 4
 * #define TYPE_CAPT 0x01
 * 
 * #define DATA_LONG_POS 5
 * #define DATA_LONG 0x01
 * #define NUL 0x00
 * 
 * #define DISCOVER_LONG 6
 * #define ACK_LONG 5
 * #define TRANSMIT_LONG (DATA_LONG + 4)
 * 
 * const uint8_t rxMsg[] = { 0x4E, 0xAD, 0x01, 0x04, 0x01 }; //Découverte Réseau Base
 * const uint8_t txMsg[] = { 0xAD, 0x4E, 0x01, 0x04, 0x01, 0x01 }; //Réponse header + network + node 4 + température + 1 octet
 * const uint8_t rxMsg[] = { 0x4E, 0xAD, 0x01, 0x04, 0x02 }; //Requête données
 * const uint8_t txMsg[] = { 0xAD, 0x4E, 0x01, 0x04, 0x03 }; //Réponse header + network + node 4 + possible ou 04 impossible
 * const uint8_t txMsg[] = { 0xAD, 0x4E, 0x01, 0x04, txMsg[i], lvlBatt }; //Réponse header + network + node 4 + possible ou 04 impossible
 * const uint8_t rxMsg[] = { 0x4E, 0xAD, 0x01, 0x04, 0x05 }; //Accusé reception ou 06 demande renvoi
 * 
 * uint8_t RXNumberOfBytes;    // to store the number of bytes received
 * uint8_t rxMsg[30];              // message reçu
 * uint8_t txMsg[] = { HEADER_1, HEADER_0, NETWORK_ID, TARGET_ID, NUL, NUL, NUL, NUL, NUL };    // message transmit
 */


static char id = 1;
byte networkAvailable[] = { NUL, NUL, NUL, NUL, NUL };  // store ids of networks
int networkRssi[] = { NUL, NUL, NUL, NUL, NUL };  // store Rssi of networks
byte txMsg[] = { HEADER_0, HEADER_1, NUL, MY_ID, NUL, NUL, NUL, NUL, NUL };    // message transmit
byte rxMsg[6];
int rxLength;
byte idInc = 0x01;
const bool debuglvl = true;

int data = 0;

void debug(void) {
    if (state == RADIOLIB_ERR_NONE) {
        // packet was successfully received
        Serial.println(F("[SX1272] Received packet!"));
        
        // print data of the packet
        Serial.print(F("[SX1272] Data:\t\t"));
        printMsg(rxMsg, rxLength);
        
        // print RSSI (Received Signal Strength Indicator)
        Serial.print(F("[SX1272] RSSI:\t\t"));
        Serial.print((int)radio.getRSSI());
        //Serial.print(rssi = ((int) radio.getRSSI()));
        Serial.print(F(" dBm "));
        //Serial.print(((meanRSSI += rssi) / ++increment));
        Serial.println();
        
        // print SNR (Signal-to-Noise Ratio)
        Serial.print(F("[SX1272] SNR:\t\t"));
        Serial.print(radio.getSNR());
        //Serial.print(snr = radio.getSNR());
        Serial.print(F(" dB "));
        //Serial.print(((meanSNR += snr) / increment));
        Serial.println();
        
        // print frequency error
        Serial.print(F("[SX1272] Frequ err:\t\t"));
        Serial.print(radio.getFrequencyError(false));
        Serial.println(F(" Hz"));
        
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        // packet was received, but is malformed
        Serial.println(F("[SX1272] CRC error!"));
    } else {
        // some other error occurred
        Serial.print(F("[SX1272] Failed, code "));
        Serial.println(state);
    }
}

void loop() {

    if (Serial.available()) {
        if(Serial.find((char*)TRANSMIT_HEADER)) {
            /*
             * T_HEADER, DEST, CMD
             * T, ISEN_ID, LED_ON
             * T 11
             */
            data = Serial.parseInt();
            Serial.println(data);
            if (data < 100) {
                txMsg[HEADER_0_POS] = HEADER_0;
                txMsg[HEADER_1_POS] = HEADER_1;
                txMsg[DEST_ID_POS] = (byte)(data / 10 % 10);
                txMsg[SOURCE_ID_POS] = MY_ID;
                txMsg[COMMAND_POS] = (byte)(data % 10);

                Serial.print(F("[SX1272] Sending : "));
                printMsg(txMsg, 5);
                state = radio.transmit(txMsg, 5);
                //delay(5)  // transmit est blocant
                isSOk();
            }
        }
    }
  
    if (operationDone) {
        // disable the interrupt service routine while
        // processing the data
        enableInterrupt = false;
    
        // reset flag
        operationDone = false;
        
        rxLength = radio.getPacketLength();
        state = radio.readData(rxMsg, rxLength);

        Serial.print(F("received data: "));
        printMsg(rxMsg, rxLength);
        if(debuglvl) debug();

        if(rxMsg[HEADER_0_POS] == HEADER_0 && rxMsg[HEADER_1_POS] == HEADER_1 && rxMsg[DEST_ID_POS] == MY_ID) {
            switch (rxMsg[COMMAND_POS]) {   // type de message 
                case ACK:
                    Serial.println(F("Accuse reception"));
                    networkAvailable[idInc-1] = rxMsg[SOURCE_ID_POS];
                    networkRssi[idInc-1] = (int)radio.getRSSI();
                    idInc++;
                    if(idInc >= 0x05) idInc = 0x1;
                    if(idInc == MY_ID) idInc++;
                    break;
                    
                case TIMEOUT:
                    Serial.println(F("Timeout"));
                    rxMsg[COMMAND_POS] = 0;
                    break;
                    
                case LED_ON:
                    digitalWrite(4, HIGH);
                    Serial.println(F("Led ON"));
                    txMsg[HEADER_0_POS] = HEADER_1;
                    txMsg[HEADER_1_POS] = HEADER_0;
                    txMsg[DEST_ID_POS] = ISEN_ID;
                    txMsg[SOURCE_ID_POS] = MY_ID;
                    txMsg[COMMAND_POS] = ACK;
                    Serial.print(F("[SX1272] Sending : "));
                    printMsg(txMsg, TRANSMIT_LONG);
                    state = radio.transmit(txMsg, TRANSMIT_LONG);
                    //delay(5)  // transmit est blocant
                    isSOk();
                    break;
                    
                case LED_OFF:
                    digitalWrite(4, LOW);
                    Serial.println(F("Led OFF"));
                    txMsg[HEADER_0_POS] = HEADER_1;
                    txMsg[HEADER_1_POS] = HEADER_0;
                    txMsg[DEST_ID_POS] = ISEN_ID;
                    txMsg[SOURCE_ID_POS] = MY_ID
                    txMsg[COMMAND_POS] = ACK;
                    Serial.print(F("[SX1272] Sending : "));
                    printMsg(txMsg, TRANSMIT_LONG);
                    state = radio.transmit(txMsg, TRANSMIT_LONG);
                    //delay(5)  // transmit est blocant
                    isSOk();
                    break;  
                    
                case PING:
                    Serial.print(F("Pinging from "));
                    printHex(rxMsg[SOURCE_ID_POS], false);
                    Serial.print(F("to "));
                    printHex(rxMsg[DEST_ID_POS]);
                    
                    if(rxMsg[DEST_ID_POS] != MY_ID) break;
                    
                    Serial.println(F("Pong"));
                    txMsg[HEADER_0_POS] = HEADER_1;
                    txMsg[HEADER_1_POS] = HEADER_0;
                    txMsg[DEST_ID_POS] = ISEN_ID;
                    txMsg[SOURCE_ID_POS] = MY_ID;
                    txMsg[COMMAND_POS] = ACK;
                    
                    Serial.print(F("[SX1272] Sending : "));
                    printMsg(txMsg, ACK_LONG);
                    
                    state = radio.transmit(txMsg, ACK_LONG);
                    //delay(5)  // transmit est blocant
                    isSOk();
                    break;
                    
                default:
                    Serial.println(F("error"));
                    
            }   // end of switch case
            
        } else {   // end of if
            Serial.print(F("[SX1272] Data:\t\t"));
            printMsg(rxMsg, rxLength);
        }
        
        Serial.println(F("-------------------------------------"));
            
    }  // end of if opdone

    // put module back to listen mode
    radio.startReceive();
    enableInterrupt = true;  // enable interrupt
    delay(1000);
    
}  // end of loop
