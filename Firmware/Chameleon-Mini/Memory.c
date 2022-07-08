/*
 * Flash.c
 *
 *  Created on: 20.03.2013
 *      Author: skuser
 */

#include "Memory.h"
#include "Configuration.h"
#include "Common.h"
#include "Settings.h"
#include "LEDHook.h"
#include "System.h"

#define USE_DMA
#define RECV_DMA DMA.CH0
#define SEND_DMA DMA.CH1

/* Convert defines from Makefile */
#define FLASH_DATA_START		FLASH_DATA_ADDR
#define FLASH_DATA_END			(FLASH_DATA_ADDR + FLASH_DATA_SIZE - 1)

/* Definitions for FRAM */
#define FRAM_USART	USARTD0
#define FRAM_PORT	PORTD
#define FRAM_CS		PIN4_bm
#define FRAM_MOSI	PIN3_bm
#define FRAM_MISO	PIN2_bm
#define FRAM_SCK	PIN1_bm

/* Declarations from assembler file */
uint16_t FlashReadWord(uint32_t Address);
void FlashEraseApplicationPage(uint32_t Address);
void FlashLoadFlashWord(uint16_t Address, uint16_t Data);
void FlashEraseWriteApplicationPage(uint32_t Address);
void FlashEraseFlashBuffer(void);
void FlashWaitForSPM(void);

static uint8_t ScrapBuffer[] = {0};

INLINE uint8_t SPITransferByte(uint8_t Data) {
    FRAM_USART.DATA = Data;

    while (!(FRAM_USART.STATUS & USART_RXCIF_bm));

    return FRAM_USART.DATA;
}

#ifdef USE_DMA
INLINE void SPIReadBlock(void *Buffer, uint16_t ByteCount) {
    /* Set up read and write transfers */
    RECV_DMA.ADDRCTRL = DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_SRCDIR_FIXED_gc | DMA_CH_DESTRELOAD_NONE_gc | DMA_CH_DESTDIR_INC_gc;
    RECV_DMA.DESTADDR0 = ((uintptr_t) Buffer >> 0) & 0xFF;
    RECV_DMA.DESTADDR1 = ((uintptr_t) Buffer >> 8) & 0xFF;
    RECV_DMA.TRFCNT = ByteCount;
    SEND_DMA.ADDRCTRL = DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_SRCDIR_FIXED_gc | DMA_CH_DESTRELOAD_NONE_gc | DMA_CH_DESTDIR_FIXED_gc;
    SEND_DMA.SRCADDR0 = ((uintptr_t) ScrapBuffer >> 0) & 0xFF;
    SEND_DMA.SRCADDR1 = ((uintptr_t) ScrapBuffer >> 8) & 0xFF;
    SEND_DMA.TRFCNT = ByteCount;


    /* Enable read and write transfers */
    RECV_DMA.CTRLA |= DMA_CH_ENABLE_bm;
    SEND_DMA.CTRLA |= DMA_CH_ENABLE_bm;

    /* Wait for DMA to finish */
    while (RECV_DMA.CTRLA & DMA_CH_ENABLE_bm)
        ;

    /* Clear Interrupt flag */
    RECV_DMA.CTRLB = DMA_CH_TRNIF_bm | DMA_CH_ERRIF_bm;
    SEND_DMA.CTRLB = DMA_CH_TRNIF_bm | DMA_CH_ERRIF_bm;
}
#else
INLINE void SPIReadBlock(void *Buffer, uint16_t ByteCount) {
    uint8_t *ByteBuffer = (uint8_t *) Buffer;

    while (ByteCount-- > 0) {
        FRAM_USART.DATA = 0;
        while (!(FRAM_USART.STATUS & USART_RXCIF_bm));

        *ByteBuffer++ = FRAM_USART.DATA;
    }
}

#endif

