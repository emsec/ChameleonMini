import binascii
import crcmod
from enum import Enum
# Parameters for CRC_A
CRC_INIT = 0x6363
POLY = 0x11021
CRC_A_func = crcmod.mkCrcFun(POLY, initCrc=CRC_INIT, xorOut=0)


class ReaderCMD(Enum):
    NONE = 0
    RATS = 1
    PPS  = 2

readerCMD = ReaderCMD.NONE

ReaderTrafficTypes = {
    "SEL":{
        0x93: "SEL_CL1 ",
        0x95: "SEL_CL2 ",
        0x97: "SEL_CL3 ",
    },
    # 1 byte commands
    "SHORTFRAME": {
        0x26: "REQA",
        0x52: "WUPA",
        0x35: "Optional Timeslot Method",
        # 40 - 4F Proprietary
        # 78 - 7F proprietary
        # Other RFU
    },
    "FSDI":{
        0x0: "FSD:16 ",
        0x1: "FSD:24 ",
        0x2: "FSD:32 ",
        0x3: "FSD:40 ",
        0x4: "FSD:48 ",
        0x5: "FSD:64 ",
        0x6: "FSD:96 ",
        0x7: "FSD:128 ",
        0x8: "FSD:256 "
    },

}

CardTrafficTypes = {
    "SAK":{
        0x04: "UID NOT Complete ",
        0x24: "UID NOT Complete, PICC compliant with 14443-4",
        0x20: "UID complete, PICC compliant with 14443-4 ",
        0x00: "UID complete, PICC NOT compliant with 14443-4"
    },
    "FSCI": {
        0x0: "FSCC:16 ",
        0x1: "FSC:24 ",
        0x2: "FSC:32 ",
        0x3: "FSC:40 ",
        0x4: "FSC:48 ",
        0x5: "FSC:64 ",
        0x6: "FSC:96 ",
        0x7: "FSC:128 ",
        0x8: "FSC:256 "
    }
}

def CRC_A(data):
    return CRC_A_func(data)

def CRC_A_check(data):
    datalen = len(data)
    # Short frame/SAK or no space for CRC skip check
    if(datalen < 3 ):
        return True

    crc = CRC_A(bytearray(data[0:datalen-2])).to_bytes(2,'little')
    if (data[datalen-2:datalen] == crc):
        return True
    else:
        return False

def parseReader_3(data):
    byteCount = len(data)
    note = ""

    # short frame commands
    if (byteCount == 1 and data[0] in ReaderTrafficTypes["SHORTFRAME"]):
        note += ReaderTrafficTypes["SHORTFRAME"][data[0]]

    # ANTICOLLISION command
    elif (byteCount < 9 and byteCount > 1 and data[0] in ReaderTrafficTypes["SEL"] and data[1] & 0x88 == 0):
        note += "ATCOLI - "
        note += ReaderTrafficTypes["SEL"][data[0]]
        # note += "UID_CLn:" + binascii.hexlify(data[2:7]).decode() + " "
        # note += str((data[1] >> 4) & 0x0f) + "bytes + " + str(data[1] & 0x0f) + "bits "

    # SELECT Command
    elif (byteCount == 9 and data[0] in ReaderTrafficTypes["SEL"] and data[1] == 0x70):
        # TODO: distinguish CT+uid012+BCC and uid0123+BCC
        note += "SELECT - "
        note += ReaderTrafficTypes["SEL"][data[0]]
        note += "UID_CLn:" + binascii.hexlify(data[2:6]).decode() + " "
        # Check CRC for SELECT
        if (not CRC_A_check(data)):
            note += " WRONG CRC "
        # note += "7bytes + 0bits "
        # note += "CRC_A:"+data[7:9]
    # HALT Command
    elif (byteCount == 4 and data[0] == 0x50 and data[1] == 0x00):
        note += "HALT"

    return note

