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
#pragma config DEBUG = ON      // Debugger Enable bit (Background debugger disabled)
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
#include "tableRoutageRepeteur.h"
#include "voltmeter.h"
#include "sendRecept.h"

/*
 * Passage au pic18f25k40 : modifier pin dans 
 * RF_LoRa_868_SO.h -> tout bon
 * spi.h -> ok
 * uart.c -> Modif des noms de registre
 * SX1272.c InitModule() à vérifier
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

    __delay_ms(2500);           // délai avant initialisation du système
    SPIInit();                  // init SPI
    initVoltmeter();            // initialisation de l'ADC
    InitRFLoRaPins();           // configure pins for RF Solutions LoRa module
    ResetRFModule();            // reset the RF Solutions LoRa module (should be optional since Power On Reset is implemented)
    UARTInit(19200);            // init UART @ 19200 bps

    __delay_ms(1);
    __delay_ms(1);
    __delay_ms(500);           // délai avant initialisation du système

    // put module in LoRa mode (see SX1272 datasheet page 107)
    UARTWriteStrLn(" ");
    UARTWriteStrLn("set mode to LoRa standby");

    WriteSXRegister(REG_OP_MODE, FSK_SLEEP_MODE);       // SLEEP mode required first to change bit n°7
    WriteSXRegister(REG_OP_MODE, LORA_SLEEP_MODE);      // switch from FSK mode to LoRa mode
    WriteSXRegister(REG_OP_MODE, LORA_STANDBY_MODE);    // STANDBY mode required fot FIFO loading
    __delay_ms(100);
    GetMode();

    // initialize the module
    UARTWriteStrLn("initialize module");
    InitModule();               // initialisation module LoRa

    // for debugging purpose only: check configuration registers content
    //CheckConfiguration();

    //const uint8_t rxMsg[] = { 0x4E, 0xAD, 0x01, 0x04, 0x01 }; //Découverte Réseau Base
    //const uint8_t txMsg[] = { 0xAD, 0x4E, 0x01, 0x04, 0x01, 0x01 }; //Réponse header + network + node 4 + température + 1 octet
    //const uint8_t rxMsg[] = { 0x4E, 0xAD, 0x01, 0x04, 0x02 }; //Requête données
    //const uint8_t txMsg[] = { 0xAD, 0x4E, 0x01, 0x04, 0x03 }; //Réponse header + network + node 4 + possible ou 04 impossible
    //const uint8_t txMsg[] = { 0xAD, 0x4E, 0x01, 0x04, txMsg[i], lvlBatt }; //Réponse header + network + node 4 + possible ou 04 impossible
    //const uint8_t rxMsg[] = { 0x4E, 0xAD, 0x01, 0x04, 0x05 }; //Accusé reception ou 06 demande renvoi

    uint8_t RXNumberOfBytes;    // to store the number of bytes received
    uint8_t rxMsg[30];              // message reçu
    uint8_t txMsg[] = { HEADER_1, HEADER_0, NUL, NUL, NUL, NUL, NUL, NUL, NUL };    // message transmit

    if(ReadSXRegister(REG_VERSION) != 0x22) { // Sécurité, si on rentre dedans cela signifie que le module LoRa n'est pas detecté
        UARTWriteStrLn("LoRa non detecté");
    }

    forever {
        Receive(rxMsg); // Récupération du message reçu
        RXNumberOfBytes = ReadSXRegister(REG_RX_NB_BYTES); // Récupère la  taille du message reçu

        if(rxMsg[DEST_ID_POS] == REPETEUR && rxMsg[GATEWAY] == REPETEUR) { // Si le répéteur se fait pinger (pour voir si il marche par exemple)
            for (uint8_t i = 0; i < RXNumberOfBytes; i++) {
                txMsg[i] = rxMsg[i];
            } 
            txMsg[HEADER_0_POS] = rxMsg[HEADER_1_POS];
            txMsg[HEADER_1_POS] = rxMsg[HEADER_0_POS];
            txMsg[DEST_ID_POS] = rxMsg[SOURCE_ID_POS];
            txMsg[SOURCE_ID_POS] = rxMsg[DEST_ID_POS];
            txMsg[GATEWAY] = rxMsg[SOURCE_ID_POS];
            txMsg[COMMAND_POS] = ACK;
            Transmit(txMsg, RXNumberOfBytes);
        }

        else if(rxMsg[GATEWAY] == REPETEUR) { // Si le répéteur reçoit un message à répéter
            for (uint8_t i = 0; i < RXNumberOfBytes; i++) {
                txMsg[i] = rxMsg[i];
            }     
            if(rxMsg[DEST_ID_POS] == HEI_ID) txMsg[GATEWAY] = HEI_ID;
            if(rxMsg[DEST_ID_POS] == ISEN_ID) txMsg[GATEWAY] = ISEN_ID;
            Transmit(txMsg, RXNumberOfBytes);
        }
    }   // end of loop forever

    return 0;
}   // end of main