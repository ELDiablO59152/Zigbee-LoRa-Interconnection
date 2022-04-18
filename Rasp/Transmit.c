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
#include "filecsv.h"

#define debug True

#define MY_ID ISEN_ID

#define MaxNodesInNetwork 5

uint8_t NodeData[255]; // data sent by a node

uint8_t TxBuffer[50]; // buffer containing data to write in SX1272 FIFO before transmission
uint8_t RxBuffer[50]; // buffer containing data read from SX1272 FIFO after reception

uint8_t NodeID;
int8_t NodeMap[MaxNodesInNetwork][2]; // to store which nodes are present in the network, and their payload length
                                      // (written during the discovery process)
                                      // 1 line per node
                                      // 1st column = 1 if node is present
                                      // 2nd column = RSSI
                                      // 3rd column = number of Zn
                                      // 4th column = number of sensors

uint8_t NbBytesReceived; // length of the received payload
                         // (after reception, read from dedicated register REG_RX_NB_BYTES)
uint8_t PayloadLength;   // length of the transmitted payload
                         // (before transmission, must be stored in the dedicated register REG_PAYLOAD_LENGTH_LORA)

uint8_t CRCError; // returned by functions WaitIncomingMessageRXContinuous and WaitIncomingMessageRXSingle
                  // 0 => no CRC error in the received message
                  // 1 => CRC error in the received message

uint8_t TimeoutOccured; // written by function WaitIncomingMessageRXSingle
                        // 0 => a message was received before end of timeout (no timeout occured)
                        // 1 => timeout occured before reception of any message

void ClearNodeMap(void);

int main(int argc, char *argv[]) {

    if (init_spi()) return -1;

    // Configure the pin used for RESET of LoRa transceiver
    // here: physical pin n°38 (GPIO20)
    create_port(20);
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

    if (argc > 1) {
        fprintf(stdout, "args %d", argc);
        for (int i; i < argc; i++) {
            fprintf(stdout, " %s", argv[i]);
        }

        TxBuffer[HEADER_0_POS] = HEADER_0;
        TxBuffer[HEADER_1_POS] = HEADER_1;
        TxBuffer[DEST_ID_POS] = ISEN_ID;
        TxBuffer[SOURCE_ID_POS] = MY_ID;

        if (!strcmp(argv[1], "LED_ON")) TxBuffer[COMMAND_POS] = LED_ON;
        else if (!strcmp(argv[1], "LED_OFF")) TxBuffer[COMMAND_POS] = LED_OFF;
        else return 0;

        LoadTxFifoWithTxBuffer(TxBuffer, 5); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                             // in order to read the values of their content and copy them in SX1272 registers
        TransmitLoRaMessage();

        WaitIncomingMessageRXSingle(&TimeoutOccured);

        if (TimeoutOccured) fprintf(stdout, "Pas de reponse\n");
        else {
            LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived); // addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
                                                                // in order to update the values of their content
            if (RxBuffer[HEADER_0_POS] == HEADER_1 
            && RxBuffer[HEADER_1_POS] == HEADER_0
            && RxBuffer[DEST_ID_POS] == MY_ID
            && RxBuffer[SOURCE_ID_POS] == TxBuffer[DEST_ID_POS]
            && RxBuffer[COMMAND_POS] == ACK) {
                fprintf(stdout, "Confirmation ");
                if (!strcmp(argv[1], "LED_ON")) fprintf(stdout, "LED switched on\n");
                else if (!strcmp(argv[1], "LED_OFF")) fprintf(stdout, "LED switched off\n");
            } else fprintf(stdout, "Reponse incorrecte\n");
        }

        return 0; // exit prog
    }             // end if

    //DeleteDataFile();
    //CreateDataFile();

    ClearNodeMap();

    // PayloadLength = 5;
    TxBuffer[HEADER_0_POS] = HEADER_0;
    TxBuffer[HEADER_1_POS] = HEADER_1;
    TxBuffer[DEST_ID_POS] = ISEN_ID;
    TxBuffer[SOURCE_ID_POS] = HEI_ID;
    TxBuffer[COMMAND_POS] = PING;
    // TxBuffer[DATA_LONG_POS] = DATA_LONG;

    LoadTxFifoWithTxBuffer(TxBuffer, DISCOVER_LONG); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                                     // in order to read the values of their content and copy them in SX1272 registers
    TransmitLoRaMessage();

    WaitIncomingMessageRXSingle(&TimeoutOccured);

    if (TimeoutOccured) fprintf(stdout, "Pas de reponse\n");
    else {
        int8_t RSSI = LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived); // addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
                                                                          // in order to update the values of their content
        if (RxBuffer[HEADER_0_POS] == HEADER_1
        && RxBuffer[HEADER_1_POS] == HEADER_0
        && RxBuffer[DEST_ID_POS] == HEI_ID
        && RxBuffer[COMMAND_POS] == ACK) {
            fprintf(stdout, "Confirmation de Decouverte\n");

            for (uint8_t i = 0; i < NbBytesReceived - 4; i++) NodeData[i] = RxBuffer[i + 4];
            // NbBytesReceived++;
            // NodeData[NbBytesReceived - 1] = RSSI;

            NodeMap[RxBuffer[SOURCE_ID_POS]][0] = 1;
            NodeMap[RxBuffer[SOURCE_ID_POS]][1] = RSSI;

            //WriteDataInFile(&RxBuffer[SOURCE_ID_POS], &NbBytesReceived, NodeData, &RSSI);
        } else fprintf(stdout, "Reponse incorrecte\n");
    }

    return 0;
} // end of main

void ClearNodeMap(void)
{
    for (NodeID = 0; NodeID < MaxNodesInNetwork; NodeID++)
    {
        NodeMap[NodeID][0] = 0;
        NodeMap[NodeID][1] = 0;
        NodeMap[NodeID][2] = 0;
    }
}