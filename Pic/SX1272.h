/*
 * File:   SX1272.h
 * Authors: BRS & JMC
 *
 * Created on 19 mai 2017
 */

#ifndef _SX1272_H
#define	_SX1272_H

#include "general.h"
#include <stdint.h>             // with this inclusion, the XC compiler will recognize standard types such as uint8_t or int16_t 
                                // (so, their definition in "general.h" is useless)

// SX1272 REGISTERS //

#define        REG_FIFO        					0x00
#define        REG_OP_MODE        				0x01
#define        REG_BITRATE_MSB    				0x02
#define        REG_BITRATE_LSB    				0x03
#define        REG_FDEV_MSB   					0x04
#define        REG_FDEV_LSB    					0x05
#define        REG_FRF_MSB    					0x06
#define        REG_FRF_MID    					0x07
#define        REG_FRF_LSB    					0x08
#define        REG_PA_CONFIG    				0x09
#define        REG_PA_RAMP    					0x0A
#define        REG_OCP    						0x0B
#define        REG_LNA    						0x0C
#define        REG_RX_CONFIG    				0x0D
#define        REG_FIFO_ADDR_PTR  				0x0D
#define        REG_RSSI_CONFIG   				0x0E
#define        REG_FIFO_TX_BASE_ADDR 		    0x0E
#define        REG_RSSI_COLLISION    			0x0F
#define        REG_FIFO_RX_BASE_ADDR   			0x0F
#define        REG_RSSI_THRESH    				0x10
#define        REG_FIFO_RX_CURRENT_ADDR   		0x10
#define        REG_RSSI_VALUE_FSK	    		0x11
#define        REG_IRQ_FLAGS_MASK    			0x11
#define        REG_RX_BW		    			0x12
#define        REG_IRQ_FLAGS	    			0x12
#define        REG_AFC_BW		    			0x13
#define        REG_RX_NB_BYTES	    			0x13
#define        REG_OOK_PEAK	    				0x14
#define        REG_RX_HEADER_CNT_VALUE_MSB  	0x14
#define        REG_OOK_FIX	    				0x15
#define        REG_RX_HEADER_CNT_VALUE_LSB  	0x15
#define        REG_OOK_AVG	 					0x16
#define        REG_RX_PACKET_CNT_VALUE_MSB  	0x16
#define        REG_RX_PACKET_CNT_VALUE_LSB  	0x17
#define        REG_MODEM_STAT	  				0x18
#define        REG_PKT_SNR_VALUE	  			0x19
#define        REG_AFC_FEI	  					0x1A
#define        REG_PKT_RSSI_VALUE	  			0x1A
#define        REG_AFC_MSB	  					0x1B
#define        REG_RSSI_VALUE_LORA	  			0x1B
#define        REG_AFC_LSB	  					0x1C
#define        REG_HOP_CHANNEL	  				0x1C
#define        REG_FEI_MSB	  					0x1D
#define        REG_MODEM_CONFIG1	 		 	0x1D
#define        REG_FEI_LSB	  					0x1E
#define        REG_MODEM_CONFIG2	  			0x1E
#define        REG_PREAMBLE_DETECT  			0x1F
#define        REG_SYMB_TIMEOUT_LSB  			0x1F
#define        REG_RX_TIMEOUT1	  				0x20
#define        REG_PREAMBLE_MSB_LORA  			0x20
#define        REG_RX_TIMEOUT2	  				0x21
#define        REG_PREAMBLE_LSB_LORA  			0x21
#define        REG_RX_TIMEOUT3	 				0x22
#define        REG_PAYLOAD_LENGTH_LORA		 	0x22
#define        REG_RX_DELAY	 					0x23
#define        REG_MAX_PAYLOAD_LENGTH 			0x23
#define        REG_OSC		 					0x24
#define        REG_HOP_PERIOD	  				0x24
#define        REG_PREAMBLE_MSB_FSK 			0x25
#define        REG_FIFO_RX_BYTE_ADDR 			0x25
#define        REG_PREAMBLE_LSB_FSK 			0x26
#define        REG_SYNC_CONFIG	  				0x27
#define        REG_SYNC_VALUE1	 				0x28
#define        REG_SYNC_VALUE2	  				0x29
#define        REG_SYNC_VALUE3	  				0x2A
#define        REG_SYNC_VALUE4	  				0x2B
#define        REG_SYNC_VALUE5	  				0x2C
#define        REG_SYNC_VALUE6	  				0x2D
#define        REG_SYNC_VALUE7	  				0x2E
#define        REG_SYNC_VALUE8	  				0x2F
#define        REG_PACKET_CONFIG1	  			0x30
#define        REG_PACKET_CONFIG2	  			0x31
#define        REG_DETECT_OPTIMIZE              0x31
#define        REG_PAYLOAD_LENGTH_FSK			0x32
#define        REG_NODE_ADRS	  				0x33
#define        REG_INVERT_IQ	  				0x33
#define        REG_BROADCAST_ADRS	 		 	0x34
#define        REG_FIFO_THRESH	  				0x35
#define        REG_SEQ_CONFIG1	  				0x36
#define        REG_SEQ_CONFIG2	  				0x37
#define        REG_DETECTION_THRESHOLD          0x37
#define        REG_TIMER_RESOL	  				0x38
#define        REG_TIMER1_COEF	  				0x39
#define        REG_SYNC_WORD	  				0x39
#define        REG_TIMER2_COEF	  				0x3A
#define        REG_IMAGE_CAL	  				0x3B
#define        REG_TEMP		  					0x3C
#define        REG_LOW_BAT	  					0x3D
#define        REG_IRQ_FLAGS1	  				0x3E
#define        REG_IRQ_FLAGS2	  				0x3F
#define        REG_DIO_MAPPING1	  				0x40
#define        REG_DIO_MAPPING2	  				0x41
#define        REG_VERSION	  					0x42
#define        REG_AGC_REF	  					0x43
#define        REG_AGC_THRESH1	  				0x44
#define        REG_AGC_THRESH2	  				0x45
#define        REG_AGC_THRESH3	  				0x46
#define        REG_PLL_HOP	  					0x4B
#define        REG_TCXO		  					0x58
#define        REG_PA_DAC		  				0x5A
#define        REG_PLL		  					0x5C
#define        REG_PLL_LOW_PN	  				0x5E
#define        REG_FORMER_TEMP	  				0x6C
#define        REG_BIT_RATE_FRAC	  			0x70

