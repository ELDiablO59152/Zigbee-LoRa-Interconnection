/*
 * File:   sendRecept.c
 * Author: Fabien AMELINCK
 *
 * Created on 04 June 2021
 */

#include "sendRecept.h"

uint8_t reg_val; // used for temporary storage of SX1272 register content

uint8_t WaitIncomingMessageRXSingle(uint8_t *PointTimeout)
{
    uint8_t CRCError = 0; // cleared when there is no CRC error, set to 1 otherwise
    // uint8_t reg_val;	// to store temporarily data read from SX1272 register
    int tempo = 0; // variable used to set the maximum time for polling flags

    AntennaRX(); // connect antenna to receiver

    WriteSXRegister(REG_IRQ_FLAGS, 0xff); // clear IRQ flags

    // 1 - set mode to RX single
    WriteSXRegister(REG_OP_MODE, LORA_RX_SINGLE_MODE);
    #if debug
    fprintf(stdout, "Waiting for incoming message (RX single mode)\n");
    #endif

    // 2 - wait reception of a complete packet or end of timeout
    reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    while ((reg_val & 0b11000000) == 0 && (tempo < 1500000)) // check bits n°7 (timeout) and n°6 (reception complete)
                                                             // with a maximum duration
    {
        reg_val = ReadSXRegister(REG_IRQ_FLAGS);
        ++tempo;
    }

    // for debugging
    #if debug
    fprintf(stdout, "IRQ flags: 0x%X\n", reg_val);
    fprintf(stdout, "tempo = %d\n", tempo);
    #endif
    // ********************************

    WriteSXRegister(REG_IRQ_FLAGS, 0xff); // clear IRQ flags

    if ((tempo > 10)) //&& (tempo < 1500000)) // RX timeout or Rx done occured
                                           // after a "reasonable" time
                                           // (not too short, not too long)
    {
        // 3 - check if timeout occured
        if ((reg_val & 0x80) != 0) // timeout occured
        {
            *PointTimeout = 1; // set TimeoutOccured to 1
            CRCError = 0;      // no CRC error (not relevant here)
            #if debug
            fprintf(stdout, "Timeout occured\n");
            #endif
        }
        else // a message was received before timeout
        {
            *PointTimeout = 0; // clear TimeoutOccured
            #if debug
            fprintf(stdout, "Message received\n");
            #endif
            // check CRC
            if ((reg_val & 0x20) == 0x00)
            {
                CRCError = 0; // no CRC error
                #if debug
                fprintf(stdout, "CRC checking => no CRC error\n");
                #endif
            }
            else
            {
                CRCError = 1; // CRC error
                #if debug
                fprintf(stdout, "CRC checking => CRC error\n");
                #endif
            }
        }
    }
    else // maximum duration exceeded
    {
        // Reset and re-initialize LoRa module
        ResetModule();
        WriteSXRegister(REG_OP_MODE, FSK_SLEEP_MODE);
        WriteSXRegister(REG_OP_MODE, LORA_SLEEP_MODE);
        WriteSXRegister(REG_OP_MODE, LORA_STANDBY_MODE);
        usleep(100000);
        //InitModule(freq,      bw,     sf, cr, sync, preamble, pout, gain, rxtimeout, hder, crc);
        InitModule(CH_17_868, BW_500, SF_12, CR_5, 0x12, 0x08, 2, G1, 0x00, HEADER_ON, CRC_ON);

        #if debug
        fprintf(stdout, "LoRa module was re-initialized\n");
        #endif
        WriteResetInFile();
    }

    return CRCError;
}

uint8_t WaitIncomingMessageRXContinuous(void)
{
    uint8_t CRCError; // cleared when there is no CRC error, set to 1 otherwise
    // uint8_t reg_val;    // to store temporarily data read from SX1272 register

    AntennaRX(); // connect antenna to receiver

    // 1 - set mode to RX continuous
    WriteSXRegister(REG_OP_MODE, LORA_RX_CONTINUOUS_MODE);
    #if debug
    fprintf(stdout, "Waiting for incoming message (RX continuous mode)\n");
    #endif

    // 2 - wait reception of a complete message
    reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    while ((reg_val & 0x40) == 0x00) // check bit n°6 (reception complete)
    {
        reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    }
    #if debug
    fprintf(stdout, "Message received\n");
    #endif

    // 3 - check CRC
    if ((reg_val & 0x20) == 0x00)
    {
        CRCError = 0; // no CRC error
        #if debug
        fprintf(stdout, "CRC checking => no CRC error\n");
        #endif
    }
    else
    {
        CRCError = 1; // CRC error
        #if debug
        fprintf(stdout, "CRC checking => CRC error\n");
        #endif
    }

    WriteSXRegister(REG_IRQ_FLAGS, 0xff); // clear IRQ flags

    return CRCError;
}

