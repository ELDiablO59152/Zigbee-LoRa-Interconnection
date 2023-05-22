/* 
 * File:   tableRoutageRepeteur.h
 * Author: Sasha LUCAS
 *
 * Created on 22 mai 2023, 16:11
 */

#ifndef _SENDRECEPT_H
#define	_SENDRECEPT_H

#define HEADER_0_POS 0
#define HEADER_0 0x4E

#define HEADER_1_POS 1
#define HEADER_1 0xAD

#define DEST_ID_POS 2
#define SOURCE_ID_POS 3
#define ISEN_ID 0x01
#define ISEN_REPETEUR_ID 0x02
#define HEI_ID 0x03
#define HEI_REPETEUR_ID 0x04

#define GATEWAY 4

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

#define SENSOR_ID_POS 5
#define T_POS 6
#define O_POS 7
#define ACK_POS 6
#define R_POS 7
#define DATA_LONG 0x03
#define NUL 0x00

#define COMMAND_LONG 5
#define ACK_LONG 5
#define TRANSMIT_LONG (DATA_LONG + COMMAND_LONG)

void Transmit(const uint8_t *data, const uint8_t data_long);
void Receive(uint8_t *data);
uint8_t hexToDec(uint8_t data);

#endif /*tableRoutageRepeteur.h */