#ifdef USE_DMA
INLINE void SPIWriteBlock(const void *Buffer, uint16_t ByteCount) {
    /* Set up read and write transfers */
    RECV_DMA.ADDRCTRL = DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_SRCDIR_FIXED_gc | DMA_CH_DESTRELOAD_NONE_gc | DMA_CH_DESTDIR_FIXED_gc;
    RECV_DMA.DESTADDR0 = ((uintptr_t) ScrapBuffer >> 0) & 0xFF;
    RECV_DMA.DESTADDR1 = ((uintptr_t) ScrapBuffer >> 8) & 0xFF;
    RECV_DMA.TRFCNT = ByteCount;
    SEND_DMA.ADDRCTRL = DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_NONE_gc | DMA_CH_DESTDIR_FIXED_gc;
    SEND_DMA.SRCADDR0 = ((uintptr_t) Buffer >> 0) & 0xFF;
    SEND_DMA.SRCADDR1 = ((uintptr_t) Buffer >> 8) & 0xFF;
    SEND_DMA.TRFCNT = ByteCount;

    /* Enable read and write transfers */
    RECV_DMA.CTRLA |= DMA_CH_ENABLE_bm;
    SEND_DMA.CTRLA |= DMA_CH_ENABLE_bm;

    /* Wait for DMA to finish */
    while (RECV_DMA.CTRLA & DMA_CH_ENABLE_bm)
        ;

    /* Clear Interrupt flag */
    RECV_DMA.CTRLB = DMA_CH_TRNIF_bm | DMA_CH_ERRIF_bm;
    SEND_DMA.CTRLB = DMA_CH_TRNIF_bm | DMA_CH_ERRIF_bm;
}
#else
INLINE void SPIWriteBlock(const void *Buffer, uint16_t ByteCount) {
    uint8_t *ByteBuffer = (uint8_t *) Buffer;

    while (ByteCount-- > 0) {
        FRAM_USART.DATA = *ByteBuffer++;
        while (!(FRAM_USART.STATUS & USART_RXCIF_bm));

        FRAM_USART.DATA; /* Flush Buffer */
    }
}
#endif

INLINE void FRAMRead(void *Buffer, uint16_t Address, uint16_t ByteCount) {
    FRAM_PORT.OUTCLR = FRAM_CS;

    SPITransferByte(0x03); /* Read command */
    SPITransferByte((Address >> 8) & 0xFF);   /* Address hi and lo byte */
    SPITransferByte((Address >> 0) & 0xFF);

    SPIReadBlock(Buffer, ByteCount);

    FRAM_PORT.OUTSET = FRAM_CS;
}

INLINE void FRAMWrite(const void *Buffer, uint16_t Address, uint16_t ByteCount) {
    FRAM_PORT.OUTCLR = FRAM_CS;
    SPITransferByte(0x06); /* Write Enable */
    FRAM_PORT.OUTSET = FRAM_CS;

    asm volatile("nop");
    asm volatile("nop");

    FRAM_PORT.OUTCLR = FRAM_CS;

    SPITransferByte(0x02); /* Write command */
    SPITransferByte((Address >> 8) & 0xFF);   /* Address hi and lo byte */
    SPITransferByte((Address >> 0) & 0xFF);

    SPIWriteBlock(Buffer, ByteCount);

    FRAM_PORT.OUTSET = FRAM_CS;
}

INLINE void FlashRead(void *Buffer, uint32_t Address, uint16_t ByteCount) {
    uint8_t *BufPtr = (uint8_t *) Buffer;

    /* We assume that ByteCount is a multiple of 2 */
    uint32_t PhysicalAddress = Address + FLASH_DATA_ADDR;

    if ((PhysicalAddress >= FLASH_DATA_START) && (PhysicalAddress <= FLASH_DATA_END)) {
        /* Sanity check to limit access to the allocated area */
        while (ByteCount > 1) {
            uint16_t Word = FlashReadWord(PhysicalAddress);

            *BufPtr++ = (Word >> 0) & 0xFF;
            *BufPtr++ = (Word >> 8) & 0xFF;

            PhysicalAddress += 2;
            ByteCount -= 2;
        }
    }
}

INLINE void FlashWrite(const void *Buffer, uint32_t Address, uint16_t ByteCount) {
    const uint8_t *BufPtr = (uint8_t *) Buffer;

    /* We assume that FlashWrite is always called for write actions that are
     * aligned to APP_SECTION_PAGE_SIZE and a multiple of APP_SECTION_PAGE_SIZE.
     * Thus only full pages are written into the flash. */
    uint16_t PageCount = ByteCount / APP_SECTION_PAGE_SIZE;
    uint32_t PhysicalAddress = Address + FLASH_DATA_ADDR;

    if ((PhysicalAddress >= FLASH_DATA_START) && (PhysicalAddress <= FLASH_DATA_END)) {
        /* Sanity check to limit access to the allocated area */

        while (PageCount-- > 0) {
            /* For each page to program, wait for NVM to get ready,
             * erase the flash page buffer, program all data to the
             * flash page buffer and write buffer to flash using
             * the atomic erase and write operation. */

            FlashWaitForSPM();

            FlashEraseFlashBuffer();
            FlashWaitForSPM();

            for (uint16_t i = 0; i < APP_SECTION_PAGE_SIZE; i += 2) {
                uint16_t Word = 0;

                Word |= ((uint16_t) * BufPtr++ << 0);
                Word |= ((uint16_t) * BufPtr++ << 8);

                FlashLoadFlashWord(i, Word);
                FlashWaitForSPM();
            }

            FlashEraseWriteApplicationPage(PhysicalAddress);
            FlashWaitForSPM();

            PhysicalAddress += APP_SECTION_PAGE_SIZE;
        }
    }
}

