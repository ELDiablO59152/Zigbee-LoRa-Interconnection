#include "ascon_utils.h"

void string2hexString(unsigned char* input, int clen, char* output)
{
    int loop;
    int i; 
    
    i=0;
    loop=0;
    for (i=0;i<clen;i+=2){
        sprintf((char*)(output+i),"%02hhx", input[loop]);
        loop+=1;

    }

    //insert NULL at the end of the output string
    output[i++] = '\0';
}

void hextobyte(char *hexstring, unsigned char* bytearray ) 
{
    int i;
    int str_len = strlen(hexstring);
    for (i = 0; i < (str_len / 2); i++) 
    {
        sscanf(hexstring + 2*i, "%02hhx", &bytearray[i]);
    }
    fprintf(stdout, "\n");
}
