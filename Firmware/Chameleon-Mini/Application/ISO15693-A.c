#if defined (CONFIG_ISO15693_SNIFF_SUPPORT) || defined (CONFIG_SL2S2002_SUPPORT) || defined (CONFIG_TITAGITSTANDARD_SUPPORT) || defined (CONFIG_TITAGITPLUS_SUPPORT)

#include "ISO15693-A.h"
#include "../Common.h"
#include <util/crc16.h>

CurrentFrame FrameInfo;
uint8_t Uid[ISO15693_GENERIC_UID_SIZE];
uint8_t MyAFI;
uint16_t ResponseByteCount;

//Refer to ISO/IEC 15693-3:2001 page 41
uint16_t calculateCRC(void *FrameBuf, uint16_t FrameBufSize) {
    uint16_t reg = ISO15693_CRC16_PRESET;
    uint8_t i, j;

    uint8_t *DataPtr = (uint8_t *)FrameBuf;

    for (i = 0; i < FrameBufSize; i++) {
        reg = reg ^ *DataPtr++;
        for (j = 0; j < 8; j++) {
            if (reg & 0x0001) {
                reg = (reg >> 1) ^ ISO15693_CRC16_POLYNORMAL;
            } else {
                reg = (reg >> 1);
            }
        }
    }

    return ~reg;
}

void ISO15693AppendCRC(uint8_t *FrameBuf, uint16_t FrameBufSize) {
    uint16_t crc;

    crc = calculateCRC(FrameBuf, FrameBufSize);

    uint8_t crcLb = crc & 0xFF;
    uint8_t crcHb = crc >> 8;


    FrameBuf[FrameBufSize] = crcLb;
    FrameBuf[FrameBufSize + 1] = crcHb;
}

bool ISO15693CheckCRC(void *FrameBuf, uint16_t FrameBufSize) {
    uint16_t crc;
    uint8_t *DataPtr = (uint8_t *)FrameBuf;

    crc = calculateCRC(DataPtr, FrameBufSize);

    uint8_t crcLb = crc & 0xFF;
    uint8_t crcHb = crc >> 8;

    return (DataPtr[FrameBufSize] == crcLb && DataPtr[FrameBufSize + 1] == crcHb);
}

/*
 * ISO15693PrepareFrame
 *
 * This function sets pointers in 'frame' struct to relevant byte(s) of 'FrameBuf'.
 * Also sets frame.Addressed as true if the command is addressed, same goes for frame.Selected
 *
 * Returns:
 *  - true:  Frame is valid and a response is needed
 *  - false: Request is not addressed to us
 *           Frame is not valid because it's too short or CRC is wrong
 *
 * Authors: ceres-c & MrMoDDoM
 */
