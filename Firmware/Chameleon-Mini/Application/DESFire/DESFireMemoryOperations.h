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
 * DESFireMemoryOperations.h :
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __DESFIRE_MEMORY_OPERATIONS_H__
#define __DESFIRE_MEMORY_OPERATIONS_H__

#include "DESFireFirmwareSettings.h"
#include "DESFireLogging.h"

/* Reserve some space on the stack (text / data segment) for intermediate
   storage of strings and data we need to write so we do not have to rely
   on a bothersome heap-based scheme for passing pointers to functions: */
#define DATA_BUFFER_SIZE_SMALL              (32)
#define STRING_BUFFER_SIZE                  (92)
extern volatile char __InternalStringBuffer[STRING_BUFFER_SIZE];
extern char __InternalStringBuffer2[DATA_BUFFER_SIZE_SMALL];

/*
 * EEPROM and FRAM memory management routines:
 */
void ReadBlockBytes(void *Buffer, SIZET StartBlock, SIZET Count);

void WriteBlockBytesMain(const void *Buffer, SIZET StartBlock, SIZET Count);
#define WriteBlockBytes(Buffer, StartBlock, Count)                 \
    WriteBlockBytesMain(Buffer, StartBlock, Count);

void CopyBlockBytes(SIZET DestBlock, SIZET SrcBlock, SIZET Count);

uint16_t AllocateBlocksMain(uint16_t BlockCount);
#define AllocateBlocks(BlockCount)                                 \
    AllocateBlocksMain(BlockCount);

BYTE GetCardCapacityBlocks(void);

void MemsetBlockBytes(uint8_t initValue, SIZET startBlock, SIZET byteCount);

/* File data transfer related routines: */
void ReadDataEEPROMSource(uint8_t *Buffer, uint8_t Count);
void WriteDataEEPROMSink(uint8_t *Buffer, uint8_t Count);

#endif
