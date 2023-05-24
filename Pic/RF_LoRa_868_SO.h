/*
 * File:   RF_LoRa_868_SO.h
 * Author: JMC
 *
 * Created on 22 mai 2017
 */

// RF Solutions RF-LoRa-868-SO module //

#ifndef _RF_LoRa_868_SO_H
#define	_RF_LoRa_868_SO_H

#include "general.h"
#include <stdint.h>             // with this inclusion, the XC compiler will recognize standard types such as uint8_t or int16_t 
                                // (so, their definition in "general.h" is useless)


#define RF_RXpin     LATBbits.LATB4                  // antenna switch RX control pin
#define RF_TXpin     LATBbits.LATB3                  // antenna switch TX control pin
#define RF_RESETpin  LATBbits.LATB2                  // Reset input

#define RF_RXpin_DIR       TRISBbits.TRISB4          // direction bit for RX control line
// attention: si on met en sortie la patte B5 au lieu de la patte B4,
// la tension à l'état haut sur la sortie TX (de l'UART) ne monte plus qu'à 500 mV au lieu de monter à Vcc
// ??????????????????? mystère ?????????????????
// problème sur le demo board ?????????
#define RF_TXpin_DIR       TRISBbits.TRISB3          // direction bit for TX control line
#define RF_RESETpin_DIR    TRISBbits.TRISB2          // direction bit for Reset

#define PAYLOAD_LENGTH 7                             // for transmission: number of bytes to transmit
                                                     // (this value will be stored before transmission in REG_PAYLOAD_LENGTH_LORA register of SX1272 chip)
#define POUT 14                                      // output power (in dBm)
                                                     // (used to compute the data to store in REG_PA_CONFIG register during configuration of SX1272 chip)

void InitRFLoRaPins(void);                          // configure pins for RF Solutions module
void ResetRFModule(void);                           // configure pins for RF Solutions module
void AntennaTX(void);                               // connect antenna to module output
void AntennaRX(void);                               // connect antenna to module input

#endif	/* _RF_LoRa_868_SO_H */