bool ISO15693PrepareFrame(uint8_t *FrameBuf, uint16_t FrameBytes, CurrentFrame *FrameStruct, uint8_t IsSelected, uint8_t *MyUid, uint8_t MyAFI) {
    /* following declarations are not dependent on addressed/unaddressed state */
    FrameStruct -> Flags        = &FrameBuf[ISO15693_ADDR_FLAGS];
    FrameStruct -> Command      = &FrameBuf[ISO15693_REQ_ADDR_CMD];

    if (!(FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_INVENTORY)) { /* if inventory flag is not set */
        FrameStruct -> Addressed = (FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_ADDRESS); /* check for addressed flag */
        FrameStruct -> Selected  = (FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_SELECT);  /* check for selected flag */
    } else { /* otherwise always false */
        FrameStruct -> Addressed = false;
        FrameStruct -> Selected  = false;
    }

    if (FrameStruct -> Addressed)
        /* UID sits between CMD and PARAM */
        FrameStruct -> Parameters = &FrameBuf[ISO15693_REQ_ADDR_PARAM + ISO15693_GENERIC_UID_SIZE];
    else
        FrameStruct -> Parameters = &FrameBuf[ISO15693_REQ_ADDR_PARAM];

    if ((*FrameStruct -> Command) >= 0xA0) {  /* if command is Custom or  Proprietary */
        /* then between CMD and UID is placed another byte which is IC Mfg Code, but we don't need it */
        if (FrameBuf[ISO15693_REQ_ADDR_PARAM] != MyUid[1])
            /* if IC Mfg Code is different from our Mfg code (2nd byte of UID), then don't respond */
            return false;
        FrameStruct -> Parameters += 0x01;
    } else if (((*FrameStruct -> Command) == ISO15693_CMD_INVENTORY) && (FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_AFI)) { /* or if it is Inventory with AFI flag set */
        /* then between CMD and UID is placed another byte which is requested AFI, but we don't need it */
        if (FrameBuf[ISO15693_REQ_ADDR_PARAM] != MyAFI)
            /* if requested AFI is different from our current one, then don't respond */
            return false;
        FrameStruct -> Parameters += 0x01;
    }

    FrameStruct -> ParamLen = FrameBuf + (FrameBytes - ISO15693_CRC16_SIZE) - (FrameStruct -> Parameters);

    /*                                                  The UID, if present, always sits right befoore the parameters */
    if (FrameStruct -> Addressed && !ISO15693CompareUid(FrameStruct -> Parameters - ISO15693_GENERIC_UID_SIZE, MyUid)) {
        /* addressed request but we're not the addressee */
        return false;
    } else if (FrameStruct -> Selected && !IsSelected) {
        /* selected request but we're not in selected state */
        return false;
    } else {
        return true;
    }
}

/*
 * ISO15693AntiColl
 *
 * This function performs ISO15693 Anticollision
 *
 * Returns:
 *  - true:  Inventory is addressed to us and a response is needed
 *  - false: Request is not addressed to us (different bitmask)
 *           and no response should be issued.
 *
 * Authors: ceres-c
 *
 * TODO:
 *  - Implement 16 slots mode (still, it seems to work as it is, even for 16 slots mode ._.)
 */
bool ISO15693AntiColl(uint8_t *FrameBuf, uint16_t FrameBytes, CurrentFrame *FrameStruct, uint8_t *MyUid) {
    uint8_t CurrentUID[ ISO15693_GENERIC_UID_SIZE ]; /* Holds the UID flipped */
    ISO15693CopyUid(CurrentUID, MyUid);

    uint8_t MaskLenght = *FrameStruct -> Parameters; /* First byte of parameters is mask lenght... */
    uint8_t *MaskValue = FrameStruct -> Parameters + 1; /* ... then the mask itself begins */
    uint8_t BytesNum = MaskLenght / 8; /* Gets the number of full bytes in mask */
    uint8_t BitsNum = MaskLenght % 8; /* Gets the number of spare bits */

    uint8_t B; /* B stands for Bytes and will be our reference point (I know, terrible name, I'm sorry) */
    /* Compare the full bytes in mask with UID */
    for (B = 0; B < BytesNum; B++) {
        if (CurrentUID[B] != MaskValue[B])
            return false; /* UID different */
    }
    /* Once we got here, B points for sure to the last byte of the mask, the one composed of spare bits */

    /* Compare spare bits in mask with bits of UID */
    uint8_t BitsMask = ((1 << BitsNum) - 1); /* (1 << Bits) = 00010000   ->   (1 << Bits) - 1 = 00001111 */
    if ((MaskValue[B] & BitsMask) != (CurrentUID[B] & BitsMask)) {
        /*  |-----------------------| Here we extract bits from the mask we received from the reader
         *                               |------------------------| Here we extract bits from the UID
         * The two extracted bit vectors are compared, if different then we're not the addressee.
         * Only the latest byte of MaskValue and UID is considered.
         */
        return false;
    }

    return true;
}

#endif /* CONFIG_ISO15693_SNIFF_SUPPORT || CONFIG_SL2S2002_SUPPORT || CONFIG_TITAGITSTANDARD_SUPPORT || CONFIG_TITAGITPLUS_SUPPORT */
