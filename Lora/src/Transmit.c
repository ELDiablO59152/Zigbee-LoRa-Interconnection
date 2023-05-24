/*
 * File:   Transmit.c
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
#include "ascon/crypto_aead.h"
#include "ascon/api.h"
#include "ascon/ascon_utils.h"

#define CRYPTO_BYTES 64
#define debug 1
#define useInit

#define MaxNodesInNetwork 5

//#define MY_ID ISEN_ID

uint8_t NodeID;
int8_t NodeMap[MaxNodesInNetwork][2]; // to store which nodes are present in the network, and their payload length
                                    // (written during the discovery process)
                                    // 1 line per node
                                    // 1st column = 1 if node is present
                                    // 2nd column = RSSI
                                    // 3rd column = number of Zn
                                    // 4th column = number of sensors

void ClearNodeMap(void);

int main(int argc, char *argv[]) {

    uint8_t NodeData[255]; // data sent by a node

    uint8_t TxBuffer[50]; // buffer containing data to write in SX1272 FIFO before transmission
    uint8_t RxBuffer[50]; // buffer containing data read from SX1272 FIFO after reception

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
    int8_t RSSI;

    clock_t t1 = clock(), t2;

    //Variables for ascon

    //Size of the message
    //Size of the cipher
    unsigned long long clen;

    //Buffer containing the message to encrypt
    unsigned char plaintext[CRYPTO_BYTES];
    //Buffer containing the message that has been encrypted
    unsigned char cipher[CRYPTO_BYTES]; 
    //Buffer containing the public nonce
    unsigned char npub[CRYPTO_NPUBBYTES]="";
    //Buffer containing additional values
    unsigned char ad[CRYPTO_ABYTES]="";
    //Buffer containing the secured nonce?
    unsigned char nsec[CRYPTO_ABYTES]="";
    //Buffer containing the secured key used to encrypt
    unsigned char key[CRYPTO_KEYBYTES];

    //Char table holding the key that we want to be stored in memory as hexa , then every 2 letter/number will be transformed in an hexa number stored in a single byte
    char keyhex[2*CRYPTO_KEYBYTES+1]="0123456789ABCDEF0123456789ABCDEF";
    //Char table holding the nonce that we want to be stored in memory as hexa , then every 2 letter/number will be transformed in an hexa number stored in a single byte
    char nonce[2*CRYPTO_NPUBBYTES+1]="000000000000111111111111";


    //int ret = crypto_aead_encrypt(cipher,&clen,plaintext,strlen(plaintext),ad,strlen(ad),nsec,npub,key);
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
    if (set_port_value(6, 0)) {
        fprintf(stdout, "Bug in port openning, please retry");
        return -1;
    }

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

    //InitModule(freq,      bw,    sf,   cr,  sync, preamble, pout, gain, rxtimeout, hder,     crc);
    InitModule(CH_17_868, BW_500, SF_7, CR_5, 0x12, 0x08,      2,    G1,   LONGT, HEADER_ON, CRC_ON);
    #endif

    //DeleteDataFile();
    //CreateDataFile();
    ClearNodeMap();

    if (argc > 1) {
        #if debug
        fprintf(stdout, "args %d", argc);  // Printing all args passed during call
        for (uint8_t i = 0; i < argc; i++) {
            fprintf(stdout, " %s", argv[i]);
        }
        fprintf(stdout, "\n");
        #endif

        #ifndef MY_ID
        uint8_t MY_ID = (uint8_t) atoi(argv[3]);
        #endif

        TxBuffer[HEADER_0_POS] = HEADER_0;
        TxBuffer[HEADER_1_POS] = HEADER_1;
        TxBuffer[SOURCE_ID_POS] = MY_ID;
        PayloadLength = COMMAND_LONG;

        if (!strcmp(argv[1], "LED_ON")) {
            if (argc != 4) {
                fprintf(stdout, "Error nb args, usage : LED_ON <destination_id> <source_id>");
                return -1;
            }
            TxBuffer[DEST_ID_POS] = (uint8_t) atoi(argv[2]);
            TxBuffer[COMMAND_POS] = LED_ON;
        } else if (!strcmp(argv[1], "LED_OFF")) {
            if (argc != 4) {
                fprintf(stdout, "Error nb args, usage : LED_OFF <destination_id> <source_id>");
                return -1;
            }
            TxBuffer[DEST_ID_POS] = (uint8_t) atoi(argv[2]);
            TxBuffer[COMMAND_POS] = LED_OFF;
        } else if (!strcmp(argv[1], "D")) { // Discover
            if (argc != 4) {
                fprintf(stdout, "Error nb args, usage : D <destination_id> <source_id>");
                return -1;
            }
            TxBuffer[DEST_ID_POS] = (uint8_t) atoi(argv[2]);
            TxBuffer[COMMAND_POS] = DISCOVER;
        } else if (!strcmp(argv[1], "P")) { // Ping
            if (argc != 4) {
                fprintf(stdout, "Error nb args, usage : P <destination_id> <source_id>");
                return -1;
            }
            TxBuffer[DEST_ID_POS] = (uint8_t) atoi(argv[2]);
            TxBuffer[COMMAND_POS] = PING;
        } else if (!strcmp(argv[1], "T")) { // Transmit
            if (argc != 7) {
                fprintf(stdout, "Error nb args, usage : T <destination_id> <source_id> <sensor_id> <T> <O>");
                return -1;
            }
            TxBuffer[DEST_ID_POS] = (uint8_t) atoi(argv[2]);
            TxBuffer[COMMAND_POS] = DATA;
            TxBuffer[SENSOR_ID_POS] = (uint8_t) atoi(argv[4]);
            TxBuffer[T_POS] = (uint8_t) atoi(argv[5]);
            TxBuffer[O_POS] = (uint8_t) atoi(argv[6]);
            PayloadLength = TRANSMIT_LONG;
        } else if (!strcmp(argv[1], "A")) { // Acknowledge
            if (argc != 7) {
                fprintf(stdout, "Error nb args, usage : A <destination_id> <source_id> <sensor_id> <ACK> <R>");
                return -1;
            }
            TxBuffer[DEST_ID_POS] = (uint8_t) atoi(argv[2]);
            TxBuffer[COMMAND_POS] = ACK_ZIGBEE;
            TxBuffer[SENSOR_ID_POS] = (uint8_t) atoi(argv[4]);
            TxBuffer[ACK_POS] = (uint8_t) atoi(argv[5]);
            TxBuffer[R_POS] = (uint8_t) atoi(argv[6]);
            PayloadLength = TRANSMIT_LONG;
        } else {
            fprintf(stdout, "Error, command does not exist");
            return -1;
        }

        //encrypt TxBuffer from id 5 to PayloadLength to have a message encrypted

        //Store values of TxBuffer from id 5 to PayloadLength inside of plaintext
        printf("Plaintext Bytes: ");
        for (int i=0;i<COMMAND_LONG-1;i++)
        {
            plaintext[i]=TxBuffer[i];
            printf("%02x ",plaintext[i]);
        }
        for (int i=0;i<DATA_LONG;i++)
        {
            plaintext[COMMAND_LONG-1+i]=TxBuffer[COMMAND_LONG+i];
            printf("%02x ",plaintext[CLEN_POS+i]);
        }
        printf("\n");

        hextobyte(keyhex,key);

        //encrypt plaintext and store it in cipher
        crypto_aead_encrypt(cipher,&clen,plaintext,strlen((const char*)plaintext),ad,strlen((const char*)ad),nsec,npub,key);
        TxBuffer[CLEN_POS]=clen;
        //Store values of cipher in the TxBuffer
        printf("Encrypted Bytes: ");
        for (int i=0;i<(int)clen;i++)
        {
            TxBuffer[COMMAND_LONG+i]=cipher[i];
            printf("%02x ",TxBuffer[COMMAND_LONG+i]);
        }
        printf("\n");
        LoadTxFifoWithTxBuffer(TxBuffer,COMMAND_LONG+clen); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                             // in order to read the values of their content and copy them in SX1272 registers
        TransmitLoRaMessage();

        fprintf(stdout, "Time of transmission = %f\n", (float)(clock()-t1)/CLOCKS_PER_SEC);
        t2 = clock();

        CRCError = WaitIncomingMessageRXSingle(&TimeoutOccured);
        if (TimeoutOccured) {
            #if debug
            fprintf(stdout, "No answer, retry\n");
            #endif
            WaitIncomingMessageRXSingle(&TimeoutOccured);
        }

        if (TimeoutOccured) {
            #if debug
            fprintf(stdout, "No answer\n");
            #endif
        }
        else if (CRCError) {
            #if debug
            fprintf(stdout, "CRC Error!\n");
            #endif
        }
        else {
            RSSI = LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived); // addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
                                                                // in order to update the values of their content
            #if debug
            fprintf(stdout, "RSSI = %d\n", RSSI);
            #endif

            if (RxBuffer[HEADER_0_POS] == HEADER_1
            && RxBuffer[HEADER_1_POS] == HEADER_0
            && RxBuffer[DEST_ID_POS] == MY_ID
            && RxBuffer[SOURCE_ID_POS] == TxBuffer[DEST_ID_POS]
            && RxBuffer[COMMAND_POS] == ACK) {
                fprintf(stdout, "Confirmation ");
                if (!strcmp(argv[1], "LED_ON")) fprintf(stdout, "LED switched on\n");           // Mother node turned his led up
                else if (!strcmp(argv[1], "LED_OFF")) fprintf(stdout, "LED switched off\n");    // Mother node turned his led off
                else if (!strcmp(argv[1], "D")) fprintf(stdout, "Mother node is active\n");     // Discover successful
                else if (!strcmp(argv[1], "P")) fprintf(stdout, "Mother node pong\n");          // Pong from mother node
                else if (!strcmp(argv[1], "T")) fprintf(stdout, "Mother node ACK\n");           // ACK from Mother node
                else if (!strcmp(argv[1], "A")) fprintf(stdout, "Zigbee ACK\n");                // ACK from a ZigBee sensor
                else fprintf(stdout, "Incorrect answer\n");

                for (uint8_t i = 0; i < NbBytesReceived - 4; i++) NodeData[i] = RxBuffer[i + 4];

                //NodeMap[RxBuffer[SOURCE_ID_POS]][0] = 1;
                //NodeMap[RxBuffer[SOURCE_ID_POS]][1] = RSSI;

                WriteDataInFile(&RxBuffer[SOURCE_ID_POS], &NbBytesReceived, NodeData, &RSSI);
            } else fprintf(stdout, "Incorrect answer\n");
        }

        fprintf(stdout, "Temps de reception = %f\nTemps total = %f\n", (float)(clock()-t2)/CLOCKS_PER_SEC, (float)(clock()-t1)/CLOCKS_PER_SEC);

    }             // end if
    else {
        fprintf(stdout, "Ping test from HEI to ISEN");
        // PayloadLength = 5;
        TxBuffer[HEADER_0_POS] = HEADER_0;
        TxBuffer[HEADER_1_POS] = HEADER_1;
        TxBuffer[DEST_ID_POS] = ISEN_ID;
        TxBuffer[SOURCE_ID_POS] = HEI_ID;
        TxBuffer[COMMAND_POS] = PING;

        LoadTxFifoWithTxBuffer(TxBuffer, COMMAND_LONG); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                                        // in order to read the values of their content and copy them in SX1272 registers
        TransmitLoRaMessage();

        WaitIncomingMessageRXSingle(&TimeoutOccured);

        if (TimeoutOccured) {
            #if debug
            fprintf(stdout, "Pas de reponse\n");
            #endif
        }
        else {
            RSSI = LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived); // addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
                                                                            // in order to update the values of their content
            #if debug
            fprintf(stdout, "RSSI = %d\n", RSSI);
            #endif
            if (RxBuffer[HEADER_0_POS] == HEADER_1
            && RxBuffer[HEADER_1_POS] == HEADER_0
            && RxBuffer[DEST_ID_POS] == HEI_ID
            && RxBuffer[SOURCE_ID_POS] == TxBuffer[DEST_ID_POS]
            && RxBuffer[COMMAND_POS] == ACK) {
                #if debug
                fprintf(stdout, "Confirmation de Decouverte\n");
                #endif

                for (uint8_t i = 0; i < NbBytesReceived - 4; i++) NodeData[i] = RxBuffer[i + 4];
                // NbBytesReceived++;
                // NodeData[NbBytesReceived - 1] = RSSI;

                NodeMap[RxBuffer[SOURCE_ID_POS]][0] = 1;
                NodeMap[RxBuffer[SOURCE_ID_POS]][1] = RSSI;

                WriteDataInFile(&RxBuffer[SOURCE_ID_POS], &NbBytesReceived, NodeData, &RSSI);
            } else {
                #if debug
                fprintf(stdout, "Reponse incorrecte\n");
                #endif
            }
        }
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
