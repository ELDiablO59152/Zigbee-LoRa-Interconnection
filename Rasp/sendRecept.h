/*
 * File:   sendRecept.h
 * Author: Fabien AMELINCK
 *
 * Created on 4 juin 2021, 16:54
 */

#ifndef _SENDRECEPT_H
#define _SENDRECEPT_H

#include <stdint.h>         // for uint
#include <unistd.h>         //usleep
#include "SX1272.h"         // for SXRegister
#include "RF_LoRa_868_SO.h" // for Antenna
#include "filecsv.h"        //write

#define HEADER_0_POS 0
#define HEADER_0 0x4E

#define HEADER_1_POS 1
#define HEADER_1 0xAD

#define DEST_ID_POS 2
#define SOURCE_ID_POS 3
#define HEI_ID 0x01
#define ISEN_ID 0x02

#define COMMAND_POS 4
#define DISCOVER 0x01
#define ASKING 0x02
#define ABLE_MEASURE 0x03
#define DISABLE_MEASURE 0x04
#define ACK 0x05
#define NACK 0x06
#define TIMEOUT 0x42
#define LED_ON 0x66
#define LED_OFF 0x67
#define PING 0x17

#define DATA_LONG_POS 5
#define DATA_LONG 0x01
#define NUL 0x00

#define DISCOVER_LONG 5
#define ACK_LONG 5
#define TRANSMIT_LONG (DATA_LONG + 4)
#define DISABLE_LONG 5

uint8_t WaitIncomingMessageRXSingle(uint8_t *PointTimeout);
uint8_t WaitIncomingMessageRXContinuous(void);
int8_t LoadRxBufferWithRxFifo(uint8_t *Table, uint8_t *PointNbBytesReceived);
void LoadTxFifoWithTxBuffer(uint8_t *Table, const uint8_t PayloadLength);
void TransmitLoRaMessage(void);

// void Transmit(const uint8_t *data, const uint8_t data_long);
// void Receive(uint8_t *data);
uint8_t hexToDec(uint8_t data);

#endif /*_SENDRECEPT_H */