/*
 * File:   sensor.c
 * Author: Fabien AMELINCK
 *
 * Created on 17 June 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <xc.h>
#include "general.h"
#include "uart.h"
#include "spi.h"
#include "sendRecept.h"
#include "sensor.h"

uint16_t humidityHex(void) { // renvoi l'humidité sur 16 bits (ex : 23E7)
    
    uint16_t value;
    
    SPI_SS_TMP_LAT = SPI_SS_ENABLE;             // enable slave
    
    //SPITransfer(0x00);
    do {
        value = (uint16_t)SPIReceive(0x00);            // send dummy data and receive register content
    } while ((value & 0x00C0) == 0x00);            // waitinf for S1 and S0 = 0
    
    value = (value << 6) | (uint16_t)SPIReceive(0x00) >> 2;            // send dummy data and receive register content
        
    SPI_SS_TMP_LAT = SPI_SS_DISABLE;            // disable slave
    return value;
}

uint16_t temperatureHex(void) { // renvoi la température sur 16 bits (ex : 19F3)
    
    uint16_t value;
    uint8_t trash;
    
    SPI_SS_TMP_LAT = SPI_SS_ENABLE;             // enable slave
    
    //SPITransfer(0x00);
    do {
        trash = SPIReceive(0x00);            // send dummy data and receive register content
    } while ((trash & 0xC0) == 0x00);            // waitinf for S1 and S0 = 0
    trash = SPIReceive(0x00);            // send dummy data and receive register content
    
    value = ((uint16_t)SPIReceive(0x00) << 6) | (uint16_t)SPIReceive(0x00) >> 2;            // send dummy data and receive register content
        
    SPI_SS_TMP_LAT = SPI_SS_DISABLE;            // disable slave
    return value;
}

uint16_t humidityDec(void) { // renvoi l'humidité (ex : 73)
    
    uint16_t humidite = (uint16_t)(humidityHex() / (16384.0 - 2) * 10000);    // convert ADC value into volts
    
    return humidite;
}

uint16_t temperatureDec(void) { // renvoi la température sur 8 bits (ex : 24)
    
    uint16_t temperature = (uint16_t)((temperatureHex() / (16384.0 - 2) * 165 - 40) * 100);
    
    return temperature;
}

void printHumidityLevel(void) { // affiche l'humidité (ex : 70.27)
    
    uint16_t value = humidityHex();
    float humidite = (float)(value / (16384.0 - 2) * 100);
    uint16_t integer = (uint16_t)(humidite * 10);
    char string[6];

    string[0] = (integer / 1000) + '0';
    string[1] = (integer / 100) % 10 + '0';
    string[2] = (integer / 10) % 10 + '0';
    string[3] = '.';
    string[4] = integer % 10 + '0';
    string[5] = '\0';
    
    UARTWriteStrLn(" ");
    UARTWriteStr("Humidite : ");
    UARTWriteStrLn(string);
    /*("Humidite x 10 : ");
    UARTWriteByteDec(integer);
    UARTWriteStr("Humidite : 0x");
    UARTWriteByteHex(hexToDec(humidityDec()));
    UARTWriteStrLn(" ");
    UARTWriteStr("Sortie capteur : ");
    UARTWriteByteHex(value);
    UARTWriteStr("/");
    UARTWriteByteHex(value >> 6);
    UARTWriteStrLn(" ");*/
    
    return;
}

void printTemperatureLevel(void) { // affiche la température (ex : 24.76)
    
    /*uint16_t value = temperatureHex();
    float degre = (float)(value / (16384.0 - 2) * 165 - 40);
    uint16_t integer = (uint16_t)(degre * 100);*/
    uint16_t integer = sendMeanTemp();
    char string[6];

    string[0] = (integer / 1000) + '0';
    string[1] = (integer / 100) % 10 + '0';
    string[2] = '.';
    string[3] = (integer / 10) % 10 + '0';
    string[4] = integer % 10 + '0';
    string[5] = '\0';
    
    //UARTWriteStrLn(" ");
    UARTWriteStr("Temperature : ");
    UARTWriteStrLn(string);
    /*UARTWriteStr("Temperature x 100 : ");
    UARTWriteByteDec(integer);
    UARTWriteStr("Temperature : 0x");
    UARTWriteByteHex(hexToDec(temperatureDec()));
    UARTWriteStrLn(" ");
    UARTWriteStr("Sortie capteur : ");
    UARTWriteByteHex(value);
    UARTWriteStr("/");
    UARTWriteByteHex(value >> 6);
    UARTWriteStrLn(" ");*/
    
    return;
}

uint16_t sendMeanHumi(void) { // renvoi la valeur moyenne d'humidité sur 20 mesures sur 14 bits (ex : 19F3)
    
    uint16_t humiMoy[50];
    uint16_t integer;
    uint8_t i;
    uint8_t j;

    for(j = 0; j < 20; j++) {
        //UARTWriteStr("Mesure n");
        //UARTWriteByteDec(j + 1);

        integer = (uint16_t)((humidityHex() / (16384.0 - 2) * 10000));

        humiMoy[j] = integer;

        if(j == 19) {
            //char string[6];
            integer = humiMoy[0];

            for(i = 1; i < 20; i++) {
                integer = (integer + humiMoy[i]) / 2;
            }

            /*string[0] = (integer / 1000) + '0';
            string[1] = (integer / 100) % 10 + '0';
            string[2] = '.';
            string[3] = (integer / 10) % 10 + '0';
            string[4] = integer % 10 + '0';
            string[5] = '\0';

            UARTWriteStr("Temperature moyenne de l'isen : ");
            UARTWriteStr(string);
            UARTWriteStrLn("C");*/
        }   // end of if
    }   // end of for
    return integer;
}

uint16_t sendMeanTemp(void) { // renvoi la valeur moyenne de température sur 20 mesures sur 14 bits (ex : 19F3)
    
    uint16_t tempMoy[50];
    uint16_t integer;
    uint8_t i;
    uint8_t j;

    for(j = 0; j < 20; j++) {
        //UARTWriteStr("Mesure n");
        //UARTWriteByteDec(j + 1);

        integer = (uint16_t)((temperatureHex() / (16384.0 - 2) * 165 - 40) * 100);

        if(integer > 5000 || integer < 500) j--;
        else tempMoy[j] = integer;

        if(j == 19) {
            //char string[6];
            integer = tempMoy[0];

            for(i = 1; i < 20; i++) {
                integer = (integer + tempMoy[i]) / 2;
            }

            /*string[0] = (integer / 1000) + '0';
            string[1] = (integer / 100) % 10 + '0';
            string[2] = '.';
            string[3] = (integer / 10) % 10 + '0';
            string[4] = integer % 10 + '0';
            string[5] = '\0';

            UARTWriteStr("Temperature moyenne de l'isen : ");
            UARTWriteStr(string);
            UARTWriteStrLn("C");*/
        }   // end of if
    }   // end of for
    return integer;
}