void TransmitLoRaMessage(void)
{
    AntennaTX(); // connect antenna to transmitter

    int tempo = 0; // variable used to set the maximum time for polling TXDone flag

    WriteSXRegister(REG_OP_MODE, LORA_TX_MODE); // set mode to TX

    // wait end of transmission
    reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    while (((reg_val & 0x08) == 0x00) && (tempo < 300000)) // check bit n°3 (TX done)
                                                           // and check if timeout is not reached
                                                           // typically, when a message of 5 bytes is transmitted, tempo ~ 30 000 when done
                                                           //            in single receive mode with timeout ~ 8 sec, tempo ~ 300 000 after timeout
                                                           // so, in TX mode, tempo = 300 000 ~ 8 sec ~ 50 tranmitted bytes
    {
        reg_val = ReadSXRegister(REG_IRQ_FLAGS);
        ++tempo;
    }

    if (tempo < 300000)
    {
        #if debug
        fprintf(stdout, "Transmission done\n");
        #endif
    }
    else
    {
        WriteSXRegister(REG_OP_MODE, LORA_STANDBY_MODE); // abort TX mode
        #if debug
        fprintf(stdout, "Transmission canceled\n");
        #endif
    }

    #if debug
    fprintf(stdout, "tempo = %d\n", tempo);
    fprintf(stdout, "*********\n");
    #endif

    WriteSXRegister(REG_IRQ_FLAGS, 0xff); // clear IRQ flags
}

int8_t LoadRxBufferWithRxFifo(uint8_t *Table, uint8_t *PointNbBytesReceived)
// the function receives the adresses of RxBuffer and NbBytesReceived and stores them in the pointers "table" and "pointNbBytesReceived"
{
    WriteSXRegister(REG_FIFO_ADDR_PTR, ReadSXRegister(REG_FIFO_RX_CURRENT_ADDR)); // fetch REG_FIFO_RX_CURRENT_ADDR and copy it in REG_FIFO_ADDR_PTR
    *PointNbBytesReceived = ReadSXRegister(REG_RX_NB_BYTES);                      // fetch REG_RX_NB_BYTES and copy it in NbBytesReceived
    // Display number of bytes received
    #if debug
    fprintf(stdout, "Received %d bytes\n", *PointNbBytesReceived);
    #endif

    int8_t SNR = ReadSXRegister(REG_PKT_SNR_VALUE) / 4.0;
    if (SNR < 0.0)
        SNR += -139 + ReadSXRegister(REG_PKT_RSSI_VALUE);
    else
        SNR = -139 + ReadSXRegister(REG_PKT_RSSI_VALUE);
    #if debug
    fprintf(stdout, "RSSI = %d\n", SNR);
    #endif

    fprintf(stdout, "received =");

    for (uint8_t i = 0; i < *PointNbBytesReceived; i++)
    {
        Table[i] = ReadSXRegister(REG_FIFO);
        // Display received data
        // fprintf(stdout, "i = %d  data = 0x%X\n", i, Table[i]);
        if (Table[i] < 0x10)
            fprintf(stdout, " | 0%X", Table[i]);
        else
            fprintf(stdout, " | %X", Table[i]);
    }
    fprintf(stdout, " |\n");
    #if debug
    fprintf(stdout, "*********\n");
    #endif

    return SNR;
}

