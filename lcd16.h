#ifndef LCD16_H
#define LCD16_H

#include "msp430g2553.h"

#define  EN BIT6
#define  RS BIT7

void waitlcd(unsigned int x);

void lcd_init(void);
void integerToLcd(int integer);
void lcdData(unsigned char l);
void prints(char *s);
void gotoXy(unsigned char  x,unsigned char y);

#endif
