/*
 * UartCmd.c
 *
 *  Created on: 20.03.2013
 *      Author: Willok
 *
 *  ChangeLog
 *    2019-09-22    Willok    Added processing of UART command
 *
 */
#ifdef CHAMELEON_TINY_UART_MODE

#include "Uart.h"
#include "UartCmd.h"
#include "Settings.h"
#include "Configuration.h"
#include "Application/Application.h"

#include "System.h"


uint16_t CmdLen = 0;
uint8_t CmdBuffer[256];

//    Check Summing
uint8_t GetChkSum(PCMD_HEAD pCmdHead, uint8_t *DataPtr) {
    uint8_t bRet = 0;

    if (!DataPtr)
        DataPtr = (uint8_t *)(pCmdHead + 1);

    bRet -= pCmdHead->bSign;
    bRet -= pCmdHead->bCmd;
    bRet -= pCmdHead->bCmdLen;

    for (uint8_t i = 0; i < pCmdHead->bCmdLen; i++) {
        bRet -= DataPtr[i];
    }

    return bRet;
}

//    UART  command initialization
void UartCmdInit(void) {
    CmdLen = 0;
    CmdBuffer[0] = 0;
}



//    USBmode or UARTmode
extern uint8_t bUSBTerminal;

//    Received UART command data. Need to be handed over to the command line
void CmdUartRx(PCMD_HEAD CmdHead, uint8_t *UartData) {
    if (!bUSBTerminal) {
        while (CmdHead->bCmdLen) {
            if (XModemProcessByte(*UartData)) {
                /* XModem handled the byte */
            } else if (CommandLineProcessByte(*UartData)) {
                /* CommandLine handled the byte */
            }

            UartData++;
            CmdHead->bCmdLen--;
        }
    }
}

//    Heartbeat marker
uint8_t bKeepAlive = 1;

//    Version number with compilation date
char bVersion[] = "v1.0 " BUILD_DATE " " __TIME__;

//    Command tick, send heartbeat packet regularly
void UartCmdTick(void) {
    static uint8_t KeepAliveTick = 200;

    //    heartbeat every 2S
    if (++KeepAliveTick >= 20) {
        KeepAliveTick = 0;

        if (bKeepAlive) {
            uint8_t Buffer[sizeof(CMD_HEAD) + sizeof(CMD_KEEP_ALIVE)];
            PCMD_HEAD CmdHead = (PCMD_HEAD)Buffer;
            PCMD_KEEP_ALIVE pKeepAlive = (PCMD_KEEP_ALIVE)(CmdHead + 1);

            //    Fill handshake package
            CmdHead->bSign = 0xA5;
            CmdHead->bCmd = CMD_TYPE_ALIVE;
            CmdHead->bCmdLen = sizeof(CMD_KEEP_ALIVE);
            pKeepAlive->bMajVer = 1;
            pKeepAlive->bMinVer = 0;
            memcpy(pKeepAlive->bVerStr, bVersion, sizeof(bVersion));

            CmdHead->bChkSum = GetChkSum(CmdHead, NULL);
            //    Send handshake package
            uart_putb((uint8_t *)CmdHead, sizeof(CMD_HEAD) + CmdHead->bCmdLen);
        }
    }
}

//    Receive shutdown notification
void CmdUartPowerDown(PCMD_HEAD CmdHead, uint8_t *UartData) {
    //    Switch to empty configuration, not saved
    ConfigurationSetById(CONFIG_NONE);

    //    Save log on demand
    LogTick();

    //    Flash two LED to warning power off
    LED_PORT.OUTSET = PIN4_bm;
    LED_PORT.OUTCLR = PIN3_bm;
    while (1) {
        if (SystemTick100ms()) {
            LED_PORT.OUTTGL = PIN4_bm;
            LED_PORT.OUTTGL = PIN3_bm;
        }
    }
}


//    Command processing callback
void UartCmdTask(void) {
    int16_t RecvData;
    //    Loop data in receive buffer
    while ((RecvData = UartFIFOGet()) >= 0) {
        CmdBuffer[CmdLen] = (uint8_t)RecvData;

        //    The first character must be special, otherwise it will be discarded
        if (CmdLen == 0 && CmdBuffer[0] != CMD_HEAD_SIGN)
            continue;


        //    Statistics received
        CmdLen++;

        //    At least one head is needed, then we can handle it.
        if (CmdLen >= sizeof(CMD_HEAD)) {
            PCMD_HEAD CmdHead = (PCMD_HEAD)CmdBuffer;

            //    If the command length is too long, there must be error. Discard the data and wait for resynchronization.
            if (CmdHead->bCmdLen >= (sizeof(CmdBuffer) - sizeof(CMD_HEAD))) {
                CmdLen = 0;
                continue;
            }

            //    Judge whether the received data is complete
            if (CmdLen >= sizeof(CMD_HEAD) + CmdHead->bCmdLen) {
                //    Calculate the check sum, otherwise discard it.
                if (CmdHead->bChkSum != GetChkSum(CmdHead, NULL)) {
                    CmdLen = 0;
                    continue;
                }

                //    Processing command
                switch (CmdHead->bCmd) {
                    case CMD_UART_RXTX:         //    Uart data
                        CmdUartRx(CmdHead, (uint8_t *)(CmdHead + 1));
                        break;
                    case CMD_TYPE_POWERDOWN:    //    Switch to empty configuration and wait for power failure
                        CmdUartPowerDown(CmdHead, (uint8_t *)(CmdHead + 1));
                        break;
                    case CMD_TYPE_ALIVE:        //    Receive the heartbeat package response, turn off the heartbeat
                        bKeepAlive = 0;
                        break;
                    default:
                        break;
                }

                //    Clear command buffer after command processing
                CmdLen = 0;
            }
        }
    }
}

#endif
