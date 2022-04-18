/* 
 * File:   general.h
 * Author: Fabien AMELINCK
 *
 * Created on 3 juin 2021, 10:11
 */

#ifndef _GENERAL_H
#define	_GENERAL_H

// Booleans
#define TRUE            1           // allow TRUE to equal 1
#define FALSE           0           // allow FALSE to equal 0
#define forever         while(TRUE) // endless loop

// Following definitions are equivalent (depending on the context), but SET or CLEAR are sufficient
#define SET             1           // allow SET to equal 1
#define CLEAR           0           // allow CLEAR to equal 0

#define ENABLE          SET         // allow ENABLE to equal 1
#define DISABLE         CLEAR       // allow DISABLE to equal 0

#define READ            SET         // allow READ to equal 1
#define WRITE           CLEAR       // allow WRITE to equal 0

#define ON              SET         // allow ON to equal 1
#define OFF             CLEAR       // allow OFF to equal 0

#define INP             SET         // PORT bit is in input mode
#define OUTP            CLEAR       // PORT bit is in output mode

#define OUTP_HIGH       SET         // high level
#define OUTP_LOW        CLEAR       // low level

// Internal oscillator frequency
#define _XTAL_FREQ	1000000UL   // default XTAL frequency is 1MHz (FOSC=250kHz)
                                    // (used by inline function _delay)
                                    // TCY = (1 / (_XTAL_FREQ / 4)) = 4µs

// New typedefs
typedef unsigned char	UINT8_T;    // UINT8_T means byte (8-bit)
typedef unsigned short	UINT16_T;   // UINT16_T means word(16-bit)
typedef unsigned char	BOOL;       // BOOL means byte (8-bit)
typedef char         BYTE;
typedef char            INT8_T;     // INT8_T means 2's complement (1 sign bit + 7-bit)
typedef int             INT16_T;    // INT16_T means 2's complement (1 sign bit + 15-bit)

#endif	/* _GENERAL_H */

