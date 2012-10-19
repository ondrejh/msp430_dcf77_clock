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
#include <inttypes.h>
#include <stdbool.h>
#include "dcf77.h" // self

// input init (pull up resistor)
//#define DCF77_INPUT_INIT() {P1DIR&=~BIT7;P1OUT|=BIT7;P1REN|=BIT7;}
#define DCF77_INPUT_INIT() {P1DIR&=~BIT7;P1OUT|=BIT7;}
// input reading (result true if input pulled down)
#define DCF77_INPUT() (((P1IN&BIT7)==0)?true:false)
// timer interval
#define DCF77_STROBE_TIMER_INTERVAL 125 // 1kHz (1MHz/8 input clock)

#define DCF77_LED 1
#if DCF77_LED
    #define DCF77_LED_INIT() {P1DIR|=0x01;P1OUT&=~0x01;}
    #define DCF77_LED_ON() {P1OUT|=0x01;}
    #define DCF77_LED_OFF() {P1OUT&=~0x01;}
    #define DCF77_LED_SWAP() {P1OUT^=0x01;}
#else
    #define DCF77_LED_INIT() {}
    #define DCF77_LED_ON() {}
    #define DCF77_LED_OFF() {}
    #define DCF77_LED_SWAP() {}
#endif

// symbol detection timing
#define DCF77_DETECT_PERIOD (int)1000
#define DCF77_S0_PERIOD (int)100
#define DCF77_S1_PERIOD (int)200
// fine synchronization offset (in dcf77 timer ticks)
#define DCF77_FINESYNC_OFFSET 10
// minimul quality of signal (out of 1000)
#define DCF77_MIN_SIGNAL_QUALITY 800
// hold over and fine synchronization timing
#define DCF77_MAX_HOLD_SYMBOLS 300
#define DCF77_FINETUNE_SYMCOUNT 60

// dcf strobe variables
typedef enum {DCF77SYNC_COARSE,DCF77SYNC_FINE,DCF77SYNC_HOLD} dcf77_sync_mode_type;
dcf77_sync_mode_type dcf77_sync_mode = DCF77SYNC_COARSE;

typedef enum {DCF77_SYMBOL_NONE,DCF77_SYMBOL_0,DCF77_SYMBOL_1,DCF77_SYMBOL_MINUTE} dcf77_symbol_type;

// detector context type
typedef struct {
    int cnt; // counter
    dcf77_symbol_type sym; // last symbol buffer
    int sigQcnt,sigQ; // signal quality
    bool ready; // just detected flag

    int s0cnt,s1cnt,sMcnt; // symbol counters (very internal)
} dcf77_detector_context;

// function finding index and value of the biggest value from three values
// it is used in symbol matching (dcf77_detect) and fine synchronization (dcf77_strobe)
int find_biggest(int val0, int val1, int val2, int *val)
{
    int i=0;
    int theMost = val0;
    if (val1>theMost) {theMost=val1;i=1;};
    if (val2>theMost) {theMost=val2;i=2;};
    *val = theMost;
    return i;
}

// function decode dcf data
void dcf77_decode(uint16_t *data,uint16_t *valid)
{

}

// function memorize one minute symbols
void dcf77_symbol_memory(dcf77_symbol_type symbol)
{
    static uint16_t data[4];
    static uint16_t valid[4];
    static int cnt = 0, dcnt = 0;
    static uint16_t mask = 1;

    // save symbol
    if (symbol==DCF77_SYMBOL_1) data[dcnt]|=mask;
    if (symbol==DCF77_SYMBOL_NONE) valid[dcnt]|=mask;

    // increase counter & mask
    mask<<=1;
    if (mask==0) {dcnt++;mask=1;};
    cnt++;

    // test if symbol counter == 60 and symbol != "0" or "1"
    if ((cnt==60)&&((symbol==DCF77_SYMBOL_MINUTE)||(symbol==DCF77_SYMBOL_NONE)))
    {
        // try to decode
        dcf77_decode(data,valid);
    }

    // if minute symbol or counter == 60 => reset counter
    if ((cnt==60)||(symbol==DCF77_SYMBOL_MINUTE))
    {
        // reset counter
        cnt=0; mask=1; dcnt=0;
    }
}

// reset dcf detector context
void dcf77_reset_context(dcf77_detector_context *detector,int offset)
{
    detector->cnt = offset;
    detector->sigQcnt = 0;
    detector->ready = false;
    detector->s0cnt = 0;
    detector->s1cnt = 0;
    detector->sMcnt = 0;
}

