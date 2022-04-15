#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>			// library used to control SPI peripheral
#include <sys/ioctl.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include "SX1272.h"
//#include "RF_LoRa_868_SO.h"
//#include "donnees.h"
//#include "general.h"
//#include "wiringPi.h"
#include <time.h>


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

#define DataFile ./data.csv

char inbuf[10];				// data received during SPI transfer
char outbuf[10];			// data sent during SPI transfer
 
struct spi_ioc_transfer xfer[1];
int fd_spi;
uint8_t mode;				// SPI mode (could be 0, 1, 2 or 3)
uint32_t speed;				// SPI clock frequency
uint8_t lsb_first;			// SPI bits order
uint8_t bits;				// SPI words width (8 bits per word)

uint8_t NodeData[255];			// data sent by a node
	
uint8_t reg_val;			// used for temporary storage of SX1272 register content
uint8_t TxBuffer[50];			// buffer containing data to write in SX1272 FIFO before transmission
uint8_t RxBuffer[50];			// buffer containing data read from SX1272 FIFO after reception

uint8_t NodeID;
uint8_t NodeMap[MaxNodesInNetwork][3];	// to store which nodes are present in the network, and their payload length
					// (written during the discovery process)
					// 1 line per node
					// 1st column = 1 if node is present
					// 2nd column = type of sensor
					// 3rd column = length (in bytes) of data sent by a node
										
uint8_t NbBytesReceived;		// length of the received payload
					// (after reception, read from dedicated register REG_RX_NB_BYTES)
uint8_t PayloadLength;			// length of the transmitted payload
					// (before transmission, must be stored in the dedicated register REG_PAYLOAD_LENGTH_LORA)
										
uint8_t CRCError;			// returned by functions WaitIncomingMessageRXContinuous and WaitIncomingMessageRXSingle
					// 0 => no CRC error in the received message
					// 1 => CRC error in the received message

uint8_t TimeoutOccured;			// written by function WaitIncomingMessageRXSingle
					// 0 => a message was received before end of timeout (no timeout occured)
					// 1 => timeout occured before reception of any message

uint8_t DiscoveryProcessDone = 0;   	// will be set to 1 when for each node, a DISCOVERY request has been sent,
					// and a DISCOVERY reply has been listen to

uint8_t MeasurementIsPossible = 0;  	// after measurement request, a sensor should reply if the measurement is possible or not
					// If YES, this variable is set
					// If NO, this variable is cleared
					// (updated in DoAction(4), when the base listens to the sensor reply)

uint8_t RetryMeasurementRequest = 0;  	// after measurement request, if a message different from ACK or NACK is received,
					// the reply of the sensor was maybe distroyed by an interferer
					// => in this case this variable is set (in DoAction(4)) to try another measurement request after some time)

uint8_t ReceivedMeasurementData = 0;	// 1 when measurement data has been received correctly
					// will be set during measurement reception, if received message is correct
                                    
uint16_t NumberOfMeasurementsReceived = 0;


// prototypes of functions used in the main function
// (functions are described after the main function)
int create_port(int portnumber);
int delete_port(int portnumber);
int set_port_direction(int portnumber, int direction);
int set_port_value(int portnumber, int value);
int read_port_value(int portnumber);
//int get_port_value(int portnumber, int * value);
void WriteSXRegister(uint8_t address,uint8_t data);
uint8_t ReadSXRegister(uint8_t address);
void ResetModule(void);
void InitModule (void);
void AntennaTX(void);
void AntennaRX(void);
uint8_t WaitIncomingMessageRXSingle(uint8_t *PointTimeout);
uint8_t WaitIncomingMessageRXContinuous(void);
void LoadRxBufferWithRxFifo(uint8_t *Table, uint8_t *PointNbBytesReceived);
void LoadTxFifoWithTxBuffer(uint8_t *Table, uint8_t PayloadLength);
void TransmitLoRaMessage(void);

void DoAction(uint8_t ActionType);
	// 0: do nothing
	// 1: transmit a DISCOVERY request
	// 2: wait for DISCOVERY reply
	// 3: transmit a MEASUREMENT request
	// 4: wait for ACK/NACK reply after MEASUREMENT request
	// 5: wait for measurement data
	// 6: transmit ACK after measurement data reception
	// 7: transmit NACK after measurement data reception

void DeleteDataFile(void);
void CreateDataFile(void);
void WriteDataInFile(void);
void WriteResetInFile(void);

void ClearNodeMap(void);

