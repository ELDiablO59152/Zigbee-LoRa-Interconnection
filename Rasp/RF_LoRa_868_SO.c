/*
 * File:   RF_LoRa_868_SO.c
 * Author: Fabien AMELINCK
 *
 * Created on 04 June 2021
 */

#include "RF_LoRa_868_SO.h"

void ResetModule(void)
{
    set_port_direction(20, 0); // switch direction to output (low impedance)
    set_port_value(20, 1);     // pull output high
    usleep(5000);              // wait for 5 ms  (SX1272 datasheet: 100 us min)
    set_port_direction(20, 1); // switch direction to input (high impedance)
    usleep(50000);             // wait for 50 ms  (SX1272 datasheet: 5 ms min)
}

// void InitModule(void)
void InitModule(const uint32_t freq, const uint8_t bw, const uint8_t sf, const uint8_t cr, const uint8_t sync, const uint8_t preambule, const uint8_t pout, const uint8_t gain, const uint8_t timeout, const uint8_t hder, const uint8_t crc)
{
    WriteSXRegister(REG_FIFO, 0x00);

    /*
      WriteSXRegister(REG_FRF_MSB, 0xD8);			// channel 10, central freq = 865.20MHz
      WriteSXRegister(REG_FRF_MID, 0x4C);
      WriteSXRegister(REG_FRF_LSB, 0xCC);
    */
    /*
      WriteSXRegister(REG_FRF_MSB, 0xD9);			// channel 16, central freq = 868.00MHz
      WriteSXRegister(REG_FRF_MID, 0x00);
      WriteSXRegister(REG_FRF_LSB, 0x00);
    */
    WriteSXRegister(REG_FRF_MSB, freq >> 16);
    WriteSXRegister(REG_FRF_MID, (freq >> 8) & ~0xFF00);
    WriteSXRegister(REG_FRF_LSB, freq & ~0xFFFF00);

    // write register REG_PA_CONFIG to select pin PA-BOOST for power amplifier output (power limited to 20 dBm = 100 mW)
    #ifdef POUT
    uint8_t pout;
    pout = (POUT - 2) & 0x0F;                    // compute pout and keep 4 LSBs (POUT is defined in file SX1272.h)
    #endif
    WriteSXRegister(REG_PA_CONFIG, 0x80 | pout); // out=PA_BOOST

    WriteSXRegister(REG_PA_RAMP, 0x19); // low cons PLL TX&RX, 40us

    WriteSXRegister(REG_OCP, 0b00101011); // OCP enabled, 100mA

    // WriteSXRegister(REG_LNA, 0b00100011);			// max gain, BOOST on
    if (gain == 0)
        WriteSXRegister(REG_LNA, 0b00100011);
    else
        WriteSXRegister(REG_LNA, gain << 5 | 0b00000011);
    #if debug
    fprintf(stdout, "gain %x\n", gain << 5 | 0b00000011);
    #endif

    WriteSXRegister(REG_FIFO_ADDR_PTR, 0x00);     // pointer to access FIFO through SPI port (read or write)
    WriteSXRegister(REG_FIFO_TX_BASE_ADDR, 0x80); // top half of the FIFO
    WriteSXRegister(REG_FIFO_RX_BASE_ADDR, 0x00); // bottom half of the FIFO

    WriteSXRegister(REG_IRQ_FLAGS_MASK, 0x00); // activate all IRQs

    WriteSXRegister(REG_IRQ_FLAGS, 0xFF); // clear all IRQs

    // in Explicit Header mode, CRC enable or disable is not relevant in case of RX operation: everything depends on TX configuration
    // REG_MODEM_CONFIG1 (7:6 BW 5:3 CR 2 Header 1 CRC 0 Opti)
    // WriteSXRegister(REG_MODEM_CONFIG1, 0b10001000); 	//BW=500k, CR=4/5, explicit header, CRC disable, LDRO disabled
    // WriteSXRegister(REG_MODEM_CONFIG1, 0b10001010);	//BW=500k, CR=4/5, explicit header, CRC enable, LDRO disabled
    // WriteSXRegister(REG_MODEM_CONFIG1, 0b00100011);	//BW=125k, CR=4/8, explicit header, CRC enable, LDRO enabled (mandatory with SF=12 and BW=125 kHz)
    // WriteSXRegister(REG_MODEM_CONFIG1, 0b00001010);	//BW=125k, CR=4/5, explicit header, CRC enable, LDRO enabled (mandatory with SF=12 and BW=125 kHz)
    if ((sf == SF_11 || sf == SF_12) && bw == BW_125)
        WriteSXRegister(REG_MODEM_CONFIG1, bw << 6 | cr << 3 | hder << 2 | crc << 1 | 0x1);
    else
        WriteSXRegister(REG_MODEM_CONFIG1, bw << 6 | cr << 3 | hder << 2 | crc << 1 | 0x0);
    #if debug
    fprintf(stdout, "config1 %x\n", bw << 6 | cr << 3 | hder << 2 | crc << 1 | 0x0);
    #endif

    // REG_MODEM_CONFIG1 (7:4 SF 3 TxMode 2 AutoGain 1:0 RxTiout)
    // WriteSXRegister(REG_MODEM_CONFIG2, 0b11000111);	// SF=12, normal TX mode, AGC auto on, RX timeout MSB = 11
    // WriteSXRegister(REG_MODEM_CONFIG2, 0b11000101);	// SF=12, normal TX mode, AGC auto on, RX timeout MSB = 01
    ////WriteSXRegister(REG_MODEM_CONFIG2, 0b11000100); // SF=12, normal TX mode, AGC auto on, RX timeout MSB = 00
    // WriteSXRegister(REG_MODEM_CONFIG2, 0b01110111); 	// SF=7, normal TX mode, AGC auto on, RX timeout MSB = 00 1s timeout
    if (gain == 0)
        WriteSXRegister(REG_MODEM_CONFIG2, sf << 4 | (timeout & 0xFD) | 0b00000100);
    else
        WriteSXRegister(REG_MODEM_CONFIG2, sf << 4 | (timeout & 0xFD) | 0b00000000);
    #if debug
    fprintf(stdout, "config2 %x\n", sf << 4 | 0x00);
    #endif

    WriteSXRegister(REG_SYMB_TIMEOUT_LSB, 0xFF); // max timeout (used in mode Receive Single)
    // timeout = REG_SYMB_TIMEOUT x Tsymbol    where    Tsymbol = 2^SF / BW
    //                                                  Tsymbol = 2^12 / 125k = 32.7 ms
    // REG_SYMB_TIMEOUT = 0x2FF = 1023   =>   timeout = 33.5 seconds
    // REG_SYMB_TIMEOUT = 0x1FF = 511    =>   timeout = 16.7 seconds
    // REG_SYMB_TIMEOUT = 0x0FF = 255    =>   timeout = 8.3 seconds

    WriteSXRegister(REG_PREAMBLE_MSB_LORA, 0x00); // default value
    WriteSXRegister(REG_PREAMBLE_LSB_LORA, preambule);

    WriteSXRegister(REG_MAX_PAYLOAD_LENGTH, 0x80); // half the FIFO

    WriteSXRegister(REG_HOP_PERIOD, 0x00); // freq hopping disabled

    WriteSXRegister(REG_DETECT_OPTIMIZE, 0xC3); // required value for SF=7 to 12

    WriteSXRegister(REG_INVERT_IQ, 0x27); // default value, IQ not inverted

    WriteSXRegister(REG_DETECTION_THRESHOLD, 0x0A); // required value for SF=7 to 12

    // sync = 0x12;
    WriteSXRegister(REG_SYNC_WORD, sync); // default value
}

void AntennaTX(void)
{
    set_port_value(5, 0); // clear RX_SWITCH
    set_port_value(6, 0); // clear TX_SWITCH
    usleep(10000);
    set_port_value(6, 1); // set TX_SWITCH
}

void AntennaRX(void)
{
    set_port_value(5, 0); // clear RX_SWITCH
    set_port_value(6, 0); // clear TX_SWITCH
    usleep(10000);
    set_port_value(5, 1); // set RX_SWITCH
}
