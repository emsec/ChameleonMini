#include "XModem.h"
#include "Terminal.h"

#define BYTE_NAK        0x15
#define BYTE_SOH        0x01
#define BYTE_ACK        0x06
#define BYTE_CAN        0x18
#define BYTE_EOF        0x1A
#define BYTE_EOT        0x04
#define BYTE_ESC		0x1B

#define XMODEM_BLOCK_SIZE   128

#define RECV_INIT_TIMEOUT   5  /* #Ticks between sending of NAKs to the sender */
#define RECV_INIT_COUNT     60 /* #Timeouts until receive failure */
#define SEND_INIT_TIMEOUT   300 /* #Ticks waiting for NAKs from the receiver before failure */

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
static uint16_t RetryTimeout;
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
        } else if ( (Byte == BYTE_CAN) || (Byte == BYTE_ESC) ) {
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
        } else if (Byte == BYTE_ESC) {
        	State = STATE_OFF;
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
