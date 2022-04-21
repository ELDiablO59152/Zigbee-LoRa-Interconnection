/*
 * File:   gpio_util.c
 * Author: Fabien AMELINCK
 *
 * Created on 13 April 2022
 */

#include "gpio_util.h"

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
