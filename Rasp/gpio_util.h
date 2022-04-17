/*
 * File:   gpio_util.h
 * Author: Fabien AMELINCK
 *
 * Created on 17 juin 2021
 */

#ifndef _GPIO_UTIL_H
#define _GPIO_UTIL_H

#include <stdio.h>  //FILE
#include <string.h> //strcat

int create_port(int portnumber);
int delete_port(int portnumber);
int set_port_direction(int portnumber, int direction);
int set_port_value(int portnumber, int value);
int read_port_value(int portnumber);
// int get_port_value(int portnumber, int * value);

#endif /* _GPIO_UTIL_H */