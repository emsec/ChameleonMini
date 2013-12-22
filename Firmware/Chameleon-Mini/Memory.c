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

#include "Memory.h"
#include "Configuration.h"
#include "Common.h"
#include "Settings.h"

#define FLASH_CMD_STATUS_REG_READ		0xD7
#define FLASH_CMD_MEM_TO_BUF1			0x53
#define FLASH_CMD_MEM_TO_BUF2			0x55
#define FLASH_CMD_BUF1_WRITE			0x84
#define FLASH_CMD_BUF2_WRITE			0x87
#define FLASH_CMD_BUF1_TO_MEM_ERASE		0x83
#define FLASH_CMD_BUF2_TO_MEM_ERASE		0x86
#define FLASH_CMD_PAGE_ERASE			0x81

#define FLASH_STATUS_REG_READY_BIT		(1<<7)
#define FLASH_STATUS_REG_COMP_BIT		(1<<6)
#define FLASH_STATUS_REG_PROTECT_BIT	(1<<1)
#define FLASH_STATUS_REG_PAGESIZE_BIT	(1<<0)

INLINE uint8_t SPITransferByte(uint8_t Data)
{
	MEMORY_FLASH_USART.DATA = Data;

	while (!(MEMORY_FLASH_USART.STATUS & USART_TXCIF_bm));

	MEMORY_FLASH_USART.STATUS = USART_TXCIF_bm;

	return MEMORY_FLASH_USART.DATA;
}

INLINE void SPIReadBlock(void* Buffer, uint16_t ByteCount)
{
	uint8_t* ByteBuffer = (uint8_t*) Buffer;

	while(ByteCount-- > 0) {
		MEMORY_FLASH_USART.DATA = 0;
		while (!(MEMORY_FLASH_USART.STATUS & USART_TXCIF_bm));
		MEMORY_FLASH_USART.STATUS = USART_TXCIF_bm;
		*ByteBuffer++ = MEMORY_FLASH_USART.DATA;
	}
}

INLINE void SPIWriteBlock(const void* Buffer, uint16_t ByteCount)
{
	uint8_t* ByteBuffer = (uint8_t*) Buffer;

	while(ByteCount-- > 0) {
		MEMORY_FLASH_USART.DATA = *ByteBuffer++;
		while (!(MEMORY_FLASH_USART.STATUS & USART_TXCIF_bm));
		MEMORY_FLASH_USART.STATUS = USART_TXCIF_bm;
		MEMORY_FLASH_USART.DATA; /* Flush Buffer */
	}
}

INLINE uint8_t FlashReadStatusRegister(void)
{
	uint8_t Register;

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPITransferByte(FLASH_CMD_STATUS_REG_READ);
	Register = SPITransferByte(0);
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;

	return Register;
}

INLINE bool FlashIsBusy(void)
{
	return !(FlashReadStatusRegister() & FLASH_STATUS_REG_READY_BIT);
}

INLINE void FlashConfigurePageSize(void)
{
	uint8_t Sequence[] = {0x3D, 0x2A, 0x80, 0xA6};

	while(FlashIsBusy());

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPIWriteBlock(Sequence, sizeof(Sequence));
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
}

INLINE void FlashMemoryToBuffer(uint16_t PageAddress)
{
	while(FlashIsBusy());

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPITransferByte(FLASH_CMD_MEM_TO_BUF1);
	SPITransferByte( (PageAddress >> 8) & 0xFF );
	SPITransferByte( (PageAddress >> 0) & 0xFF );
	SPITransferByte( 0 );
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
}

INLINE void FlashWriteBuffer(const void* Buffer, uint8_t Address, uint16_t ByteCount)
{
	while(FlashIsBusy());

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPITransferByte(FLASH_CMD_BUF1_WRITE);
	SPITransferByte( 0 );
	SPITransferByte( 0 );
	SPITransferByte( Address );
	SPIWriteBlock(Buffer, ByteCount);
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
}

INLINE void FlashBufferToMemory(uint16_t PageAddress)
{
	while(FlashIsBusy());

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPITransferByte(FLASH_CMD_BUF1_TO_MEM_ERASE);
	SPITransferByte( (PageAddress >> 8) & 0xFF );
	SPITransferByte( (PageAddress >> 0) & 0xFF );
	SPITransferByte( 0 );
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
}

INLINE void FlashRead(void* Buffer, uint32_t Address, uint16_t ByteCount)
{
	while(FlashIsBusy());

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPITransferByte(0x03);
	SPITransferByte( (Address >> 16) & 0xFF );
	SPITransferByte( (Address >> 8) & 0xFF );
	SPITransferByte( (Address >> 0) & 0xFF );
	SPIReadBlock(Buffer, ByteCount);
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
}

