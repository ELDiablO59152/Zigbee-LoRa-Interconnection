/*
 * File:   affichage.c
 * Author: Maximilien
 *
 * Created on 17 juin 2021, 17:22
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xc.h>
#include "general.h"
#include "affichage.h"
#include "spi.h"

uint8_t inverser_binaire(uint8_t data) {    //magie noire
    uint8_t data_inverse = 0x00;
    int i;
    for (i = 0; i < 8; i++) {
        if (i < 4) {
            data_inverse += ((data & (0x01 << i)) << (7-2*i));  //inverse les 4 premiers bits
        }
        else {
            data_inverse += ((data & (0x01 << i)) >> (1+2*(i-4)));  //inverse le reste
        }
    }
    return(data_inverse);
}

void ecrire_ecran_start_byte(uint8_t RSW) {
    __delay_us(200);
    SPITransfer(RSW);
    
    return;
}

void ecrire_ecran_data(uint8_t data) {
    uint8_t data_inverse = inverser_binaire(data);
    __delay_us(200);  //delai entre deux donn?es
    SPITransfer(data_inverse & 0xF0);  //Select 4 bits de gauche, clear le reste
    __delay_us(200);
    SPITransfer((uint8_t)((uint16_t)(data_inverse & 0x0F) << 4));   //Select 4 bits de droite, clear le reste puis d?cale tout ? gauche
    
    return;
}

void init_ecran(void) {
    
    LCD_RESET_DIR = OUTP;
    LCD_RESET_LAT = OUTP_HIGH;  //reset ? l'init de l'?cran
    __delay_ms(50);             //? faire lorsque l'?cran est aliment?
    LCD_RESET_LAT = OUTP_LOW;   //probl?me : comment savoir quand est-ce qu'il l'est avec le bouton ?
    __delay_us(200);
    LCD_RESET_LAT = OUTP_HIGH;
    
    SPI_SS_LCD_LAT = SPI_SS_ENABLE;
    ecrire_ecran_start_byte(0xF8);  //RS = 0, R/W = 0, Besoin de l'envoyer qu'une seule fois si config reste la m?me
    
    //Voir tableau d'exemple d'init dans datasheet ?cran
    ecrire_ecran_data(0x3A);    //Function Set : 8 bit data length extension Bit RE=1; REV=0
    ecrire_ecran_data(0x09);    //Extended Function Set : 4 line display
    ecrire_ecran_data(0x06);    //Entry mode set : bottom view
    ecrire_ecran_data(0x1E);    //Bias setting : BS1 = 1
    
    ecrire_ecran_data(0x39);    //Function Set : 8 bit data length extension Bit RE=0; IS=1
    ecrire_ecran_data(0x1B);    //Internal OSC : BS0=1 -> Bias=1/6
    ecrire_ecran_data(0x6E);    //Follower control : Devider on and set value
    ecrire_ecran_data(0x56);    //Power control : Booster on and set contrast (DB1=C5, DB0=C4)
    ecrire_ecran_data(0x7A);    //Contrast set : Set contrast (DB3-DB0=C3-C0)
    ecrire_ecran_data(0x38);    //Function Set : 8 bit data length extension Bit RE=0; IS=0
    
    ecrire_ecran_data(0x0F);    //Display On : Display on, cursor on, blink on
    
    SPI_SS_LCD_LAT = SPI_SS_DISABLE;
    
    return;
}

void clear_ecran(void) {    //G?re RWB mais pas CS
    ecrire_ecran_start_byte(0xF8);
    ecrire_ecran_data(0x01); //clear display
    
    return;
}

void afficher_temperature(uint16_t data) {   //Ne g?re pas CS 
    //S?lection de la ligne
    ecrire_ecran_start_byte(0xF8);  //RS = 0, R/W = 0
    ecrire_ecran_data(LIGNE_1);
    
    //?criture de la temperature
    ecrire_ecran_start_byte(0xFA);//RS = 0, R/W = 1
    ecrire_ecran_data('T');    //"T"
    ecrire_ecran_data('e');    //"e"
    ecrire_ecran_data('m');    //"m"
    ecrire_ecran_data(':');    //":"
    //ecrire_ecran_data(' ');    //" "
    
    if(data >= 1000) ecrire_ecran_data((data / 1000) % 10 + '0'); //Donn?e : dizaine
    else ecrire_ecran_data(' ');
    ecrire_ecran_data((data / 100) % 10 + '0'); //Donn?e : unit?
    ecrire_ecran_data('.');                //"."
    ecrire_ecran_data((data / 10) % 10 +'0');  //Premier chiffre apr?s la virgule
    ecrire_ecran_data(data % 10 +'0');  //Premier chiffre apr?s la virgule
    
    ecrire_ecran_data(0xDF);    //"°"
    
    return;
}

void afficher_humidite(uint16_t data) { //Ne g?re pas CS
    //S?lection de la ligne
    ecrire_ecran_start_byte(0xF8);  //RS = 0, R/W = 0
    ecrire_ecran_data(LIGNE_2);
    
    //?criture de l'Humidit?
    ecrire_ecran_start_byte(0xFA); //RS = 0, R/W = 1
    ecrire_ecran_data('H');    //"H"
    ecrire_ecran_data('u');    //"u"
    ecrire_ecran_data('m');    //"m"
    ecrire_ecran_data(':');    //":"
    ecrire_ecran_data(' ');    //" "
    
    if(data >= 1000) ecrire_ecran_data((data / 1000) % 10 + '0'); //Donn?e : dizaine
    else ecrire_ecran_data(' ');
    ecrire_ecran_data((data / 100) % 10 + '0'); //Donn?e : unit?
    ecrire_ecran_data('.');                //"."
    ecrire_ecran_data((data / 10) % 10 +'0');  //Premier chiffre apr?s la virgule
    //ecrire_ecran_data(data % 10 +'0');  //Premier chiffre apr?s la virgule
    
    ecrire_ecran_data('%');    //"%"
    
    return;
}

void afficher_batterie(uint16_t data) { //Ne g?re pas CS
    //S?lection de la ligne
    ecrire_ecran_start_byte(0xF8);  //RS = 0, R/W = 0
    ecrire_ecran_data(LIGNE_3);
    
    //?criture de la Batterie
    ecrire_ecran_start_byte(0xFA); //RS = 0, R/W = 1
    ecrire_ecran_data('B');    //"B"
    ecrire_ecran_data('a');    //"a"
    ecrire_ecran_data('t');    //"t"
    ecrire_ecran_data(':');    //":"
    ecrire_ecran_data(' ');    //" "
    
    if(data >= 1000) ecrire_ecran_data((data / 1000) % 10 + '0'); //Donn?e : dizaine
    else ecrire_ecran_data(' ');
    ecrire_ecran_data((data / 100) % 10 + '0'); //Donn?e : unit?
    ecrire_ecran_data('.');                //"."
    ecrire_ecran_data((data / 10) % 10 +'0');  //Premier chiffre apr?s la virgule
    //ecrire_ecran_data(data % 10 +'0');  //Premier chiffre apr?s la virgule
    
    
    ecrire_ecran_data('%');    //"%"
    
    return;
}

void afficher_donnees(uint16_t tem, uint16_t hum, uint16_t bat) {
    SPI_SS_LCD_LAT = SPI_SS_ENABLE;
    
    clear_ecran();
    /*ecrire_ecran_data(0x3C);
    ecrire_ecran_data(0x3A);
    ecrire_ecran_data(0x1F);
    ecrire_ecran_data(0x3C);*/
    
    afficher_temperature(tem);
    afficher_humidite(hum);
    afficher_batterie(bat);

    SPI_SS_LCD_LAT = SPI_SS_DISABLE;
    
    return;
}

