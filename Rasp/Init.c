#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include "gpio_util.h"
#include "SX1272.h"
#include "RF_LoRa_868_SO.h"
#include "sendRecept.h"

#define debug True

int main(int argc, char *argv[])
{

    if (init_spi())
        return -1;

    // Configure the pin used for RESET of LoRa transceiver
    // here: physical pin n�38 (GPIO20)
    create_port(20);
    set_port_direction(20, 1);

    // Configure the pin used for RX_SWITCH of LoRa transceiver
    // here: physical pin n�29 (GPIO5)
    create_port(5);
    set_port_direction(5, 0);
    set_port_value(5, 0);

    // Configure the pin used for TX_SWITCH of LoRa transceiver
    // here: physical pin n�31 (GPIO6)
    create_port(6);
    set_port_direction(6, 0);
    set_port_value(6, 0);

    // Configure the pin used for LED
    // here: physical pin n�40 (GPIO21)
    create_port(21);
    set_port_direction(21, 0);
    set_port_value(21, 0);

    ResetModule();

    // put module in LoRa mode (see SX1272 datasheet page 107)
    WriteSXRegister(REG_OP_MODE, FSK_SLEEP_MODE);    // SLEEP mode required first to switch from FSK to LoRa
    WriteSXRegister(REG_OP_MODE, LORA_SLEEP_MODE);   // switch from FSK mode to LoRa mode
    WriteSXRegister(REG_OP_MODE, LORA_STANDBY_MODE); // STANDBY mode required for FIFO loading
    usleep(100000);                                  // delay since switching mode takes time

    // clearing buffers
    //	memset(inbuf, 0, sizeof inbuf);
    //	memset(outbuf, 0, sizeof outbuf);

    InitModule(CH_17_868, BW_500, SF_12, CR_5, 0x12, 1, HEADER_ON, CRC_ON);

    return 0;
} // end main