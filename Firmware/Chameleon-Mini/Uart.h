#ifdef CHAMELEON_TINY_UART_MODE
#ifndef UART_H
#define UART_H

#include <avr/io.h>
#include <avr/interrupt.h>

#define USART USARTE0

void UartInit(void);
void UartTask(void);
void uart_putc(uint8_t c);
void uart_putb(uint8_t *data, uint8_t len);

void UartFIFOPut(uint8_t *buf, uint16_t len);

int16_t UartFIFOGet(void);
//uint16_t user_get(uint8_t *buf);
//uint8_t user_read(uint8_t *rx, uint16_t *len);


//+------------------------------------------------------------------------------

/*Macros. */

/*! \brief Macro that sets the USART frame format.
 *
 *  Sets the frame format, Frame Size, parity mode and number of stop bits.
 *
 *  \param _usart        Pointer to the USART module
 *  \param _charSize     The character size. Use USART_CHSIZE_t type.
 *  \param _parityMode   The parity Mode. Use USART_PMODE_t type.
 *  \param _twoStopBits  Enable two stop bit mode. Use bool type.
 */
#define USART_Format_Set(_usart, _charSize, _parityMode, _twoStopBits)        \
    (_usart)->CTRLC = (uint8_t) _charSize | _parityMode |                    \
                      (_twoStopBits ? USART_SBMODE_bm : 0)


/*! \brief Set USART baud rate.
 *
 *  Sets the USART's baud rate register.
 *
 *  UBRR_Value   : Value written to UBRR
 *  ScaleFactor  : Time Base Generator Scale Factor
 *
 *  Equation for calculation of BSEL value in asynchronous normal speed mode:
 *      If ScaleFactor >= 0
 *          BSEL = ((I/O clock frequency)/(2^(ScaleFactor)*16*Baudrate))-1
 *      If ScaleFactor < 0
 *          BSEL = (1/(2^(ScaleFactor)*16))*(((I/O clock frequency)/Baudrate)-1)
 *
 *    \note See XMEGA manual for equations for calculation of BSEL value in other
 *        modes.
 *
 *  \param _usart          Pointer to the USART module.
 *  \param _bselValue      Value to write to BSEL part of Baud control register.
 *                         Use uint16_t type.
 *  \param _bScaleFactor   USART baud rate scale factor.
 *                         Use uint8_t type
 */
#define USART_Baudrate_Set(_usart, _bselValue, _bScaleFactor)                  \
    (_usart)->BAUDCTRLA =(uint8_t)_bselValue;                                           \
    (_usart)->BAUDCTRLB =(_bScaleFactor << USART_BSCALE0_bp)|(_bselValue >> 8)


/*! \brief Enable USART receiver.
 *
 *  \param _usart    Pointer to the USART module
 */
#define USART_Rx_Enable(_usart) ((_usart)->CTRLB |= USART_RXEN_bm)


/*! \brief Disable USART receiver.
 *
 *  \param _usart Pointer to the USART module.
 */
#define USART_Rx_Disable(_usart) ((_usart)->CTRLB &= ~USART_RXEN_bm)


/*! \brief Enable USART transmitter.
 *
 *  \param _usart Pointer to the USART module.
 */
#define USART_Tx_Enable(_usart)    ((_usart)->CTRLB |= USART_TXEN_bm)


/*! \brief Disable USART transmitter.
 *
 *  \param _usart Pointer to the USART module.
 */
#define USART_Tx_Disable(_usart) ((_usart)->CTRLB &= ~USART_TXEN_bm)


/*! \brief Set USART RXD interrupt level.
 *
 *  Sets the interrupt level on RX Complete interrupt.
 *
 *  \param _usart        Pointer to the USART module.
 *  \param _rxdIntLevel  Interrupt level of the RXD interrupt.
 *                       Use USART_RXCINTLVL_t type.
 */
#define USART_RxdInterruptLevel_Set(_usart, _rxdIntLevel)                      \
    ((_usart)->CTRLA = ((_usart)->CTRLA & ~USART_RXCINTLVL_gm) | _rxdIntLevel)


/*! \brief Set USART TXD interrupt level.
 *
 *  Sets the interrupt level on TX Complete interrupt.
 *
 *  \param _usart        Pointer to the USART module.
 *  \param _txdIntLevel  Interrupt level of the TXD interrupt.
 *                       Use USART_TXCINTLVL_t type.
 */
#define USART_TxdInterruptLevel_Set(_usart, _txdIntLevel)                      \
    (_usart)->CTRLA = ((_usart)->CTRLA & ~USART_TXCINTLVL_gm) | _txdIntLevel



/*! \brief Set USART DRE interrupt level.
 *
 *  Sets the interrupt level on Data Register interrupt.
 *
 *  \param _usart        Pointer to the USART module.
 *  \param _dreIntLevel  Interrupt level of the DRE interrupt.
 *                       Use USART_DREINTLVL_t type.
 */
#define USART_DreInterruptLevel_Set(_usart, _dreIntLevel)                      \
    (_usart)->CTRLA = ((_usart)->CTRLA & ~USART_DREINTLVL_gm) | _dreIntLevel


/*! \brief Set the mode the USART run in.
 *
 * Set the mode the USART run in. The default mode is asynchronous mode.
 *
 *  \param  _usart       Pointer to the USART module register section.
 *  \param  _usartMode   Selects the USART mode. Use  USART_CMODE_t type.
 *
 *  USART modes:
 *  - 0x0        : Asynchronous mode.
 *  - 0x1        : Synchronous mode.
 *  - 0x2        : IrDA mode.
 *  - 0x3        : Master SPI mode.
 */
#define USART_SetMode(_usart, _usartMode)                                      \
    ((_usart)->CTRLC = ((_usart)->CTRLC & (~USART_CMODE_gm)) | _usartMode)



/*! \brief Check if data register empty flag is set.
 *
 *  \param _usart      The USART module.
 */
#define USART_IsTXDataRegisterEmpty(_usart) (((_usart)->STATUS & USART_DREIF_bm) != 0)



/*! \brief Put data (5-8 bit character).
 *
 *  Use the macro USART_IsTXDataRegisterEmpty before using this function to
 *  put data to the TX register.
 *
 *  \param _usart      The USART module.
 *  \param _data       The data to send.
 */
#define USART_PutChar(_usart, _data) ((_usart)->DATA = _data)



/*! \brief Checks if the RX complete interrupt flag is set.
 *
 *   Checks if the RX complete interrupt flag is set.
 *
 *  \param _usart     The USART module.
 */
#define USART_IsRXComplete(_usart) (((_usart)->STATUS & USART_RXCIF_bm) != 0)




/*! \brief Get received data (5-8 bit character).
 *
 *  This macro reads out the RX register.
 *  Use the macro USART_RX_Complete to check if anything is received.
 *
 *  \param _usart     The USART module.
 *
 *  \retval           Received data.
 */
#define USART_GetChar(_usart)  ((_usart)->DATA)

#endif // ifndef UART_H
#endif // ifdef CHAMELEON_TINY_UART_MODE
