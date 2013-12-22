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

#include "Terminal.h"
#include "../System.h"
#include "../LED.h"

#include "../LUFADescriptors.h"

#define INIT_DELAY		(2000 / SYSTEM_TICK_MS)


USB_ClassInfo_CDC_Device_t TerminalHandle = {
    .Config = {
        .ControlInterfaceNumber = 0,
        .DataINEndpoint = {
            .Address = CDC_TX_EPADDR,
            .Size = CDC_TXRX_EPSIZE,
            .Banks = 1,
        }, .DataOUTEndpoint = {
            .Address = CDC_RX_EPADDR,
            .Size = CDC_TXRX_EPSIZE,
            .Banks = 1,
        }, .NotificationEndpoint = {
            .Address = CDC_NOTIFICATION_EPADDR,
            .Size = CDC_NOTIFICATION_EPSIZE,
            .Banks = 1,
        },
    }
};

uint8_t TerminalBuffer[TERMINAL_BUFFER_SIZE];
TerminalStateEnum TerminalState = TERMINAL_UNINITIALIZED;
static uint8_t TerminalInitDelay = INIT_DELAY;

void TerminalSendString(const char* s) {
    CDC_Device_SendString(&TerminalHandle, s);
}

void TerminalSendStringP(const char* s) {
    char c;

    while( (c = pgm_read_byte(s++)) != '\0' ) {
        TerminalSendChar(c);
    }
}

#if 0
void TerminalSendBuffer(void* Buffer, uint16_t ByteCount)
{
    char* pTerminalBuffer = (char*) TerminalBuffer;

    BufferToHexString(pTerminalBuffer, sizeof(TerminalBuffer), Buffer, ByteCount);

    TerminalSendString(pTerminalBuffer);
}
#endif



void TerminalSendBlock(void* Buffer, uint16_t ByteCount)
{
    CDC_Device_SendData(&TerminalHandle, Buffer, ByteCount);
}


static void ProcessByte(void) {
    int16_t Byte = CDC_Device_ReceiveByte(&TerminalHandle);

    if (Byte >= 0) {
        /* Byte received */
    	LEDPulse(LED_RED);

        if (XModemProcessByte(Byte)) {
            /* XModem handled the byte */
        } else if (CommandLineProcessByte(Byte)) {
            /* CommandLine handled the byte */
        }
    }
}

static void SenseVBus(void)
{
    switch(TerminalState) {
    case TERMINAL_UNINITIALIZED:
    	if (TERMINAL_VBUS_PORT.IN & TERMINAL_VBUS_MASK) {
    		/* Not initialized and VBUS sense high */
    		TerminalInitDelay = INIT_DELAY;
    		TerminalState = TERMINAL_INITIALIZING;
    	}
    break;

    case TERMINAL_INITIALIZING:
    	if (--TerminalInitDelay == 0) {
            SystemStartUSBClock();
            USB_Init();
            TerminalState = TERMINAL_INITIALIZED;
    	}
    	break;

    case TERMINAL_INITIALIZED:
    	if (!(TERMINAL_VBUS_PORT.IN & TERMINAL_VBUS_MASK)) {
    		/* Initialized and VBUS sense low */
    		TerminalInitDelay = INIT_DELAY;
    		TerminalState = TERMINAL_UNITIALIZING;
    	}
    	break;

    case TERMINAL_UNITIALIZING:
    	if (--TerminalInitDelay == 0) {
        	USB_Disable();
        	SystemStopUSBClock();
        	TerminalState = TERMINAL_UNINITIALIZED;
    	}
    	break;

    default:
    	break;
    }
}

void TerminalInit(void)
{
    TERMINAL_VBUS_PORT.DIRCLR = TERMINAL_VBUS_MASK;
}

void TerminalTask(void)
{
	CDC_Device_USBTask(&TerminalHandle);
	USB_USBTask();

    ProcessByte();
}

void TerminalTick(void)
{
	SenseVBus();

    XModemTick();
    CommandLineTick();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
    LEDSetOn(LED_GREEN);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
    LEDSetOff(LED_GREEN);
}


/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    CDC_Device_ConfigureEndpoints(&TerminalHandle);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
    CDC_Device_ProcessControlRequest(&TerminalHandle);
}


