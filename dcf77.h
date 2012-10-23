/**
 *
 * dcf77 module header
 *
 **/

#ifndef __DCF77_H__
#define __DCF77_H__

#include <inttypes.h>
#include <stdbool.h>

volatile uint8_t last_symbol;
int last_Q;
volatile bool symbol_ready;
int tunestatus;
int finetune;

void dcf77_init(void);
void dcf77_strobe(void);

#endif
