#ifndef _ASCON_UTILS_H
#define _ASCON_UTILS_H 1

#include <stdio.h>
#include <string.h>

void string2hexString(unsigned char* input, int clen, char* output);
void hextobyte(char *hexstring, unsigned char* bytearray );

#endif 