/*
 * File:   RF_LoRa_868_SO.c
 * Author: JMC
 *
 * Created on 22 mai 2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xc.h>
#include "general.h"
#include "spi.h"
#include "SX1272.h"
#include "RF_LoRa_868_SO.h"


void InitRFLoRaPins(void) {
    RF_RESETpin_DIR = INP;             // on power-up, RESET must be high impedance, so configured as input (see SX1272 datasheet page 114)
    RF_TXpin = 0;                      // on power-up, no transmission
    RF_TXpin_DIR = OUTP;
    RF_RXpin = 0;                      // on power-up, no reception
    RF_RXpin_DIR = OUTP;
}

void ResetRFModule(void) {          // apply sequence given on SX1272 datasheet page 115:
    RF_RESETpin = 1;
    RF_RESETpin_DIR = OUTP;         // RESET = High for 50 ms
    __delay_ms(50);
    RF_RESETpin_DIR = INP;          // then RESET back to high-imedance
    __delay_ms(50);
}

void AntennaTX(void) {              // control antenna swith and connect antenna to RF module output
    RF_RXpin = 0;
    RF_TXpin = 0;
    __delay_ms(10);
    RF_TXpin = 1;
}

void AntennaRX(void) {              // control antenna swith and connect antenna to RF module input
    RF_RXpin = 0;
    RF_TXpin = 0;
    __delay_ms(10);
    RF_RXpin = 1;
}