INLINE void FlashErase(uint32_t Address, uint16_t ByteCount) {
    uint16_t PageCount = ByteCount / APP_SECTION_PAGE_SIZE;
    uint32_t PhysicalAddress = Address + FLASH_DATA_ADDR;

    if ((PhysicalAddress >= FLASH_DATA_START) && (PhysicalAddress <= FLASH_DATA_END)) {
        /* Sanity check to limit access to the allocated area */
        while (PageCount-- > 0) {
            FlashWaitForSPM();

            FlashEraseApplicationPage(PhysicalAddress);
            FlashWaitForSPM();

            PhysicalAddress += APP_SECTION_PAGE_SIZE;
        }
    }
}

INLINE void FlashToFRAM(uint32_t Address, uint16_t ByteCount) {
    /* We assume that ByteCount is a multiple of 2 */
    uint32_t PhysicalAddress = Address + FLASH_DATA_ADDR;

    if ((PhysicalAddress >= FLASH_DATA_START) && (PhysicalAddress <= FLASH_DATA_END)) {
        /* Sanity check to limit access to the allocated area.
         * Set up FRAM memory for writing. */
        FRAM_PORT.OUTCLR = FRAM_CS;
        SPITransferByte(0x06); /* Write Enable */
        FRAM_PORT.OUTSET = FRAM_CS;

        asm volatile("nop");
        asm volatile("nop");

        FRAM_PORT.OUTCLR = FRAM_CS;

        SPITransferByte(0x02); /* Write command */
        SPITransferByte(0); /* Address hi and lo byte */
        SPITransferByte(0);

        /* Loop through bytes, read words from flash and write
         * double byte into FRAM. */
        while (ByteCount > 1) {
            uint16_t Word = FlashReadWord(PhysicalAddress);

            SPITransferByte((Word >> 0) & 0xFF);
            SPITransferByte((Word >> 8) & 0xFF);

            PhysicalAddress += 2;
            ByteCount -= 2;
        }

        /* End write procedure of FRAM */
        FRAM_PORT.OUTSET = FRAM_CS;
    }
}

INLINE void FRAMToFlash(uint32_t Address, uint16_t ByteCount) {
    /* We assume that FlashWrite is always called for write actions that are
     * aligned to APP_SECTION_PAGE_SIZE and a multiple of APP_SECTION_PAGE_SIZE.
     * Thus only full pages are written into the flash. */
    uint16_t PageCount = ByteCount / APP_SECTION_PAGE_SIZE;
    uint32_t PhysicalAddress = Address + FLASH_DATA_ADDR;

    if ((PhysicalAddress >= FLASH_DATA_START) && (PhysicalAddress <= FLASH_DATA_END)) {
        /* Sanity check to limit access to the allocated area and setup FRAM
         * read. */
        FRAM_PORT.OUTCLR = FRAM_CS;

        SPITransferByte(0x03); /* Read command */
        SPITransferByte(0); /* Address hi and lo byte */
        SPITransferByte(0);

        while (PageCount-- > 0) {
            /* For each page to program, wait for NVM to get ready,
             * erase the flash page buffer, program all data to the
             * flash page buffer and write buffer to flash using
             * the atomic erase and write operation. */

            FlashWaitForSPM();

            FlashEraseFlashBuffer();
            FlashWaitForSPM();

            /* Write one page worth of data into flash buffer */
            for (uint16_t i = 0; i < APP_SECTION_PAGE_SIZE; i += 2) {
                uint16_t Word = 0;

                Word |= ((uint16_t) SPITransferByte(0) << 0);
                Word |= ((uint16_t) SPITransferByte(0) << 8);

                FlashLoadFlashWord(i, Word);
                FlashWaitForSPM();
            }

            /* Program flash buffer into flash */
            FlashEraseWriteApplicationPage(PhysicalAddress);
            FlashWaitForSPM();

            PhysicalAddress += APP_SECTION_PAGE_SIZE;
        }

        /* End read procedure of FRAM */
        FRAM_PORT.OUTSET = FRAM_CS;
    }
}

