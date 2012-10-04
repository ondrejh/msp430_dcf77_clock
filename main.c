//
// MSP430 LCD Code
//
// Interface MSP430 Launchpad with LCD Module (LCM) in 4 bit mode. Thanks to:
//  http://cacheattack.blogspot.cz/2011/06/quick-overview-on-interfacing-msp430.html
//
#include "msp430x20x2.h"
#include "lcd.h"

//
// Routine Desc
//
// main entry point to the sketch
//
// Parameters
//
// void.
//
// Returns
//
// void.
//
void main(void)
{
    WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer

    lcm_init(); //InitializeLcm();

    lcm_clearscr(); //ClearLcmScreen();

    lcm_prints("Hello World!"); //PrintStr("Hello World!");

    while (1)
    {
        __delay_cycles(1000);
    }
}
