//#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <linux/types.h>
//#include <linux/spi/spidev.h>			// library used to control SPI peripheral
//#include <sys/ioctl.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
//#include <time.h>
#include "gpio_util.h"
#include "SX1272.h"
#include "RF_LoRa_868_SO.h"
#include "sendRecept.h"
#include "filecsv.h"

#define Header0 0x4E
#define Header1 0xAD
#define NetID 0x01
#define MaxNodesInNetwork 5

#define CommandDiscovery 0x01
#define CommandMeasurement 0x02
#define ACKMeasurementRequest 0x03
#define NACKMeasurementRequest 0x04
#define ACKMeasurementReception 0x05
#define NACKMeasurementReception 0x06

uint8_t NodeData[255]; // data sent by a node

uint8_t TxBuffer[50]; // buffer containing data to write in SX1272 FIFO before transmission
uint8_t RxBuffer[50]; // buffer containing data read from SX1272 FIFO after reception

uint8_t NodeID;
uint8_t NodeMap[MaxNodesInNetwork][3]; // to store which nodes are present in the network, and their payload length
                                       // (written during the discovery process)
                                       // 1 line per node
                                       // 1st column = 1 if node is present
                                       // 2nd column = type of sensor
                                       // 3rd column = length (in bytes) of data sent by a node

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

uint8_t DiscoveryProcessDone = 0; // will be set to 1 when for each node, a DISCOVERY request has been sent,
                                  // and a DISCOVERY reply has been listen to

uint8_t MeasurementIsPossible = 0; // after measurement request, a sensor should reply if the measurement is possible or not
                                   // If YES, this variable is set
                                   // If NO, this variable is cleared
                                   // (updated in DoAction(4), when the base listens to the sensor reply)

uint8_t RetryMeasurementRequest = 0; // after measurement request, if a message different from ACK or NACK is received,
                                     // the reply of the sensor was maybe distroyed by an interferer
                                     // => in this case this variable is set (in DoAction(4)) to try another measurement request after some time)

uint8_t ReceivedMeasurementData = 0; // 1 when measurement data has been received correctly
                                     // will be set during measurement reception, if received message is correct

uint16_t NumberOfMeasurementsReceived = 0;

// prototypes of functions used in the main function
// (functions are described after the main function)