void MemoryInit(void) {
    /* Configure FRAM_USART for SPI master mode 0 with maximum clock frequency */
    FRAM_PORT.OUTSET = FRAM_CS;

    FRAM_PORT.OUTCLR = FRAM_SCK;
    FRAM_PORT.OUTSET = FRAM_MOSI;

    FRAM_PORT.DIRSET = FRAM_SCK | FRAM_MOSI | FRAM_CS;

    FRAM_USART.BAUDCTRLA = 0;
    FRAM_USART.BAUDCTRLB = 0;
    FRAM_USART.CTRLC = USART_CMODE_MSPI_gc;
    FRAM_USART.CTRLB = USART_RXEN_bm | USART_TXEN_bm;

    /* Init DMAs for reading and writing */
    RECV_DMA.ADDRCTRL = DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_SRCDIR_FIXED_gc | DMA_CH_DESTRELOAD_NONE_gc | DMA_CH_DESTDIR_FIXED_gc;
    RECV_DMA.TRIGSRC = DMA_CH_TRIGSRC_USARTD0_RXC_gc;
    RECV_DMA.TRFCNT = 0;
    RECV_DMA.SRCADDR0 = ((uintptr_t) &FRAM_USART.DATA >> 0) & 0xFF;
    RECV_DMA.SRCADDR1 = ((uintptr_t) &FRAM_USART.DATA >> 8) & 0xFF;
    RECV_DMA.SRCADDR2 = 0;
    RECV_DMA.DESTADDR0 = 0;
    RECV_DMA.DESTADDR1 = 0;
    RECV_DMA.DESTADDR2 = 0;
    RECV_DMA.CTRLA = DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_1BYTE_gc;

    SEND_DMA.ADDRCTRL = DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_SRCDIR_FIXED_gc | DMA_CH_DESTRELOAD_NONE_gc | DMA_CH_DESTDIR_FIXED_gc;
    SEND_DMA.TRIGSRC = DMA_CH_TRIGSRC_USARTD0_DRE_gc;
    SEND_DMA.TRFCNT = 0;
    SEND_DMA.SRCADDR0 = 0;
    SEND_DMA.SRCADDR1 = 0;
    SEND_DMA.SRCADDR2 = 0;
    SEND_DMA.DESTADDR0 = ((uintptr_t) &FRAM_USART.DATA >> 0) & 0xFF;
    SEND_DMA.DESTADDR1 = ((uintptr_t) &FRAM_USART.DATA >> 8) & 0xFF;
    SEND_DMA.DESTADDR2 = 0;
    SEND_DMA.CTRLA = DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_1BYTE_gc;
}

void MemoryReadBlock(void *Buffer, uint16_t Address, uint16_t ByteCount) {
    if (ByteCount == 0)
        return;
    FRAMRead(Buffer, Address, ByteCount);
}

void MemoryWriteBlock(const void *Buffer, uint16_t Address, uint16_t ByteCount) {
    if (ByteCount == 0)
        return;
    FRAMWrite(Buffer, Address, ByteCount);
    LEDHook(LED_MEMORY_CHANGED, LED_ON);
}

void MemoryReadBlockInSetting(void *Buffer, uint16_t Address, uint16_t ByteCount) {
    if (ByteCount == 0 || ByteCount >= MEMORY_SIZE_PER_SETTING)
        return;
    uint16_t ShiftedAddress = Address + GlobalSettings.ActiveSettingIdx * MEMORY_SIZE_PER_SETTING;
    if (ShiftedAddress < Address)
        return;
    FRAMRead(Buffer, ShiftedAddress, ByteCount);
}

void MemoryWriteBlockInSetting(const void *Buffer, uint16_t Address, uint16_t ByteCount) {
    if (ByteCount == 0 || ByteCount >= MEMORY_SIZE_PER_SETTING)
        return;
    uint16_t ShiftedAddress = Address + GlobalSettings.ActiveSettingIdx * MEMORY_SIZE_PER_SETTING;
    if (ShiftedAddress < Address)
        return;
    FRAMWrite(Buffer, ShiftedAddress, ByteCount);
    LEDHook(LED_MEMORY_CHANGED, LED_ON);
}

void MemoryClear(void) {
    FlashErase((uint32_t) GlobalSettings.ActiveSettingIdx * MEMORY_SIZE_PER_SETTING, MEMORY_SIZE_PER_SETTING);
    MemoryRecall();
}

void MemoryRecall(void) {
    /* Recall memory from permanent flash */
    FlashToFRAM((uint32_t) GlobalSettings.ActiveSettingIdx * MEMORY_SIZE_PER_SETTING, MEMORY_SIZE_PER_SETTING);
    SystemTickClearFlag();
}

