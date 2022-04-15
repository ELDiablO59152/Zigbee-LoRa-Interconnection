/*
 * File:   SX1272.c
 * Authors: BRS & JMC
 *
 * Created on 19 mai 2017
 */

#include <stdint.h>
#include "SX1272.h"
#include "spi.h"

// function used to write in SX1272 registers
void WriteSXRegister(uint8_t address,uint8_t data)
{
	address = address | 0x80;					// set MSB (required to write SX1272 registers)
    outbuf[0] = address;
    outbuf[1] = data;
    
	xfer[0].rx_buf = (unsigned long) inbuf;	
	xfer[0].tx_buf = (unsigned long)outbuf;
	xfer[0].len = 2; 							// number of bytes to send
    
    if(ioctl(fd_spi, SPI_IOC_MESSAGE(1), xfer) < 0) {
        perror("SPI_IOC_MESSAGE");
    }
}

// function used to read the content of SX1272 registers
uint8_t ReadSXRegister(uint8_t address)
{
    address = address & 0x7F;					// clear MSB (required to read SX1272 registers)
    //usleep(100000);							// ligne commentée par JMC
    
    outbuf[0] = address;
    outbuf[1] = 0x00;							// dummy byte sent to receive data
    
	xfer[0].rx_buf = (unsigned long) inbuf;	
	xfer[0].tx_buf = (unsigned long)outbuf;
	xfer[0].len = 2; 							// number of bytes to send
    
    if(ioctl(fd_spi, SPI_IOC_MESSAGE(1), xfer) < 0) {
        perror("SPI_IOC_MESSAGE");
    }
    //fprintf(stdout, "rx0 = %d\n", inbuf[0]);	// may be useful for debug
    //fprintf(stdout, "rx1 = %d\n", inbuf[1]);
    return inbuf[1];
 }


// read REG_OP_MODE register to check operating mode
// and send information to serial ouput
/*void GetMode (void){
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
            afficher_string("LoRa mode ");
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
*/