void DoAction(uint8_t ActionType);
// 0: do nothing
// 1: transmit a DISCOVERY request
// 2: wait for DISCOVERY reply
// 3: transmit a MEASUREMENT request
// 4: wait for ACK/NACK reply after MEASUREMENT request
// 5: wait for measurement data
// 6: transmit ACK after measurement data reception
// 7: transmit NACK after measurement data reception

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

    if (argc > 1)
    {
        fprintf(stdout, "args %d %s %s\n", argc, argv[0], argv[1]);
        TxBuffer[0] = Header0;
        TxBuffer[1] = Header1;
        TxBuffer[2] = NetID;
        TxBuffer[3] = 0x04;
        if (!strcmp(argv[1], "LED_ON"))
            TxBuffer[4] = 0x66;
        else if (!strcmp(argv[1], "LED_OFF"))
            TxBuffer[4] = 0x67;
        else
            return 0;

        LoadTxFifoWithTxBuffer(TxBuffer, 5); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                             // in order to read the values of their content and copy them in SX1272 registers
        TransmitLoRaMessage();

        WaitIncomingMessageRXSingle(&TimeoutOccured);

        if (TimeoutOccured)
            fprintf(stdout, "Pas de reponse de Arduino\n");
        else
        {
            LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived); // addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
                                                                // in order to update the values of their content
            if (RxBuffer[0] == Header1 && RxBuffer[1] == Header0 && RxBuffer[2] == NetID && RxBuffer[3] == 0x04 && RxBuffer[4] == 0x05)
            {
                fprintf(stdout, "Confirmation de Arduino");
                if (!strcmp(argv[1], "LED_ON"))
                    fprintf(stdout, " LED switched on\n");
                else if (!strcmp(argv[1], "LED_OFF"))
                    fprintf(stdout, " LED switched off\n");
            }
            else
                fprintf(stdout, "Reponse incorrecte de Arduino");
        }

        return 0;
    }

    NodeID = 0;

    DeleteDataFile();
    CreateDataFile();

    while (1)
    {
        // launch DISCOVERY process
        if (DiscoveryProcessDone == 0)
        {
            ClearNodeMap();

            fprintf(stdout, "Start DISCOVERY process\n");
            fprintf(stdout, "*********\n");

            // for (uint8_t DiscoveryTry = 0; DiscoveryTry < 2; DiscoveryTry++)
            for (uint8_t DiscoveryTry = 0; DiscoveryTry < 1; DiscoveryTry++)
            // launch the DISCOVERY process several times in case LoRa messages are lost
            {
                for (NodeID = 0; NodeID < MaxNodesInNetwork; NodeID++)
                {
                    set_port_value(21, 1); // switch on LED
                    DoAction(1);           // send a DISCOVERY request to node n° NodeID
                    set_port_value(21, 0); // switch off LED

                    DoAction(2); // wait reply and fill table NodeMap
                }
            }

            // print the state of the network
            for (NodeID = 0; NodeID < MaxNodesInNetwork; NodeID++)
            {
                if (NodeMap[NodeID][0] == 0)
                {
                    fprintf(stdout, "Node %d:  not responding\n", NodeID);
                }
                else
                {
                    fprintf(stdout, "Node %d:  active, type = 0x%X,  data length = %d bytes\n", NodeID, NodeMap[NodeID][1], NodeMap[NodeID][2]);
                }
            }
            fprintf(stdout, "*********\n");

            // wait before next DISCOVERY sequence
            for (uint16_t WaitSeconds = 0; WaitSeconds < 2; WaitSeconds++)
            {
                usleep(1000000); // wait for 1 second
            }

            DiscoveryProcessDone = 1;
        }

        DiscoveryProcessDone = 0; // reset DiscoveryProcessDone in order to launch again DISCOVERY sequence after MEASUREMENT sequence

        // launch MEASUREMENT process
        fprintf(stdout, "Start MEASUREMENT process\n");
        fprintf(stdout, "*********\n");

        for (NodeID = 0; NodeID < MaxNodesInNetwork; NodeID++)
        {
            if (NodeMap[NodeID][0] != 0) // for each Node discovered in the network
            {

                DoAction(3); // send a MEASUREMENT request to node n° NodeID
                DoAction(4); // wait ACK/NACK for request

                if (RetryMeasurementRequest) // variable updated in DoAction(4), whatever the situation
                                             // timeout => no retry (sensor may be offline)
                                             // packet with wrong length received => retry
                                             // packet with correct length + ACK received => no retry
                                             // packet with correct length + NACK received => no retry
                                             // packet with correct length + neither ACK or NACK received => retry
                {
                    // wait a little (10 seconds) before retry
                    for (uint16_t WaitSeconds = 0; WaitSeconds < 10; WaitSeconds++)
                    {
                        usleep(1000000); // wait for 1 second
                    }
                    fprintf(stdout, "Resend a MEASUREMENT request (2nd try)\n");
                    fprintf(stdout, "*********\n");
                    DoAction(3); // resend a MEASUREMENT request
                    DoAction(4); // wait ACK/NACK for request
                }

                if (MeasurementIsPossible) // variable updated in DoAction(4), whatever the situation
                                           // ACK received	=> MeasurementIsPossible = 1
                                           // otherwise	=> MeasurementIsPossible = 0
                {
                    ReceivedMeasurementData = 0;
                    DoAction(5); // receive measurement data (1st try)

                    if (ReceivedMeasurementData == 1) // variable updated in DoAction(5), whatever the situation
                                                      // correct message received	=> ReceivedMeasurementData = 1
                                                      // timeout	=> ReceivedMeasurementData = 0
                                                      // message with bad length	=> ReceivedMeasurementData = 0
                                                      // message with correct length but wrong header => ReceivedMeasurementData = 0
                    {
                        DoAction(6); // send ACK for measurement reception
                    }
                    else // data was not received correctly
                         // - NACK will be sent
                         // - the base will wait for data reception
                         // - if data is received correctly, ACK will be sent
                    {
                        DoAction(7); // send NACK for measurement reception
                        fprintf(stdout, "Waiting measurement data (2nd try)\n");
                        fprintf(stdout, "*********\n");
                        DoAction(5); // receive measurement data (2nd try)

                        if (ReceivedMeasurementData == 1)
                        {
                            DoAction(6); // send ACK for measurement reception
                        }
                        else
                        {
                            DoAction(7); // send NACK for measurement reception
                        }
                    }

                    if (ReceivedMeasurementData == 1)
                    {
                        // Display received data
                        fprintf(stdout, "Number of measurements received: %d\n", NumberOfMeasurementsReceived);
                        fprintf(stdout, "*********\n");

                        if (NodeID == 0) {
                            fprintf(stdout, "Data received from node %d:\n", NodeID);
                            for (uint8_t x = 0; x < (NbBytesReceived - 4); x++)
                            {
                                fprintf(stdout, "Ox%X ", NodeData[x]);
                            }
                            fprintf(stdout, "\n");
                            fprintf(stdout, "*********\n");
                        }

                        WriteDataInFile(&NodeID, &NbBytesReceived, NodeData, &RSSI);
                    }
                }

                // wait before next measurement
                for (uint16_t WaitSeconds = 0; WaitSeconds < 2; WaitSeconds++)
                {
                    usleep(1000000); // wait for 1 second
                }

            } // end of "for each Node discovered in the network"

        } // end of "for each  Node from 0 to MaxNodesInNetwork"

        // wait before next  sequence
        fprintf(stdout, "*********\n");
        fprintf(stdout, "Wait and start a new DISCOVERY + MEASUREMENT sequence\n");
        fprintf(stdout, "*********\n");
        for (uint16_t WaitSeconds = 0; WaitSeconds < 300; WaitSeconds++)
        {
            usleep(1000000); // wait for 1 second
        }

    } // end while(1)

} // end main