#define        LORA_SLEEP_MODE                  0x80
#define        LORA_STANDBY_MODE                0x81
#define        LORA_TX_MODE                     0x83
#define        LORA_RX_CONTINUOUS_MODE          0x85
#define        LORA_RX_SINGLE_MODE              0x86
#define        LORA_CAD_MODE                    0x87
#define        LORA_STANDBY_FSK_REGS_MODE       0xC1

#define        FSK_SLEEP_MODE                   0x00
#define        FSK_STANDBY_MODE                 0x01
#define        FSK_TX_MODE                      0x03
#define        FSK_RX_MODE                      0x05
/*
//FREQUENCY CHANNELS:
const uint32_t CH_10_868 = 0xD84CCC; // channel 10, central freq = 865.20MHz
const uint32_t CH_11_868 = 0xD86000; // channel 11, central freq = 865.50MHz
const uint32_t CH_12_868 = 0xD87333; // channel 12, central freq = 865.80MHz
const uint32_t CH_13_868 = 0xD88666; // channel 13, central freq = 866.10MHz
const uint32_t CH_14_868 = 0xD89999; // channel 14, central freq = 866.40MHz
const uint32_t CH_15_868 = 0xD8ACCC; // channel 15, central freq = 866.70MHz
const uint32_t CH_16_868 = 0xD8C000; // channel 16, central freq = 867.00MHz
const uint32_t CH_17_868 = 0xD90000; // channel 16, central freq = 868.00MHz
const uint32_t CH_00_900 = 0xE1C51E; // channel 00, central freq = 903.08MHz
const uint32_t CH_01_900 = 0xE24F5C; // channel 01, central freq = 905.24MHz
const uint32_t CH_02_900 = 0xE2D999; // channel 02, central freq = 907.40MHz
const uint32_t CH_03_900 = 0xE363D7; // channel 03, central freq = 909.56MHz
const uint32_t CH_04_900 = 0xE3EE14; // channel 04, central freq = 911.72MHz
const uint32_t CH_05_900 = 0xE47851; // channel 05, central freq = 913.88MHz
const uint32_t CH_06_900 = 0xE5028F; // channel 06, central freq = 916.04MHz
const uint32_t CH_07_900 = 0xE58CCC; // channel 07, central freq = 918.20MHz
const uint32_t CH_08_900 = 0xE6170A; // channel 08, central freq = 920.36MHz
const uint32_t CH_09_900 = 0xE6A147; // channel 09, central freq = 922.52MHz
const uint32_t CH_10_900 = 0xE72B85; // channel 10, central freq = 924.68MHz
const uint32_t CH_11_900 = 0xE7B5C2; // channel 11, central freq = 926.84MHz
const uint32_t CH_12_900 = 0xE4C000; // default channel 915MHz, the module is configured with it
*/