int main(void){ 

// Open the file for SPI control
// here SPI peripheral n°0 is used
// SCK = pin n°23 (GPIO11)
// MOSI = pin n°19 (GPIO10)
// MISO = pin n°21 (GPIO9)
// nCS = pin n°24 (GPIO8)
	fd_spi = open("/dev/spidev0.0", O_RDWR);
	if (fd_spi < 0) {
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
	speed = 15000000;	// 15MHz asked

// setting bits per word
	bits = 8;

	xfer[0].cs_change = 0;			/* Keep CS activated if = 1 */
	xfer[0].delay_usecs = 0;		//delay in us
	xfer[0].speed_hz = speed;		//speed
	xfer[0].bits_per_word = bits;	// bits per word 8	
	

// Configure the pin used for RESET of LoRa transceiver
// here: physical pin n°38 (GPI20)
	create_port(20);
	set_port_direction(20,1);

// Configure the pin used for RX_SWITCH of LoRa transceiver
// here: physical pin n°29 (GPIO5)
	create_port(5);
	set_port_direction(5,0);
	set_port_value(5,0);
	
// Configure the pin used for TX_SWITCH of LoRa transceiver
// here: physical pin n°31 (GPIO6)
	create_port(6);
	set_port_direction(6,0);
	set_port_value(6,0);

// Configure the pin used for LED
// here: physical pin n°40 (GPIO21)
	create_port(21);
	set_port_direction(21,0);
	set_port_value(21,0);

	
	ResetModule();


// put module in LoRa mode (see SX1272 datasheet page 107)
    WriteSXRegister(REG_OP_MODE, FSK_SLEEP_MODE);       // SLEEP mode required first to switch from FSK to LoRa
    WriteSXRegister(REG_OP_MODE, LORA_SLEEP_MODE);      // switch from FSK mode to LoRa mode
    WriteSXRegister(REG_OP_MODE, LORA_STANDBY_MODE);    // STANDBY mode required for FIFO loading
    usleep(100000);										// delay since switching mode takes time
 
// clearing buffers
//	memset(inbuf, 0, sizeof inbuf);
//	memset(outbuf, 0, sizeof outbuf);
	

    //uint8_t count=0;

	InitModule();
	
	NodeID = 0;
	
	DeleteDataFile();
	CreateDataFile();



	while(1)
	{
		// launch DISCOVERY process
		if (DiscoveryProcessDone == 0)
		{
			ClearNodeMap();
			
			fprintf(stdout, "Start DISCOVERY process\n");
			fprintf(stdout, "*********\n");
			
			//for (uint8_t DiscoveryTry = 0; DiscoveryTry < 2; DiscoveryTry++)
			for (uint8_t DiscoveryTry = 0; DiscoveryTry < 1; DiscoveryTry++)
			// launch the DISCOVERY process several times in case LoRa messages are lost
			{
				for (NodeID = 0; NodeID < MaxNodesInNetwork; NodeID++)
				{
					set_port_value(21,1);	// switch on LED
					DoAction(1);		// send a DISCOVERY request to node n° NodeID
					set_port_value(21,0);	// switch off LED
					
					DoAction(2);		// wait reply and fill table NodeMap
					
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
				usleep(1000000);		// wait for 1 second
			}
			
			 DiscoveryProcessDone = 1;		
		}
		
		DiscoveryProcessDone = 0;	// reset DiscoveryProcessDone in order to launch again DISCOVERY sequence after MEASUREMENT sequence
		
		// launch MEASUREMENT process
		fprintf(stdout, "Start MEASUREMENT process\n");
		fprintf(stdout, "*********\n");
			
		for (NodeID = 0; NodeID < MaxNodesInNetwork; NodeID++)
		{
			if (NodeMap[NodeID][0] != 0)      // for each Node discovered in the network
			{

				DoAction(3);	// send a MEASUREMENT request to node n° NodeID
				DoAction(4);    // wait ACK/NACK for request

				if (RetryMeasurementRequest)	// variable updated in DoAction(4), whatever the situation
									// timeout => no retry (sensor may be offline)
									// packet with wrong length received => retry
									// packet with correct length + ACK received => no retry
									// packet with correct length + NACK received => no retry
									// packet with correct length + neither ACK or NACK received => retry
				{
					// wait a little (10 seconds) before retry
					for (uint16_t WaitSeconds = 0; WaitSeconds < 10; WaitSeconds++)
					{
						usleep(1000000);		// wait for 1 second
					}
					fprintf(stdout, "Resend a MEASUREMENT request (2nd try)\n");
					fprintf(stdout, "*********\n");
					DoAction(3);	// resend a MEASUREMENT request
					DoAction(4);    // wait ACK/NACK for request
				}
          
				if (MeasurementIsPossible)		// variable updated in DoAction(4), whatever the situation
										// ACK received	=> MeasurementIsPossible = 1
										// otherwise	=> MeasurementIsPossible = 0
				{
					ReceivedMeasurementData = 0;
					DoAction(5);    // receive measurement data (1st try)

						if (ReceivedMeasurementData == 1)	// variable updated in DoAction(5), whatever the situation
												// correct message received	=> ReceivedMeasurementData = 1
												// timeout	=> ReceivedMeasurementData = 0
												// message with bad length	=> ReceivedMeasurementData = 0
												// message with correct length but wrong header => ReceivedMeasurementData = 0
						{
							DoAction(6);    // send ACK for measurement reception
						}
						else 	// data was not received correctly
							// - NACK will be sent
							// - the base will wait for data reception
							// - if data is received correctly, ACK will be sent
						{
							DoAction(7);    // send NACK for measurement reception
							fprintf(stdout, "Waiting measurement data (2nd try)\n");
							fprintf(stdout, "*********\n");
							DoAction(5);    // receive measurement data (2nd try)
							
							if (ReceivedMeasurementData == 1)
							{
								DoAction(6);    // send ACK for measurement reception
							}
							else
							{
								DoAction(7);    // send NACK for measurement reception
							}
							
						}
						
						if (ReceivedMeasurementData == 1)
						{
							// Display received data
							fprintf(stdout, "Number of measurements received: %d\n", NumberOfMeasurementsReceived);
							fprintf(stdout, "*********\n");
							
							if (NodeID == 0)
							{
								fprintf(stdout, "Data received from node %d:\n", NodeID);
								fprintf(stdout, "Hygrometry: %X.%X, %X.%X, %X.%X, %X.%X, %X.%X, %X.%X\n", NodeData[0], NodeData[1],NodeData[2],NodeData[3],NodeData[4],NodeData[5],NodeData[6], NodeData[7],NodeData[8],NodeData[9],NodeData[10],NodeData[11]);
								fprintf(stdout, "Temperature: %X.%X, %X.%X, %X.%X, %X.%X, %X.%X, %X.%X\n", NodeData[12], NodeData[13],NodeData[14],NodeData[15],NodeData[16],NodeData[17],NodeData[18], NodeData[19],NodeData[20],NodeData[21],NodeData[22],NodeData[23]);
								fprintf(stdout, "Battery Voltage: %X\n", NodeData[24]);
								fprintf(stdout, "*********\n");
								// WriteDataInFile();
							}
							else
							{
								fprintf(stdout, "Data received from node %d:\n", NodeID);
								for (uint8_t x = 0; x < (NbBytesReceived - 4); x++)
								{
									fprintf(stdout, "Ox%X ", NodeData[x]);
								}
								fprintf(stdout, "\n");
								fprintf(stdout, "*********\n");
							}
						
							WriteDataInFile();
						
						}
						
						
				}
          
					// wait before next measurement
					for (uint16_t WaitSeconds = 0; WaitSeconds < 2; WaitSeconds++)
					{
						usleep(1000000);		// wait for 1 second
					}
                  
			}		// end of "for each Node discovered in the network"
      
		}			// end of "for each  Node from 0 to MaxNodesInNetwork"		
		
		
		
		
		
		
		// wait before next  sequence
		fprintf(stdout, "*********\n");
		fprintf(stdout, "Wait and start a new DISCOVERY + MEASUREMENT sequence\n");
		fprintf(stdout, "*********\n");
		for (uint16_t WaitSeconds = 0; WaitSeconds < 300; WaitSeconds++)
		{
			usleep(1000000);		// wait for 1 second
		}
		
		
		

	}	// end while(1)
    
}	// end main

 
//*************************************************************************
//************************  FUNCTIONS  ************************************
//*************************************************************************

// creates the file corresponding to the given number in /sys/class/gpio
// arguments: portnumber = the port number
// returns: 0 = success, -1 = can't open /sys/class/gpio/export
// -2 = can't write to /sys/class/gpio/export
int create_port(int portnumber){
FILE *fd;

fd = fopen("/sys/class/gpio/export", "w");
if (fd == NULL){
	fprintf(stdout, "open export error\n");
	return -1;
}

if (fprintf(fd, "%d",  portnumber) < 0){
	fprintf(stdout, "write export error\n");
	return -2;
}

fclose(fd);
return 0;
}

// deletes the file corresponding to the given number in /sys/class/gpio
// arguments: portnumber = the port number
// returns: 0 = success, -1 = can't open /sys/class/gpio/unexport
// -2 = can't write to /sys/class/gpio/unexport
int delete_port(int portnumber){
FILE *fd;


fd = fopen("/sys/class/gpio/unexport", "w");
if (fd == NULL){
	fprintf(stdout, "open unexport error\n");
	return -1;
}

if (fprintf(fd, "%d", portnumber) < 0){
	fprintf(stdout, "write unexport error\n");
	return -2;
}

fclose(fd);
return 0;
}

// Determines the direction  of the port given 
// arguments: portnumlber = the port number, direction = 1 (in) | 0 (out)
// returns: 0 = success, -1 = can't open /sys/class/gpioXX/direction
// -2 = can't write to /sys/class/gpioXX/direction

int set_port_direction(int portnumber, int direction){
FILE *fd;
char pnum[255]="";

char  filename[255]="/sys/class/gpio/gpio";

sprintf(pnum, "%d",portnumber);
strcat(filename, pnum);
strcat(filename,"/direction");


fd = fopen(filename, "w");
if (fd == NULL)
{
	fprintf(stdout, "open direction error\n");
	return -1;
}

if (direction == 1)
{
	fprintf(fd, "%s", "in");
}
else
{
	fprintf(fd, "%s", "out");
}

fclose(fd);
return 0;
}

// Sets the value  of the port given if configured as output
// arguments: portnumlber = the port number, value = 0|1
// returns: 0 = success, -1 = can't open /sys/class/gpioXX/value
// -2 = can't write to /sys/class/gpioXX/value
int set_port_value(int portnumber, int value){
FILE *fd;

char pnum[255]="";

char  filename[255]="/sys/class/gpio/gpio";

sprintf(pnum, "%d",portnumber);
strcat(filename, pnum);
strcat(filename,"/value");

fd = fopen(filename, "w");
if (fd == NULL){
	fprintf(stdout, "open value error\n");
	return -1;
}

if (fprintf(fd, "%d", value) < 0){
	fprintf(stdout, "write value error\n");
	return -2;
}

fclose(fd);
return 0;
}

// Reads the value  of the port given if configured as input
// arguments: portnumlber = the port number
// returns: 0 = success, -1 = can't open /sys/class/gpioXX/value
// -2 = can't write to /sys/class/gpioXX/value
int read_port_value(int portnumber){
FILE *fd;
int value;

char pnum[255]="";

char  filename[255]="/sys/class/gpio/gpio";

sprintf(pnum, "%d",portnumber);
strcat(filename, pnum);
strcat(filename,"/value");

fd = fopen(filename, "r");
if (fd == NULL){
	fprintf(stdout, "open value error\n");
	return -1;
}

//value = fgetc(FILE * fd);
value = fgetc(fd);

// if (fprintf(fd, "%d", value) < 0){
// 	fprintf(stdout, "write value error\n");
// 	return -2;
// }

fclose(fd);
return value;
}





// function used to write in SX1272 registers
void WriteSXRegister(uint8_t address,uint8_t data)
{
	address = address | 0x80;					// set MSB (required to write SX1272 registers)
    outbuf[0] = address;
    outbuf[1] = data;
    
	xfer[0].rx_buf = (unsigned long) inbuf;	
	xfer[0].tx_buf = (unsigned long)outbuf;
	xfer[0].len = 2; 							// number of bytes to send
    
    if(ioctl(fd_spi, SPI_IOC_MESSAGE(1), xfer) < 0) {
        perror("SPI_IOC_MESSAGE");
    }
}

// function used to read the content of SX1272 registers
uint8_t ReadSXRegister(uint8_t address)
{
    address = address & 0x7F;					// clear MSB (required to read SX1272 registers)
    //usleep(100000);							// ligne commentée par JMC
    
    outbuf[0] = address;
    outbuf[1] = 0x00;							// dummy byte sent to receive data
    
	xfer[0].rx_buf = (unsigned long) inbuf;	
	xfer[0].tx_buf = (unsigned long)outbuf;
	xfer[0].len = 2; 							// number of bytes to send
    
    if(ioctl(fd_spi, SPI_IOC_MESSAGE(1), xfer) < 0) {
        perror("SPI_IOC_MESSAGE");
    }
    //fprintf(stdout, "rx0 = %d\n", inbuf[0]);	// may be useful for debug
    //fprintf(stdout, "rx1 = %d\n", inbuf[1]);
    return inbuf[1];
 }

void ResetModule(void)
{
  set_port_direction(20,0);		// switch direction to output (low impedance)
  set_port_value(20,1);			// pull output high
  usleep(5000);				// wait for 5 ms  (SX1272 datasheet: 100 us min)
  set_port_direction(20,1);		// switch direction to input (high impedance)
  usleep(50000);			// wait for 50 ms  (SX1272 datasheet: 5 ms min)
}

void InitModule(void)
{
   uint8_t pout;
    
  WriteSXRegister(REG_FIFO, 0x00);
  
/*
  WriteSXRegister(REG_FRF_MSB, 0xD8);			// channel 10, central freq = 865.20MHz
  WriteSXRegister(REG_FRF_MID, 0x4C);
  WriteSXRegister(REG_FRF_LSB, 0xCC); 
*/  
  WriteSXRegister(REG_FRF_MSB, 0xD9);			// channel 16, central freq = 868.00MHz
  WriteSXRegister(REG_FRF_MID, 0x00);
  WriteSXRegister(REG_FRF_LSB, 0x00); 

  // write register REG_PA_CONFIG to select pin PA-BOOST for power amplifier output (power limited to 20 dBm = 100 mW)
  pout = (POUT - 2) & 0x0F;				// compute pout and keep 4 LSBs (POUT is defined in file SX1272.h)
  WriteSXRegister(REG_PA_CONFIG, 0x80 | pout);		// out=PA_BOOST

  WriteSXRegister(REG_PA_RAMP, 0x19);			// low cons PLL TX&RX, 40us

  WriteSXRegister(REG_OCP, 0b00101011);			// OCP enabled, 100mA

  WriteSXRegister(REG_LNA, 0b00100011);			// max gain, BOOST on

  WriteSXRegister(REG_FIFO_ADDR_PTR, 0x00);		// pointer to access FIFO through SPI port (read or write)
  WriteSXRegister(REG_FIFO_TX_BASE_ADDR, 0x80);		// top half of the FIFO
  WriteSXRegister(REG_FIFO_RX_BASE_ADDR, 0x00);		// bottom half of the FIFO

  WriteSXRegister(REG_IRQ_FLAGS_MASK, 0x00);		// activate all IRQs

  WriteSXRegister(REG_IRQ_FLAGS, 0xFF);			// clear all IRQs

  // in Explicit Header mode, CRC enable or disable is not relevant in case of RX operation: everything depends on TX configuration
  //WriteSXRegister(REG_MODEM_CONFIG1, 0b10001000); 	//BW=500k, CR=4/5, explicit header, CRC disable, LDRO disabled
  //writeRegister(REG_MODEM_CONFIG1, 0b10001010);	//BW=500k, CR=4/5, explicit header, CRC enable, LDRO disabled
  WriteSXRegister(REG_MODEM_CONFIG1, 0b00100011);	//BW=125k, CR=4/8, explicit header, CRC enable, LDRO enabled (mandatory with SF=12 and BW=125 kHz)

  //WriteSXRegister(REG_MODEM_CONFIG2, 0b11000111);	// SF=12, normal TX mode, AGC auto on, RX timeout MSB = 11
  //WriteSXRegister(REG_MODEM_CONFIG2, 0b11000101);	// SF=12, normal TX mode, AGC auto on, RX timeout MSB = 01
  WriteSXRegister(REG_MODEM_CONFIG2, 0b11000100); 	// SF=12, normal TX mode, AGC auto on, RX timeout MSB = 00

  WriteSXRegister(REG_SYMB_TIMEOUT_LSB, 0xFF);		// max timeout (used in mode Receive Single)
  // timeout = REG_SYMB_TIMEOUT x Tsymbol    where    Tsymbol = 2^SF / BW
  //                                                  Tsymbol = 2^12 / 125k = 32.7 ms
  // REG_SYMB_TIMEOUT = 0x2FF = 1023   =>   timeout = 33.5 seconds
  // REG_SYMB_TIMEOUT = 0x1FF = 511    =>   timeout = 16.7 seconds
  // REG_SYMB_TIMEOUT = 0x0FF = 255    =>   timeout = 8.3 seconds

  WriteSXRegister(REG_PREAMBLE_MSB_LORA, 0x00);		// default value
  WriteSXRegister(REG_PREAMBLE_LSB_LORA, 0x08);

  WriteSXRegister(REG_MAX_PAYLOAD_LENGTH, 0x80);	// half the FIFO

  WriteSXRegister(REG_HOP_PERIOD, 0x00);		// freq hopping disabled

  WriteSXRegister(REG_DETECT_OPTIMIZE, 0xC3);		// required value for SF=12

  WriteSXRegister(REG_INVERT_IQ, 0x27);			// default value, IQ not inverted

  WriteSXRegister(REG_DETECTION_THRESHOLD, 0x0A);	// required value for SF=12

  WriteSXRegister(REG_SYNC_WORD, 0x12);			// default value
}

void AntennaTX(void)
{
	set_port_value(5,0);		// clear RX_SWITCH
	set_port_value(6,0);		// clear TX_SWITCH
	usleep(10000);
	set_port_value(6,1);		// set TX_SWITCH
}

void AntennaRX(void)
{
	set_port_value(5,0);		// clear RX_SWITCH
	set_port_value(6,0);		// clear TX_SWITCH
	usleep(10000);
	set_port_value(5,1);		// set RX_SWITCH
}

uint8_t WaitIncomingMessageRXSingle(uint8_t *PointTimeout)
{
  uint8_t CRCError = 0;	// cleared when there is no CRC error, set to 1 otherwise
  uint8_t reg_val;	// to store temporarily data read from SX1272 register
  int tempo = 0;	// variable used to set the maximum time for polling flags

  AntennaRX();        // connect antenna to receiver

  WriteSXRegister(REG_IRQ_FLAGS, 0xff);           // clear IRQ flags
  
  // 1 - set mode to RX single
  WriteSXRegister(REG_OP_MODE, LORA_RX_SINGLE_MODE);
  fprintf(stdout, "Waiting for incoming message (RX single mode)\n");

  // 2 - wait reception of a complete packet or end of timeout
  reg_val = ReadSXRegister(REG_IRQ_FLAGS);
  while ((reg_val & 0b11000000) == 0 && (tempo <  1500000))		// check bits n°7 (timeout) and n°6 (reception complete)
									// with a maximum duration
  {
	reg_val = ReadSXRegister(REG_IRQ_FLAGS);
	++ tempo;
  }
  
  // for debugging
  fprintf(stdout, "IRQ flags: 0x%X\n", reg_val);
  fprintf(stdout, "tempo = %d\n", tempo);
  // ********************************
  
  WriteSXRegister(REG_IRQ_FLAGS, 0xff);           // clear IRQ flags
  
  if ((tempo > 10) && (tempo < 1500000))	// RX timeout or Rx done occured
						// after a "reasonable" time
						// (not too short, not too long)
  {  
	// 3 - check if timeout occured
	if ((reg_val & 0x80) != 0)                    // timeout occured
	{
		*PointTimeout = 1;                          // set TimeoutOccured to 1
		CRCError = 0;                               // no CRC error (not relevant here)
		fprintf(stdout, "Timeout occured\n");
	}
	else                                          // a message was received before timeout
	{
		*PointTimeout = 0;                          // clear TimeoutOccured
		fprintf(stdout, "Message received\n");
		// check CRC
		if ((reg_val & 0x20) == 0x00)
		{
			CRCError = 0;   // no CRC error
			fprintf(stdout, "CRC checking => no CRC error\n");      
		}
		else
		{
			CRCError = 1;   // CRC error
			fprintf(stdout, "CRC checking => CRC error\n");
		}
	}
  }
  else 		// maximum duration exceeded
  {
	// Reset and re-initialize LoRa module
	ResetModule();
	WriteSXRegister(REG_OP_MODE, FSK_SLEEP_MODE);
	WriteSXRegister(REG_OP_MODE, LORA_SLEEP_MODE);
	WriteSXRegister(REG_OP_MODE, LORA_STANDBY_MODE);
	usleep(100000);
	InitModule();
	
	fprintf(stdout, "LoRa module was re-initialized\n");
	WriteResetInFile();
	  
  }
  
  return CRCError;
}

uint8_t WaitIncomingMessageRXContinuous(void)
{
  uint8_t CRCError;   // cleared when there is no CRC error, set to 1 otherwise
  uint8_t reg_val;    // to store temporarily data read from SX1272 register
  
  AntennaRX();        // connect antenna to receiver
      
  // 1 - set mode to RX continuous
  WriteSXRegister(REG_OP_MODE, LORA_RX_CONTINUOUS_MODE);
  fprintf(stdout, "Waiting for incoming message (RX continuous mode)\n");

  // 2 - wait reception of a complete message
  reg_val = ReadSXRegister(REG_IRQ_FLAGS);
  while ((reg_val & 0x40) == 0x00)                // check bit n°6 (reception complete)
  { 
    reg_val = ReadSXRegister(REG_IRQ_FLAGS);
  }
  fprintf(stdout, "Message received\n");

  // 3 - check CRC
  if ((reg_val & 0x20) == 0x00)
  {
    CRCError = 0;   // no CRC error
    fprintf(stdout, "CRC checking => no CRC error\n");
  }
  else
  {
    CRCError = 1;   // CRC error
	fprintf(stdout, "CRC checking => CRC error\n");
  }

  WriteSXRegister(REG_IRQ_FLAGS, 0xff);     // clear IRQ flags

  return CRCError;
}

void TransmitLoRaMessage(void)
{
  AntennaTX();  // connect antenna to transmitter
  
  int tempo = 0;				// variable used to set the maximum time for polling TXDone flag
  
  WriteSXRegister(REG_OP_MODE, LORA_TX_MODE);     // set mode to TX

  // wait end of transmission
  reg_val = ReadSXRegister(REG_IRQ_FLAGS);
  while (((reg_val & 0x08) == 0x00) && (tempo <  300000))	// check bit n°3 (TX done)
								// and check if timeout is not reached
								// typically, when a message of 5 bytes is transmitted, tempo ~ 30 000 when done
								//            in single receive mode with timeout ~ 8 sec, tempo ~ 300 000 after timeout
								// so, in TX mode, tempo = 300 000 ~ 8 sec ~ 50 tranmitted bytes
  {
    reg_val = ReadSXRegister(REG_IRQ_FLAGS);
    ++ tempo;
  }
  
  if (tempo < 300000)
  {
	fprintf(stdout, "Transmission done\n");
  }
  else
  {
	WriteSXRegister(REG_OP_MODE, LORA_STANDBY_MODE);    // abort TX mode
	fprintf(stdout, "Transmission canceled\n");
  }

  fprintf(stdout, "tempo = %d\n", tempo);
  fprintf(stdout, "*********\n");
  
  
  
  WriteSXRegister(REG_IRQ_FLAGS, 0xff);     // clear IRQ flags
}

void LoadRxBufferWithRxFifo(uint8_t *Table, uint8_t *PointNbBytesReceived)
// the function receives the adresses of RxBuffer and NbBytesReceived and stores them in the pointers "table" and "pointNbBytesReceived"
{
  WriteSXRegister(REG_FIFO_ADDR_PTR, ReadSXRegister(REG_FIFO_RX_CURRENT_ADDR));	// fetch REG_FIFO_RX_CURRENT_ADDR and copy it in REG_FIFO_ADDR_PTR
  *PointNbBytesReceived = ReadSXRegister(REG_RX_NB_BYTES);			// fetch REG_RX_NB_BYTES and copy it in NbBytesReceived
  // Display number of bytes received
  fprintf(stdout, "Received %d bytes\n", *PointNbBytesReceived);

      
  for (uint8_t i = 0; i < *PointNbBytesReceived; i++)
  {
    Table[i] = ReadSXRegister(REG_FIFO);
    // Display received data
    fprintf(stdout, "i = %d  data = 0x%X\n", i, Table[i]);
  }
  fprintf(stdout, "*********\n");
}

void LoadTxFifoWithTxBuffer(uint8_t *Table, uint8_t PayloadLength)
{
  WriteSXRegister(REG_FIFO_ADDR_PTR, ReadSXRegister(REG_FIFO_TX_BASE_ADDR));	// fetch REG_FIFO_TX_BASE_ADDR and copy it in REG_FIFO_ADDR_PTR
  WriteSXRegister(REG_PAYLOAD_LENGTH_LORA, PayloadLength);                        // copy value of PayloadLength in REG_PAYLOAD_LENGTH_LORA
  // Display number of bytes to send
  fprintf(stdout, "Ready to send %d bytes\n", PayloadLength);
  
  for (uint8_t i = 0; i < PayloadLength; i++)
  {
    WriteSXRegister(REG_FIFO, Table[i]);
    // Display data to transmit
    fprintf(stdout, "i = %d  data = 0x%X\n", i, Table[i]);
  }
  fprintf(stdout, "*********\n");
}


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
	
	case 1:		// transmit a DISCOVERY command to node n° NodeID
				// **********************************************

		// 1 - load in FIFO data to transmit
		PayloadLength = 5;
		TxBuffer[0] = Header0;
		TxBuffer[1] = Header1;
		TxBuffer[2] = NetID;
		TxBuffer[3] = NodeID;
		TxBuffer[4] = CommandDiscovery;

		LoadTxFifoWithTxBuffer(TxBuffer, PayloadLength);	// address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                                            // in order to read the values of their content and copy them in SX1272 registers

		// 2 - switch to TX mode to transmit data
		TransmitLoRaMessage();
      
		fprintf(stdout, "DISCOVERY command sent to node %d\n", NodeID);
		fprintf(stdout, "*********\n");
	
		break;
	
	case 2:     // wait DISCOVERY reply from node n° NodeID
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
			LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived);	// addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
										// in order to update the values of their content
			// 3.2 - ckeck ik packet length is correct (should be 6 bytes long)
			if (NbBytesReceived != 6)
			{
				fprintf(stdout, "Received a message with bad length\n");
				fprintf(stdout, "*********\n");
			}
			else
			{
				if ( (RxBuffer[0] == Header1) && (RxBuffer[1] == Header0) && (RxBuffer[2] == NetID) && (RxBuffer[3] == NodeID) )
				// message is a correct DISCOVERY reply
				{
					fprintf(stdout, "Node %d is a sensor of type %d\n", NodeID, RxBuffer[4]);
					fprintf(stdout, "with a payload length equal to %d bytes\n", RxBuffer[5]);
					fprintf(stdout, "*********\n");
					
					NodeMap[NodeID][0] = 1;			// write in table NodeMap that node is present
					NodeMap[NodeID][1] = RxBuffer[4];	// write in table NodeMap the type of sensor
					NodeMap[NodeID][2] = RxBuffer[5];	// write in table NodeMap the length of the payload
				}
				else
				{
					fprintf(stdout, "Didn't receive a correct DISCOVERY reply from node %d\n", NodeID);
					fprintf(stdout, "*********\n");
				}
			}
		}

		break;
	
	case 3:		// transmit a MEASUREMENT request to node n° NodeID
				// **********************************************

		// 1 - load in FIFO data to transmit
		PayloadLength = 5;
		TxBuffer[0] = Header0;
		TxBuffer[1] = Header1;
		TxBuffer[2] = NetID;
		TxBuffer[3] = NodeID;
		TxBuffer[4] = CommandMeasurement;

		LoadTxFifoWithTxBuffer(TxBuffer, PayloadLength);		// address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
                                                            // in order to read the values of their content and copy them in SX1272 registers

		// 2 - switch to TX mode to transmit data
		TransmitLoRaMessage();
      
		fprintf(stdout, "MEASUREMENT command sent to node %d\n", NodeID);
		fprintf(stdout, "*********\n");

		break;
	
	case 4:		// wait ACK/NACK reply after MEASUREMENT request
                // *********************************************
		fprintf(stdout, "listening for ACK/NACK reply of node %d\n", NodeID);
		fprintf(stdout, "*********\n");
		
		// 1 - set mode to RX Single and wait incoming packet
		CRCError = WaitIncomingMessageRXSingle(&TimeoutOccured);

		// 2 - check if message was received
		if(TimeoutOccured)
		{
			fprintf(stdout, "No message received from node %d\n", NodeID);
			fprintf(stdout, "*********\n");

			MeasurementIsPossible = 0;
			RetryMeasurementRequest = 0;    // don't retry now: the sensor may be offline
		}
		else
		// 3 - if yes, check the message
		{
			// 3.1 - fetch data and copy it in RxBuffer
			LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived);	// addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
										// in order to update the values of their content
			// 3.2 - ckeck ik message length is correct (should be 5 bytes long)
			if (NbBytesReceived != 5)
			{
				fprintf(stdout, "Received a message with bad length\n");
				fprintf(stdout, "Measurement request will be sent again\n");
				fprintf(stdout, "*********\n");
				
				MeasurementIsPossible = 0;
				RetryMeasurementRequest = 1;			// the sensor reply may have been destroyed by an interferer
										// => the measurement request will be resent
			}
			else 	// message length is correct
			{
				if ( (RxBuffer[0] == Header1) && (RxBuffer[1] == Header0) && (RxBuffer[2] == NetID) && (RxBuffer[3] == NodeID))
				// message seems to have a consistent content
				{
					if (RxBuffer[4] == ACKMeasurementRequest)			// received ACK
					{
						fprintf(stdout, "Received ACK MEASUREMENT from node %d\n", NodeID);
						fprintf(stdout, "*********\n");
						
						MeasurementIsPossible = 1;
						RetryMeasurementRequest = 0;
					}
					else if (RxBuffer[4] == NACKMeasurementRequest)		// received NACK
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
						RetryMeasurementRequest = 1;	// the sensor reply may have been destroyed by an interferer
														// => the measurement request will be resent
					}
            
				}
				else
				{
					fprintf(stdout, "Received a MEASUREMENT reply with correct length but not consistent content\n");
					fprintf(stdout, "Measurement request will be sent again\n");
					fprintf(stdout, "*********\n");
					
					MeasurementIsPossible = 0;
					RetryMeasurementRequest = 1;		// the sensor reply may have been destroyed by an interferer
										// => the measurement request will be resent
				}
			}
		}
	
		break;
	
	case 5:		// receive measurement data
                // *********************************************
		fprintf(stdout, "Waiting for measurement data of node %d\n", NodeID);
		fprintf(stdout, "*********\n");
      
		// 1 - set mode to RX Single and wait incoming message
		CRCError = WaitIncomingMessageRXSingle(&TimeoutOccured);

		// 2 - check if a message was received
		if(TimeoutOccured)
		{
			fprintf(stdout, "No measurement data received from node %d\n", NodeID);
			fprintf(stdout, "*********\n");

			ReceivedMeasurementData = 0;
		}
		else
		// 3 - if yes, check the message
		{
			// 3.1 - fetch data and copy it in RxBuffer
			LoadRxBufferWithRxFifo(RxBuffer, &NbBytesReceived);	// addresses of RxBuffer and NbBytesReceived are passed to function LoadRxBufferWithRxFifo
										// in order to update the values of their content
			// 3.2 - check ik packet length is correct
			//       this should be the payload length stored in NodeMap during DISCOVERY sequence, added with 4 bytes for Header0, Header1, NetID, NodeID
			if (NbBytesReceived != (NodeMap[NodeID][2] + 4))
			{
				fprintf(stdout, "Received a message with bad length\n");
				fprintf(stdout, "*********\n");
				
				ReceivedMeasurementData = 0;
			}
			else 	// message length is correct
			{
				if ( (RxBuffer[0] == Header1) && (RxBuffer[1] == Header0) && (RxBuffer[2] == NetID) && (RxBuffer[3] == NodeID))
				// message is actually a measurement data
				{
					fprintf(stdout, "Received measurement data from node %d\n", NodeID);
					fprintf(stdout, "*********\n");
					
					// store measurement data
					for (uint8_t i = 0; i < NbBytesReceived; i++)
					{
						NodeData[i] = RxBuffer[i+4];
					}

					ReceivedMeasurementData = 1;
            
					// increment NumberOfMeasurementsReceived: displaying this variable helps checking correct operation
					++NumberOfMeasurementsReceived;
				}
				else 	// message length is correct but the content is not
				{
					fprintf(stdout, "Didn't receive a correct measurement data from node %d\n", NodeID);
					fprintf(stdout, "*********\n");
				
					ReceivedMeasurementData = 0;
				}
			}
		}
	
		break;
	
	
	case 6:		// send ACK for measurement reception
                // **********************************

		// 1 - load in FIFO data to transmit
		PayloadLength = 5;
		TxBuffer[0] = Header0;
		TxBuffer[1] = Header1;
		TxBuffer[2] = NetID;
		TxBuffer[3] = NodeID;
		TxBuffer[4] = ACKMeasurementReception;

		LoadTxFifoWithTxBuffer(TxBuffer, PayloadLength);	// address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
									// in order to read the values of their content and copy them in SX1272 registers

		// 2 - switch to TX mode to transmit data
		TransmitLoRaMessage();
		
		fprintf(stdout, "MEASUREMENT RECEPTION ACK sent to node %d\n", NodeID);
		fprintf(stdout, "*********\n");
	
		break;
	
	case 7:     // send NACK for measurement reception
                // **********************************

		// 1 - load in FIFO data to transmit
		PayloadLength = 5;
		TxBuffer[0] = Header0;
		TxBuffer[1] = Header1;
		TxBuffer[2] = NetID;
		TxBuffer[3] = NodeID;
		TxBuffer[4] = NACKMeasurementReception;

		LoadTxFifoWithTxBuffer(TxBuffer, PayloadLength);	// address of TxBuffer and value of PayloadLength are passed to function LoadTxFifoWithTxBuffer
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

}		// end of DoAction description

