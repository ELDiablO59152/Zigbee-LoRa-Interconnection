/*
 * File:   Receive.c
 * Author: Fabien AMELINCK
 *
 * Created on 13 April 2022
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>
#include "gpio_util.h"
#include "SX1272.h"
#include "RF_LoRa_868_SO.h"
#include "sendRecept.h"
#include "filecsv.h"

#define debug 1
#define useInit

//#define MY_ID ISEN_ID

int main(int argc, char *argv[]) {

    uint8_t TxBuffer[50]; // buffer containing data to write in SX1272 FIFO before transmission
    uint8_t RxBuffer[50]; // buffer containing data read from SX1272 FIFO after reception

    uint8_t NbBytesReceived; // length of the received payload
                            // (after reception, read from dedicated register REG_RX_NB_BYTES)
    //uint8_t PayloadLength;   // length of the transmitted payload
                            // (before transmission, must be stored in the dedicated register REG_PAYLOAD_LENGTH_LORA)

    uint8_t CRCError; // returned by functions WaitIncomingMessageRXContinuous and WaitIncomingMessageRXSingle
                    // 0 => no CRC error in the received message
                    // 1 => CRC error in the received message

    uint8_t TimeoutOccured = 0; // written by function WaitIncomingMessageRXSingle
                            // 0 => a message was received before end of timeout (no timeout occured)
                            // 1 => timeout occured before reception of any message

    if (init_spi()) return -1;

    // Configure the pin used for RESET of LoRa transceiver
    // here: physical pin n°38 (GPIO20)
    if (create_port(20)) {
        fprintf(stdout, "Bug in port openning, please retry");
        return -1;
    }
    set_port_direction(20, 1);

    // Configure the pin used for RX_SWITCH of LoRa transceiver
    // here: physical pin n°29 (GPIO5)
    create_port(5);
    set_port_direction(5, 0);
    set_port_value(5, 0);

    // Configure the pin used for TX_SWITCH of LoRa transceiver
    // here: physical pin n°31 (GPIO6)
    create_port(6);
    set_port_direction(6, 0);
    set_port_value(6, 0);

    // Configure the pin used for LED
    // here: physical pin n°40 (GPIO21)
    /*create_port(21);
    set_port_direction(21, 0);
    set_port_value(21, 0);*/

    #ifndef useInit
    ResetModule();

    if (ReadSXRegister(REG_VERSION) != 0x22) {
        fprintf(stdout, "Wrong module or not working !");
        return -1;
    }

    // put module in LoRa mode (see SX1272 datasheet page 107)
    WriteSXRegister(REG_OP_MODE, FSK_SLEEP_MODE);    // SLEEP mode required first to switch from FSK to LoRa
    WriteSXRegister(REG_OP_MODE, LORA_SLEEP_MODE);   // switch from FSK mode to LoRa mode
    WriteSXRegister(REG_OP_MODE, LORA_STANDBY_MODE); // STANDBY mode required for FIFO loading
    usleep(100000);                                  // delay since switching mode takes time

    // clearing buffers
    //	memset(inbuf, 0, sizeof inbuf);
    //	memset(outbuf, 0, sizeof outbuf);

    //InitModule(freq,      bw,     sf, cr, sync, preamble, pout, gain, rxtimeout, hder, crc);
    InitModule(CH_17_868, BW_500, SF_12, CR_5, 0x12, 0x08, 2, G1, 0x00, HEADER_ON, CRC_ON);
    #endif

    if (argc > 1) {
        #if debug
        fprintf(stdout, "args %d", argc);
        for (int i; i < argc; i++) {
            fprintf(stdout, " %s", argv[i]);
        }
        fprintf(stdout, "\n");
        #endif
        
        #ifndef MY_ID
        uint8_t MY_ID = (uint8_t) atoi(argv[1]);
        #endif

        uint8_t received = 0, loop = 0, maxLoop = 10;
        if (argc == 3) maxLoop = atoi(argv[2]);

        while (!received && loop < maxLoop) {
            if (argc == 3) CRCError = WaitIncomingMessageRXSingle(&TimeoutOccured);
            else CRCError = WaitIncomingMessageRXContinuous();

            if (TimeoutOccured) {
                #if debug
                //fprintf(stdout, "Pas de reponse\n");
                #endif
            } else if (CRCError) {
                #if debug
                fprintf(stdout, "CRC Error!\n");
                #endif
            } else {
                received = 1;
                clock_t t1 = clock();

                #if debug
                int8_t RSSI = 
                #endif
                LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived); // addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
                                                                // in order to update the values of their content
                #if debug
                fprintf(stdout, "RSSI = %d\n", RSSI);
                #endif

                if (RxBuffer[HEADER_0_POS] == HEADER_0
                && RxBuffer[HEADER_1_POS] == HEADER_1
                && RxBuffer[DEST_ID_POS] == MY_ID) {

                    TxBuffer[HEADER_0_POS] = HEADER_1;
                    TxBuffer[HEADER_1_POS] = HEADER_0;
                    TxBuffer[DEST_ID_POS] = RxBuffer[SOURCE_ID_POS];
                    TxBuffer[SOURCE_ID_POS] = MY_ID;
                    TxBuffer[COMMAND_POS] = ACK;

                    LoadTxFifoWithTxBuffer(TxBuffer, COMMAND_LONG); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                             // in order to read the values of their content and copy them in SX1272 registers
                    TransmitLoRaMessage();

                    fprintf(stdout, "%fms\n", (float)(clock()-t1)/CLOCKS_PER_SEC);
                    
                    if (RxBuffer[COMMAND_POS] == DATA) {
                        fprintf(stdout, "T%d,%d,%d,%d,%d\n", RxBuffer[SENSOR_ID_POS], RxBuffer[T_POS], RxBuffer[O_POS], RxBuffer[DEST_ID_POS], RxBuffer[SOURCE_ID_POS]);
                    } else if (RxBuffer[COMMAND_POS] == ACK_ZIGBEE) {
                        fprintf(stdout, "A%d,%d,%d,%d,%d\n", RxBuffer[SENSOR_ID_POS], RxBuffer[ACK_POS], RxBuffer[R_POS], RxBuffer[DEST_ID_POS], RxBuffer[SOURCE_ID_POS]);
                    } else if (RxBuffer[COMMAND_POS] == LED_ON) {
                        fprintf(stdout, "LED_ON\n");
                    } else if (RxBuffer[COMMAND_POS] == LED_OFF) {
                        fprintf(stdout, "LED_OFF\n");
                    }
                }
            }
            loop++;
            if (loop == maxLoop) {
                #if debug
                fprintf(stdout, "Pas de reponse\n");
                #endif
            }
        } // end of while
    } // end of if
    else fprintf(stdout, "Error nb args, usage : <prog> <source_id>\n");
    return 0;
} // end of main
