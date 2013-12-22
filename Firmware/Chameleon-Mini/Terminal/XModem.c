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

#include "XModem.h"
#include "Terminal.h"

#define BYTE_NAK        0x15
#define BYTE_SOH        0x01
#define BYTE_ACK        0x06
#define BYTE_CAN        0x18
#define BYTE_EOF        0x1A
#define BYTE_EOT        0x04

#define XMODEM_BLOCK_SIZE   128

#define RECV_INIT_TIMEOUT   5  /* x 100ms */
#define RECV_INIT_COUNT     20 /* 10 secs */
#define SEND_INIT_TIMEOUT   100 /* x 100ms */

#define FIRST_FRAME_NUMBER  1
#define CHECKSUM_INIT_VALUE 0

static enum {
    STATE_OFF,
    STATE_RECEIVE_INIT,
    STATE_RECEIVE_WAIT,
    STATE_RECEIVE_FRAMENUM1,
    STATE_RECEIVE_FRAMENUM2,
    STATE_RECEIVE_DATA,
    STATE_RECEIVE_PROCESS,
    STATE_SEND_INIT,
    STATE_SEND_WAIT,
    STATE_SEND_EOT
} State = STATE_OFF;

static uint8_t CurrentFrameNumber;
static uint8_t ReceivedFrameNumber;
static uint8_t Checksum;
static uint8_t RetryCount;
static uint8_t RetryTimeout;
static uint16_t BufferIdx;
static uint32_t BlockAddress;

static XModemCallbackType CallbackFunc;

static uint8_t CalcChecksum(const void* Buffer, uint16_t ByteCount) {
    uint8_t Checksum = CHECKSUM_INIT_VALUE;
    uint8_t* DataPtr = (uint8_t*) Buffer;

    while(ByteCount--) {
        Checksum += *DataPtr++;
    }

    return Checksum;
}

void XModemReceive(XModemCallbackType TheCallbackFunc)
{
    State = STATE_RECEIVE_INIT;
    CurrentFrameNumber = FIRST_FRAME_NUMBER;
    RetryCount = RECV_INIT_COUNT;
    RetryTimeout = RECV_INIT_TIMEOUT;
    BlockAddress = 0;

    CallbackFunc = TheCallbackFunc;
}

void XModemSend(XModemCallbackType TheCallbackFunc)
{
    State = STATE_SEND_INIT;
    RetryTimeout = SEND_INIT_TIMEOUT;
    BlockAddress = 0;

    CallbackFunc = TheCallbackFunc;
}

