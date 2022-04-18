/* 
 * File:   sensor.h
 * Author: Fabien AMELINCK
 *
 * Created on 17 juin 2021
 */

#ifndef _SENSOR_H
#define	_SENSOR_H

uint16_t humidityHex(void);
uint16_t temperatureHex(void);
uint16_t humidityDec(void);
uint16_t temperatureDec(void);
void printTemperatureLevel(void);
void printHumidityLevel(void);
uint16_t sendMeanHumi(void);
uint16_t sendMeanTemp(void);

#endif	/* _SENSOR_H */

