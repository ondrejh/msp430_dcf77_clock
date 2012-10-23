#ifndef __RTC_H__
#define __RTC_H__

#include <inttypes.h>

// rtc sampling frequency (less - less interrupts, more - lower delay)
#define RTC_SAMPLING_FREQV 512 // should be power of 2 (2,4,8 tested)

// time structure
typedef struct
{
    uint8_t second; // 0..59
    uint8_t minute; // 0..59
    uint8_t hour; // 0..23
    uint8_t dayow; // (day of week) 0..6
} tstruct;

void rtc_set_time(tstruct *tset); // time synchronization
void rtc_get_time(tstruct *tget); // get time function

void rtc_timer_init(void); // init function


#endif // __RTC_H__
