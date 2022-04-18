/* 
 * File:   uart.h
 * Author: lal
 *
 * Created on 9 avril 2015, 07:42
 */

#ifndef _UART_H
#define	_UART_H

#include "general.h"
#include <stdint.h>


void UARTInit(uint16_t baudRate);           // init UART with specified baud rate
uint8_t UARTReadByte(void);                 // read a byte from UART
void UARTWriteByte(uint8_t data);           // send a byte to UART
void UARTWriteStr(char *Str);               // output strings to UART
void UARTWriteStrLn(char *Str);             // output strings to UART
void UARTWriteByteHex(uint16_t data);        // send the hexadecimal value of a byte so that it is readable in a terminal window
void UARTWriteByteDec(uint16_t data);        // send the decimal value of a byte so that it is readable in a terminal window

#endif	/* _UART_H */

