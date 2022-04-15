/* 
 * File:   spi.h
 * Author: Fabien AMELINCK
 *
 * Created on 17 juin 2021
 */

#ifndef _SPI_H
#define	_SPI_H

#include <stdio.h> //FILE
#include <stdlib.h> //EXIT
#include <stdint.h> //uint

//#include <fcntl.h>
//#include <unistd.h>
//#include <linux/types.h>
//#include <linux/spi/spidev.h>			// library used to control SPI peripheral
#include <sys/ioctl.h>
//#include <string.h>
//#include <signal.h>

void init_spi(void);
int create_port(int portnumber);
int delete_port(int portnumber);
int set_port_direction(int portnumber, int direction);
int set_port_value(int portnumber, int value);
int read_port_value(int portnumber);
//int get_port_value(int portnumber, int * value);

#endif	/* _SPI_H */