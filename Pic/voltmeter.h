/* 
 * File:   voltmeter.h
 * Author: Fabien AMELINCK
 *
 * Created on 17 juin 2021
 */

#ifndef _VOLTMETER_H
#define	_VOLTMETER_H

#include <stdint.h>

void initVoltmeter(void);
uint16_t voltmeterHex(void);
uint8_t voltmeterDec(void);
uint16_t pourcentBatt(void);
void printBatteryLevel(void);

#endif	/* _VOLTMETER_H */