void afficher_string(char * string) {
    SPI_SS_LCD_LAT = SPI_SS_ENABLE;
    
    /*clear_ecran();
    ecrire_ecran_data(0x3C);
    ecrire_ecran_data(0x3A);
    ecrire_ecran_data(0x1F);
    ecrire_ecran_data(0x3C);*/
    
    ecrire_ecran_start_byte(0xF8);
    ecrire_ecran_data(LIGNE_4);
    
    ecrire_ecran_start_byte(0xFA); //RS = 0, R/W = 1
    
    do ecrire_ecran_data(*string++);
	while(*string);

    SPI_SS_LCD_LAT = SPI_SS_DISABLE;
    
    return;
}

void afficher_string_hex(uint16_t data) {
    SPI_SS_LCD_LAT = SPI_SS_ENABLE;
    
    char *hexa = "0123456789ABCDEF";
    
    ecrire_ecran_start_byte(0xFA); //RS = 0, R/W = 1
    
    if (data > 4095) ecrire_ecran_data(hexa[data / 4096]);
    if (data > 255) ecrire_ecran_data(hexa[data / 256 % 16]);     // write ASCII value of hexadecimal high nibble
    if (data > 15) ecrire_ecran_data(hexa[data / 16 % 16]);     // write ASCII value of hexadecimal high nibble
    ecrire_ecran_data(hexa[data % 16]);     // write ASCII value of hexadecimal low nibble

    SPI_SS_LCD_LAT = SPI_SS_DISABLE;
    
    return;
}