INLINE void FlashWrite(const void* Buffer, uint32_t Address, uint16_t ByteCount)
{
	while(ByteCount > 0) {
		uint16_t PageAddress = Address / MEMORY_PAGE_SIZE;
		uint8_t ByteAddress = Address % MEMORY_PAGE_SIZE;
		uint16_t PageBytes = MIN(MEMORY_PAGE_SIZE - ByteAddress, ByteCount);

		FlashMemoryToBuffer(PageAddress);
		FlashWriteBuffer(Buffer, ByteAddress, PageBytes);
		FlashBufferToMemory(PageAddress);

		ByteCount -= PageBytes;
		Address += PageBytes;
	}
}

INLINE void FlashClearPage(uint16_t PageAddress)
{
	while(FlashIsBusy());

	MEMORY_FLASH_PORT.OUTCLR = MEMORY_FLASH_CS;
	SPITransferByte(FLASH_CMD_PAGE_ERASE);
	SPITransferByte( (PageAddress >> 8) & 0xFF );
	SPITransferByte( (PageAddress >> 0) & 0xFF );
	SPITransferByte( 0 );
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
}

void MemoryInit(void)
{
	/* Configure MEMORY_FLASH_USART for SPI master mode 0 with maximum clock frequency */
	MEMORY_FLASH_PORT.OUTSET = MEMORY_FLASH_CS;
	MEMORY_FLASH_PORT.DIRSET = MEMORY_FLASH_SCK | MEMORY_FLASH_MOSI | MEMORY_FLASH_CS;

    MEMORY_FLASH_USART.BAUDCTRLA = 0;
    MEMORY_FLASH_USART.BAUDCTRLB = 0;
	MEMORY_FLASH_USART.CTRLC = USART_CMODE_MSPI_gc | USART_CHSIZE_8BIT_gc;
	MEMORY_FLASH_USART.CTRLB = USART_RXEN_bm | USART_TXEN_bm;


	if ( !(FlashReadStatusRegister() & FLASH_STATUS_REG_PAGESIZE_BIT) ) {
		/* Configure for 256 byte Dataflash if not already done. */
		FlashConfigurePageSize();
	}
}

void MemoryReadBlock(void* Buffer, uint16_t Address, uint16_t ByteCount)
{
	uint32_t FlashAddress = (uint32_t) Address + (uint32_t) GlobalSettings.ActiveSetting * MEMORY_SIZE_PER_SETTING;
	FlashRead(Buffer, FlashAddress, ByteCount);
}

void MemoryWriteBlock(const void* Buffer, uint16_t Address, uint16_t ByteCount)
{
	uint32_t FlashAddress = (uint32_t) Address + (uint32_t) GlobalSettings.ActiveSetting * MEMORY_SIZE_PER_SETTING;
	FlashWrite(Buffer, FlashAddress, ByteCount);
}

void MemoryClear(void)
{
	uint32_t PageAddress = ((uint32_t) GlobalSettings.ActiveSetting * MEMORY_SIZE_PER_SETTING) / MEMORY_PAGE_SIZE;
	uint16_t PageCount = MEMORY_SIZE_PER_SETTING / MEMORY_PAGE_SIZE;

	while(PageCount > 0) {
		FlashClearPage(PageAddress);
		PageCount--;
		PageAddress++;
	}
}

bool MemoryUploadBlock(void* Buffer, uint32_t BlockAddress, uint16_t ByteCount)
{
    if (BlockAddress >= MEMORY_SIZE_PER_SETTING) {
        /* Prevent writing out of bounds by silently ignoring it */
        return true;
    } else {
    	/* Calculate bytes left in memory and start writing */
    	uint32_t BytesLeft = MEMORY_SIZE_PER_SETTING - BlockAddress;
		ByteCount = MIN(ByteCount, BytesLeft);
		MemoryWriteBlock(Buffer, BlockAddress, ByteCount);
		return true;
    }
}

bool MemoryDownloadBlock(void* Buffer, uint32_t BlockAddress, uint16_t ByteCount)
{
    if (BlockAddress >= MEMORY_SIZE_PER_SETTING) {
        /* There are bytes out of bounds to be read. Notify that we are done. */
        return false;
    } else {
    	/* Calculate bytes left in memory and issue reading */
    	uint32_t BytesLeft = MEMORY_SIZE_PER_SETTING - BlockAddress;
		ByteCount = MIN(ByteCount, BytesLeft);
    	MemoryReadBlock(Buffer, BlockAddress, ByteCount);
        return true;
    }
}