void MemoryStore(void) {
    /* Store current memory into permanent flash */
    FRAMToFlash((uint32_t) GlobalSettings.ActiveSettingIdx * MEMORY_SIZE_PER_SETTING, MEMORY_SIZE_PER_SETTING);

    LEDHook(LED_MEMORY_CHANGED, LED_OFF);
    LEDHook(LED_MEMORY_STORED, LED_PULSE);

    SystemTickClearFlag();
}

bool MemoryUploadBlock(void *Buffer, uint32_t BlockAddress, uint16_t ByteCount) {
    if (BlockAddress >= MEMORY_SIZE_PER_SETTING) {
        /* Prevent writing out of bounds by silently ignoring it */
        return true;
    } else {
        /* Calculate bytes left in memory and start writing */
        uint32_t BytesLeft = MEMORY_SIZE_PER_SETTING - BlockAddress;
        ByteCount = MIN(ByteCount, BytesLeft);

        /* Store to local memory */
        FRAMWrite(Buffer, BlockAddress, ByteCount);

        return true;
    }
}

bool MemoryDownloadBlock(void *Buffer, uint32_t BlockAddress, uint16_t ByteCount) {
    if (BlockAddress >= MEMORY_SIZE_PER_SETTING) {
        /* There are bytes out of bounds to be read. Notify that we are done. */
        return false;
    } else {
        /* Calculate bytes left in memory and issue reading */
        uint32_t BytesLeft = MEMORY_SIZE_PER_SETTING - BlockAddress;
        ByteCount = MIN(ByteCount, BytesLeft);

        /* Output local memory contents */
        FRAMRead(Buffer, BlockAddress, ByteCount);

        return true;
    }
}

// EEPROM functions

static inline void NVM_EXEC(void) {
    void *z = (void *)&NVM_CTRLA;

    __asm__ volatile("out %[ccp], %[ioreg]"  "\n\t"
                     "st z, %[cmdex]"
                     :
                     : [ccp] "I"(_SFR_IO_ADDR(CCP)),
                     [ioreg] "d"(CCP_IOREG_gc),
                     [cmdex] "r"(NVM_CMDEX_bm),
                     [z] "z"(z)
                    );
}

void WaitForNVM(void) {
    while (NVM.STATUS & NVM_NVMBUSY_bm) { };
}

void FlushNVMBuffer(void) {
    WaitForNVM();

    if ((NVM.STATUS & NVM_EELOAD_bm) != 0) {
        NVM.CMD = NVM_CMD_ERASE_EEPROM_BUFFER_gc;
        NVM_EXEC();
    }
}

uint16_t ReadEEPBlock(uint16_t Address, void *DestPtr, uint16_t ByteCount) {
    uint16_t BytesRead = 0;
    uint8_t *BytePtr = (uint8_t *) DestPtr;
    NVM.ADDR2 = 0;

    WaitForNVM();

    while (ByteCount > 0) {
        NVM.ADDR0 = Address & 0xFF;
        NVM.ADDR1 = (Address >> 8) & 0x1F;

        NVM.CMD = NVM_CMD_READ_EEPROM_gc;
        NVM_EXEC();

        *BytePtr++ = NVM.DATA0;
        Address++;

        ByteCount--;
        BytesRead++;
    }

    return BytesRead;
}


uint16_t WriteEEPBlock(uint16_t Address, const void *SrcPtr, uint16_t ByteCount) {
    const uint8_t *BytePtr = (const uint8_t *) SrcPtr;
    uint8_t ByteAddress = Address % EEPROM_PAGE_SIZE;
    uint16_t PageAddress = Address - ByteAddress;
    uint16_t BytesWritten = 0;

    FlushNVMBuffer();
    WaitForNVM();
    NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;

    NVM.ADDR1 = 0;
    NVM.ADDR2 = 0;

    while (ByteCount > 0) {
        NVM.ADDR0 = ByteAddress;

        NVM.DATA0 = *BytePtr++;

        ByteAddress++;
        ByteCount--;

        if (ByteCount == 0 || ByteAddress >= EEPROM_PAGE_SIZE) {
            NVM.ADDR0 = PageAddress & 0xFF;
            NVM.ADDR1 = (PageAddress >> 8) & 0x1F;

            NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
            NVM_EXEC();

            PageAddress += EEPROM_PAGE_SIZE;
            ByteAddress = 0;

            WaitForNVM();

            NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;
        }

        BytesWritten++;
    }

    return BytesWritten;
}
