/*
 * System.c
 *
 *  Created on: 10.02.2013
 *      Author: skuser
 */

#include "System.h"
#include "LED.h"
#include <avr/interrupt.h>

#ifndef WDT_PER_500CLK_gc
#define WDT_PER_500CLK_gc WDT_PER_512CLK_gc
#endif

ISR(BADISR_vect) {
    //LED_PORT.OUTSET = LED_RED;
    while (1);
}

ISR(RTC_OVF_vect) {
    SYSTEM_TICK_REGISTER += SYSTEM_TICK_PERIOD;
}

void SystemInit(void) {
    if (RST.STATUS & RST_WDRF_bm) {
        /* On Watchdog reset clear WDRF bit, disable watchdog
         * and jump into bootloader */
        RST.STATUS = RST_WDRF_bm;

        CCP = CCP_IOREG_gc;
        WDT.CTRL = WDT_CEN_bm;

        asm volatile("jmp %0"::"i"(BOOT_SECTION_START + 0x1FC));
    }

    /* XTAL x 2 as SysCLK */
    OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
    OSC.CTRL |= OSC_XOSCEN_bm;
    while (!(OSC.STATUS & OSC_XOSCRDY_bm))
        ;

    OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | (2 << OSC_PLLFAC_gp);
    OSC.CTRL |= OSC_PLLEN_bm;

    while (!(OSC.STATUS & OSC_PLLRDY_bm))
        ;

    /* Set PLL as main clock */
    CCP = CCP_IOREG_gc;
    CLK.CTRL = CLK_SCLKSEL_PLL_gc;

    SYSTEM_TICK_REGISTER = 0;

    /* Enable RTC with roughly 1kHz clock for system tick
     * and to wake up while sleeping. */
    CLK.RTCCTRL = CLK_RTCSRC_ULP_gc | CLK_RTCEN_bm;
    RTC.PER = SYSTEM_TICK_PERIOD - 1;
    RTC.COMP = SYSTEM_TICK_PERIOD - 1;
    RTC.CTRL = RTC_PRESCALER_DIV1_gc;
    RTC.INTCTRL = RTC_OVFINTLVL_LO_gc;

    /* Enable EEPROM data memory mapping */
    //NVM.CTRLB |= NVM_EEMAPEN_bm;

    /* Enable DMA */
    DMA.CTRL = DMA_ENABLE_bm | DMA_DBUFMODE_DISABLED_gc | DMA_PRIMODE_RR0123_gc;

}

void SystemReset(void) {
    CCP = CCP_IOREG_gc;
    RST.CTRL = RST_SWRST_bm;
}

void SystemEnterBootloader(void) {
    /* Use Watchdog timer to reset into bootloader. */
    CCP = CCP_IOREG_gc;
    WDT.CTRL = WDT_PER_500CLK_gc | WDT_ENABLE_bm | WDT_CEN_bm;
}


void SystemStartUSBClock(void) {
    //SystemSleepDisable();
#if 0
    /* 48MHz USB Clock using 12MHz XTAL */
    OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
    OSC.CTRL |= OSC_XOSCEN_bm;
    while (!(OSC.STATUS & OSC_XOSCRDY_bm))
        ;

    OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | (4 << OSC_PLLFAC_gp);

    OSC.CTRL |= OSC_PLLEN_bm;
    while (!(OSC.STATUS & OSC_PLLRDY_bm))
        ;
#else
    /* Use internal HS RC for USB */
    OSC.CTRL |= OSC_RC32MEN_bm;
    while (!(OSC.STATUS & OSC_RC32MRDY_bm))
        ;

    /* Load RC32 CAL values for 48MHz use */
    NVM.CMD  = NVM_CMD_READ_CALIB_ROW_gc;
    DFLLRC32M.CALA = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, USBRCOSCA));
    DFLLRC32M.CALB = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, USBRCOSC));
    NVM.CMD  = NVM_CMD_NO_OPERATION_gc;

    /* For USBSOF operation, whe need to use COMP = 48MHz/1kHz = 0xBB80 */
    DFLLRC32M.COMP1 = 0x80;
    DFLLRC32M.COMP2 = 0xBB;

    /* Select USBSOF as DFLL source */
    OSC.DFLLCTRL &= ~OSC_RC32MCREF_gm;
    OSC.DFLLCTRL |= OSC_RC32MCREF_USBSOF_gc;

    /* Enable DFLL */
    DFLLRC32M.CTRL |= DFLL_ENABLE_bm;
#endif
}

void SystemStopUSBClock(void) {
    //SystemSleepEnable();

#if 0
    /* Disable USB Clock to minimize power consumption */
    CLK.USBCTRL &= ~CLK_USBSEN_bm;
    OSC.CTRL &= ~OSC_PLLEN_bm;
    OSC.CTRL &= ~OSC_XOSCEN_bm;
#else
    /* Disable USB Clock to minimize power consumption */
    CLK.USBCTRL &= ~CLK_USBSEN_bm;
    DFLLRC32M.CTRL &= ~DFLL_ENABLE_bm;
    OSC.CTRL &= ~OSC_RC32MEN_bm;
#endif
}

void SystemInterruptInit(void) {
    /* Enable all interrupt levels */
    PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
    sei();
}
