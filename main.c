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
//  Interface MSP430 Launchpad with LCD Module (LCM) in 4 bit mode .. thanks to:
//   "http://cacheattack.blogspot.cz/2011/06/quick-overview-on-interfacing-msp430.html"
//
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
//            |                 |      -----------    +5V
//            |            P2.0 |---->| D4 |       |   |
//            |            P2.1 |---->| D5 |       |---
//            |            P2.2 |---->| D6 |  LCD  |
//            |            P2.3 |---->| D7 |  16x2 |
//            |            P2.4 |---->| EN |       |---
//            |            P2.5 |---->| RS |       |   |
//            |                 |      ------------   ---
//
//******************************************************************************

// include section
#include <msp430g2553.h>

#include "uart.h"
#include "rtc.h"
#include "lcd.h"


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
    tstr[ptr++]='\0';
}

int str_add_lineend(char *s,int len)
{
    int i=0;
    for (i=0;i<len;i++) if (s[i]=='\0') break;
    if ((i+2)<len)
    {
        s[i++]='\r';
        s[i++]='\n';
        s[i]='\0';
        return 0;
    }
    return -1;
}

// buttons connected (port1)
#define BTN1 BIT3
#define BTN2 BIT4
#define BTN3 BIT5

// last pressed button buffer
uint8_t btn = 0;

// get last pressed button
uint8_t get_button(void)
{
    if (btn)
    {
        if (btn&BTN1)
        {
            btn=0;
            return 1;
        }
        if (btn&BTN2)
        {
            btn=0;
            return 2;
        }
        if (btn&BTN3)
        {
            btn=0;
            return 3;
        }
    }
    return 0;
}

// initialize buttons
void buttons_init(void)
{
    P1DIR &= ~(BTN1 | BTN2 | BTN3); // inputs
    P1REN |= (BTN1 | BTN2 | BTN3); // pullups
    P1IES |= (BTN1 | BTN2 | BTN3); // hi/lo edge
    P1IE |= (BTN1 | BTN2 | BTN3); // interrupt enable
    P1IFG &= ~(BTN1 | BTN2 | BTN3); // clear interrupt flags
}

// main program body
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

	board_init(); // init dco and leds
	lcm_init(); // lcd
	rtc_timer_init(); // init 32kHz timer
	uart_init(); // init uart (communication)
	buttons_init();

    lcm_clearscr();
    lcm_goto(0,0);
    lcm_prints("Button: ");

	while(1)
	{
        __bis_SR_register(CPUOFF + GIE); // enter sleep mode (leave on rtc second event)
        tstruct tnow;
        rtc_get_time(&tnow);
        char tstr[16];
        sprint_time(&tnow,tstr);
        lcm_goto(1,3);
        lcm_prints(tstr);
        str_add_lineend(tstr,16);
        uart_puts(tstr);
        uint8_t b=get_button();
        if (b)
        {
            lcm_goto(0,8);
            tstr[0]='0'+b;
            tstr[1]='\0';
            lcm_prints(tstr);
        }
	}

	return -1;
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
    btn = ~P1IN & (BTN1 | BTN2 | BTN3);
    P1IFG &= ~(BTN1 | BTN2 | BTN3); // clear IFG
    __bic_SR_register_on_exit(CPUOFF); // Clear CPUOFF bit from 0(SR)
}
