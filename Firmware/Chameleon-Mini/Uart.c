/*
 * Uart.c
 *
 *  Created on: 20.03.2013
 *      Author: Willok
 *
 *  ChangeLog
 *    2019-09-22    Willok    Add some UART PROCESS
 *
 */
#ifdef CHAMELEON_TINY_UART_MODE

#include "Chameleon-Mini.h"
#include "Uart.h"
#include "UartCmd.h"

//    This defines the  buffer used by the UART

#define RBUF_SIZE   512
typedef struct _buf_rx {
    uint16_t in;
    uint16_t out;
    uint8_t buf[RBUF_SIZE];
} buf_rx;

#define TBUF_SIZE   512
typedef struct _buf_tx {
    uint16_t in;
    uint16_t out;
    uint8_t buf[TBUF_SIZE];
} buf_tx;

buf_rx rbuf = { 0, 0, };

buf_tx tbuf = { 0, 0, };

//#pragma GCC push_options
//#pragma GCC optimize ("O0")

ISR(USARTE0_RXC_vect) {
    if (0 == (USART.STATUS & (USART_BUFOVF_bm | USART_FERR_bm | USART_PERR_bm))) {
        if (((rbuf.in - rbuf.out) & ~(RBUF_SIZE - 1)) == 0) {
            rbuf.buf[rbuf.in & (RBUF_SIZE - 1)] = USART_GetChar(&USART);
            rbuf.in++;
        } else {
            //    It means that the buffer is full. Find a way to deal with it
            uart_putc('E');
            uart_putc('0');
            uart_putc(USART_GetChar(&USART));
        }
    } else {
        uart_putc('E');
        uart_putc('1');
        uart_putc(USART.STATUS);
        uart_putc(USART_GetChar(&USART));
    }
}

int16_t UartFIFOGet(void) {
    if (rbuf.in - rbuf.out)
        return rbuf.buf[(rbuf.out++) & (RBUF_SIZE - 1)];

    return -1;
}

void UartFIFOPut(uint8_t *buf, uint16_t len) {
    while (len) {
        if (((tbuf.in - tbuf.out) & ~(TBUF_SIZE - 1)) == 0) {
            tbuf.buf[tbuf.in & (TBUF_SIZE - 1)] = *buf;
            tbuf.in++;
        } else {
            //    It means that the buffer is full. Find a way to deal with it
            uart_putc('E');
            uart_putc('2');
        }

        buf++;
        len--;
    }
}

//    UART loop, sending data, fetching data from sending buffer
void UartTask(void) {
    uint8_t sendbuf[sizeof(CMD_HEAD) + 64];
    uint16_t DataLen = tbuf.in - tbuf.out;
    uint16_t CurrentLen;
    PCMD_HEAD SendHead = (PCMD_HEAD)sendbuf;
    uint8_t i;

    while (DataLen) {
        //    Maximum 64 bytes at one time
        CurrentLen = DataLen;
        if (CurrentLen > 64)
            CurrentLen = 64;

        SendHead->bSign = CMD_HEAD_SIGN;
        SendHead->bCmd = CMD_UART_RXTX;
        SendHead->bCmdLen = CurrentLen;
        SendHead->bChkSum = 0;

        //    It has to be copied here because the circular buffer is discontinuous
        for (i = 0; i < CurrentLen; i++) {
            sendbuf[sizeof(CMD_HEAD) + i] = tbuf.buf[(tbuf.out++) & (TBUF_SIZE - 1)];
        }
        //    Check Summing
        SendHead->bChkSum = GetChkSum(SendHead, NULL);

        //    Send filled buffer
        uart_putb(sendbuf, CurrentLen + sizeof(CMD_HEAD));

        //    Resize data
        DataLen -= CurrentLen;
    }
}

//    Send a byte
void uart_putc(uint8_t c) {
    //    Wait buffer empty
    while (!(USART.STATUS & USART_DREIF_bm));

    /* send next byte */
    USART.DATA = c;
}

//    Send a data block
void uart_putb(uint8_t *data, uint8_t len) {
    while (len) {
        uart_putc(*data);
        data++;
        len--;
    }
}


uint32_t dwBaudRate = 115200;
//    Set UART rate
uint8_t uart_baudrate(uint32_t NewBaudrate) {
    uint8_t bRet = 1;

    switch (NewBaudrate) {
        case 115200:
            USART_Baudrate_Set(&USART, 878, -6);         //    115200 -0.04%
            break;
        case 230400:
            USART_Baudrate_Set(&USART, 407, -6);         //    230400 -0.04%
            break;
        case 460800:
            USART_Baudrate_Set(&USART, 343, -7);         //    460800 -0.04%
            break;
        default:
            dwBaudRate = 460800;
            USART_Baudrate_Set(&USART, 343, -7);         //    460800 -0.04%
            bRet = 0;
            break;
    }

    if (bRet) {
        dwBaudRate = NewBaudrate;
    }

    return bRet;
}

//    UART port initialization
void UartInit(void) {
    //    Buffer initialization
    rbuf.in = 0;
    rbuf.out = 0;
    rbuf.buf[0] = 0;
    tbuf.in = 0;
    tbuf.out = 0;
    tbuf.buf[0] = 0;

    /* PE3 (TXD0) output */
    PORTE.DIRSET   = PIN3_bm;
    PORTE.OUTSET   = PIN3_bm;
    /* PE2 (RXD0) input */
    PORTE.DIRCLR   = PIN2_bm;
    PORTE.PIN2CTRL = PORT_OPC_PULLUP_gc;

    /* USART Mode - Asynchronous*/
    USART_SetMode(&USART, USART_CMODE_ASYNCHRONOUS_gc);
    /* USARTE0 Frame structure, 8-bit data bits, no check, 1 stop bit */
    USART_Format_Set(&USART, USART_CHSIZE_8BIT_gc, USART_PMODE_DISABLED_gc, 0);

    // Set baud rate @ 27.12M = 13.56 * 2
    uart_baudrate(460800);        //    460800
//    uart_baudrate(230400);        //    115200
//    uart_baudrate(115200);        //    115200

    /* USART enable TX*/
    USART_Tx_Enable(&USART);
    /* USART enable RX*/
    USART_Rx_Enable(&USART);

    ///* USART Receive interrupt level*/
    //USART_RxdInterruptLevel_Set(&USART,USART_RXCINTLVL_LO_gc);
    USART_RxdInterruptLevel_Set(&USART, USART_RXCINTLVL_HI_gc);
}


//#pragma GCC pop_options

#endif // CHAMELEON_TINY_UART_MODE
