/*
 * File:   voltmeter.c
 * Author: Fabien AMELINCK
 *
 * Created on 17 June 2021
 */

#include <stdint.h>
#include <xc.h>
#include "general.h"
#include "uart.h"
#include "sendRecept.h"
#include "voltmeter.h"

void initVoltmeter(void) { // initialisation de l'adc
    
    /*
    ADCON1 = 0 de base
    ADCON2 = 0 de base
    ADCON3 = 0 de base
    ADCLK = b'00011111' // Fosc/2*n+1
    ADREF = 0 de base
    ADPCH = RA3 00000011
    ADPRE = 0 de base
    ADACQ = 0x014 // 20 TAD
    ADCAP = 0 de base
    ADRPT = 0 de base
    ADACT = 0 de base
    ADCON0 = b'10000000' ; 7 ADC On 4 fosc avec ADCLK 2 left justified
    */
    
    
    //    _delay(10);                         // wait for 40µs
    
    //Setup ADC
    ADCLK = 0b00011111;             // Fosc/2*n+1
    ADPCH = 0b00000011;             // RA3
    ADACQ = 0x014;                  // 20 TAD
    ADCON0bits.ADON = ON;

    TRISAbits.TRISA3 = INP;         // RA3 input
    ANSELAbits.ANSELA3 = ENABLE;    // RA3 analog
    
    return;
}

uint16_t voltmeterHex(void) { // renvoi la valeur hexa de l'adc sur 10 bits (ex : 03FF)
    
    uint16_t value;
    
    //start conversion
    ADCON0bits.ADGO = ON;         // start conversion
    while (ADCON0bits.ADGO);      // wait until conversion is finished
    value = ((uint16_t)ADRESH << 2) | ((uint16_t)ADRESL >> 6);          // get 10 bits of ADC value
    
    return value;
}

uint8_t voltmeterDec(void) { // renvoi la valeur décimale de l'adc sur 8 bits (ex : 82v)
    
    float volts = (float)((3.2 / 1023) * voltmeterHex());    // convert ADC value into volts
    uint8_t vBatt = (uint8_t)(volts * 10 * 2.765);
    
    return vBatt;
}

uint16_t pourcentBatt(void) { // renvoi le poucentage de batterie sur 8 bits (ex : 95%)
    
    uint16_t pourcentBattDec = (uint16_t)(((((3.2 / 1023) * voltmeterHex()) * 100) - 253.16) / (303.8 - 253.16) * 10000);
    
    return pourcentBattDec;
}

void printBatteryLevel(void) { // affiche et renvoi le poucentage de batterie sur 10 bits (ex : 95.35%)
    
    uint16_t value = voltmeterHex();
    //float volts = (float)((3.2 / 255) * value);    // convert ADC value into volts
    float volts = (float)((3.2 / 1023) * value);    // convert ADC value into volts
    uint16_t integer = (uint16_t)(volts * 1000) % 10000;
    uint16_t vBatt = (uint16_t)(integer * 2.765);
    uint16_t pourcentBatt = (uint16_t)(((float)vBatt / 10.0 - 700.0) / (840.0 - 700.0) * 10000);    // max batt 8,32 min batt 7 min rég 5,3
    //uint8_t pourcentBattHex = (uint8_t)(((float)vBatt / 10.0 - 700.0) / (840.0 - 700.0) * 255);    // max batt 8,32 min batt 7 min rég 5,3
    char string[6];

    string[0] = (pourcentBatt / 1000) + '0';
    string[1] = (pourcentBatt / 100) % 10 + '0';
    string[2] = '.';
    string[3] = (pourcentBatt / 10) % 10 + '0';
    string[4] = pourcentBatt % 10 + '0';
    string[5] = '\0';
    
    UARTWriteStrLn(" ");
    UARTWriteStr("Pourcentage de batterie : ");
    UARTWriteStr(string);
    UARTWriteStrLn("%");
    //UARTWriteByteHex(pourcentBattHex);
    //UARTWriteStr(" ou ");
    //UARTWriteByteDec(pourcentBatt);
    //UARTWriteStr("Voltage pont diviseur : ");
    //UARTWriteByteDec(integer);
    /*UARTWriteStr("Voltage batterie : 0x");
    UARTWriteByteHex(hexToDec(voltmeterDec() / 100));
    UARTWriteByteHex(hexToDec(voltmeterDec() % 100));
    UARTWriteStrLn(" ");*/
    /*UARTWriteStr("Sortie ADC : ");
    UARTWriteByteHex(value);
    UARTWriteStr("/");
    UARTWriteByteHex(value >> 2);
    UARTWriteStrLn(" ");*/
    
    return;
}