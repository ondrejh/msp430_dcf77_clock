/**
 *
 * dcf77 module
 *
 * author: ondrejh dot ck at gmail dot com
 * date: 18.10.2012
 *
 * uses: timer A1 interrupt
 *       input
 *
 * it should do (todo):
 *      1ms strobing of input signal
 *      coarse synchronization of input char decoder
 *      input char decoding with fine sync. and hold over
 *      minute block decoding and verifiing
 *
 **/

/// include section
#include <msp430g2553.h>
#include "dcf77.h" // self

/// module initialization function
// timer and input init
void dcf77_init(void)
{

}
