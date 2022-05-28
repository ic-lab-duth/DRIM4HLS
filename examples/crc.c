//#include <stdio.h>

#define POLYNOMIAL 0xD8  /* 11011 followed by 0's */

unsigned char crc(unsigned char message) {
    unsigned char remainder;	

     //Initially, the dividend is the remainder.
    remainder = message;

     // For each bit position in the message....
    for (int bit = 8; bit > 0; --bit) {

        //If the uppermost bit is a 1...
        if (remainder & 0x80) 
            remainder ^= POLYNOMIAL;

        // Shift the next bit of the message into the remainder.
        remainder = (remainder << 1);
    }

    // Return the relevant bits of the remainder as CRC.
    return (remainder >> 4);
}   

//int main() {
void notmain() {
    unsigned char msg = 15;

    unsigned char res = crc(msg);
    //printf("Message = %x, CRC = %x \n", msg, res);
    //return 0;
}