void LoadTxFifoWithTxBuffer(uint8_t *Table, uint8_t PayloadLength)
{
    WriteSXRegister(REG_FIFO_ADDR_PTR, ReadSXRegister(REG_FIFO_TX_BASE_ADDR)); // fetch REG_FIFO_TX_BASE_ADDR and copy it in REG_FIFO_ADDR_PTR
    WriteSXRegister(REG_PAYLOAD_LENGTH_LORA, PayloadLength);                   // copy value of PayloadLength in REG_PAYLOAD_LENGTH_LORA
    // Display number of bytes to send
    #if debug
    fprintf(stdout, "Ready to send %d bytes\n", PayloadLength);
    #endif
    fprintf(stdout, "transmit =");

    for (uint8_t i = 0; i < PayloadLength; i++)
    {
        WriteSXRegister(REG_FIFO, Table[i]);
        // Display data to transmit
        // fprintf(stdout, "i = %d  data = 0x%X\n", i, Table[i]);
        #if debug
        #endif
        if (Table[i] < 0x10)
            fprintf(stdout, " | 0%X", Table[i]);
        else
            fprintf(stdout, " | %X", Table[i]);
    }
    fprintf(stdout, " |\n");
    #if debug
    fprintf(stdout, "*********\n");
    #endif
}
/*
void Transmit(const uint8_t *data, const uint8_t data_long) { // transmission des data fournis avec la longueur de trame adéquate

    //uint8_t txBuffer[256];
    uint8_t reg_val;                // when reading SX1272 registers, stores the content (variable read in main and typically updated by ReadSXRegister function)
    uint8_t i;

    //strcpy(( char* )txBuffer, ( char* )data);          // load txBuffer with content of txMsg
                                                        // txMsg is a table of constant values, so it is stored in Flash Memory
                                                        // txBuffer is a table of variables, so it is stored in RAM

    AntennaTX();                // connect antenna to module output

    // load FIFO with data to transmit
    //UARTWriteStrLn(" ");
    //UARTWriteStrLn("step 1: load FIFO");
    WriteSXRegister(REG_FIFO_ADDR_PTR, ReadSXRegister(REG_FIFO_TX_BASE_ADDR));      // FifoAddrPtr takes value of FifoTxBaseAddr
    WriteSXRegister(REG_PAYLOAD_LENGTH_LORA, data_long);                       // set the number of bytes to transmit (PAYLOAD_LENGTH is defined in RF_LoRa868_SO.h)

    for (i = 0; i < data_long; i++) {
        WriteSXRegister(REG_FIFO, data[i]);         // load FIFO with data to transmit
    }

    // set mode to LoRa TX
    //UARTWriteStrLn(" ");
    //UARTWriteStrLn("step 2: set mode to LoRa TX");
    WriteSXRegister(REG_OP_MODE, LORA_TX_MODE);
    usleep(1000);                                    // delay required to start oscillator and PLL
    //GetMode();

    // wait end of transmission
    reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    while ((reg_val & 0x08) == 0x00) {                    // wait for end of transmission (wait until TxDone is set)
        reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    }
    //UARTWriteStrLn(" ");
    //UARTWriteStrLn("step 3: TxDone flag set");

    //__delay_ms(200);        // delay is required before checking mode: it takes some time to go from TX mode to STDBY mode
    //GetMode();              // check that mode is back to STDBY


    // reset all IRQs
    //UARTWriteStrLn(" ");
    //UARTWriteStrLn("step 4: clear flags");
    //UARTWriteStr("before clear: REG_IRQ_FLAGS = 0x");
    //PrintSXRegContent(REG_IRQ_FLAGS);


    WriteSXRegister(REG_IRQ_FLAGS, 0xFF);           // clear flags: writing 1 clears flag


    // check that flags are actually cleared (useless if not debugging)
    //UARTWriteStr("after clear: REG_IRQ_FLAGS = 0x");
    //PrintSXRegContent(REG_IRQ_FLAGS);


    //UARTWriteStrLn(" ");
    UARTWriteStr("Message envoye : ");
    for(i = 0; i < data_long; i++) {
            UARTWriteByteHex(data[i]);
            UARTWriteStr(" ");
    }
    UARTWriteStrLn(" ");

    return;
}

void Receive(uint8_t *data) {  // recoit les data et les insère dans le tableau donné en argument

    uint8_t reg_val;                // when reading SX1272 registers, stores the content (variable read in main and typically updated by ReadSXRegister function)
    uint8_t RXNumberOfBytes;        // to store the number of bytes received
    uint8_t i;

    AntennaRX();                    // connect antenna to module input

    // set FIFO_ADDR_PTR to FIFO_RX_BASE_ADDR
    WriteSXRegister(REG_FIFO_ADDR_PTR, ReadSXRegister(REG_FIFO_RX_BASE_ADDR));

    // set mode to LoRa continuous RX
    //UARTWriteStrLn(" ");
    //UARTWriteStrLn("set mode to LoRa continuous RX");
    //WriteSXRegister(REG_OP_MODE, LORA_RX_CONTINUOUS_MODE);

    //UARTWriteStrLn("set mode to LoRa single RX");
    WriteSXRegister(REG_OP_MODE, LORA_RX_SINGLE_MODE);
    __delay_ms(100);                                    // delay required to start oscillator and PLL
    //GetMode();

    // wait for valid header reception
    UARTWriteStrLn("-----------------------");
    UARTWriteStrLn("waiting for valid header");

    do {
        reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    } while (((reg_val & 0x10) == 0x00) && ((reg_val & 0x80) == 0x00));     // check Valid Header flag (bit n°4) and timeout (bit n°3)

    if ((ReadSXRegister(REG_IRQ_FLAGS) & 0x10) == 0x00) {
        data[MSG_POS] = TIMEOUT;
        WriteSXRegister(REG_IRQ_FLAGS, 0xFF);           // clear flags: writing 1 clears flag
        return;
    }

    //UARTWriteStrLn(" ");
    UARTWriteStrLn("valid header received");

    // --- for debugging: display IRQ flags ---
    // * UARTWriteStr("REG_IRQ_FLAGS = 0x");
    // * PrintSXRegContent(REG_IRQ_FLAGS);
    // * // ----------------------------------------
    //

    // wait for end of packet reception
    reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    while ((reg_val & 0x40) == 0x00) {                  // check Packet Reception Complete flag (bit n°6)
        reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    }
    //UARTWriteStrLn(" ");
    //UARTWriteStrLn("packet reception complete");

    // --- for debugging: display IRQ flags ---
    // * UARTWriteStr("REG_IRQ_FLAGS = 0x");
    // * PrintSXRegContent(REG_IRQ_FLAGS);
    // * // ----------------------------------------
    //

    // check CRC
    if((reg_val & 0x20) != 0x00){                       // check Payload CRC Error flag (bit n°5)
        UARTWriteStrLn(" ");                            // CRC is wrong  => display warning message
        UARTWriteStrLn("payload CRC error");
    }
    else {                                              // CRC correct => read received data
        //UARTWriteStrLn(" ");
        UARTWriteStr("received data: ");
        RXNumberOfBytes = ReadSXRegister(REG_RX_NB_BYTES);      // read how many bytes have been received
        WriteSXRegister(REG_FIFO_ADDR_PTR, ReadSXRegister(REG_FIFO_RX_CURRENT_ADDR));   // to read FIFO at correct location, load REG_FIFO_ADDR_PTR with REG_FIFO_RX_CURRENT_ADDR value

        for (i = 0; i < RXNumberOfBytes; i++) {
            //reg_val = ReadSXRegister(REG_FIFO);       // read FIFO
            //UARTWriteByteHex(reg_val);              // to display in Hexadecimal format
            //UARTWriteStr(" ");
            //data[i] = reg_val;
            //UARTWriteByte(reg_val);                   // to send raw data: readable in terminal if ASCII code
            //UARTWriteStr(" ");
            data[i] = ReadSXRegister(REG_FIFO);         // chargement des données dans rxMsg
            UARTWriteByteHex(data[i]);
            UARTWriteStr(" ");
        }
        UARTWriteStrLn(" ");
    }


    // display FIFO information
    //UARTWriteStr("REG_FIFO_RX_CURRENT_ADDR = 0x");
    //PrintSXRegContent(REG_FIFO_RX_CURRENT_ADDR);

    //UARTWriteStr("REG_RX_NB_BYTES = 0x");
    //PrintSXRegContent(REG_RX_NB_BYTES);

    // display RSSI
    //reg_val = ReadSXRegister(REG_PKT_RSSI_VALUE);
    //UARTWriteStr("REG_PKT_RSSI_VALUE (decimal) = ");
    //UARTWriteByteDec(reg_val);


    // reset all IRQs
    //UARTWriteStrLn(" ");
    //UARTWriteStrLn("clear flags");

    //UARTWriteStr("before clear: REG_IRQ_FLAGS = 0x");
    //PrintSXRegContent(REG_IRQ_FLAGS);

    //WriteSXRegister(REG_IRQ_FLAGS, 0xFF);           // clear flags: writing 1 clears flag

    //    check that flags are actually cleared (useless if not debugging)
    // * UARTWriteStr("after clear: REG_IRQ_FLAGS = 0x");
    // * PrintSXRegContent(REG_IRQ_FLAGS);
    //
    //UARTWriteStrLn("-----------------------");

    return;
}
*/
uint8_t hexToDec(uint8_t data)
{ // renvoi une valeur décimale sous forme hexadécimale (ex : 24 -> 0x24)

    uint8_t upperHex = data / 10;
    uint8_t lowerHex = data % 10;

    data = 0x10 * upperHex + 0x01 * lowerHex;

    return data;
}