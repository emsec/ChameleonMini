/* Copyright 2013 Timo Kasper, Simon Küppers, David Oswald ("ORIGINAL
 * AUTHORS"). All rights reserved.
 *
 * DEFINITIONS:
 *
 * "WORK": The material covered by this license includes the schematic
 * diagrams, designs, circuit or circuit board layouts, mechanical
 * drawings, documentation (in electronic or printed form), source code,
 * binary software, data files, assembled devices, and any additional
 * material provided by the ORIGINAL AUTHORS in the ChameleonMini project
 * (https://github.com/skuep/ChameleonMini).
 *
 * LICENSE TERMS:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK are permitted provided that the
 * following conditions are met:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK must include the above copyright
 * notice, this list of conditions, the below disclaimer, and the following
 * attribution:
 *
 * "Based on ChameleonMini an open-source RFID emulator:
 * https://github.com/skuep/ChameleonMini"
 *
 * The attribution must be clearly visible to a user, for example, by being
 * printed on the circuit board and an enclosure, and by being displayed by
 * software (both in binary and source code form).
 *
 * At any time, the majority of the ORIGINAL AUTHORS may decide to give
 * written permission to an entity to use or redistribute the WORK (with or
 * without modification) WITHOUT having to include the above copyright
 * notice, this list of conditions, the below disclaimer, and the above
 * attribution.
 *
 * DISCLAIMER:
 *
 * THIS PRODUCT IS PROVIDED BY THE ORIGINAL AUTHORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE ORIGINAL AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS PRODUCT, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the hardware, software, and
 * documentation should not be interpreted as representing official
 * policies, either expressed or implied, of the ORIGINAL AUTHORS.
 */

#include "System.h"
#include "LED.h"

ISR(BADISR_vect)
{
    while(1);
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

    /* Enable RTC with roughly 1kHz clock */
    CLK.RTCCTRL = CLK_RTCSRC_ULP_gc | CLK_RTCEN_bm;
    RTC.CTRL = RTC_PRESCALER_DIV1_gc;

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
