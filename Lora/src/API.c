/*
 * File:   API.c
 * Author: Fabien AMELINCK
 *
 * Created on 28 February 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>
#include <poll.h>
#include "gpio_util.h"
#include "SX1272.h"
#include "RF_LoRa_868_SO.h"
#include "sendRecept.h"
#include "ascon/crypto_aead.h"
#include "ascon/api.h"
#include "ascon/ascon_utils.h"

#define debug 1
//#define useInit
#define CRYPTO_BYTES 64

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

    uint8_t TimeoutOccured = 0; // written by function WaitIncomingMessageRXSingle
                            // 0 => a message was received before end of timeout (no timeout occured)
                            // 1 => timeout occured before reception of any message
    int8_t RSSI; // store the signal strength

    clock_t t1, t2; // time delta

    //Variables for ascon

    //Size of the message
    uint8_t mlen;
    //Size of the cipher
    uint8_t clen;

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
    //char nonce[2*CRYPTO_NPUBBYTES+1]="000000000000111111111111";

    if (init_spi()) return -1;

    uint8_t error = 0;
    do {
        // Configure the pin used for RESET of LoRa transceiver
        // here: physical pin n°38 (GPIO20)
        create_port(20);
        usleep(10000);
        set_port_direction(20, 1);  // switch direction to input (high impedance)

        // Configure the pin used for RX_SWITCH of LoRa transceiver
        // here: physical pin n°29 (GPIO5)
        create_port(5);
        usleep(10000);
        set_port_direction(5, 0);
        set_port_value(5, 0);

        // Configure the pin used for TX_SWITCH of LoRa transceiver
        // here: physical pin n°31 (GPIO6)
        create_port(6);
        usleep(10000);
        set_port_direction(6, 0);
        if (set_port_value(6, 0)) {
            fprintf(stdout, "Bug in port openning, please retry\n");
            error = 1;
        } else error = 0;
    } while (error);

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

    setbuf(stdout, NULL);

    if (argc > 1) {
        #if debug
        fprintf(stdout, "args %d", argc);  // Printing all args passed during call
        for (uint8_t i = 0; i < argc; i++) {
            fprintf(stdout, " %s", argv[i]);
        }
        fprintf(stdout, "\n");
        #endif

        #ifndef MY_ID
        uint8_t MY_ID = (uint8_t) atoi(argv[1]);
        #endif

        uint8_t received = 0, loop = 0, maxLoop = 10;
        uint8_t noending = 0, halt = 0;
        if (argc == 3) !atoi(argv[2]) ? (noending = 1) : (maxLoop = atoi(argv[2]));

        struct pollfd mypoll = { STDIN_FILENO, POLLIN|POLLPRI, POLLIN|POLLPRI };

        char command[] = "         "; // Message written through stdin
        char transmitMsg[10];

        uint8_t waitingTransmit = 0, waitingAck = 0;

        while (loop < maxLoop || noending) {
            for (uint8_t i = 0; i < strlen(command); i++) command[i] = ' ';

            if( poll(&mypoll, 1, 10) ) {
                scanf("%9s", command);
                fprintf(stdout, "You have typed : %s\n", command);

                if (command[0] == 'D' || command[0] == 'P' || command[0] == 'T' || command[0] == 'A') {
                    strcpy(transmitMsg, command);
                    waitingTransmit = 1;
                }
                else if (!strcmp(command, "stop")) halt = 1;
                else if (!strcmp(command, "exit")) return 0;
                else fprintf(stdout, "No such command\n");

                while (halt) {
                    if( poll(&mypoll, 1, 5000) ) {
                        char restartCmd[10];
                        scanf("%9s", restartCmd);
                        fprintf(stdout, "You have typed : %s\n", restartCmd);
                        if (!strcmp(restartCmd, "restart")) halt = 0;
                        else if (!strcmp(command, "exit")) return 0;
                    } else fprintf(stdout, "Waiting for restart command\n");
                };
            }


            // Transmission part


            if (waitingTransmit) {
                t1 = clock();

                TxBuffer[HEADER_0_POS] = HEADER_0;
                TxBuffer[HEADER_1_POS] = HEADER_1;
                TxBuffer[SOURCE_ID_POS] = MY_ID;
                PayloadLength = COMMAND_LONG;

                if (transmitMsg[0] == 'D') { // Discover
                    // if (argc != 4) {
                    //     fprintf(stdout, "Error nb args, usage : D <destination_id> <source_id>");
                    //     return -1;
                    // }
                    fprintf(stdout, "Discover : %s\n", transmitMsg);
                    TxBuffer[DEST_ID_POS] = (uint8_t) (transmitMsg[1] - '0');
                    TxBuffer[GTW_POS] = (uint8_t) (transmitMsg[2] - '0');
                    TxBuffer[COMMAND_POS] = DISCOVER;
                } else if (transmitMsg[0] == 'P') { // Ping
                    // if (argc != 4) {
                    //     fprintf(stdout, "Error nb args, usage : P <destination_id> <source_id>");
                    //     return -1;
                    // }
                    fprintf(stdout, "Ping : %s\n", transmitMsg);
                    TxBuffer[DEST_ID_POS] = (uint8_t) (transmitMsg[1] - '0');
                    TxBuffer[GTW_POS] = (uint8_t) (transmitMsg[2] - '0');
                    TxBuffer[COMMAND_POS] = PING;
                } else if (transmitMsg[0] == 'T' && transmitMsg[1] == 'O') { // Timeout ZigBee
                    // if (argc != 5) {
                    //     fprintf(stdout, "Error nb args, usage : TO <destination_id> <source_id> <sensor_id>");
                    //     return -1;
                    // }
                    fprintf(stdout, "Timeout ZigBee : %s\n", transmitMsg);
                    TxBuffer[DEST_ID_POS] = (uint8_t) (transmitMsg[2] - '0');
                    TxBuffer[GTW_POS] = (uint8_t) (transmitMsg[3] - '0');
                    TxBuffer[COMMAND_POS] = TIMEOUT;
                    TxBuffer[SENSOR_ID_POS] = (uint8_t) (transmitMsg[4] - '0');
                    PayloadLength = TIMEOUT_LONG;
                } else if (transmitMsg[0] == 'T') { // Transmit
                    // if (argc != 7) {
                    //     fprintf(stdout, "Error nb args, usage : T <destination_id> <source_id> <sensor_id> <T> <O>");
                    //     return -1;
                    // }
                    fprintf(stdout, "Transmit : %s\n", transmitMsg);
                    TxBuffer[DEST_ID_POS] = (uint8_t) (transmitMsg[1] - '0');
                    TxBuffer[GTW_POS] = (uint8_t) (transmitMsg[2] - '0');
                    TxBuffer[COMMAND_POS] = DATA;
                    TxBuffer[SENSOR_ID_POS] = (uint8_t) (transmitMsg[3] - '0');
                    TxBuffer[T_POS] = (uint8_t) (transmitMsg[4] - '0');
                    TxBuffer[O_POS] = (uint8_t) (transmitMsg[5] - '0');
                    PayloadLength = TRANSMIT_LONG;
                } else if (transmitMsg[0] == 'A') { // Acknowledge
                    // if (argc != 7) {
                    //     fprintf(stdout, "Error nb args, usage : A <destination_id> <source_id> <sensor_id> <ACK> <R>");
                    //     return -1;
                    // }
                    fprintf(stdout, "Acknowledge : %s\n", transmitMsg);
                    TxBuffer[DEST_ID_POS] = (uint8_t) (transmitMsg[1] - '0');
                    TxBuffer[GTW_POS] = (uint8_t) (transmitMsg[2] - '0');
                    TxBuffer[COMMAND_POS] = ACK_ZIGBEE;
                    TxBuffer[SENSOR_ID_POS] = (uint8_t) (transmitMsg[2] - '0');
                    TxBuffer[ACK_POS] = (uint8_t) (transmitMsg[3] - '0');
                    TxBuffer[R_POS] = (uint8_t) (transmitMsg[4] - '0');
                    PayloadLength = TRANSMIT_LONG;
                } else {
                    fprintf(stdout, "Error, command does not exist");
                    return -1;
                }

                //encrypt TxBuffer from id 6 to PayloadLength to have a message encrypted
                if (transmitMsg[0] == 'T' || transmitMsg[0] == 'A') {
                    //Store values of TxBuffer from id 6 to PayloadLength inside of plaintext
                    fprintf(stdout, "Plaintext Bytes : ");
                    for (uint8_t i = 0; i < COMMAND_LONG - 1; i++) {
                        plaintext[i] = TxBuffer[i];
                        fprintf(stdout, "%02x ", plaintext[i]);
                    }
                    for (uint8_t i = 0; i < DATA_LONG; i++) {
                        plaintext[COMMAND_LONG - 1 + i] = TxBuffer[COMMAND_LONG + i];
                        fprintf(stdout, "%02x ", plaintext[CLEN_POS + i]);
                    }
                    fprintf(stdout, "\n");

                    hextobyte(keyhex,key);

                    //encrypt plaintext and store it in cipher
                    crypto_aead_encrypt(cipher, &clen, plaintext, TRANSMIT_LONG - 1, ad, strlen((const char*) ad), nsec, npub, key);

                    TxBuffer[CLEN_POS] = clen;

                    //Store values of cipher in the TxBuffer
                    fprintf(stdout, "Encrypted Bytes : ");
                    for (uint8_t i = 0; i < clen; i++) {
                        TxBuffer[COMMAND_LONG + i] = cipher[i];
                        fprintf(stdout, "%02x ", TxBuffer[COMMAND_LONG + i]);
                    }
                    fprintf(stdout, "\n");

                    LoadTxFifoWithTxBuffer(TxBuffer, COMMAND_LONG + clen); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                                        // in order to read the values of their content and copy them in SX1272 registers
                } else {
                    LoadTxFifoWithTxBuffer(TxBuffer, PayloadLength); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                                        // in order to read the values of their content and copy them in SX1272 registers
                }

                TransmitLoRaMessage();

                fprintf(stdout, "Time of transmission : %fms\n", (float)(clock()-t1)/CLOCKS_PER_SEC);
                t2 = clock();
                waitingTransmit = 0;
                waitingAck = 1;
            }


            // Reception part


            if (argc == 3) CRCError = WaitIncomingMessageRXSingle(&TimeoutOccured);
            else CRCError = WaitIncomingMessageRXContinuous();

            if (TimeoutOccured) {
                #if debug
                //fprintf(stdout, "No answer\n");
                #endif
                if (waitingAck) {
                    waitingAck = 0;
                    //fprintf(stdout, "Timeout\n");
                }
            } else if (CRCError) {
                #if debug
                fprintf(stdout, "CRC Error!\n");
                #endif
            } else {
                t1 = clock();
                received = 1;

                RSSI = LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived); // addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
                                                                // in order to update the values of their content
                #if debug
                fprintf(stdout, "RSSI = %d\n", RSSI);
                #endif

                if (RxBuffer[HEADER_0_POS] == HEADER_0
                && RxBuffer[HEADER_1_POS] == HEADER_1
                && RxBuffer[DEST_ID_POS] == MY_ID
                && RxBuffer[GTW_POS] == MY_ID) {

                    if (RxBuffer[SOURCE_ID_POS] == TxBuffer[DEST_ID_POS]
                    && RxBuffer[COMMAND_POS] == ACK) {
                        fprintf(stdout, "Confirmation ");
                        if (transmitMsg[0] == 'D') fprintf(stdout, "Mother node is active\n");          // Discover successful
                        else if (transmitMsg[0] == 'P') fprintf(stdout, "Mother node pong\n");          // Pong from mother node
                        else if (transmitMsg[0] == 'T') fprintf(stdout, "Mother node ACK\n");           // ACK from Mother node
                        else if (transmitMsg[0] == 'A') fprintf(stdout, "Zigbee ACK\n");                // ACK from a ZigBee sensor
                        else fprintf(stdout, "Incorrect answer\n");

                        fprintf(stdout, "Time of reception : %fms\n", (float)(clock()-t2)/CLOCKS_PER_SEC);
                    }

                    TxBuffer[HEADER_0_POS] = HEADER_1;
                    TxBuffer[HEADER_1_POS] = HEADER_0;
                    TxBuffer[DEST_ID_POS] = RxBuffer[SOURCE_ID_POS];
                    TxBuffer[SOURCE_ID_POS] = MY_ID;
                    if (RxBuffer[GTW_POS] == RxBuffer[SOURCE_ID_POS]) {  // Direct reply if no GTW providen
                        TxBuffer[GTW_POS] = RxBuffer[SOURCE_ID_POS];
                    } else TxBuffer[GTW_POS] = RxBuffer[GTW_POS];  // Same gateway otherwise
                    TxBuffer[GTW_POS] = RxBuffer[SOURCE_ID_POS];
                    TxBuffer[COMMAND_POS] = ACK;

                    LoadTxFifoWithTxBuffer(TxBuffer, ACK_LONG); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                             // in order to read the values of their content and copy them in SX1272 registers
                    TransmitLoRaMessage();

                    fprintf(stdout, "Time of transmission : %fms\n", (float)(clock()-t1)/CLOCKS_PER_SEC);

                    for (uint8_t i = 0; i < NbBytesReceived - 4; i++) NodeData[i] = RxBuffer[i + 4];
                    WriteDataInFile(&RxBuffer[SOURCE_ID_POS], &NbBytesReceived, NodeData, &RSSI);

                    //encrypt TxBuffer from id 5 to PayloadLength to have a message encrypted
                    if (RxBuffer[COMMAND_POS] == DATA || RxBuffer[COMMAND_POS] == ACK_ZIGBEE) {
                        //Store values of TxBuffer from id 5 to PayloadLength inside of plaintext
                        fprintf(stdout, "Encrypted Bytes : ");
                        clen = RxBuffer[CLEN_POS];
                        for (uint8_t i = 0; i < clen; i++) {
                            cipher[i] = RxBuffer[COMMAND_LONG + i];
                            fprintf(stdout, "%02x ", cipher[i]);
                        }
                        fprintf(stdout, "\n");

                        hextobyte(keyhex, key);

                        //encrypt plaintext and store it in cipher
                        crypto_aead_decrypt(plaintext, &mlen, nsec, cipher, clen, ad, strlen((const char*) ad), npub, key);

                        //Store values of cipher in the TxBuffer
                        fprintf(stdout, "Plaintext Bytes : ");
                        for (int i=0;i<mlen;i++)
                            fprintf(stdout, "%02x ",plaintext[i]);
                        fprintf(stdout, "\n");

                        for (uint8_t i = 0; i < DATA_LONG; i++) {
                            RxBuffer[COMMAND_LONG + i] = plaintext[COMMAND_LONG - 1 + i];
                            fprintf(stdout, "%02x ", plaintext[COMMAND_LONG - 1 + i]);
                        }
                        fprintf(stdout, "\n");
                    }

                    if (RxBuffer[COMMAND_POS] == DISCOVER) {
                        fprintf(stdout, "{\"DISCO\":%d,\"NETD\":%d,\"NETS\":%d,\"GTW\":%d,\"RSSI\":%d}\n", RxBuffer[DEST_ID_POS], RxBuffer[DEST_ID_POS], RxBuffer[SOURCE_ID_POS], RxBuffer[GTW_POS], RSSI);
                    } else if (RxBuffer[COMMAND_POS] == PING) {
                        fprintf(stdout, "{\"PING\":%d,\"NETD\":%d,\"NETS\":%d,\"GTW\":%d,\"RSSI\":%d}\n", RxBuffer[DEST_ID_POS], RxBuffer[DEST_ID_POS], RxBuffer[SOURCE_ID_POS], RxBuffer[GTW_POS], RSSI);
                    } else if (RxBuffer[COMMAND_POS] == TIMEOUT) {
                        fprintf(stdout, "{\"TIMEOUT\":%d,\"NETD\":%d,\"NETS\":%d,\"GTW\":%d,\"RSSI\":%d}\n", RxBuffer[DEST_ID_POS], RxBuffer[DEST_ID_POS], RxBuffer[SOURCE_ID_POS], RxBuffer[GTW_POS], RSSI);
                    } else if (RxBuffer[COMMAND_POS] == DATA) {
                        fprintf(stdout, "{\"ID\":%d,\"T\":%d,\"O\":%d,\"NETD\":%d,\"NETS\":%d,\"GTW\":%d}\n", RxBuffer[SENSOR_ID_POS], RxBuffer[T_POS], RxBuffer[O_POS], RxBuffer[DEST_ID_POS], RxBuffer[SOURCE_ID_POS], RxBuffer[GTW_POS]);
                    } else if (RxBuffer[COMMAND_POS] == ACK_ZIGBEE) {
                        fprintf(stdout, "{\"ID\":%d,\"ACK\":%d,\"R\":%d,\"NETD\":%d,\"NETS\":%d,\"GTW\":%d}\n", RxBuffer[SENSOR_ID_POS], RxBuffer[ACK_POS], RxBuffer[R_POS], RxBuffer[DEST_ID_POS], RxBuffer[SOURCE_ID_POS], RxBuffer[GTW_POS]);
                    }
                    for (uint8_t i = 0; i < NbBytesReceived; i++) RxBuffer[i] = NUL;
                }
            }
            if (!noending) loop++;
        } // end of while
        if (!received) {
            #if debug
            fprintf(stdout, "Pas de reponse\n");
            #endif
        }
    } // end of if
    else fprintf(stdout, "Error nb args, usage : <prog> <source_id> [loop]\n");
    return 0;
} // end of main