// dcf signal detect function
void dcf77_detect(dcf77_detector_context *detector, bool signal)
{
    // if cnt full, reset context
    if (detector->cnt>=DCF77_DETECT_PERIOD)
        dcf77_reset_context(detector,detector->cnt-DCF77_DETECT_PERIOD);

    // test symbols
    if (detector->cnt<DCF77_S0_PERIOD) // logic 0 (<100ms)
    {
        if (signal==true)
        {
            detector->s0cnt++;
            detector->s1cnt++;
        }
        else
        {
            detector->sMcnt++;
        }
    }
    else if (detector->cnt<DCF77_S1_PERIOD) // logic 1 (<200ms)
    {
        if (signal==true)
        {
            detector->s1cnt++;
        }
        else
        {
            detector->s0cnt++;
            detector->sMcnt++;
        }
    }
    else // signal quality (rest of strobed signal)
    {
        detector->sigQcnt+=signal?0:1;
    }

    detector->cnt++;
    if (detector->cnt>=DCF77_DETECT_PERIOD) // it should be all
    {
        int symQ;
        int symI = find_biggest(detector->s0cnt,detector->s1cnt,detector->sMcnt,&symQ);
        // save overall signal quality value (symbol and pause)
        detector->sigQ = detector->sigQcnt+symQ;
        // decode symbol
        if (detector->sigQ>=DCF77_MIN_SIGNAL_QUALITY)
            detector->sym=symI+1;
        else
            detector->sym=DCF77_SYMBOL_NONE;
        /*switch (symI)
        {
            case 0: detector->sym=DCF77_SYMBOL_0; break;
            case 1: detector->sym=DCF77_SYMBOL_1; break;
            case 2: detector->sym=DCF77_SYMBOL_MINUTE; break;
            default: detector->sym=DCF77_SYMBOL_NONE; break;
        }*/
        // rise it's ready flag
        detector->ready=true;
    }
}

// strobe function
void dcf77_strobe(void)
{
    // dcf detectors (3 for fine synchronization: sooner, now, later)
    static dcf77_detector_context detector[3];

    static bool last_dcf77sig = false;
    bool dcf77sig = DCF77_INPUT();
    int i;

    static int FineTune = 0;

    static int hold_counter = 0;


    // coarse synchronization
    if (dcf77_sync_mode == DCF77SYNC_COARSE)
    {
        if (detector[0].ready==true)
        {
            if ((detector[0].sym!=DCF77_SYMBOL_MINUTE)&&(detector[0].sym!=DCF77_SYMBOL_NONE))
            {
                dcf77_sync_mode=DCF77SYNC_FINE;
                DCF77_LED_ON();
            }
        }
        else
        {
            if ((dcf77sig!=last_dcf77sig)&&dcf77sig) // detect rising edge
            {
                // reset contexts
                dcf77_reset_context(&detector[0],+DCF77_FINESYNC_OFFSET);
                dcf77_reset_context(&detector[1],0);
                dcf77_reset_context(&detector[2],-DCF77_FINESYNC_OFFSET);
                //DCF77_LED_SWAP(); // debug
            }
        }
    }

    // detection and decoding
    for (i=0;i<3;i++) dcf77_detect(&detector[i],dcf77sig); // detection
    //if (detector[1].ready==true) dcf77_decode(detector[1].sym);

    // fine synchronization
    if (dcf77_sync_mode==DCF77SYNC_FINE)
    {
        if (detector[1].ready==true)
        {
            if (detector[1].sym==DCF77_SYMBOL_NONE)
            {
                dcf77_sync_mode=DCF77SYNC_HOLD;
                hold_counter=0;
            }
        }
        if (detector[2].ready==true)
        {
            int Q,b;
            b=find_biggest(detector[0].sigQ,detector[1].sigQ,detector[2].sigQ,&Q);
            if (b==0)
            {
                FineTune--;
                if (FineTune<-DCF77_FINETUNE_SYMCOUNT)
                {
                    FineTune=0;
                    detector[0].cnt++;
                    detector[1].cnt++;
                    detector[2].cnt++;
                }
            }
            if (b==2)
            {
                FineTune++;
                if (FineTune>DCF77_FINETUNE_SYMCOUNT)
                {
                    FineTune=0;
                    detector[0].cnt--;
                    detector[1].cnt--;
                    detector[2].cnt--;
                }
            }
        }
    }

    // hold over
    if (dcf77_sync_mode==DCF77SYNC_HOLD)
    {
        if (detector[1].ready==true)
        {
            DCF77_LED_SWAP();
            if (detector[1].sigQ!=DCF77_SYMBOL_NONE)
            {
                if (detector[1].sym!=DCF77_SYMBOL_MINUTE)
                {
                    dcf77_sync_mode=DCF77SYNC_FINE;
                    DCF77_LED_ON();
                }
            }
            else
            {
                hold_counter++;
                if (hold_counter>DCF77_MAX_HOLD_SYMBOLS)
                {
                    dcf77_sync_mode=DCF77SYNC_COARSE;
                    DCF77_LED_OFF();
                }
            }
        }
    }

    // save last input (for coarse sync.)
    last_dcf77sig = dcf77sig;
}

/// module initialization function
// timer and input init
void dcf77_init(void)
{
    // input init
    DCF77_INPUT_INIT();
    DCF77_LED_INIT(); // debug led (en/dis by DCF77_LED macro value)

    // timer init
	TA1CCTL0 = CCIE;				// CCR0 interrupt enabled
	TA1CCR0 = DCF77_STROBE_TIMER_INTERVAL;
	TA1CTL = TASSEL_2 + MC_2 + ID_3;	// SMCLK, contmode, fosc/8
}

// Timer A1 interrupt service routine
#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer1 (void)
{
    TA1CCR0 += DCF77_STROBE_TIMER_INTERVAL;	// Add Offset to CCR0

    dcf77_strobe();
}
