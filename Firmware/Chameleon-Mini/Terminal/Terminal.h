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

#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "../Common.h"
#include "../LUFA/Drivers/USB/USB.h"
#include "XModem.h"
#include "CommandLine.h"

#define TERMINAL_VBUS_PORT      PORTD
#define TERMINAL_VBUS_MASK      PIN5_bm

#define TERMINAL_BUFFER_SIZE	256

typedef enum {
	TERMINAL_UNINITIALIZED,
	TERMINAL_INITIALIZING,
	TERMINAL_INITIALIZED,
	TERMINAL_UNITIALIZING
} TerminalStateEnum;

extern uint8_t TerminalBuffer[TERMINAL_BUFFER_SIZE];
extern USB_ClassInfo_CDC_Device_t TerminalHandle;
extern TerminalStateEnum TerminalState;

void TerminalInit(void);
void TerminalTask(void);
void TerminalTick(void);

/*void TerminalSendBuffer(void* Buffer, uint16_t ByteCount);*/
INLINE void TerminalSendByte(uint8_t Byte);
void TerminalSendBlock(void* Buffer, uint16_t ByteCount);

INLINE void TerminalSendChar(char c);
void TerminalSendString(const char* s);
void TerminalSendStringP(const char* s);

void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);

INLINE void TerminalSendChar(char c) { CDC_Device_SendByte(&TerminalHandle, c); }
INLINE void TerminalSendByte(uint8_t Byte) { CDC_Device_SendByte(&TerminalHandle, Byte); }

#endif /* TERMINAL_H_ */
