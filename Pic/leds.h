/* 
 * File:   leds.h
 * Author: Fabien AMELINCK
 *
 * Created on 29 juin 2021, 14:32
 */

#ifndef _LEDS_H
#define	_LEDS_H

#define RED_DIR         TRISAbits.TRISA6	// �tat Pin RA6 en entr�e ou sortie (rouge)
#define GREEN_DIR       TRISAbits.TRISA7	// �tat Pin RA7 en entr�e ou sortie (vert)
#define BLUE_DIR        TRISAbits.TRISA5	// �tat Pin RA5 en entr�e ou sortie (bleu)
#define SMALL_RED_DIR   TRISAbits.TRISA4    // �tat Pin RA4 en entr�e ou sortie (rouge solo)

#define RED_LAT         LATAbits.LATA6		// �tat de la led rouge ON ou OFF
#define GREEN_LAT       LATAbits.LATA7		// �tat de la led vert ON ou OFF 
#define BLUE_LAT        LATAbits.LATA5		// �tat de la led bleu ON ou OFF
#define SMALL_RED_LAT   LATAbits.LATA4      // �tat de la led rouge solo ON ou OFF

void initLeds(void);
void allumer_rouge(void);
void allumer_bleu(void);
void allumer_vert(void);
void allumer_rouge_solo(void);
void eteindre_rouge_solo(void);
void refresh_leds(const uint16_t *temperature, const uint16_t *batterie);

#endif	/* _LEDS_H */

