/*
 * File:   usart_async.c
 * Author: lal
 *
 * Created on 9 avril 2015, 07:44
 */

#include "uart.h"

void UARTInit(uint16_t baudRate) {

    // � modifier pour pic18f25k40 -> �a ne bouge ap
    TRISCbits.TRISC6 = INP;                     // RC6 is TxD (but should be declared first as input)
    TRISCbits.TRISC7 = INP;                     // RC7 is RxD
    ANSELCbits.ANSELC7 = DISABLE;                 // disable analog input
    
    TRISAbits.TRISA0 = OUTP;
    LATAbits.LATA0 = CLEAR; // pour corriger les erreurs du petit con de Max
    

    TX1STA = CLEAR;                             // reset USART registers
    RC1STA = CLEAR;
    RC6PPS = 0x09;

    TX1STAbits.SYNC = CLEAR;                    // asynchronous mode
    TX1STAbits.TX9 = CLEAR;                     // 8-bit mode
    RC1STAbits.RX9 = CLEAR;
    RC1STAbits.CREN = SET;                      // continous receive
    RC1STAbits.ADDEN = 0;                       // address detect disable
//    RC1STAbits.ADDEN = SET;                     // address detect enable
    PIE3bits.RC1IE = SET;                       // interrupt on receive
    PIE3bits.TX1IE = CLEAR;                     // no interrupt on transmission

    TX1STAbits.BRGH = SET;                      // baud rate select (high speed)
    BAUD1CONbits.BRG16 = SET;                   // see PIC18F44K22 Silicon Errata (5. Module: EUSART, page 2)
    //SP1BRG = (uint8_t)(((_XTAL_FREQ / (4 * baudRate)) - 1));
    //(uint16_t)(((_XTAL_FREQ / (4 * 19200)) / 6.75 - 1)); // c de la 2mer
    //SP1BRG = 25;                                // baudrate = 9600 bps
    SP1BRG = 12;                                // baudrate = 19200 bps
    SP1BRGH = 0;

//    SPBRG1 = (uint8_t)((_XTAL_FREQ / baudRate) / 16 - 1);   // set baud rate divider

    TX1STAbits.TXEN = ENABLE;                   // enable transmitter
    RC1STAbits.SPEN = ENABLE;                   // enable serial port

    PIR3bits.RC1IF = CLEAR;                               // reset RX pin flag
//    RCIP = CLEAR;                               // not high priority
    PIE3bits.RC1IE = ENABLE;                              // enable RX interrupt
    INTCONbits.PEIE = ENABLE;                              // enable pheripheral interrupt (serial port is a pheripheral)
}

void UARTWriteByte(uint8_t data) {

    while (TX1STAbits.TRMT == CLEAR);           // wait for transmission idle

    TX1REG = data;
}

/*uint8_t UARTReadByte(void) {
    return(RC1REG);
}*/

void UARTWriteStr(char *string) {
  uint8_t i = 0;

  while (string[i])
    UARTWriteByte(string[i++]);
}

void UARTWriteStrLn(char *string) {
  uint8_t i = 0;

  while (string[i])
    UARTWriteByte(string[i++]);
  
  UARTWriteByte(0x0D);      // write Carriage Return
  UARTWriteByte(0x0A);      // write Line Feed (New Line)
}

void UARTWriteByteHex(uint16_t data) {
    char *hexa = "0123456789ABCDEF";
    
    if (data > 4095) UARTWriteByte(hexa[data / 4096]);
    if (data > 255) UARTWriteByte(hexa[data / 256 % 16]);     // write ASCII value of hexadecimal high nibble
    UARTWriteByte(hexa[data / 16 % 16]);     // write ASCII value of hexadecimal high nibble
    UARTWriteByte(hexa[data % 16]);     // write ASCII value of hexadecimal low nibble
    //UARTWriteByte(0x0D);      // write Carriage Return
    //UARTWriteByte(0x0A);      // write Line Feed (New Line))
}

/*void UARTWriteByteDec(uint16_t data) {
    
    if (data > 1000) UARTWriteByte(data / 10000 + '0');
    if (data > 100) UARTWriteByte(data / 1000 % 10 + '0');
    if (data > 10) UARTWriteByte(data / 100 % 10 + '0');              // write ASCII value of hundreds digit
    UARTWriteByte(data / 10 % 10 + '0');       // write ASCII value of tenths digit
    UARTWriteByte(data % 10 + '0');       // write ASCII value of tenths digit

    UARTWriteByte(0x0D);      // write Carriage Return
    UARTWriteByte(0x0A);      // write Line Feed (New Line))
}
*/