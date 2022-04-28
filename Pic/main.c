/*
 * File:   Capteur_Temp.c
 * Authors: Fabien AMELINCK
 *
 * Created on 03 June 2021
 */

// CONFIG1L
#pragma config FEXTOSC = OFF    // External Oscillator mode Selection bits (EC (external clock) above 8 MHz; PFM set to high power)
#pragma config RSTOSC = HFINTOSC_1MHZ  // Power-up default value for COSC bits (EXTOSC operating per FEXTOSC bits (device manufacturing default))

// CONFIG1H
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)

// CONFIG2L
#pragma config MCLRE = EXTMCLR  // Master Clear Enable bit (If LVP = 0, MCLR pin is MCLR; If LVP = 1, RE3 pin function is MCLR )
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (Power up timer disabled)
#pragma config LPBOREN = OFF    // Low-power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled , SBOREN bit is ignored)

// CONFIG2H
#pragma config BORV = VBOR_2P45 // Brown Out Reset Voltage selection bits (Brown-out Reset Voltage (VBOR) set to 2.45V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD disabled. ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit (PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config DEBUG = OFF      // Debugger Enable bit (Background debugger disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Extended Instruction Set and Indexed Addressing Mode disabled)

// CONFIG3L
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = OFF        // WDT operating mode (WDT enabled regardless of sleep)

// CONFIG3H
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT input clock selector (Software Control)

// CONFIG4L
#pragma config WRT0 = OFF       // Write Protection Block 0 (Block 0 (000800-001FFFh) not write-protected)
#pragma config WRT1 = OFF       // Write Protection Block 1 (Block 1 (002000-003FFFh) not write-protected)
#pragma config WRT2 = OFF       // Write Protection Block 2 (Block 2 (004000-005FFFh) not write-protected)
#pragma config WRT3 = OFF       // Write Protection Block 3 (Block 3 (006000-007FFFh) not write-protected)

// CONFIG4H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-30000Bh) not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block (000000-0007FFh) not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)
#pragma config SCANE = ON       // Scanner Enable bit (Scanner module is available for use, SCANMD bit can control the module)
#pragma config LVP = ON         // Low Voltage Programming Enable bit (Low voltage programming enabled. MCLR/VPP pin function is MCLR. MCLRE configuration bit is ignored)

// CONFIG5L
#pragma config CP = OFF         // UserNVM Program Memory Code Protection bit (UserNVM code protection disabled)
#pragma config CPD = OFF        // DataNVM Memory Code Protection bit (DataNVM code protection disabled)

// CONFIG5H

// CONFIG6L
#pragma config EBTR0 = OFF      // Table Read Protection Block 0 (Block 0 (000800-001FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection Block 1 (Block 1 (002000-003FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR2 = OFF      // Table Read Protection Block 2 (Block 2 (004000-005FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR3 = OFF      // Table Read Protection Block 3 (Block 3 (006000-007FFFh) not protected from table reads executed in other blocks)

// CONFIG6H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot Block (000000-0007FFh) not protected from table reads executed in other blocks)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#define USE_AND_MASKS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <xc.h>
#include "general.h"
#include "uart.h"
#include "spi.h"
#include "SX1272.h"
#include "RF_LoRa_868_SO.h"
#include "sendRecept.h"
#include "voltmeter.h"

/*
 * Passage au pic18f25k40 : modifier pin dans 
 * RF_LoRa_868_SO.h -> tout bon
 * spi.h -> ok
 * uart.c -> Modif des noms de registre
 * SX1272.c InitModule() � v�rifier
 * Ans = ansel
 *  
 * RC5 MOSI 15
 * RC4 MISO 14
 * RC3 Clock 13
 * RC2 CS LoRa 16
 * RC1 CS Lcd
 * RC0 CS Capteur
 * RB4 RX Pic 5
 * RB3 TX Pic 4
 * RB2 Reset 12
 */

