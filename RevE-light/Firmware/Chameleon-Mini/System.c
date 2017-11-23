/*
 * System.c
 *
 *  Created on: 10.02.2013
 *      Author: skuser
 */

#include "System.h"
#include "LED.h"
#include <avr/interrupt.h>

/* AVR Toolchain 3.4.5 changed defines */
#ifndef WDT_PER_500CLK_gc
#define WDT_PER_500CLK_gc WDT_PER_512CLK_gc
#endif


ISR(BADISR_vect)
{
    while(1);
}

ISR(RTC_OVF_vect)
{
	SYSTEM_TICK_REGISTER += SYSTEM_TICK_PERIOD;
}

void SystemInit(void)
{
    if (RST.STATUS & RST_WDRF_bm) {
        /* On Watchdog reset clear WDRF bit, disable watchdog
        * and jump into bootloader */
        RST.STATUS = RST_WDRF_bm;

        CCP = CCP_IOREG_gc;
        WDT.CTRL = WDT_CEN_bm;

        asm volatile ("jmp %0"::"i" (BOOT_SECTION_START + 0x1FC));
    }

    /* 32MHz system clock using internal RC and 32K DFLL*/
    OSC.CTRL |= OSC_RC32MEN_bm | OSC_RC32KEN_bm;
    while(!(OSC.STATUS & OSC_RC32MRDY_bm))
        ;
    while(!(OSC.STATUS & OSC_RC32KRDY_bm))
        ;

    OSC.DFLLCTRL = OSC_RC32MCREF_RC32K_gc;
    DFLLRC32M.CTRL = DFLL_ENABLE_bm;

    CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_RC32M_gc;

    /* Use TCE0 as system tick */
    TCE0.PER = F_CPU / 256 / SYSTEM_TICK_FREQ - 1;
    TCE0.CTRLA = TC_CLKSEL_DIV256_gc;

    /* Enable RTC with roughly 1kHz clock for system tick
     * and to wake up while sleeping. */
    CLK.RTCCTRL = CLK_RTCSRC_ULP_gc | CLK_RTCEN_bm;
    RTC.PER = 1000 / SYSTEM_TICK_FREQ - 1;
    RTC.COMP = 1000 / SYSTEM_TICK_FREQ - 1;
    RTC.CTRL = RTC_PRESCALER_DIV1_gc;
    RTC.INTCTRL = RTC_OVFINTLVL_LO_gc;


    /* Enable EEPROM data memory mapping */
    NVM.CTRLB |= NVM_EEMAPEN_bm;
}

void SystemReset(void)
{
    CCP = CCP_IOREG_gc;
    RST.CTRL = RST_SWRST_bm;

}

void SystemEnterBootloader(void)
{
    /* Use Watchdog timer to reset into bootloader. */
    CCP = CCP_IOREG_gc;
    WDT.CTRL = WDT_PER_500CLK_gc | WDT_ENABLE_bm | WDT_CEN_bm;
}


void SystemStartUSBClock(void)
{
	SystemSleepDisable();

    /* 48MHz USB Clock using 12MHz XTAL */
    OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
    OSC.CTRL |= OSC_XOSCEN_bm;
    while(!(OSC.STATUS & OSC_XOSCRDY_bm))
        ;

    OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | (4 << OSC_PLLFAC_gp);

    OSC.CTRL |= OSC_PLLEN_bm;
    while(!(OSC.STATUS & OSC_PLLRDY_bm))
        ;
}

void SystemStopUSBClock(void)
{
	SystemSleepEnable();

    /* Disable USB Clock to minimize power consumption */
    CLK.USBCTRL &= ~CLK_USBSEN_bm;
    OSC.CTRL &= ~OSC_PLLEN_bm;
    OSC.CTRL &= ~OSC_XOSCEN_bm;
}

void SystemInterruptInit(void)
{
    /* Enable all interrupt levels */
    PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
    sei();
}

