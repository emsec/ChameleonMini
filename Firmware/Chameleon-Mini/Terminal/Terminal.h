/*
 * CommandLine.h
 *
 *  Created on: 10.02.2013
 *      Author: skuser
 */

#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "../Common.h"
#include "../LUFA/Drivers/USB/USB.h"
#include "XModem.h"
#include "CommandLine.h"

#define TERMINAL_VBUS_PORT      PORTD
#define TERMINAL_VBUS_MASK      PIN5_bm

#ifdef MEMORY_LIMITED_TESTING
#define TERMINAL_BUFFER_SIZE    256 // 384
#else
#define TERMINAL_BUFFER_SIZE	512
#endif

typedef enum {
    TERMINAL_UNINITIALIZED,
    TERMINAL_INITIALIZING,
    TERMINAL_INITIALIZED,
    TERMINAL_UNITIALIZING
} TerminalStateEnum;

extern uint8_t TerminalBuffer[TERMINAL_BUFFER_SIZE];
extern uint16_t TerminalBufferIdx;
extern USB_ClassInfo_CDC_Device_t TerminalHandle;
extern TerminalStateEnum TerminalState;

void TerminalInit(void);
void TerminalTask(void);
void TerminalTick(void);

/*void TerminalSendHex(void* Buffer, uint16_t ByteCount);*/
INLINE void TerminalSendByte(uint8_t Byte);
INLINE void TerminalFlushBuffer(void);
void TerminalSendBlock(const void *Buffer, uint16_t ByteCount);

INLINE void TerminalSendChar(char c);
void TerminalSendString(const char *s);
void TerminalSendStringP(const char *s);

void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);

INLINE void TerminalSendChar(char c) { CDC_Device_SendByte(&TerminalHandle, c); }
INLINE void TerminalSendByte(uint8_t Byte) { CDC_Device_SendByte(&TerminalHandle, Byte); }

INLINE void TerminalFlushBuffer(void) {
    CDC_Device_Flush(&TerminalHandle);
    TerminalBufferIdx = 0;
    TerminalBuffer[TerminalBufferIdx] = '\0';
}

#endif /* TERMINAL_H_ */
