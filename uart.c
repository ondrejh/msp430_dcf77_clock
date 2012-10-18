/*
 * uart.c
 *
 *  Created on: 20.6.2012
 *      Author: O. Hejda
 *
 *  Description: uart module template implementing char reception and
 *  	circular transmit buffer with functions putc and puts
 *  	if it receives '?' char it answers with "Hello World !" string
 *  	runs completely in interrupts
 *  	have fun!
 */

// include section
#include <msp430g2553.h>

#include "uart.h"

// uart TX led
#define UART_TX_LED 0
#if UART_TX_LED==1
	// TX led port 1 pin 0, active high
	#define UART_TX_LED_INIT() {P1DIR|=0x01;UART_TX_LED_OFF();}
	#define UART_TX_LED_OFF() {P1OUT&=~0x01;}
	#define UART_TX_LED_ON() {P1OUT|=0x01;}
#else
	// No TX led
	#define UART_TX_LED_INIT()
	#define UART_TX_LED_OFF()
	#define UART_TX_LED_ON()
#endif
#undef UART_TX_LED

// uart buffer length (mask preferred)
#define UART_TX_BUFLEN 16
#define UART_TX_BUFMASK 0x0F

// uart circular buffer
char uart_tx_buffer[UART_TX_BUFLEN]={'\0'};
unsigned int uart_tx_inptr=0, uart_tx_outptr=0;
// uart transmit flag (0 not transmitting, 1 transmitting)
bool uart_tx_transmitt = false;

// local function definition
int uart_start_tx(void);

// implementation section

char h2c(unsigned int h)
{
	unsigned int hx = h&0xF;
	if (hx<0xA) return ('0'+hx);
	return ('A'+hx-10);
}

int8_t c2h(char c)
{
    if ((c>='0') && (c<='9')) return c-'0';
    if ((c>='A') && (c<='F')) return c-'A'+10;
    if ((c>='a') && (c<='f')) return c-'a'+10;
    return -1;
}

// RS485 like data direction controll
void tx_output_enable(bool enable)
{
    /*if (enable)
    {
        P1SEL |= BIT2;
        P1SEL2 |= BIT2;
    }
    else
    {
        P1SEL &= ~BIT2;
        P1SEL2 &= ~BIT2;
    }*/
}

// uart initialization
void uart_init(void)
{
	UART_TX_LED_INIT();

	P1SEL = BIT1 + BIT2 ;   // P1.1 = RXD, P1.2=TXD
	P1SEL2 = BIT1 + BIT2 ;  // P1.1 = RXD, P1.2=TXD
	UCA0CTL1 |= UCSSEL_2;   // SMCLK
	UCA0BR0 = 104;        // 1MHz 9600
	UCA0BR1 = 0;            // 1MHz 9600*/
	UCA0MCTL = UCBRS0;      // Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;   // **Initialize USCI state machine**
	IE2 |= UCA0RXIE;        // Enable USCI_A0 RX interrupt
}

// uart start transmitting (transmit next character in buffer)
int uart_start_tx(void)
{
	if (uart_tx_inptr==uart_tx_outptr)
	{
		uart_tx_transmitt=false; // clear transmit flag
		return -1; // don't start when buffer empty
	}
	UART_TX_LED_ON(); // LED ON
	//while (!(IFG2&UCA0TXIFG));	// USCI_A0 TX buffer ready?
#ifdef UART_TX_BUFMASK
	unsigned int new_ptr = (uart_tx_outptr+1)&UART_TX_BUFMASK;
#else
	unsigned int new_ptr = (uart_tx_outptr+1)%UART_TX_BUFLEN;
#endif
	uart_tx_transmitt=true; // set transmit flag
	UCA0TXBUF = uart_tx_buffer[new_ptr]; // TX character
	uart_tx_outptr = new_ptr;
	IE2 |= UCA0TXIE;		// Enable USCI_A0 TX interrupt
	return 0; // return ok
}

// uart put char function
int uart_putc(char c)
{
#ifdef UART_TX_BUFMASK
	unsigned int new_ptr = (uart_tx_inptr+1)&UART_TX_BUFMASK;
#else
	int new_ptr = (uart_tx_inptr+1)%UART_TX_BUFLEN;
#endif
	if (new_ptr==uart_tx_outptr) return -1; // buffer full
	uart_tx_buffer[new_ptr] = c;
	uart_tx_inptr=new_ptr;
	if (!uart_tx_transmitt) return uart_start_tx(); // return ok (if buffer not empty)
	return 0; // return ok
}

// uart put string function
int uart_puts(char *s)
{
	unsigned int ptr = 0;
	while (s[ptr]!='\0')
		if (uart_putc(s[ptr++]))
			break;
	return ptr;
}

// interrupt handlers

// uart RX interrupt handler
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    tx_output_enable(true);
	//UART_TX_LED_ON();
	char c = UCA0RXBUF;		// read char
    if (c=='?')
    {
        uart_puts("Hello World!\n");
	}
}

// uart TX interrupt handler
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
    if (uart_start_tx()!=0)
    {
        UART_TX_LED_OFF();
        IE2 &= ~UCA0TXIE;		// Disable USCI_A0 TX interrupt
    }
}
