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
 * DESFireUtils.h
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __DESFIRE_UTILS_H__
#define __DESFIRE_UTILS_H__

#include <stdarg.h>

#include "DESFireFirmwareSettings.h"

#define ASBYTES(bc)  (((bc) + BITS_PER_BYTE - 1) / BITS_PER_BYTE)
#define ASBITS(bc)   ((bc) * BITS_PER_BYTE)

#define GET_LE16(p)     (*((uint16_t*)&(p)[0]))
#define GET_LE24(p)     (*((__uint24*)&(p)[0]))
#define GET_LE32(p)     (*((uint32_t*)&(p)[0]))

#define UnsignedTypeToUINT(typeValue) \
    ((UINT) typeValue)
#define ExtractLSBLE(ui) \
    ((BYTE) (((UnsignedTypeToUINT(ui) & 0xff000000) >> 24) & 0x000000ff))
#define ExtractLSBBE(ui) \
    ((BYTE) (UnsignedTypeToUINT(ui) & 0x000000ff))

void RotateArrayRight(BYTE *srcBuf, BYTE *destBuf, SIZET bufSize);
void RotateArrayLeft(BYTE *srcBuf, BYTE *destBuf, SIZET bufSize);
void ConcatByteArrays(BYTE *arrA, SIZET arrASize, BYTE *arrB, SIZET arrBSize, BYTE *destArr);

void Int32ToByteBuffer(uint8_t *byteBuffer, int32_t int32Value);
void Int24ToByteBuffer(uint8_t *byteBuffer, uint32_t int24Value);
int32_t Int32FromByteBuffer(uint8_t *byteBuffer);

SIZET RoundBlockSize(SIZET byteSize, SIZET blockSize);

/* Copied from the READER configuration sources: */
uint16_t DesfireAddParityBits(uint8_t *Buffer, uint16_t bits);
uint16_t DesfireRemoveParityBits(uint8_t *Buffer, uint16_t BitCount);
bool DesfireCheckParityBits(uint8_t *Buffer, uint16_t BitCount);

#ifdef DESFIRE_DEBUGGING
#define DesfireDebuggingOn      (DESFIRE_DEBUGGING != 0)
#else
#define DesfireDebuggingOn      (false)
#endif

/* Add utility wrapper functions to perform pre and postprocessing on
 * the raw input APDU commands sent by the PCD depending on which
 * CommMode (PLAINTEXT|PLAINTEXT-MAC|ENCIPHERED-CMAC-3DES|ECIPHERED-CMAC-AES128)
 * setting is active.
 *
 * The implementation is adapted from the Java sources at
 * @github/andrade/nfcjlib (in the DESFireEV1 source files).
 * We will use the conventions in that library to update the SessionIV buffer
 * when the next rounds of data are exchanged. Note that the SessionIV and
 * SessionKey arrays are initialized in the Authenticate(Legacy|ISO|AES) commands
 * used to initiate the working session from PCD <--> PICC.
 *
 * Helper methods to format and encode quirky cases of the
 * CommSettings and wrapped APDU format combinations are defined statically in the
 * C source file to save space in the symbol table for the firmware (ELF) binary.
 */
uint16_t DesfirePreprocessAPDUWrapper(uint8_t CommMode, uint8_t *Buffer, uint16_t BufferSize, bool TruncateChecksumBytes);
#define DesfirePreprocessAPDU(CommMode, Buffer, BufferSize)                \
        DesfirePreprocessAPDUWrapper(CommMode, Buffer, BufferSize, false)
#define DesfirePreprocessAPDUAndTruncate(CommMode, Buffer, BufferSize)     \
        DesfirePreprocessAPDUWrapper(CommMode, Buffer, BufferSize, true)
uint16_t DesfirePostprocessAPDU(uint8_t CommMode, uint8_t *Buffer, uint16_t BufferSize);

#endif
