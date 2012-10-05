/**
 *
 * buttons module
 *
 * author: ondrejh DOT ck AT gmail DOT com
 * date: 5.10.2012
 *
 * leave sleep mode when any of 3 buttons pressed
 * last pressed buttons buffered (access by get_button function)
 *
 **/

/// include section
#include <msp430g2553.h>
#include "button.h" // self

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

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
    btn = ~P1IN & (BTN1 | BTN2 | BTN3);
    P1IFG &= ~(BTN1 | BTN2 | BTN3); // clear IFG
    __bic_SR_register_on_exit(CPUOFF); // Clear CPUOFF bit from 0(SR)
}
