/*
 * rtc clock module
 *
 * author: Ondrej Hejda
 * date: 2.10.2012
 *
 */

// include section
#include <msp430g2553.h>
#include <stdbool.h>
#include <string.h>
#include "rtc.h"

// switch on (1) and off (0) debug blinking
#define RTC_LED 1
#if RTC_LED==1
    // launchpad green led (P1.4 active high)
    #define RTC_LED_INIT() {P1DIR|=0x40;P1OUT&=~0x40;}
    #define RTC_LED_ON() {P1OUT|=0x40;}
    #define RTC_LED_OFF() {P1OUT&=~0x40;}
#else
    // no led
    #define RTC_LED_INIT() {}
    #define RTC_LED_ON() {}
    #define RTC_LED_OFF() {}
#endif

/** local (hiden) variables **/

tstruct tbuff[2]; // time buffer (2 iterations swapping)
uint8_t tptr = 0; // pointer to valid time value

bool treset = true; // reset timer flag

/** local functions section **/

// increase time by one second
void inc_one_second(tstruct *tbefore, tstruct *tafter)
{
    // copy tbefore value into tafter
    memcpy(tafter,tbefore,sizeof(tstruct));
    // increase it by one second
    tafter->second++; // second
    if (tafter->second>=60)
    {
        tafter->second=0;
        tafter->minute++; // minute
        if (tafter->minute>=60)
        {
            tafter->minute=0;
            tafter->hour++; // hour
            if (tafter->hour>=24)
            {
                tafter->hour=0;
                tafter->dayow++; // day
                if (tafter->dayow>=7)
                {
                    tafter->dayow=0;
                }
            }
        }
    }
}

/** global functions section **/

// set time function (use for synchronization)
void rtc_set_time(tstruct *tset)
{
    // reset timer (next time tick)
    treset = true;
    // set time
    memcpy(&tbuff[tptr],tset,sizeof(tstruct));
}

// get time function
void rtc_get_time(tstruct *tget)
{
    uint8_t ptr = tptr;
    memcpy(tget,&tbuff[ptr],sizeof(tstruct));
}

// init rtc timer (32kHz Xtal)
void rtc_timer_init(void)
{
    RTC_LED_INIT();

	CCTL0 = CCIE; // CCR0 interrupt enabled
	#ifdef RTC_SAMPLING_FREQV
	CCR0 = (32768/8/RTC_SAMPLING_FREQV);
	#else
	//CCR0 = 512;	  // f = 32768 / 8(ID_3) / 512(CCR0) = 8Hz
	CCR0 = 1024;	  // f = 32768 / 8(ID_3) / 1024(CCR0) = 4Hz
	//CCR0 = 2048;	  // f = 32768 / 8(ID_3) / 2048(CCR0) = 2Hz
	#endif
	TACTL = TASSEL_1 + ID_3 + MC_1; // ACLK, /8, upmode
}

/** interrupt section **/

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
    static uint16_t tdiv = 0;
    if (treset==false)
    {
        // normal timing
        tdiv++;
        #ifdef RTC_SAMPLING_FREQV
        if (tdiv>=RTC_SAMPLING_FREQV) // every one second
        #else
        if (tdiv>=4) // every one second
        #endif
        {
            tdiv=0;
            RTC_LED_ON();
            // continue with main after interrupt (as there was an second event)
            __bic_SR_register_on_exit(CPUOFF); // Clear CPUOFF bit from 0(SR)

            uint8_t nextptr=tptr^0x01;
            inc_one_second(&tbuff[tptr],&tbuff[nextptr]);
            tptr=nextptr;
        }
        else RTC_LED_OFF();
    }
    else
    {
        // reset timer (there was an sync. event)
        tdiv=0;
        RTC_LED_ON();
        // continue with main after interrupt (as there was an sync. event)
        __bic_SR_register_on_exit(CPUOFF); // Clear CPUOFF bit from 0(SR)

        treset=false; // clear sync. flag
    }
}
