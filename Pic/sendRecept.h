/* 
 * File:   SendRecept.h
 * Author: Fabien AMELINCK
 *
 * Created on 4 juin 2021, 16:54
 */

#ifndef _SENDRECEPT_H
#define	_SENDRECEPT_H

#include "general.h"
#include "uart.h"
#include "SX1272.h"
#include "RF_LoRa_868_SO.h"

#define HEADER_0_POS 0
#define HEADER_0 0x4E

#define HEADER_1_POS 1
#define HEADER_1 0xAD

#define DEST_ID_POS 2
#define SOURCE_ID_POS 3
#define HEI_ID 0x01
#define ISEN_ID 0x02
#define REPEATER_ID 0x03

#define GTW_POS 4

#define COMMAND_POS 5
#define DISCOVER 0x01
#define DATA 0x02
#define ACK_ZIGBEE 0x03
#define NACK_ZIGBEE 0x04
#define ACK 0x05
#define NACK 0x06
#define PING 0x17
#define TIMEOUT 0x42
#define LED_ON 0x66
#define LED_OFF 0x67

#define CLEN_POS 6

#define SENSOR_ID_POS 7
#define T_POS 8
#define O_POS 9
#define ACK_POS 8
#define R_POS 9
#define DATA_LONG 0x03
#define NUL 0x00

#define COMMAND_LONG 7
#define ACK_LONG 6
#define TIMEOUT_LONG 7
#define TRANSMIT_LONG (DATA_LONG + COMMAND_LONG)

void Transmit(const uint8_t *data, const uint8_t data_long);
void Receive(uint8_t *data);
uint8_t hexToDec(uint8_t data);

#endif /*_SENDRECEPT_H */