bool XModemProcessByte(uint8_t Byte)
{
    switch(State) {
    case STATE_RECEIVE_INIT:
    case STATE_RECEIVE_WAIT:
        if (Byte == BYTE_SOH) {
            /* Next frame incoming */
            BufferIdx = 0;
            Checksum = CHECKSUM_INIT_VALUE;
            State = STATE_RECEIVE_FRAMENUM1;
        } else if (Byte == BYTE_EOT) {
            /* Transmission finished */
            TerminalSendByte(BYTE_ACK);
            State = STATE_OFF;
        } else if (Byte == BYTE_CAN) {
            /* Cancel transmission */
            State = STATE_OFF;
        } else {
            /* Ignore other bytes */
        }

        break;

    case STATE_RECEIVE_FRAMENUM1:
        /* Store frame number */
        ReceivedFrameNumber = Byte;
        State = STATE_RECEIVE_FRAMENUM2;
        break;

    case STATE_RECEIVE_FRAMENUM2:
        if (Byte == (255 - ReceivedFrameNumber)) {
            /* frame-number check passed. */
            State = STATE_RECEIVE_DATA;
        } else {
            /* Something went wrong. Try to recover by sending NAK */
            TerminalSendByte(BYTE_NAK);
            State = STATE_RECEIVE_WAIT;
        }

        break;

    case STATE_RECEIVE_DATA:
        /* Process byte and update checksum */
        TerminalBuffer[BufferIdx++] = Byte;

        if (BufferIdx == XMODEM_BLOCK_SIZE) {
            /* Block full */
            State = STATE_RECEIVE_PROCESS;
        }

        break;

    case STATE_RECEIVE_PROCESS:
        if (ReceivedFrameNumber == CurrentFrameNumber) {
            /* This is the expected frame. Calculate and verify checksum */

            if (CalcChecksum(TerminalBuffer, XMODEM_BLOCK_SIZE) == Byte) {
                /* Checksum is valid. Pass received data to callback function */
                if (CallbackFunc(TerminalBuffer, BlockAddress, XMODEM_BLOCK_SIZE)) {
                    /* Proceed to next frame and send ACK */
                    CurrentFrameNumber++;
                    BlockAddress += XMODEM_BLOCK_SIZE;
                    TerminalSendChar(BYTE_ACK);
                    State = STATE_RECEIVE_WAIT;
                } else {
                    /* Application signals to cancel the transmission */
                    TerminalSendByte(BYTE_CAN);
                    TerminalSendByte(BYTE_CAN);
                    State = STATE_OFF;
                }
            } else {
                /* Data seems to be damaged */
                TerminalSendByte(BYTE_NAK);
                State = STATE_RECEIVE_WAIT;
            }
        } else if (ReceivedFrameNumber == (CurrentFrameNumber - 1)) {
            /* This is a retransmission */
            TerminalSendByte(BYTE_ACK);
            State = STATE_RECEIVE_WAIT;
        } else {
            /* This frame is completely out of order. Just cancel */
            TerminalSendByte(BYTE_CAN);
            State = STATE_OFF;
        }

        break;

    case STATE_SEND_INIT:
        /* Start sending on NAK */
        if (Byte == BYTE_NAK) {
            CurrentFrameNumber = FIRST_FRAME_NUMBER - 1;
            Byte = BYTE_ACK;
        }

        /* Fallthrough */

    case STATE_SEND_WAIT:
        if (Byte == BYTE_CAN) {
            /* Cancel */
            TerminalSendByte(BYTE_ACK);
            State = STATE_OFF;
        } else if (Byte == BYTE_ACK) {
            /* Acknowledge. Proceed to next frame, get data and calc checksum */
            CurrentFrameNumber++;

            if (CallbackFunc(TerminalBuffer, BlockAddress, XMODEM_BLOCK_SIZE)) {
                TerminalSendByte(BYTE_SOH);
                TerminalSendByte(CurrentFrameNumber);
                TerminalSendByte(255 - CurrentFrameNumber);
                TerminalSendBlock(TerminalBuffer, XMODEM_BLOCK_SIZE);
                TerminalSendByte(CalcChecksum(TerminalBuffer, XMODEM_BLOCK_SIZE));

                BlockAddress += XMODEM_BLOCK_SIZE;
            } else {
                TerminalSendByte(BYTE_EOT);
                State = STATE_SEND_EOT;
            }
        } else if (Byte == BYTE_NAK){
            /* Resend frame */
            TerminalSendByte(BYTE_SOH);
            TerminalSendByte(CurrentFrameNumber);
            TerminalSendByte(255 - CurrentFrameNumber);
            TerminalSendBlock(TerminalBuffer, XMODEM_BLOCK_SIZE);
            TerminalSendByte(CalcChecksum(TerminalBuffer, XMODEM_BLOCK_SIZE));
        } else {
            /* Ignore other chars */
        }

        break;

    case STATE_SEND_EOT:
        /* Receive Ack */
        State = STATE_OFF;
        break;

    default:
        return false;
        break;
    }

    return true;
}

void XModemTick(void)
{
    /* Timeouts go here */
    switch(State) {
    case STATE_RECEIVE_INIT:
        if (RetryTimeout-- == 0) {
            if (RetryCount-- > 0) {
                /* Put out communication request */
                TerminalSendChar(BYTE_NAK);
            } else {
                /* Just shut off after some time. */
                State = STATE_OFF;
            }

            RetryTimeout = RECV_INIT_TIMEOUT;
        }
        break;

    case STATE_SEND_INIT:
        if (RetryTimeout-- == 0) {
            /* Abort */
            State = STATE_OFF;
        }
        break;

    default:
        break;
    }
}