int main(int argc, char** argv) {
    
    __delay_ms(2500);           // d�lai avant initialisation du syst�me
    SPIInit();                  // init SPI
    initVoltmeter();            // initialisation de l'ADC
    InitRFLoRaPins();           // configure pins for RF Solutions LoRa module
    ResetRFModule();            // reset the RF Solutions LoRa module (should be optional since Power On Reset is implemented)
    UARTInit(19200);            // init UART @ 19200 bps

    clear_ecran();
    __delay_ms(1);
    clear_ecran();
    __delay_ms(1);
    __delay_ms(500);           // d�lai avant initialisation du syst�me
    
    // put module in LoRa mode (see SX1272 datasheet page 107)
    UARTWriteStrLn(" ");
    UARTWriteStrLn("set mode to LoRa standby");

    WriteSXRegister(REG_OP_MODE, FSK_SLEEP_MODE);       // SLEEP mode required first to change bit n�7
    WriteSXRegister(REG_OP_MODE, LORA_SLEEP_MODE);      // switch from FSK mode to LoRa mode
    WriteSXRegister(REG_OP_MODE, LORA_STANDBY_MODE);    // STANDBY mode required fot FIFO loading
    __delay_ms(100);
    GetMode();
    
    // initialize the module
    UARTWriteStrLn("initialize module");
    InitModule();               // initialisation module LoRa
    
    // for debugging purpose only: check configuration registers content
    //CheckConfiguration();
    
    //const uint8_t rxMsg[] = { 0x4E, 0xAD, 0x01, 0x04, 0x01 }; //D�couverte R�seau Base
    //const uint8_t txMsg[] = { 0xAD, 0x4E, 0x01, 0x04, 0x01, 0x01 }; //R�ponse header + network + node 4 + temp�rature + 1 octet
    //const uint8_t rxMsg[] = { 0x4E, 0xAD, 0x01, 0x04, 0x02 }; //Requ�te donn�es
    //const uint8_t txMsg[] = { 0xAD, 0x4E, 0x01, 0x04, 0x03 }; //R�ponse header + network + node 4 + possible ou 04 impossible
    //const uint8_t txMsg[] = { 0xAD, 0x4E, 0x01, 0x04, txMsg[i], lvlBatt }; //R�ponse header + network + node 4 + possible ou 04 impossible
    //const uint8_t rxMsg[] = { 0x4E, 0xAD, 0x01, 0x04, 0x05 }; //Accus� reception ou 06 demande renvoi
    
    uint8_t RXNumberOfBytes;    // to store the number of bytes received
    uint8_t rxMsg[30];              // message re�u
    uint8_t txMsg[] = { HEADER_1, HEADER_0, NUL, NUL, NUL, NUL, NUL, NUL, NUL };    // message transmit
    uint8_t i;
    uint16_t batterie = pourcentBatt();
    
    forever {
        
        //UARTWriteStrLn("-----------------------");
        
        //for(i = 0; i < RXNumberOfBytes; i++) rxMsg[i] = 0;  // prends trop de temps ptn
        
        Receive(rxMsg);             // r�cup�ration du message re�u
        
        if(rxMsg[COMMAND_POS] == 42) {
            UARTWriteStrLn(" ");
            UARTWriteStr("Message recu : ");

            RXNumberOfBytes = ReadSXRegister(REG_RX_NB_BYTES);  // r�cup�ration de la taille du payload

            for(i = 0; i < RXNumberOfBytes; i++) {      // affichage du message re�u
                UARTWriteByteHex(rxMsg[i]);             //
                UARTWriteStr(" ");                      //
            }                                           //
            UARTWriteStrLn(" ");                        //
        }
        
        txMsg[COMMAND_POS] = 0x00;
        txMsg[COMMAND_POS + 1] = 0x00;
        
        if(rxMsg[DEST_ID_POS] == NODE_ID) {    // si message de notre r�seau..
            switch (rxMsg[COMMAND_POS]) {               // type de message
                case DISCOVER:
                    UARTWriteStr("Discover net : ");
                    UARTWriteByteHex(rxMsg[DEST_ID_POS]);   // affichage du network en d�couverte
                    UARTWriteStrLn(" ");
                    
                    UARTWriteStrLn(" ");
                    UARTWriteStrLn("Enregistrement");
                    txMsg[HEADER_0_POS] = HEADER_1;     // headers retourn�s
                    txMsg[HEADER_1_POS] = HEADER_0;     //
                    txMsg[DEST_ID_POS] = rxMsg[SOURCE_ID_POS]; // network 1
                    txMsg[SOURCE_ID_POS] = NODE_ID;       // groupe 4
                    txMsg[COMMAND_POS + 1] = DATA_LONG;   // 3 bytes de longueur

                    Transmit(txMsg, COMMAND_LONG);     // transmission
                    break;
                    
                case DATA:
                    UARTWriteStrLn("Requete de donnees");
                    
                    if(rxMsg[DEST_ID_POS] != NODE_ID) break;
                    
                    UARTWriteStrLn(" ");
                    UARTWriteStrLn("Mesure possible");
                    txMsg[COMMAND_POS] = ACK;          // mesure possible 3
                    
                    Transmit(txMsg, ACK_LONG);              // transmission
                    
                    UARTWriteStrLn(" ");
                    UARTWriteStrLn("Mesure en cours");
                    __delay_ms(100);
                    
                    //uint8_t pourcentBattHex = (uint8_t)(((((3.2 / 1023) * voltmeterHex()) * 100) - 253.2) / (300.9 - 253.2) * 255);    // max batt 8,32 min batt 7 min r�g 5,3
                    
                    batterie = pourcentBatt();

                    txMsg[COMMAND_POS + DATA_LONG - 2] = hexToDec((uint8_t)(batterie / 100));  // pourcentage de batterie
                    txMsg[COMMAND_POS + DATA_LONG - 1] = hexToDec(voltmeterDec());  // tension de batterie
                    
                    Transmit(txMsg, TRANSMIT_LONG);         // transmission
                    break;
                    
                case ACK_ZIGBEE:
                    UARTWriteStrLn("Mesure possible");
                    break;
                    
                case NACK_ZIGBEE:
                    UARTWriteStrLn("Mesure impossible");
                    
                    if(rxMsg[DEST_ID_POS] != NODE_ID) break;
                    
                    txMsg[COMMAND_POS] = NACK;
                    
                    Transmit(txMsg, COMMAND_LONG);
                    break;
                    
                case ACK:
                    UARTWriteStrLn("Accuse reception");
                    break;
                    
                case NACK:
                    UARTWriteStrLn("Erreur de transfert");
                    
                    if(rxMsg[DEST_ID_POS] != NODE_ID) break;
                    
                    txMsg[COMMAND_POS + DATA_LONG - 2] = hexToDec((uint8_t)(batterie / 100));  // pourcentage de batterie
                    txMsg[COMMAND_POS + DATA_LONG - 1] = hexToDec(voltmeterDec());  // tension de batterie
                    
                    Transmit(txMsg, TRANSMIT_LONG);         // transmission
                    break;
                    
                case TIMEOUT:
                    UARTWriteStrLn("Timeout");
                    rxMsg[COMMAND_POS] = 0;
                    break;
                    
                default:
                    UARTWriteStrLn("error");
                    
            }   // end of switch case
            
        }   // end of if
        
    }   // end of loop forever
    
    return 0;
    
}   // end of main