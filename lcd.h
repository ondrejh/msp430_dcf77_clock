/**
 *
 * MSP430 LCD header
 *
 **/

#ifndef __LCD_H__
#define __LCD_H__

void lcm_init(void); // initialization
void lcm_clearscr(void); // clear screen
void lcm_goto(char Row, char Col); // goto cursor position
void lcm_prints(char *Text); // print string function

#endif