/*
//LORA BANDWIDTH:
const uint8_t BW_125 = 0x00;
const uint8_t BW_250 = 0x01;
const uint8_t BW_500 = 0x02;
const double SignalBwLog[] =
{
    5.0969100130080564143587833158265,
    5.397940008672037609572522210551,
    5.6989700043360188047862611052755
};

//LORA CODING RATE:
const uint8_t CR_5 = 0x01;
const uint8_t CR_6 = 0x02;
const uint8_t CR_7 = 0x03;
const uint8_t CR_8 = 0x04;

//LORA SPREADING FACTOR:
const uint8_t SF_6 = 0x06;
const uint8_t SF_7 = 0x07;
const uint8_t SF_8 = 0x08;
const uint8_t SF_9 = 0x09;
const uint8_t SF_10 = 0x0A;
const uint8_t SF_11 = 0x0B;
const uint8_t SF_12 = 0x0C;
*/


/*
//OTHER CONSTANTS:

const uint8_t HEADER_ON = 0;
const uint8_t HEADER_OFF = 1;
const uint8_t CRC_ON = 1;
const uint8_t CRC_OFF = 0;
const uint8_t LORA = 1;
const uint8_t FSK = 0;
const uint8_t BROADCAST_0 = 0x00;
const uint8_t MAX_LENGTH = 255;
const uint8_t MAX_PAYLOAD = 251;
const uint8_t MAX_LENGTH_FSK = 64;
const uint8_t MAX_PAYLOAD_FSK = 60;
const uint8_t ACK_LENGTH = 5;
const uint8_t OFFSET_PAYLOADLENGTH = 5;
const uint8_t OFFSET_RSSI = 137;
const uint8_t NOISE_FIGURE = 6.0;
const uint8_t NOISE_ABSOLUTE_ZERO = 174.0;
const uint16_t MAX_TIMEOUT = 8000;		//8000 msec = 8.0 sec
const uint16_t MAX_WAIT = 12000;		//12000 msec = 12.0 sec
const uint8_t MAX_RETRIES = 5;
const uint8_t CORRECT_PACKET = 0;
const uint8_t INCORRECT_PACKET = 1;
*/

//! Structure :
/*!
 */

/*
struct pack
{
	//! Structure Variable : Packet destination

	uint8_t dst;

	//! Structure Variable : Packet source

	uint8_t src;

	//! Structure Variable : Packet number

	uint8_t packnum;

	//! Structure Variable : Packet length

	uint8_t length;

	//! Structure Variable : Packet payload

	uint8_t data[MAX_PAYLOAD];

	//! Structure Variable : Retry number

	uint8_t retry;
};
*/

void WriteSXRegister(uint8_t address, uint8_t data);        // write data in a SX1272 register
uint8_t ReadSXRegister(uint8_t address);                    // read data which is in a SX1272 register
void GetMod(void);                                        // read operating mode
void InitModule(void);                                     // initialize the module (for TX or RX)
//void PrintSXRegContent(uint8_t address);                    // read SX1272 register and send its content to serial output
//void CheckConfiguration (void);                             // for debugging purpose: read configuration registers and send information to serial output

#endif	/* _SX1272_H */