void ClearNodeMap(void)
{
	for (NodeID = 0; NodeID < MaxNodesInNetwork; NodeID++)
			{
				NodeMap[NodeID][0] = 0;
				NodeMap[NodeID][1] = 0;
				NodeMap[NodeID][2] = 0;
			}
}

void DeleteDataFile(void)
{
	int del = remove("./data.csv");
	
	if (!del)
	{
		printf("Mesurement data file deleted\n");
	}
	else
	{
		printf("Mesurement data file couldn't be deleted\n");
	}
}


void CreateDataFile(void)
{
	FILE *df;
	//df = fopen("DataFile", "a+");
	df = fopen("./data.csv", "a+");
	
	
	if (df == NULL)
	{
		printf("Unable to create data file\n");
		exit(EXIT_FAILURE);
	}
	
	fprintf(df, "Date;Time;Network_ID;Node_ID;Data;\n");
//	fprintf(df, "Hygrometry[0];Hygrometry[1];Hygrometry[2];Hygrometry[3];Hygrometry[4];Hygrometry[5];Hygrometry[6];Hygrometry[7];Hygrometry[8];Hygrometry[9];Hygrometry[10];Hygrometry[11];");
//	fprintf(df, "Temperature[0];Temperature[1];Temperature[2];Temperature[3];Temperature[4];Temperature[5];Temperature[6];Temperature[7];Temperature[8];Temperature[9];Temperature[10];Temperature[11];");
//	fprintf(df, "Battery_Voltage;\n");
	
	//fprintf(df, "Date;Time;Network ID;Node ID;Sensor ID;Hygrometry[0];Hygrometry[1];Hygrometry[2];Hygrometry[3];Hygrometry[4];Hygrometry[5];Hygrometry[6];Hygrometry[7];Hygrometry[8];Hygrometry[9];Hygrometry[10];Hygrometry[11];Temperature[0];Temperature[1];Temperature[2];Temperature[3];Temperature[4];Temperature[5];Temperature[6];Temperature[7];Temperature[8];Temperature[9];Temperature[10];Temperature[11];Battery Voltage;");
	//fprintf(df, "%c",10);		// add Line Feed in the file
	// note: \n could be used in the 1st fprintf line
	
	fclose(df);
}

