/**
 *
 * MSP430 LCD Code
 *
 * Interface MSP430 Launchpad with LCD Module (LCM) in 4 bit mode. Thanks to:
 *      http://cacheattack.blogspot.cz/2011/06/quick-overview-on-interfacing-msp430.html
 *
 */

/// include section
#include <msp430g2553.h>
#include "lcd.h" // self

/// defines
/*#define LCM_DIR P1DIR
#define LCM_OUT P1OUT*/
#define LCM_DIR P2DIR
#define LCM_OUT P2OUT

//
// Define symbolic LCM - MCU pin mappings
// We've set DATA PIN TO (4,5,6,7) 0,1,2,3 for easy translation
//
/*#define LCM_PIN_RS BIT0 // P1.0
#define LCM_PIN_EN BIT1 // P1.1
#define LCM_PIN_D7 BIT7 // P1.7
#define LCM_PIN_D6 BIT6 // P1.6
#define LCM_PIN_D5 BIT5 // P1.5
#define LCM_PIN_D4 BIT4 // P1.4*/
#define LCM_PIN_RS BIT5 // P2.5
#define LCM_PIN_EN BIT4 // P2.4
#define LCM_PIN_D7 BIT3 // P2.3
#define LCM_PIN_D6 BIT2 // P2.2
#define LCM_PIN_D5 BIT1 // P2.1
#define LCM_PIN_D4 BIT0 // P2.0


#define LCM_PIN_MASK ((LCM_PIN_RS | LCM_PIN_EN | LCM_PIN_D7 | LCM_PIN_D6 | LCM_PIN_D5 | LCM_PIN_D4))

/*#define FALSE 0
#define TRUE 1*/
#define LCM_SEND_COMMAND 0
#define LCM_SEND_DATA 1

/*#define LCM_PULSE_DELAY 200
#define LCM_INIT_DELAY 100000*/
#define LCM_PULSE_DELAY 1600
#define LCM_INIT_DELAY 800000

#define LCM_CURSOR_ON 0x02
#define LCM_CURSOR_BLINK 0x01

/// local function implementations

//
// Routine Desc:
//
// This is the function that must be called
// whenever the LCM needs to be told to
// scan it's data bus.
//
// Parameters:
//
// void.
//
// Return
//
// void.
//
void PulseLcm()
{
    //
    // pull EN bit low
    //
    LCM_OUT &= ~LCM_PIN_EN;
    __delay_cycles(LCM_PULSE_DELAY);

    //
    // pull EN bit high
    //
    LCM_OUT |= LCM_PIN_EN;
    __delay_cycles(LCM_PULSE_DELAY);

    //
    // pull EN bit low again
    //
    LCM_OUT &= (~LCM_PIN_EN);
    __delay_cycles(LCM_PULSE_DELAY);
}

//
// Routine Desc:
//
// Send a byte on the data bus in the 4 bit mode
// This requires sending the data in two chunks.
// The high nibble first and then the low nible
//
// Parameters:
//
// ByteToSend - the single byte to send
//
// IsData - set to TRUE if the byte is character data
// FALSE if its a command
//
// Return
//
// void.
//
void SendByte(char ByteToSend, int IsData)
{
    //
    // clear out all pins
    //
    LCM_OUT &= (~LCM_PIN_MASK);

    //
    // set High Nibble (HN) -
    // usefulness of the identity mapping
    // apparent here. We can set the
    // // DB7 - DB4 just by setting P1.7 - P1.4
    // DB7 - DB4 just by setting P2.3 - P2.0
    // using a simple assignment
    //
    //LCM_OUT |= (ByteToSend & 0xF0);
    LCM_OUT |= ((ByteToSend & 0xF0) >> 4);

    if (IsData != LCM_SEND_COMMAND)
    {
        LCM_OUT |= LCM_PIN_RS;
    }
    else
    {
        LCM_OUT &= ~LCM_PIN_RS;
    }
    //
    // we've set up the input voltages to the LCM.
    // Now tell it to read them.
    //
    PulseLcm();

    //
    // set Low Nibble (LN) -
    // usefulness of the identity mapping
    // apparent here. We can set the
    // // DB7 - DB4 just by setting P1.7 - P1.4
    // DB7 - DB4 just by setting P2.3 - P2.0
    // using a simple assignment
    //
    LCM_OUT &= (~LCM_PIN_MASK);
    //LCM_OUT |= ((ByteToSend & 0x0F) << 4);
    LCM_OUT |= (ByteToSend & 0x0F);

    if (IsData != LCM_SEND_COMMAND)
    {
        LCM_OUT |= LCM_PIN_RS;
    }
    else
    {
        LCM_OUT &= ~LCM_PIN_RS;
    }

    //
    // we've set up the input voltages to the LCM.
    // Now tell it to read them.
    //
    PulseLcm();
}

/// interface function implementations

//
// Routine Desc:
//
// Set the position of the cursor on the screen
//
// Parameters:
//
// Row - zero based row number
//
// Col - zero based col number
//
// Return
//
// void.
//
void lcm_goto(char Row, char Col)
{
    char address;

    //
    // construct address from (Row, Col) pair
    //

    if (Row == 0)
    {
        address = 0;
    }
    else
    {
        address = 0x40;
    }

    address |= Col;
    SendByte(0x80 | address, LCM_SEND_COMMAND);
}

//
// Routine Desc:
//
// Clear the screen data and return the
// cursor to home position
//
// Parameters:
//
// void.
//
// Return
//
// void.
//
void lcm_clearscr()
{
    //
    // Clear display, return home
    //
    SendByte(0x01, LCM_SEND_COMMAND);
    SendByte(0x02, LCM_SEND_COMMAND);
}

//
// Routine Desc:
//
// Initialize the LCM after power-up.
//
// Note: This routine must not be called twice on the
// LCM. This is not so uncommon when the power
// for the MCU and LCM are separate.
//
// Parameters:
//
// void.
//
// Return
//
// void.
//
void lcm_init(void)
{
    //
    // set the MSP pin configurations
    // and bring them to low
    //
    LCM_DIR |= LCM_PIN_MASK;
    LCM_OUT &= ~(LCM_PIN_MASK);
    LCM_OUT |= LCM_PIN_EN;
    //
    // wait for the LCM to warm up and reach
    // active regions. Remember MSPs can power
    // up much faster than the LCM.
    //
    __delay_cycles(LCM_INIT_DELAY);

    //
    // initialize the LCM module
    //
    // 1. Set 4-bit input
    //
    LCM_OUT &= ~LCM_PIN_RS;
    LCM_OUT &= ~LCM_PIN_EN;

    //LCM_OUT = 0x20;
    LCM_OUT = LCM_PIN_D5;
    PulseLcm();

    //
    // set 4-bit input - second time.
    // (as reqd by the spec.)
    //
    SendByte(0x28, LCM_SEND_COMMAND);

    //
    // 2. Display on, cursor off, blink off
    //
    SendByte(0x0C/*|LCM_CURSOR_ON|LCM_CURSOR_BLINK*/, LCM_SEND_COMMAND);

    //
    // 3. Cursor move auto-increment
    //
    SendByte(0x06, LCM_SEND_COMMAND);
}

//
// Routine Desc
//
// Print a string of characters to the screen
//
// Parameters:
//
// Text - null terminated string of chars
//
// Returns
//
// void.
//
void lcm_prints(char *Text)
{
    char *c;
    c = Text;

    while ((c != 0) && (*c != 0))
    {
        SendByte(*c, LCM_SEND_DATA);
        c++;
    }
}
