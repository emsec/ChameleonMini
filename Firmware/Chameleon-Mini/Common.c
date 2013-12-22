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

#include "Common.h"

uint16_t BufferToHexString(char* HexOut, uint16_t MaxChars, const void* Buffer, uint16_t ByteCount)
{
    uint8_t* ByteBuffer = (uint8_t*) Buffer;
    uint16_t CharCount = 0;

    /* Account for '\0' at the end */
    MaxChars--;

    while( (ByteCount > 0) && (MaxChars >= 2) ) {
        uint8_t Byte = *ByteBuffer;

        HexOut[0] = NIBBLE_TO_HEXCHAR( (Byte >> 4) & 0x0F );
        HexOut[1] = NIBBLE_TO_HEXCHAR( (Byte >> 0) & 0x0F );

        HexOut += 2;
        MaxChars -= 2;
        CharCount += 2;
        ByteBuffer++;
        ByteCount -= 1;
    }

    *HexOut = '\0';

    return CharCount;
}

uint16_t HexStringToBuffer(void* Buffer, uint16_t MaxBytes, const char* HexIn)
{
    uint8_t* ByteBuffer = (uint8_t*) Buffer;
    uint16_t ByteCount = 0;

    while( (HexIn[0] != '\0') && (HexIn[1] != '\0') && (MaxBytes > 0) ) {
        if (VALID_HEXCHAR(HexIn[0]) && VALID_HEXCHAR(HexIn[1])) {
            uint8_t Byte = 0;

            Byte |= HEXCHAR_TO_NIBBLE(HexIn[0]) << 4;
            Byte |= HEXCHAR_TO_NIBBLE(HexIn[1]) << 0;

            *ByteBuffer = Byte;

            ByteBuffer++;
            MaxBytes--;
            ByteCount++;
            HexIn += 2;
        } else {
            /* HEX chars only */
            return 0;
        }
    }

    if ( (HexIn[0] != '\0') && (HexIn[1] == '\0') ) {
        /* Odd number of characters */
        return 0;
    }

    return ByteCount;
}