def parseReader_4(data):
    byteCount = len(data)
    note = ""

    # RATS
    if (byteCount == 4 and data[0] == 0xe0 and ((data[1] & 0x0f) < 15) and (
            (data[1] & 0xf0 >> 8) in ReaderTrafficTypes["FSDI"])):
        note += "RATS - "
        note += ReaderTrafficTypes["FSDI"][data[1] >> 8]
        note += "CID:" + str(data[1] & 0x0f) + " "
        if (not CRC_A_check(data)):
            note += " WRONG CRC "
        else:
            readerCMD = ReaderCMD.RATS

    # PPS Protocol and parameter selection request
    # PSS0 only
    elif (byteCount == 4 and (data[0] & 0xf0 == 0xd0) and (data[1] == 0x01)):
        note += "PSS0 - "
        note += "CID:" + str(data[1] & 0x0f) + " "
        readerCMD = ReaderCMD.PPS
    # PSS0+1
    elif (byteCount == 5 and (data[0] & 0xf0 == 0xd0) and (data[1] == 0x11) and (data[2] & 0xf0 == 0x00)):
        note += "PSS0+1 - "
        note += "CID:" + str(data[1] & 0x0f) + " "
        note += "DSI:" + str(pow(2, (data[2] >> 2) & 0x03)) + " "
        note += "DRI:" + str(pow(2, data[2] & 0x03)) + " "
    # BLOCK S-block PCB DESELECT
    # Without CID
    elif (byteCount == 3 and data[0] == 0xc2):
        note += "DESEL"
    # With CID
    elif (byteCount == 4 and data[0] == 0xca and data[1] & 0x30 == 0x00):
        note += "DESEL - "
        note += "CID:" + str(data[1] & 0x0f) + " "

    return note

def parseCard_3(data):
    byteCount = len(data)
    note = ""

    # ATQA: RRRR XXXX  XXRX XXXX
    if(byteCount == 2 and (data[0] & 0x20 == 0x00) and (data[1] & 0xf0 == 0x00)):
        note += "ATQA - "
        note += binascii.hexlify(data).decode()
    # SAK
    elif (byteCount == 3 and ((data[0] & (0x24)) in CardTrafficTypes["SAK"])):
        note += "SAK - "
        note += CardTrafficTypes["SAK"][(data[2] & 0x24)]
        if not CRC_A_check(data):
            note += " WRONG CRC "
    # UID
    elif (byteCount == 5 and (data[0] ^ data[1] ^ data[2] ^ data[3]) == data[4] ):
        note += "UID Resp - CLn "

    return note

def parseCard_4(data):
    byteCount = len(data)
    note = ""

    # ATS
    # TL + T0 + TA + TB + TC + T1 ... + CRC
    # TL: length without CRC
    # ATS without data
    if(byteCount == 3 and data[0] == (byteCount-2)):
        note += "ATS - NO DATA"
    # ATS with data mush have T0, T0 b8=0
    elif (byteCount > 3 and data[0] == (byteCount-2)
            and data[1] & 0x80 == 0x00
            and data[1] & 0x0f in CardTrafficTypes["FSCI"]):
        # Decode T0
        hasTA = data[1] & 0x10
        hasTB = data[1] & 0x20
        hasTC = data[1] & 0x40
        note += CardTrafficTypes["FSCI"][data[1] & 0x0f]

        # Which byte to decode next
        byteNext = 2        # T0 Decoded, next is TA/TB/TC
        #  TA b4=0
        if (hasTA and data[byteNext] & 0x08 == 0x00):
            note += "TA:" + hex(data[byteNext]) + " "
            byteNext += 1

        if (hasTB):
            note += "TB:" + hex(data[byteNext]) + " "
            byteNext += 1

        # TC b3-8=0
        if (hasTC and data[byteNext] & 0xFC == 0x00):
            note += "TC:" + hex(data[byteNext]) + " "
            byteNext += 1

        # TODO: decode historical bytes

        # Check CRC_A
        if not CRC_A_check(data):
            note += " WRONG CRC "

    elif ():
        pass
    return note

def parseReader(data):
    return parseReader_3(data) + parseReader_4(data)

def parseCard(data):
    return parseCard_3(data) + parseCard_4(data)
