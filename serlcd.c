#include <xc.h>
#include "serlcd.h"

char charactersSinceFill = 0;

void setupLCD(void) {
    TRISCbits.TRISC6 = 1;
    RCSTA1bits.SPEN = 1;
    TXSTA1bits.TXEN = 1;

    TXSTA1bits.SYNC = LCD_SYNC;
    BAUDCON1bits.BRG16 = LCD_BRG16;
    TXSTA1bits.BRGH = LCD_BRGH;
    
    SPBRG1 = LCD_SPBRG;
}

void sendSpecialCommand(unsigned char byte) {
    sendByte(0x7C); //control character
    sendByte(byte);
}

void sendCommand(unsigned char byte) {
    sendByte(0xFE); //control character
    sendByte(byte);
}

void setPosition(unsigned char row, unsigned char column) {
    charactersSinceFill = row*16+column;
    sendCommand(0x80 + 64*row + column);
}

void setBacklight(unsigned char brightness) {
    sendSpecialCommand(128+brightness);
}

void clear() {
    sendCommand(0x01);
}

void sendByte(unsigned char byte) {
    while(!TXSTA1bits.TRMT) Nop();
    TXREG1 = byte;
}

void sendVisibleByte(unsigned char byte) {
    charactersSinceFill++;
    sendByte(byte);
}

void sendLiteralBytes(const char * bytes) {
    while(*bytes) {
        sendVisibleByte(*bytes++);
    }
}

void sendDigit(unsigned char digit) {
    if (digit >= 10)
        sendVisibleByte(digit + 65 - 10);
    else
        sendVisibleByte(digit + 48);
}

void sendCharAsBase(unsigned char num, unsigned char base, unsigned char padOutput) {
    unsigned char quotient;
    unsigned char remainder;
    unsigned char i = 0;
    unsigned char digits[8];
    
    while((padOutput == 0 && quotient != 0) || (padOutput != 0 && i < 8)) {
        quotient = num / base;

        remainder = num - quotient*base;
        num = quotient;

        digits[i++] = remainder;
    }
    
    while (i != 0) {
        sendDigit(digits[--i]);
    }
}

void sendIntAsBase(unsigned int num, unsigned int base) {
    unsigned int quotient;
    char remainder;
    char i = 0;
    char digits[16];

    while(1) {
        quotient = num / base;
        remainder = (char)(num - quotient*base);
        num = quotient;

        digits[i++] = remainder;

        if (quotient == 0) break;
    }

    while (i != 0) {
        sendDigit(digits[--i]);
    }
}

void sendDec(unsigned char num) {
    sendCharAsBase(num,10,0);
}

void sendIntDec(unsigned int num) {
    sendIntAsBase(num,10);
}

void sendIntArray(int * arr, int len) {
    int i;
    sendLiteralBytes("[");
    for (int i=0; i<len; i++) {
        if (i != 0) sendLiteralBytes(", ");
        sendIntAsBase(arr[i],10);
    }
    sendLiteralBytes("]");
}

void sendHex(unsigned char num) {
    sendLiteralBytes("0x");
    sendCharAsBase(num,16,0);
}

void sendBin(unsigned char num) {
    sendLiteralBytes("0b");
    sendCharAsBase(num,2,0);
}

void sendBinPad(unsigned char num) {
    sendLiteralBytes("0b");
    sendCharAsBase(num,2,1);
}

void fill(void) {
    char i = 16*2-charactersSinceFill-1;
    //sendDec(i);
    
    while(i-- >= 0) {
        sendLiteralBytes(" ");
    }
    charactersSinceFill = 0;
}

void fillLine(void) {
    char i = 16-charactersSinceFill-1;
    if (charactersSinceFill >= 16) {
        fill();
        return;
    }

    while(i-- >= 0) {
        sendLiteralBytes(" ");
        charactersSinceFill ++;
    }
}