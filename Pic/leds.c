/*
 * File:   leds.c
 * Author: Fabien AMELINCK
 *
 * Created on 04 June 2021
 */

#include <stdint.h>
#include <xc.h>
#include "general.h"
#include "leds.h"

void initLeds(void) {
    RED_DIR = OUTP;
    GREEN_DIR = OUTP;
    BLUE_DIR = OUTP;
    SMALL_RED_DIR = OUTP;
    
    RED_LAT = OFF;
    GREEN_LAT = OFF;
    BLUE_LAT = OFF;
    eteindre_rouge_solo();
    
}

void allumer_rouge(void) {
    RED_LAT = ON;
    GREEN_LAT = OFF;
    BLUE_LAT = OFF;
    
    return;
}

void allumer_bleu(void) {
    RED_LAT = OFF;
    GREEN_LAT = OFF;
    BLUE_LAT = ON;
    
    return;
}

void allumer_vert(void) {
    RED_LAT = OFF;
    GREEN_LAT = ON;
    BLUE_LAT = OFF;
    
    return;
}

void allumer_rouge_solo(void) {
    SMALL_RED_LAT = ON;
    
    return;
}

void eteindre_rouge_solo(void) {
    SMALL_RED_LAT = OFF;
    
    return;
}

void refresh_leds(const uint16_t *temperature, const uint16_t *batterie) {
    if(*temperature >= 1000){
        if(*temperature >= 3000) allumer_rouge();    //allumer LED rouge car temperature >= 25 degré Celsius
        else allumer_vert();                        //allumer LED verte car temperature entre 10 et 25 degré Celsius
    } else allumer_bleu();                          //allumer LED bleu car temperature <= 10 degré Celsius

    if (*batterie <= 1000){                            //Si la charge est inférieure à 10 % on allume la led rouge
        allumer_rouge_solo();
    } else return; //eteindre_rouge_solo();         //Si la charge est supérieure à 10 % on éteins la led rouge
    
    return;
}