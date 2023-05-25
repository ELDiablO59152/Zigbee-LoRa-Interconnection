/*
 * File:   sendRecept.c
 * Author: Fabien AMELINCK
 *
 * Created on 04 June 2021
 */

#include <stdint.h>
#include "general.h"
#include "uart.h"
#include "SX1272.h"
#include "RF_LoRa_868_SO.h"
#include "tableRoutageRepeteur.h"


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
    __delay_ms(100);                                    // delay required to start oscillator and PLL
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

    /*
    // reset all IRQs
    UARTWriteStrLn(" ");
    UARTWriteStrLn("step 4: clear flags");
    UARTWriteStr("before clear: REG_IRQ_FLAGS = 0x");
    PrintSXRegContent(REG_IRQ_FLAGS);
     */
    
    WriteSXRegister(REG_IRQ_FLAGS, 0xFF);           // clear flags: writing 1 clears flag

    /*
    // check that flags are actually cleared (useless if not debugging)
    //UARTWriteStr("after clear: REG_IRQ_FLAGS = 0x");
    PrintSXRegContent(REG_IRQ_FLAGS);
     */
    
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
    WriteSXRegister(REG_OP_MODE, LORA_RX_CONTINUOUS_MODE);
    
    //UARTWriteStrLn("set mode to LoRa single RX");
    //WriteSXRegister(REG_OP_MODE, LORA_RX_SINGLE_MODE);
    __delay_ms(100);                                    // delay required to start oscillator and PLL
    //GetMode();

    // wait for valid header reception
    UARTWriteStrLn("-----------------------");
    UARTWriteStrLn("waiting for valid header");
    
    do {
        reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    } while (((reg_val & 0x10) == 0x00) && ((reg_val & 0x80) == 0x00));     // check Valid Header flag (bit n°4) and timeout (bit n°3)
    
    if ((ReadSXRegister(REG_IRQ_FLAGS) & 0x10) == 0x00) {
      //  data[MSG_POS] = TIMEOUT;
        WriteSXRegister(REG_IRQ_FLAGS, 0xFF);           // clear flags: writing 1 clears flag
        return;
    }

    //UARTWriteStrLn(" ");
    UARTWriteStrLn("valid header received");

    /* --- for debugging: display IRQ flags ---
     * UARTWriteStr("REG_IRQ_FLAGS = 0x");
     * PrintSXRegContent(REG_IRQ_FLAGS);
     * // ----------------------------------------
     */
    
    // wait for end of packet reception
    reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    while ((reg_val & 0x40) == 0x00) {                  // check Packet Reception Complete flag (bit n°6)
        reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    }
    //UARTWriteStrLn(" ");
    //UARTWriteStrLn("packet reception complete");

    /* --- for debugging: display IRQ flags ---
     * UARTWriteStr("REG_IRQ_FLAGS = 0x");
     * PrintSXRegContent(REG_IRQ_FLAGS);
     * // ----------------------------------------
     */
    
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
            /*reg_val = ReadSXRegister(REG_FIFO);       // read FIFO
            UARTWriteByteHex(reg_val);              // to display in Hexadecimal format
            UARTWriteStr(" ");
            data[i] = reg_val;*/
            /*UARTWriteByte(reg_val);                   // to send raw data: readable in terminal if ASCII code
            UARTWriteStr(" ");*/
            data[i] = ReadSXRegister(REG_FIFO);         // chargement des données dans rxMsg
            UARTWriteByteHex(data[i]);
            UARTWriteStr(" ");
        }
        UARTWriteStrLn(" ");
    }

    /*
    // display FIFO information
    UARTWriteStr("REG_FIFO_RX_CURRENT_ADDR = 0x");
    PrintSXRegContent(REG_FIFO_RX_CURRENT_ADDR);
    
    //UARTWriteStr("REG_RX_NB_BYTES = 0x");
    PrintSXRegContent(REG_RX_NB_BYTES);

    // display RSSI
    reg_val = ReadSXRegister(REG_PKT_RSSI_VALUE);
    UARTWriteStr("REG_PKT_RSSI_VALUE (decimal) = ");
    UARTWriteByteDec(reg_val);
    
    
    // reset all IRQs
    //UARTWriteStrLn(" ");
    //UARTWriteStrLn("clear flags");
     
    UARTWriteStr("before clear: REG_IRQ_FLAGS = 0x");
    PrintSXRegContent(REG_IRQ_FLAGS);
    */
    WriteSXRegister(REG_IRQ_FLAGS, 0xFF);           // clear flags: writing 1 clears flag

    /*    check that flags are actually cleared (useless if not debugging)
     * UARTWriteStr("after clear: REG_IRQ_FLAGS = 0x");
     * PrintSXRegContent(REG_IRQ_FLAGS);
    */
    //UARTWriteStrLn("-----------------------");
    
    return;
}

uint8_t hexToDec(uint8_t data) { // renvoi une valeur décimale sous forme hexadécimale (ex : 24 -> 0x24)
    
    uint8_t upperHex = data / 10;
    uint8_t lowerHex = data % 10;
    
    data = 0x10 * upperHex + 0x01 * lowerHex;
    
    return data;
}