/*
 * File:   spi.c
 * Authors: LAL & JMC
 *
 * Created(LaL) on 9 avril 2015
 * Modified (JMC) on 18 mai 2017
 */

#include "spi.h"

void SPIInit(void) {

    SPI_SCK_DIR = OUTP;                     // SCK = output
    ANSELCbits.ANSELC3 = DISABLE;             // disable analog input for SCK
    SPI_MISO_DIR = INP;                     // SDI = input
    ANSELCbits.ANSELC4 = DISABLE;             // disable analog input for MISO
    SPI_MOSI_DIR = OUTP;                    // MOSI

    SPI_SS_LORA_DIR = OUTP;                      // SS = output
    SPI_SS_LORA_LAT = SPI_SS_DISABLE;            // disable SS (SS = High)
    
    SPI_SS_TMP_DIR = OUTP;                      // SS = output
    SPI_SS_TMP_LAT = SPI_SS_DISABLE;            // disable SS (SS = High)
    
    SPI_SS_LCD_DIR = OUTP;
    SPI_SS_LCD_LAT = SPI_SS_DISABLE;            // disable SS (SS = High)
    
    RC3PPS = 0x0D;
    RC5PPS = 0x0E;
    SSP1CLKPPS = 0b00010011;
    SSP1DATPPS = 0b00010100;
    SSP1STAT = 0b01000000;
    //SSP1STATbits.SMP = CLEAR;               // SMP = 0 (input sampled at middle of output)
    //SSP1STATbits.CKE = SET;                 // CKE = 1 (transmit occurs from active (1) to idle (0) SCK state, and sampling occurs from idle to active state)
    SSP1CON1 = 0b00100000;                  // WCOL not used in SPI mode, SSPOV not used in SPI master mode
                                            // SSPEN set (enable serial port), CKP cleared (idle state is low level)
                                            // SSP1CON1<3:0> = 0000 (clock = FOSC / 4) = 250kHz
    //SSP1STAT = 0b11000000;  //SMP = 1, CKE = 1, Data sampled at the end of data output time, transmit transition from active to idle
    //SSP1CON1 = 0b00110000; // SSPEN = 1; CKP = 1 = CLOCK HIGH, Fosc/4
    
    SSP1CON3 = 0b00000000;
}


void SPITransfer(uint8_t data_out) {        // Warning: Slave Select is not managed in this function
                                            // don't forget to control SS before and after calling this function
    uint8_t dummy_byte;
    dummy_byte = SSP1BUF;                   // clear BF (Buffer Full Status bit)                    
    PIR3bits.SSP1IF = CLEAR;                // clear interrupt flag
    SSP1BUF = data_out;                     // transmit data
    while (!PIR3bits.SSP1IF);               // wait until transmission is complete
}

uint8_t SPIReceive(uint8_t data_out) {      // Warning: Slave Select is not managed in this function
                                            // don't forget to control SS before and after calling this function
    uint8_t data_in, dummy_byte;
    dummy_byte = SSP1BUF;                   // clear BF (Buffer Full Status bit) 
    PIR3bits.SSP1IF = CLEAR;                // clear interrupt flag
    SSP1BUF = data_out;                     // transmit data
    while (!PIR3bits.SSP1IF);               // wait until transmission is complete
    data_in = SSP1BUF;                      // store received data
    return(data_in);
}