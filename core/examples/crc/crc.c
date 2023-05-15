//#include <stdio.h>

#define POLYNOMIAL 0x000000D8  /* 11011 followed by 0's */
#define message 15

unsigned char remainder;
void crc() {	

     //Initially, the dividend is the remainder.
    remainder = message;
    //printf("remainder = %x \n", remainder);
     // For each bit position in the message....
    for (int bit = 8; bit > 0; --bit) {

        //If the uppermost bit is a 1...
        if (remainder & 0x00000080) 
            remainder ^= POLYNOMIAL;
	//printf("remainder = %x \n", remainder);
        // Shift the next bit of the message into the remainder.
        remainder = (remainder << 1);
        //printf("remainder = %x \n", remainder);
    }

    // Return the relevant bits of the remainder as CRC.
    remainder = (remainder >> 4);
    //printf("remainder = %x \n", remainder);
}   

//int main() {
void notmain() {
    //unsigned char msg = 15;

    crc();
    //printf("Message = %x, CRC = %x \n", msg, res);
    //return 0;
}
