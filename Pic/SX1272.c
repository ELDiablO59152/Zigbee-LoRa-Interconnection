/*
 * File:   SX1272.c
 * Authors: BRS & JMC
 *
 * Created on 19 mai 2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xc.h>
#include "general.h"
#include "uart.h"
#include "spi.h"
#include "SX1272.h"
#include "RF_LoRa_868_SO.h"


void WriteSXRegister(uint8_t address, uint8_t data) {

    SPI_SS_LORA_LAT = SPI_SS_ENABLE;             // enable slave
    address = address | 0x80;               // MSB of address must be high for write operation
                                            // (see SX1272 datasheet page 76)
    SPITransfer(address);
    SPITransfer(data);
    SPI_SS_LORA_LAT = SPI_SS_DISABLE;            // disable slave
}

uint8_t ReadSXRegister(uint8_t address) {
    uint8_t RegValue;
    SPI_SS_LORA_LAT = SPI_SS_ENABLE;             // enable slave
    address = address & 0x7F;               // MSB of address must be low for read operation
                                            // (see SX1272 datasheet page 76)
    SPITransfer(address);                   // send register address
    RegValue = SPIReceive(0x00);            // send dummy data and receive register content
    SPI_SS_LORA_LAT = SPI_SS_DISABLE;            // disable slave
    return RegValue;
}

// read REG_OP_MODE register to check operating mode
// and send information to serial ouput
void GetMode (void){
    uint8_t reg, masked_reg;
    reg = ReadSXRegister(REG_OP_MODE);
    
    // for debugging: send reg value to terminal
        UARTWriteStr("REG_OP_MODE = 0x");
        UARTWriteByteHex(reg);
        UARTWriteStrLn(" ");
    
    masked_reg = reg & 0x80;        // to check bit n�7
    if (masked_reg)
        {
        // MSB of RegOpMode is 1, so mode = LoRa
        masked_reg = reg & 0x40;        // to check bit n�6
        if (!masked_reg) {
            UARTWriteStrLn("mode = LoRa");
        }
        else
            UARTWriteStrLn("mode = LoRa with FSK registers access");
        }
    else
        // MSB of RegOpMode is 0, so mode = FSK
        UARTWriteStrLn("mode = FSK");
            
    masked_reg = reg & 0x07;       // test bits 2-0 of RegOpMode
        switch (masked_reg){
        case 0x00:
            UARTWriteStrLn("sleep mode");
            break;
        case 0x01:
            UARTWriteStrLn("standby mode");
            break;
        case 0x02:
            UARTWriteStrLn("frequency synthesis TX");
            break;
        case 0x03:
            UARTWriteStrLn("TX mode");
            break;
        case 0x04:
            UARTWriteStrLn("frequency synthesis RX");
            break;
        case 0x05:
            UARTWriteStrLn("continuous receive mode");
            break;
        case 0x06:
            UARTWriteStrLn("single receive mode");
            break;
        case 0x07:
            UARTWriteStrLn("Channel Activity Detection");
            break;
        }
            
}

void InitModule(void){
  uint8_t pout;

  WriteSXRegister(REG_FIFO, 0x00);

  WriteSXRegister(REG_FRF_MSB, 0xD8);			// center freq = 866.40MHz
  WriteSXRegister(REG_FRF_MID, 0x99);
  WriteSXRegister(REG_FRF_LSB, 0x99);

  // select pin PA-BOOST for power amplifier output (power limited to 20 dBm = 100 mW)
  pout = (POUT - 2) & 0x0F;         			// compute pout and keep 4 LSBs (POUT is defined in RF_LoRa_868_SO.h)
  WriteSXRegister(REG_PA_CONFIG, 0x80 | pout); 		// out=PA_BOOST

  WriteSXRegister(REG_PA_RAMP, 0x19);			// low cons PLL TX&RX, 40us

  WriteSXRegister(REG_OCP, 0b00101011);			// OCP enabled, 100mA

  WriteSXRegister(REG_LNA, 0b00100011);			// max gain, BOOST on

  WriteSXRegister(REG_FIFO_ADDR_PTR, 0x00);		// pointer to access the FIFO through SPI port (read or write)
  WriteSXRegister(REG_FIFO_TX_BASE_ADDR, 0x80);		// top half of the FIFO for TX
  WriteSXRegister(REG_FIFO_RX_BASE_ADDR, 0x00);		// bottom half of the FIFO for RX

  WriteSXRegister(REG_IRQ_FLAGS_MASK, 0x00);		// enable all IRQs

  WriteSXRegister(REG_IRQ_FLAGS, 0xFF);			// clear all IRQs

  // in Explicit Header mode, CRC enable or disable is not relevant for RX operation: everything depends on TX configuration
  //WriteSXRegister(REG_MODEM_CONFIG1, 0b10001000);	// BW=500k, CR=4/5, explicit header, CRC disable, LDRO disabled
  WriteSXRegister(REG_MODEM_CONFIG1, 0b10001010);	// BW=500k, CR=4/5, explicit header, CRC enable, LDRO disabled
 // WriteSXRegister(REG_MODEM_CONFIG1, 0b00100011);	// BW=125k, CR=4/8, explicit header, CRC enable, LDRO enabled (mandatory with SF=12 and BW=125kHz)

  //WriteSXRegister(REG_MODEM_CONFIG2, 0b11000111);	// SF=12, normal TX mode, AGC auto on, RX timeout MSB = 11
  WriteSXRegister(REG_MODEM_CONFIG2, 0b01110010);	// SF=7, normal TX mode, AGC auto on, RX timeout MSB = 10
  //WriteSXRegister(REG_MODEM_CONFIG2, 0b11000101);	// SF=12, normal TX mode, AGC auto on, RX timeout MSB = 01
  //WriteSXRegister(REG_MODEM_CONFIG2, 0b11000100);	// SF=12, normal TX mode, AGC auto on, RX timeout MSB = 00

  WriteSXRegister(REG_SYMB_TIMEOUT_LSB, 0xFF);		// max timeout (used in mode RECEIVE SINGLE)
  // timeout = REG_SYMB_TIMEOUT x Tsymbol	where	Tsymbol = 2^SF / BW
  //						here	Tsymbol = 2^12 / 125k = 32.7 ms
  // REG_SYMB_TIMEOUT = 0x2FF = 1023	=>	timeout = 33.5 seconds
  // REG_SYMB_TIMEOUT = 0x1FF = 511	=>	timeout = 16.7 seconds
  // REG_SYMB_TIMEOUT = 0x0FF = 255	=>	timeout = 8.3 seconds

  WriteSXRegister(REG_PREAMBLE_MSB_LORA, 0x00);		// default value
  WriteSXRegister(REG_PREAMBLE_LSB_LORA, 0x08);

  WriteSXRegister(REG_MAX_PAYLOAD_LENGTH, 0x80);	// half the FIFO

  WriteSXRegister(REG_HOP_PERIOD, 0x00);		// frequency hopping disabled

  WriteSXRegister(REG_DETECT_OPTIMIZE, 0xC3);		// required value for SF=12

  WriteSXRegister(REG_INVERT_IQ, 0x27);			// default value, IQ not inverted

  WriteSXRegister(REG_DETECTION_THRESHOLD, 0x0A);	// required value for SF=12

  WriteSXRegister(REG_SYNC_WORD, 0x12);			// default value
}

// read a register and send its content to serial output
/*void PrintSXRegContent(uint8_t address){
    uint8_t RegValue;
    SPI_SS_LAT = SPI_SS_ENABLE;             // enable slave
    address = address & 0x7F;               // MSB of address must be low for read operation
                                            // (see SX1272 datasheet page 76)
    SPITransfer(address);                   // send register address
    RegValue = SPIReceive(0x00);            // send dummy data and receive register content
    SPI_SS_LAT = SPI_SS_DISABLE;            // disable slave
    UARTWriteByteHex(RegValue);             // send to serial output received register content
}*/

