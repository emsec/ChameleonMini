/*
The DESFire stack portion of this firmware source
is free software written by Maxie Dion Schmidt (@maxieds):
You can redistribute it and/or modify
it under the terms of this license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

The complete source distribution of
this firmware is available at the following link:
https://github.com/maxieds/ChameleonMiniFirmwareDESFireStack.

Based in part on the original DESFire code created by
@dev-zzo (GitHub handle) [Dmitry Janushkevich] available at
https://github.com/dev-zzo/ChameleonMini/tree/desfire.

This notice must be retained at the top of all source files where indicated.
*/

/*
 * DESFireMemoryOperations.c
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "../../Memory.h"

#include "DESFireMemoryOperations.h"
#include "DESFirePICCControl.h"
#include "DESFireFile.h"
#include "DESFireLogging.h"

volatile char __InternalStringBuffer[STRING_BUFFER_SIZE] = { 0 };
char __InternalStringBuffer2[DATA_BUFFER_SIZE_SMALL] = { 0 };

void ReadBlockBytes(void *Buffer, SIZET StartBlock, SIZET Count) {
    if (StartBlock * DESFIRE_EEPROM_BLOCK_SIZE >= MEMORY_SIZE_PER_SETTING) {
        const char *rbbLogMsg = PSTR("RBB Start Block Too Large -- %d -- %d");
        DEBUG_PRINT_P(rbbLogMsg, StartBlock, StartBlock * DESFIRE_EEPROM_BLOCK_SIZE);
        return;
    }
    //else if(StartBlock == 0) {
    //    const char *logWarningMsg = PSTR("WARNING: Reading NULL Address!");
    //    DEBUG_PRINT_P(logWarningMsg);
    //}
    MemoryReadBlockInSetting(Buffer, StartBlock * DESFIRE_EEPROM_BLOCK_SIZE, Count);
}

void WriteBlockBytesMain(const void *Buffer, SIZET StartBlock, SIZET Count) {
    if (StartBlock * DESFIRE_EEPROM_BLOCK_SIZE >= MEMORY_SIZE_PER_SETTING) {
        const char *wbbLogMsg = PSTR("WBB Start Block Too Large -- %d -- %s");
        DEBUG_PRINT_P(wbbLogMsg, StartBlock, __InternalStringBuffer2);
        return;
    }
    //else if(StartBlock == 0) {
    //    const char *logWarningMsg = PSTR("WARNING: Writing NULL! -- %s");
    //    DEBUG_PRINT_P(logWarningMsg, __InternalStringBuffer2);
    //}
    MemoryWriteBlockInSetting(Buffer, StartBlock * DESFIRE_EEPROM_BLOCK_SIZE, Count);
}

void CopyBlockBytes(SIZET DestBlock, SIZET SrcBlock, SIZET Count) {
    uint8_t Buffer[DESFIRE_EEPROM_BLOCK_SIZE];
    uint16_t SrcOffset = SrcBlock;
    uint16_t DestOffset = DestBlock;
    while (Count > 0) {
        SIZET bytesToWrite = MIN(Count, DESFIRE_EEPROM_BLOCK_SIZE);
        ReadBlockBytes(Buffer, SrcOffset, bytesToWrite);
        WriteBlockBytes(Buffer, DestOffset, bytesToWrite);
        SrcOffset += 1;
        DestOffset += 1;
        Count -= DESFIRE_EEPROM_BLOCK_SIZE;
    }
}

uint16_t AllocateBlocksMain(uint16_t BlockCount) {
    uint16_t Block;
    /* Check if we have space */
    Block = Picc.FirstFreeBlock;
    if (Block + BlockCount < Block || Block + BlockCount >= MEMORY_SIZE_PER_SETTING / DESFIRE_EEPROM_BLOCK_SIZE) {
        return 0;
    }
    Picc.FirstFreeBlock = Block + BlockCount;
    DESFIRE_INITIAL_FIRST_FREE_BLOCK_ID = Picc.FirstFreeBlock;
    SynchronizePICCInfo();
    return Block;
}

/* TODO: Why doesn't this work ??? -- It freezes the AVR chip when run !! */
void MemsetBlockBytes(uint8_t initValue, SIZET startBlock, SIZET byteCount) {
    BYTE fillerBuf[DESFIRE_EEPROM_BLOCK_SIZE];
    memset(fillerBuf, initValue, DESFIRE_EEPROM_BLOCK_SIZE);
    SIZET writeAddr = startBlock;
    while (byteCount > 0) {
        WriteBlockBytes(&fillerBuf[0], writeAddr, MIN(DESFIRE_EEPROM_BLOCK_SIZE, byteCount));
        ++writeAddr;
        if (byteCount > DESFIRE_EEPROM_BLOCK_SIZE) {
            byteCount -= DESFIRE_EEPROM_BLOCK_SIZE;
        } else {
            break;
        }
    }
}

uint8_t GetCardCapacityBlocks(void) {
    uint8_t MaxFreeBlocks;

    switch (Picc.StorageSize) {
        case DESFIRE_STORAGE_SIZE_2K:
            MaxFreeBlocks = 0x40 - DESFIRE_FIRST_FREE_BLOCK_ID;
            break;
        case DESFIRE_STORAGE_SIZE_4K:
            MaxFreeBlocks = 0x80 - DESFIRE_FIRST_FREE_BLOCK_ID;
            break;
        case DESFIRE_STORAGE_SIZE_8K:
            MaxFreeBlocks = (BYTE)(0x100 - DESFIRE_FIRST_FREE_BLOCK_ID);
            break;
        default:
            return 0;
    }
    return MaxFreeBlocks - Picc.FirstFreeBlock;
}

void ReadDataEEPROMSource(uint8_t *Buffer, uint8_t Count) {
    MemoryReadBlock(Buffer, TransferState.ReadData.Source.Pointer, Count);
    TransferState.ReadData.Source.Pointer += DESFIRE_BYTES_TO_BLOCKS(Count);
}

void WriteDataEEPROMSink(uint8_t *Buffer, uint8_t Count) {
    MemoryWriteBlock(Buffer, TransferState.WriteData.Sink.Pointer, Count);
    TransferState.WriteData.Sink.Pointer += DESFIRE_BYTES_TO_BLOCKS(Count);
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