//*************************************************************************
//************************  FUNCTIONS  ************************************
//*************************************************************************

void DoAction(uint8_t ActionType)
{
    switch (ActionType)
    {
        // 0: do nothing
        // 1: transmit a DISCOVERY request
        // 2: wait for DISCOVERY reply
        // 3: transmit a MEASUREMENT request
        // 4: wait for ACK/NACK reply after MEASUREMENT request
        // 5: wait for measurement data
        // 6: transmit ACK after measurement data reception
        // 7: transmit NACK after measurement data reception

    case 0:
        // do nothing
        break;

    case 1: // transmit a DISCOVERY command to node n° NodeID
            // **********************************************

        // 1 - load in FIFO data to transmit
        PayloadLength = 5;
        TxBuffer[0] = Header0;
        TxBuffer[1] = Header1;
        TxBuffer[2] = NetID;
        TxBuffer[3] = NodeID;
        TxBuffer[4] = CommandDiscovery;

        LoadTxFifoWithTxBuffer(TxBuffer, PayloadLength); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                                         // in order to read the values of their content and copy them in SX1272 registers

        // 2 - switch to TX mode to transmit data
        TransmitLoRaMessage();

        fprintf(stdout, "DISCOVERY command sent to node %d\n", NodeID);
        fprintf(stdout, "*********\n");

        break;

    case 2: // wait DISCOVERY reply from node n° NodeID
            // ****************************************
        // fprintf(stdout, "Listening for DISCOVERY reply of node %d\n", NodeID);
        // fprintf(stdout, "*********\n");

        // 1 - set mode to RX Single and wait incoming message
        CRCError = WaitIncomingMessageRXSingle(&TimeoutOccured);

        // 2 - check if a message was received
        if (TimeoutOccured)
        {
            fprintf(stdout, "No message received from node %d\n", NodeID);
            fprintf(stdout, "*********\n");
        }
        else
        // 3 - if yes, check the message
        {
            // 3.1 - fetch data and copy it in RxBuffer
            LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived); // addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
                                                                // in order to update the values of their content
            // 3.2 - ckeck ik packet length is correct (should be 6 bytes long)
            if (NbBytesReceived != 6)
            {
                fprintf(stdout, "Received a message with bad length\n");
                fprintf(stdout, "*********\n");
            }
            else
            {
                if ((RxBuffer[0] == Header1) && (RxBuffer[1] == Header0) && (RxBuffer[2] == NetID) && (RxBuffer[3] == NodeID))
                // message is a correct DISCOVERY reply
                {
                    fprintf(stdout, "Node %d is a sensor of type %d\n", NodeID, RxBuffer[4]);
                    fprintf(stdout, "with a payload length equal to %d bytes\n", RxBuffer[5]);
                    fprintf(stdout, "*********\n");

                    NodeMap[NodeID][0] = 1;           // write in table NodeMap that node is present
                    NodeMap[NodeID][1] = RxBuffer[4]; // write in table NodeMap the type of sensor
                    NodeMap[NodeID][2] = RxBuffer[5]; // write in table NodeMap the length of the payload
                }
                else
                {
                    fprintf(stdout, "Didn't receive a correct DISCOVERY reply from node %d\n", NodeID);
                    fprintf(stdout, "*********\n");
                }
            }
        }

        break;

    case 3: // transmit a MEASUREMENT request to node n° NodeID
            // **********************************************

        // 1 - load in FIFO data to transmit
        PayloadLength = 5;
        TxBuffer[0] = Header0;
        TxBuffer[1] = Header1;
        TxBuffer[2] = NetID;
        TxBuffer[3] = NodeID;
        TxBuffer[4] = CommandMeasurement;

        LoadTxFifoWithTxBuffer(TxBuffer, PayloadLength); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                                         // in order to read the values of their content and copy them in SX1272 registers

        // 2 - switch to TX mode to transmit data
        TransmitLoRaMessage();

        fprintf(stdout, "MEASUREMENT command sent to node %d\n", NodeID);
        fprintf(stdout, "*********\n");

        break;

    case 4: // wait ACK/NACK reply after MEASUREMENT request
            // *********************************************
        fprintf(stdout, "listening for ACK/NACK reply of node %d\n", NodeID);
        fprintf(stdout, "*********\n");

        // 1 - set mode to RX Single and wait incoming packet
        CRCError = WaitIncomingMessageRXSingle(&TimeoutOccured);

        // 2 - check if message was received
        if (TimeoutOccured)
        {
            fprintf(stdout, "No message received from node %d\n", NodeID);
            fprintf(stdout, "*********\n");

            MeasurementIsPossible = 0;
            RetryMeasurementRequest = 0; // don't retry now: the sensor may be offline
        }
        else
        // 3 - if yes, check the message
        {
            // 3.1 - fetch data and copy it in RxBuffer
            LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived); // addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
                                                                // in order to update the values of their content
            // 3.2 - ckeck ik message length is correct (should be 5 bytes long)
            if (NbBytesReceived != 5)
            {
                fprintf(stdout, "Received a message with bad length\n");
                fprintf(stdout, "Measurement request will be sent again\n");
                fprintf(stdout, "*********\n");

                MeasurementIsPossible = 0;
                RetryMeasurementRequest = 1; // the sensor reply may have been destroyed by an interferer
                                             // => the measurement request will be resent
            }
            else // message length is correct
            {
                if ((RxBuffer[0] == Header1) && (RxBuffer[1] == Header0) && (RxBuffer[2] == NetID) && (RxBuffer[3] == NodeID))
                // message seems to have a consistent content
                {
                    if (RxBuffer[4] == ACKMeasurementRequest) // received ACK
                    {
                        fprintf(stdout, "Received ACK MEASUREMENT from node %d\n", NodeID);
                        fprintf(stdout, "*********\n");

                        MeasurementIsPossible = 1;
                        RetryMeasurementRequest = 0;
                    }
                    else if (RxBuffer[4] == NACKMeasurementRequest) // received NACK
                    {
                        fprintf(stdout, "Received NACK MEASUREMENT from node %d\n", NodeID);
                        fprintf(stdout, "*********\n");

                        MeasurementIsPossible = 0;
                        RetryMeasurementRequest = 0;
                    }
                    else
                    {
                        fprintf(stdout, "Received something different from ACK or NACK MEASUREMENT from node %d\n", NodeID);
                        fprintf(stdout, "Measurement request will be sent again\n");
                        fprintf(stdout, "*********\n");

                        MeasurementIsPossible = 0;
                        RetryMeasurementRequest = 1; // the sensor reply may have been destroyed by an interferer
                                                     // => the measurement request will be resent
                    }
                }
                else
                {
                    fprintf(stdout, "Received a MEASUREMENT reply with correct length but not consistent content\n");
                    fprintf(stdout, "Measurement request will be sent again\n");
                    fprintf(stdout, "*********\n");

                    MeasurementIsPossible = 0;
                    RetryMeasurementRequest = 1; // the sensor reply may have been destroyed by an interferer
                                                 // => the measurement request will be resent
                }
            }
        }

        break;

    case 5: // receive measurement data
            // *********************************************
        fprintf(stdout, "Waiting for measurement data of node %d\n", NodeID);
        fprintf(stdout, "*********\n");

        // 1 - set mode to RX Single and wait incoming message
        CRCError = WaitIncomingMessageRXSingle(&TimeoutOccured);

        // 2 - check if a message was received
        if (TimeoutOccured)
        {
            fprintf(stdout, "No measurement data received from node %d\n", NodeID);
            fprintf(stdout, "*********\n");

            ReceivedMeasurementData = 0;
        }
        else
        // 3 - if yes, check the message
        {
            // 3.1 - fetch data and copy it in RxBuffer
            LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived); // addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
                                                                // in order to update the values of their content
            // 3.2 - check ik packet length is correct
            //       this should be the payload length stored in NodeMap during DISCOVERY sequence, added with 4 bytes for Header0, Header1, NetID, NodeID
            if (NbBytesReceived != (NodeMap[NodeID][2] + 4))
            {
                fprintf(stdout, "Received a message with bad length\n");
                fprintf(stdout, "*********\n");

                ReceivedMeasurementData = 0;
            }
            else // message length is correct
            {
                if ((RxBuffer[0] == Header1) && (RxBuffer[1] == Header0) && (RxBuffer[2] == NetID) && (RxBuffer[3] == NodeID))
                // message is actually a measurement data
                {
                    fprintf(stdout, "Received measurement data from node %d\n", NodeID);
                    fprintf(stdout, "*********\n");

                    // store measurement data
                    for (uint8_t i = 0; i < NbBytesReceived; i++)
                    {
                        NodeData[i] = RxBuffer[i + 4];
                    }

                    ReceivedMeasurementData = 1;

                    // increment NumberOfMeasurementsReceived: displaying this variable helps checking correct operation
                    ++NumberOfMeasurementsReceived;
                }
                else // message length is correct but the content is not
                {
                    fprintf(stdout, "Didn't receive a correct measurement data from node %d\n", NodeID);
                    fprintf(stdout, "*********\n");

                    ReceivedMeasurementData = 0;
                }
            }
        }

        break;

    case 6: // send ACK for measurement reception
            // **********************************

        // 1 - load in FIFO data to transmit
        PayloadLength = 5;
        TxBuffer[0] = Header0;
        TxBuffer[1] = Header1;
        TxBuffer[2] = NetID;
        TxBuffer[3] = NodeID;
        TxBuffer[4] = ACKMeasurementReception;

        LoadTxFifoWithTxBuffer(TxBuffer, PayloadLength); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                                         // in order to read the values of their content and copy them in SX1272 registers

        // 2 - switch to TX mode to transmit data
        TransmitLoRaMessage();

        fprintf(stdout, "MEASUREMENT RECEPTION ACK sent to node %d\n", NodeID);
        fprintf(stdout, "*********\n");

        break;

    case 7: // send NACK for measurement reception
            // **********************************

        // 1 - load in FIFO data to transmit
        PayloadLength = 5;
        TxBuffer[0] = Header0;
        TxBuffer[1] = Header1;
        TxBuffer[2] = NetID;
        TxBuffer[3] = NodeID;
        TxBuffer[4] = NACKMeasurementReception;

        LoadTxFifoWithTxBuffer(TxBuffer, PayloadLength); // address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                                         // in order to read the values of their content and copy them in SX1272 registers

        // 2 - switch to TX mode to transmit data
        TransmitLoRaMessage();

        fprintf(stdout, "MEASUREMENT RECEPTION NACK sent to node %d\n", NodeID);
        fprintf(stdout, "*********\n");

        break;

    default:
        // do nothing
        break;
    }

} // end of DoAction description

void ClearNodeMap(void)
{
    for (NodeID = 0; NodeID < MaxNodesInNetwork; NodeID++)
    {
        NodeMap[NodeID][0] = 0;
        NodeMap[NodeID][1] = 0;
        NodeMap[NodeID][2] = 0;
    }
}
