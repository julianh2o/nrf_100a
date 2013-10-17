#include "serlcd_config.h"

void setupLCD(void);
void sendByte(unsigned char byte);
void sendVisibleByte(unsigned char byte);

void sendCommand(unsigned char byte);
void sendSpecialCommand(unsigned char byte);
void setPosition(unsigned char row, unsigned char column);
void setBacklight(unsigned char brightness); //between 0 and 29
void clear(void);

void sendLiteralBytes(rom const char * bytes);
void sendBytes(unsigned char * bytes);

void sendDigit(unsigned char digit);
void sendCharAsBase(unsigned char num, unsigned char base, unsigned char pad);
void sendIntAsBase(unsigned int num, unsigned int base);

void sendDec(unsigned char num);
void sendIntDec(unsigned int num);
void sendHex(unsigned char num);
void sendBin(unsigned char num);
void sendBinPad(unsigned char num);

void fill(void);
