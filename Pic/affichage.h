/*
 * File:   affichage.c
 * Author: Maximilien
 *
 * Created on 17 juin 2021, 17:22
 */

#ifndef _AFFICHAGE_H
#define _AFFICHAGE_H

#include <xc.h>
#include "general.h"

#define LCD_RESET_DIR           TRISBbits.TRISB5
#define LCD_RESET_LAT           LATBbits.LATB5
#define LIGNE_1 0x80    //made in datasheet
#define LIGNE_2 0xA0    //pour coinfig ? 4 lignes
#define LIGNE_3 0xC0
#define LIGNE_4 0xE0

uint8_t inverser_binaire(uint8_t data);
void ecrire_ecran_start_byte(uint8_t RSW);
void ecrire_ecran_data(uint8_t data);
void init_ecran(void);
void clear_ecran(void);
void afficher_temperature(uint16_t data);
void afficher_humidite(uint16_t data);
void afficher_batterie(uint16_t data);
void afficher_donnees(uint16_t tem, uint16_t hum, uint16_t bat);
void afficher_string(char * string);
void afficher_string_hex(uint16_t data);
//uint8_t CheckBusy(void);

#endif	/* _AFFICHAGE_H */