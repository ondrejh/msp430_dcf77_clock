/**
 *
 * dcf77 module header
 *
 **/

#ifndef __DCF77_H__
#define __DCF77_H__

#include <inttypes.h>
#include <stdbool.h>

// code size and performace controll
#define DCF77_TEST_PARITY 0 // set 1 to test parities and static bits in dcf77 code
#define DCF77_DEBUG 1 // set 1 to output some debug variables

#if DCF77_DEBUG
// debug variables
volatile uint8_t last_symbol;
int last_Q;
volatile bool symbol_ready;
int tunestatus;
int finetune;
#endif

void dcf77_init(void);
void dcf77_strobe(void);

#endif