void WriteDataInFile(void)
{
    FILE *df;
	
	//df = fopen("DataFile", "a+");
	df = fopen("./data.csv", "a+");
	
	if (df == NULL)
	{
		printf("Unable to open data file\n");
		exit(EXIT_FAILURE);
	}

	int hour, min, sec, day, month, year;
  	time_t now;
	char date[255]="";
    
	char current_date = time(&now);				// returns the current date (unused here)
	printf("%s", ctime(&now));
	struct tm *local = localtime(&now);			// converts to local time

	hour = local->tm_hour;        
	min = local->tm_min;       
	sec = local->tm_sec;       
	day = local->tm_mday;          
	month = local->tm_mon + 1;     
	year = local->tm_year + 1900;  

	date[0]=day;
	date[1]=month;
	date[2]=year;
	date[3]=hour;
	date[4]=min;
	date[5]=sec;
	

	fprintf(df, "%02d/%02d/%d; ", day, month, year);
	fprintf(df, "%02d:%02d:%02d; ", hour, min, sec);
	fprintf(df, "%02d; %02d; ", NetID, NodeID);
	
	for (uint8_t x = 0; x < (NbBytesReceived - 4); x++)
	{
		fprintf(df, "0x%X; ", NodeData[x]);
	}
	fprintf(df, "\n");

/*	
	for (uint8_t i = 0; i < 12; i++)
	{
		fprintf(df, "%X;", Hygrometry[i]);
	}
	for (uint8_t i = 0; i < 12; i++)
	{
		fprintf(df, "%X;", Temperature[i]);
	}
	fprintf(df, "%X\n", BatteryVoltage);
*/	
	fclose(df);
}

void WriteResetInFile(void)
{
    FILE *df;
	
	//df = fopen("DataFile", "a+");
	df = fopen("./data.csv", "a+");
	
	if (df == NULL)
	{
		printf("Unable to open data file\n");
		exit(EXIT_FAILURE);
	}

	int hour, min, sec, day, month, year;
  	time_t now;
	char date[255]="";
    
	char current_date = time(&now);				// returns the current date (unused here)
	printf("%s", ctime(&now));
	struct tm *local = localtime(&now);			// converts to local time

	hour = local->tm_hour;        
	min = local->tm_min;       
	sec = local->tm_sec;       
	day = local->tm_mday;          
	month = local->tm_mon + 1;     
	year = local->tm_year + 1900;  

	date[0]=day;
	date[1]=month;
	date[2]=year;
	date[3]=hour;
	date[4]=min;
	date[5]=sec;
	

	fprintf(df, "%02d/%02d/%d; ", day, month, year);
	fprintf(df, "%02d:%02d:%02d; ", hour, min, sec);
	fprintf(df, "LORa module reset\n");
	
	fclose(df);
}

