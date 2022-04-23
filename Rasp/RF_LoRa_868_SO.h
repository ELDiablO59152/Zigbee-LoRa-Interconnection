/*
 * File:   RF_LoRa_868_SO.h
 * Author: Fabien AMELINCK
 *
 * Created on 04 June 2021
 */

#ifndef _RF_LORA_868_SO_H
#define _RF_LORA_868_SO_H

#include <stdint.h>    //uint
#include <unistd.h>    //usleep
#include "gpio_util.h" //reset
#include "SX1272.h"    //SXregister

//#define POUT 2 // output power (in dBm)
                // used to compute the value loaded in register REG_PA_CONFIG

void ResetModule(void);
void InitModule(const uint32_t freq, const uint8_t bw, const uint8_t sf, const uint8_t cr, const uint8_t sync, const uint8_t preamble, const uint8_t pout, const uint8_t gain, const uint8_t timeout, const uint8_t hder, const uint8_t crc);
// void InitModule(void);
void AntennaTX(void); // connect antenna to module output
void AntennaRX(void); // connect antenna to module input

#endif /* _RF_LORA_868_SO_H */