// check the content of configuration registers
/*void CheckConfiguration (void){
    
    UARTWriteStrLn(" ");
    UARTWriteStrLn("configuration registers content:");
    
    UARTWriteStr("REG_FRF_MSB = 0x");
    PrintSXRegContent(REG_FRF_MSB);
    UARTWriteStr("REG_FRF_MID = 0x");
    PrintSXRegContent(REG_FRF_MID);
    UARTWriteStr("REG_FRF_LSB = 0x");
    PrintSXRegContent(REG_FRF_LSB);
    UARTWriteStr("REG_PA_CONFIG = 0x");
    PrintSXRegContent(REG_PA_CONFIG);
    UARTWriteStr("REG_PA_RAMP = 0x");
    PrintSXRegContent(REG_PA_RAMP);
    UARTWriteStr("REG_OCP = 0x");
    PrintSXRegContent(REG_OCP);
    UARTWriteStr("REG_LNA = 0x");
    PrintSXRegContent(REG_LNA);
    UARTWriteStr("REG_FIFO_ADDR_PTR = 0x");
    PrintSXRegContent(REG_FIFO_ADDR_PTR);
    UARTWriteStr("REG_FIFO_TX_BASE_ADDR = 0x");
    PrintSXRegContent(REG_FIFO_TX_BASE_ADDR);
    UARTWriteStr("REG_FIFO_RX_BASE_ADDR = 0x");
    PrintSXRegContent(REG_FIFO_RX_BASE_ADDR);
    UARTWriteStr("REG_IRQ_FLAGS_MASK = 0x");
    PrintSXRegContent(REG_IRQ_FLAGS_MASK);
    UARTWriteStr("REG_IRQ_FLAGS = 0x");
    PrintSXRegContent(REG_IRQ_FLAGS);
    UARTWriteStr("REG_MODEM_CONFIG1 = 0x");
    PrintSXRegContent(REG_MODEM_CONFIG1);
    UARTWriteStr("REG_MODEM_CONFIG2 = 0x");
    PrintSXRegContent(REG_MODEM_CONFIG2);
    UARTWriteStr("REG_SYMB_TIMEOUT_LSB = 0x");
    PrintSXRegContent(REG_SYMB_TIMEOUT_LSB);
    UARTWriteStr("REG_PREAMBLE_MSB_LORA = 0x");
    PrintSXRegContent(REG_PREAMBLE_MSB_LORA);
    UARTWriteStr("REG_PREAMBLE_LSB_LORA = 0x");
    PrintSXRegContent(REG_PREAMBLE_LSB_LORA);
    UARTWriteStr("REG_MAX_PAYLOAD_LENGTH = 0x");
    PrintSXRegContent(REG_MAX_PAYLOAD_LENGTH);
    UARTWriteStr("REG_HOP_PERIOD = 0x");
    PrintSXRegContent(REG_HOP_PERIOD);
    UARTWriteStr("REG_DETECT_OPTIMIZE = 0x");
    PrintSXRegContent(REG_DETECT_OPTIMIZE);
    UARTWriteStr("REG_INVERT_IQ = 0x");
    PrintSXRegContent(REG_INVERT_IQ);
    UARTWriteStr("REG_DETECTION_THRESHOLD = 0x");
    PrintSXRegContent(REG_DETECTION_THRESHOLD);
    UARTWriteStr("REG_SYNC_WORD = 0x");
    PrintSXRegContent(REG_SYNC_WORD);
}*/