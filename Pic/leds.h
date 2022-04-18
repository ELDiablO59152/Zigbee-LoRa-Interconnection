/* 
 * File:   leds.h
 * Author: Fabien AMELINCK
 *
 * Created on 29 juin 2021, 14:32
 */

#ifndef _LEDS_H
#define	_LEDS_H

#define RED_DIR         TRISAbits.TRISA6	// état Pin RA6 en entrée ou sortie (rouge)
#define GREEN_DIR       TRISAbits.TRISA7	// état Pin RA7 en entrée ou sortie (vert)
#define BLUE_DIR        TRISAbits.TRISA5	// état Pin RA5 en entrée ou sortie (bleu)
#define SMALL_RED_DIR   TRISAbits.TRISA4    // état Pin RA4 en entrée ou sortie (rouge solo)

#define RED_LAT         LATAbits.LATA6		// état de la led rouge ON ou OFF
#define GREEN_LAT       LATAbits.LATA7		// état de la led vert ON ou OFF 
#define BLUE_LAT        LATAbits.LATA5		// état de la led bleu ON ou OFF
#define SMALL_RED_LAT   LATAbits.LATA4      // état de la led rouge solo ON ou OFF

void initLeds(void);
void allumer_rouge(void);
void allumer_bleu(void);
void allumer_vert(void);
void allumer_rouge_solo(void);
void eteindre_rouge_solo(void);
void refresh_leds(const uint16_t *temperature, const uint16_t *batterie);

#endif	/* _LEDS_H */

