//******************************************************************************
// DCF77 home automation clock
//
// author: Ondrej Hejda
// date:   2.10.2012
//
// resources:
//
//  Using ACLK and the 32kHz Crystal .. thanks to:
//   "http://www.msp430launchpad.com/2012/03/using-aclk-and-32khz-crystal.html"
//
// hardware: MSP430G2553 (launchpad)
//
//                MSP4302553
//             -----------------
//         /|\|              XIN|----  -----------
//          | |                 |     | 32.768kHz |---
//          --|RST          XOUT|----  -----------    |
//            |                 |                    ---
//            |                 |
//            |           P1.1,2|--> UART (debug output 9.6kBaud)
//            |                 |
//            |             P1.0|--> RED LED (active high)
//            |             P1.6|--> GREEN LED (active high)
//            |                 |
//
//******************************************************************************

// include section
#include <msp430g2553.h>

#include "uart.h"
#include "rtc.h"


// board (leds, button)
#define LED_INIT() {P1DIR|=0x41;P1OUT&=~0x41;}
#define LED_RED_ON() {P1OUT|=0x01;}
#define LED_RED_OFF() {P1OUT&=~0x01;}
#define LED_RED_SWAP() {P1OUT^=0x01;}
#define LED_GREEN_ON() {P1OUT|=0x40;}
#define LED_GREEN_OFF() {P1OUT&=~0x40;}
#define LED_GREEN_SWAP() {P1OUT^=0x40;}


// leds and dco init
void board_init(void)
{
	// oscillator
	BCSCTL1 = CALBC1_1MHZ;		// Set DCO
	DCOCTL = CALDCO_1MHZ;
	/*BCSCTL1 = CALBC1_16MHZ;		// Set DCO
	DCOCTL = CALDCO_16MHZ;*/

	LED_INIT(); // leds
}

// time output debug function
void sprint_time(tstruct *t, char *tstr)
{
    uint8_t ptr = 0;
    switch (t->dayow)
    {
        case 0:
            tstr[ptr++]='P';
            tstr[ptr++]='o';
            break;
        case 1:
            tstr[ptr++]='U';
            tstr[ptr++]='t';
            break;
        case 2:
            tstr[ptr++]='S';
            tstr[ptr++]='t';
            break;
        case 3:
            tstr[ptr++]='C';
            tstr[ptr++]='t';
            break;
        case 4:
            tstr[ptr++]='P';
            tstr[ptr++]='a';
            break;
        case 5:
            tstr[ptr++]='S';
            tstr[ptr++]='o';
            break;
        case 6:
            tstr[ptr++]='N';
            tstr[ptr++]='e';
            break;
        default:
            tstr[ptr++]='-';
            tstr[ptr++]='-';
            break;
    }
    tstr[ptr++]=' ';
    tstr[ptr++]=h2c(t->hour/10);
    tstr[ptr++]=h2c(t->hour%10);
    tstr[ptr++]=':';
    tstr[ptr++]=h2c(t->minute/10);
    tstr[ptr++]=h2c(t->minute%10);
    tstr[ptr++]=':';
    tstr[ptr++]=h2c(t->second/10);
    tstr[ptr++]=h2c(t->second%10);
    tstr[ptr++]='\r';
    tstr[ptr++]='\n';
    tstr[ptr++]='\0';
}

// main program body
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

	board_init(); // init dco and leds
	rtc_timer_init(); // init 32kHz timer
	uart_init(); // init uart (communication)

	while(1)
	{
        __bis_SR_register(CPUOFF + GIE); // enter sleep mode (leave on rtc second event)
        tstruct tnow;
        rtc_get_time(&tnow);
        char tstr[16];
        sprint_time(&tnow,tstr);
        uart_puts(tstr);
	}

	return -1;
}
