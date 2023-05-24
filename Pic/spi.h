/*
 * File:   spi.h
 * Authors: LAL & JMC
 *
 * Created (LAL) on 9 avril 2015
 * Modified (JMC) on 18 mai 2017
 */

#ifndef _SPI_H
#define	_SPI_H

#include "general.h"
#include <stdint.h>             // with this inclusion, the XC compiler will recognize standard types such as uint8_t or int16_t 
                                // (so, their definition in "general.h" is useless)

// PIC18F46K22 SPI master mode
// for MSSP n°2:    SCK is D0
//                  MISO is D1
//                  MOSI is D4
//
/*
#define SPI_SCK_DIR             TRISDbits.TRISD0
#define SPI_MISO_DIR            TRISDbits.TRISD1
#define SPI_MOSI_DIR            TRISDbits.TRISD4
//
// Slave Select is wired on E0
//
#define SPI_SS_DIR              TRISEbits.TRISE0
#define SPI_SS_LAT              LATEbits.LATE0
*/

// pour pic18f25k40
#define SPI_SCK_DIR             TRISCbits.TRISC3
#define SPI_MISO_DIR            TRISCbits.TRISC4
#define SPI_MOSI_DIR            TRISCbits.TRISC5
#define SPI_SS_LORA_DIR         TRISCbits.TRISC2
#define SPI_SS_LORA_LAT         LATCbits.LATC2
#define SPI_SS_LCD_DIR          TRISCbits.TRISC1
#define SPI_SS_LCD_LAT          LATCbits.LATC1
#define SPI_SS_TMP_DIR          TRISCbits.TRISC0
#define SPI_SS_TMP_LAT          LATCbits.LATC0

#define SPI_SS_DISABLE          OUTP_HIGH
#define SPI_SS_ENABLE           OUTP_LOW


void SPIInit(void);                                                         // init SPI in master mode
void SPITransfer(uint8_t data_out);                                        // send a byte
uint8_t SPIReceive(uint8_t data_out);                                      // receive a byte and send another byte

#endif	/* _SPI_H */

