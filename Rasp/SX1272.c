/*
 * File:   SX1272.c
 * Authors: BRS & JMC
 *
 * Created on 19 mai 2017
 */

#include <fcntl.h>            //open&perror
#include <stdio.h>            //FILE
#include <stdlib.h>           //EXIT
#include <stdint.h>           //uint
#include <linux/spi/spidev.h> // library used to control SPI peripheral
#include <sys/ioctl.h>
#include "SX1272.h"

char inbuf[10];  // data received during SPI transfer
char outbuf[10]; // data sent during SPI transfer

struct spi_ioc_transfer xfer[1];
int fd_spi;
uint8_t mode;      // SPI mode (could be 0, 1, 2 or 3)
uint32_t speed;    // SPI clock frequency
uint8_t lsb_first; // SPI bits order
uint8_t bits;      // SPI words width (8 bits per word)

int init_spi(void)
{

    // Open the file for SPI control
    // here SPI peripheral n°0 is used
    // SCK = pin n°23 (GPIO11)
    // MOSI = pin n°19 (GPIO10)
    // MISO = pin n°21 (GPIO9)
    // nCS = pin n°24 (GPIO8)
    fd_spi = open("/dev/spidev0.0", O_RDWR);
    if (fd_spi < 0)
    {
        perror("/dev/spidev0.0");
        exit(EXIT_FAILURE);
    }

    // setting mode : 0, 1, 2 or 3 (depends on the desired phase and/or polarity of the clock)
    mode = 0;
    ioctl(fd_spi, SPI_IOC_WR_MODE, &mode);

    // getting some parameters
    if (ioctl(fd_spi, SPI_IOC_RD_MODE, &mode) < 0)
    {
        perror("SPI rd_mode");
        return -1;
    }
    if (ioctl(fd_spi, SPI_IOC_RD_LSB_FIRST, &lsb_first) < 0)
    {
        perror("SPI rd_lsb_fist");
        return -1;
    }

    // setting maximum transfer speed, the effective speed will probably be different
    speed = 15000000; // 15MHz asked

    // setting bits per word
    bits = 8;

    xfer[0].cs_change = 0;        /* Keep CS activated if = 1 */
    xfer[0].delay_usecs = 0;      // delay in us
    xfer[0].speed_hz = speed;     // speed
    xfer[0].bits_per_word = bits; // bits per word 8

    return 0;
}

// function used to write in SX1272 registers
void WriteSXRegister(uint8_t address, uint8_t data)
{
    address = address | 0x80; // set MSB (required to write SX1272 registers)
    outbuf[0] = address;
    outbuf[1] = data;

    xfer[0].rx_buf = (unsigned long)inbuf;
    xfer[0].tx_buf = (unsigned long)outbuf;
    xfer[0].len = 2; // number of bytes to send

    if (ioctl(fd_spi, SPI_IOC_MESSAGE(1), xfer) < 0)
    {
        perror("SPI_IOC_MESSAGE");
    }
}

// function used to read the content of SX1272 registers
uint8_t ReadSXRegister(uint8_t address)
{
    address = address & 0x7F; // clear MSB (required to read SX1272 registers)
    // usleep(100000);							// ligne commentée par JMC

    outbuf[0] = address;
    outbuf[1] = 0x00; // dummy byte sent to receive data

    xfer[0].rx_buf = (unsigned long)inbuf;
    xfer[0].tx_buf = (unsigned long)outbuf;
    xfer[0].len = 2; // number of bytes to send

    if (ioctl(fd_spi, SPI_IOC_MESSAGE(1), xfer) < 0)
    {
        perror("SPI_IOC_MESSAGE");
    }
    // fprintf(stdout, "rx0 = %d\n", inbuf[0]);	// may be useful for debug
    // fprintf(stdout, "rx1 = %d\n", inbuf[1]);
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