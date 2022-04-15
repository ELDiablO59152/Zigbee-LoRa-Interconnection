/* 
 * File:   spi.c
 * Author: Fabien AMELINCK
 *
 * Created on 17 juin 2021
 */

#include "spi.h"

struct spi_ioc_transfer xfer[1];
int fd_spi;
uint8_t mode;				// SPI mode (could be 0, 1, 2 or 3)
uint32_t speed;				// SPI clock frequency
uint8_t lsb_first;			// SPI bits order
uint8_t bits;				// SPI words width (8 bits per word)

void init_spi(void) {
    
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
	

}

// creates the file corresponding to the given number in /sys/class/gpio
// arguments: portnumber = the port number
// returns: 0 = success, -1 = can't open /sys/class/gpio/export
// -2 = can't write to /sys/class/gpio/export
int create_port(int portnumber)
{
    FILE *fd;

    fd = fopen("/sys/class/gpio/export", "w");
    if (fd == NULL)
    {
        fprintf(stdout, "open export error\n");
        return -1;
    }

    if (fprintf(fd, "%d", portnumber) < 0)
    {
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
int delete_port(int portnumber)
{
    FILE *fd;

    fd = fopen("/sys/class/gpio/unexport", "w");
    if (fd == NULL)
    {
        fprintf(stdout, "open unexport error\n");
        return -1;
    }

    if (fprintf(fd, "%d", portnumber) < 0)
    {
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

int set_port_direction(int portnumber, int direction)
{
    FILE *fd;
    char pnum[255] = "";

    char filename[255] = "/sys/class/gpio/gpio";

    sprintf(pnum, "%d", portnumber);
    strcat(filename, pnum);
    strcat(filename, "/direction");

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
int set_port_value(int portnumber, int value)
{
    FILE *fd;

    char pnum[255] = "";

    char filename[255] = "/sys/class/gpio/gpio";

    sprintf(pnum, "%d", portnumber);
    strcat(filename, pnum);
    strcat(filename, "/value");

    fd = fopen(filename, "w");
    if (fd == NULL)
    {
        fprintf(stdout, "open value error\n");
        return -1;
    }

    if (fprintf(fd, "%d", value) < 0)
    {
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
int read_port_value(int portnumber)
{
    FILE *fd;
    int value;

    char pnum[255] = "";

    char filename[255] = "/sys/class/gpio/gpio";

    sprintf(pnum, "%d", portnumber);
    strcat(filename, pnum);
    strcat(filename, "/value");

    fd = fopen(filename, "r");
    if (fd == NULL)
    {
        fprintf(stdout, "open value error\n");
        return -1;
    }

    // value = fgetc(FILE * fd);
    value = fgetc(fd);

    // if (fprintf(fd, "%d", value) < 0){
    // 	fprintf(stdout, "write value error\n");
    // 	return -2;
    // }

    fclose(fd);
    return value;
}
