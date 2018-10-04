/*
 * System.h
 *
 *  Created on: 10.02.2013
 *      Author: skuser
 */

#ifndef SYSTEM_H
#define SYSTEM_H

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stddef.h>
#include <stdbool.h>
#include "Common.h"

typedef uint16_t SystemRTCType;

#define F_RTC   1000
#define SYSTEM_MILLISECONDS_TO_RTC_CYCLES(x) \
    ( (uint16_t) ( (double) F_RTC * x / 1E3 + 0.5) )

#define SYSTEM_TICK_WIDTH	7 /* Bits */
#define SYSTEM_TICK_PERIOD  (1<<7)
#define SYSTEM_TICK_FREQ    10
#define SYSTEM_TICK_MS		(1000/SYSTEM_TICK_FREQ)

#define SYSTEM_SMODE_PSAVE	SLEEP_SMODE_PSAVE_gc
#define SYSTEM_SMODE_IDLE	SLEEP_SMODE_IDLE_gc

/* Use GPIORE and GPIORF as global tick register */
#define SYSTEM_TICK_REGISTER	(*((volatile uint16_t*) &GPIORE))

void SystemInit(void);
void SystemReset(void);
void SystemEnterBootloader(void);
void SystemStartUSBClock(void);
void SystemStopUSBClock(void);
void SystemInterruptInit(void);
INLINE bool SystemTick100ms(void);
INLINE void SystemSleep(void);
INLINE void SystemSleepEnable(void);
INLINE void SystemSleepDisable(void);
INLINE void SystemSleepSetMode(uint8_t SleepMode);

INLINE bool SystemTick100ms(void)
{
    if (RTC.INTFLAGS & RTC_COMPIF_bm) {
    	while(RTC.STATUS & RTC_SYNCBUSY_bm)
    		;

    	RTC.INTFLAGS = RTC_COMPIF_bm;
    	return true;
    }

    return false;
}

INLINE uint16_t SystemGetSysTick(void) {
	return SYSTEM_TICK_REGISTER | RTC.CNT;
}

INLINE void SystemSleep(void)
{
	asm volatile("sleep");
}

INLINE void SystemSleepEnable(void)
{
	SLEEP.CTRL |= SLEEP_SEN_bm;
}

INLINE void SystemSleepDisable(void)
{
	SLEEP.CTRL &= ~SLEEP_SEN_bm;
}

INLINE void SystemSleepSetMode(uint8_t SleepMode)
{
	SLEEP.CTRL = (SLEEP.CTRL & ~SLEEP_SMODE_gm) | SleepMode;
}

#endif /* SYSTEM_H */
