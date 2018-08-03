import binascii
import crcmod
from enum import Enum

from Chameleon.MFDESFire import MFDESFireDecode
from Chameleon.utils import TrafficSource

# Parameters for CRC_A
CRC_INIT = 0x6363
POLY = 0x11021
CRC_A_func = crcmod.mkCrcFun(POLY, initCrc=CRC_INIT, xorOut=0)


class ReaderCMD(Enum):
    NONE = 0
    SELECT = 1
    RATS = 2
    PPS  = 3


readerCMD = ReaderCMD.NONE

# Map card types string to decoder
def DummyCardDecoder(data, source):
    return ""

CardTypesMap = {
    "None":      {"ApplicationDecoder": DummyCardDecoder},
    "MFDESFire": {"ApplicationDecoder": MFDESFireDecode},

}


class BlockData:
    @staticmethod
    def isBlockData(byteCount, data):
        if(byteCount >= 3 and (data[0]& 0xE6) in ReaderTrafficTypes["PCB"]):
            return True
        else:
            return False


    def __init__(self, byteCount, data, source, Cardtype):
        self.byteCount = byteCount
        self.data = data
        self.PCB = data[0]
        self.type = ReaderTrafficTypes["PCB"][self.PCB & 0xE6]
        self.CID = None
        self.NAD = None
        self.INF = None
        self.source = source
        self.CRCChecked = CRC_A_check(data)
        self.CardApplicationDecoder = CardTypesMap[Cardtype]["ApplicationDecoder"]

        if self.CRCChecked:

            hasCID = self.PCB & 0x08  # PCB b4 indicate CID
            hasNAD = 0

            if (self.type == "IBlock"):
                hasNAD = self.PCB & 0x04  # IBlock PCB b2 indicate NAD

            byteNext = 1
            # CID
            if (hasCID):
                self.CID = self.data[byteNext]
                byteNext += 1

            # NAD
            if (hasNAD):
                self.NAD = self.data[byteNext]
                byteNext += 1

            # INF field not empty
            if(byteNext < byteCount -2):
                self.INF = self.data[byteNext: byteCount-2]


    def decode(self):
        note = ""
        # Prologue
        # PCB
        note += self.type + " "

        # Block number
        if ((self.type == "IBlock" or self.type == "RBlock") and self.PCB & 0x01):
            note += "BlkNo:1 "

        # Chaining?
        if (self.type == "IBlock" and self.PCB & 0x10):
            note += "Chaining "

        # ACK/NAK? for R-Block
        if (self.type == "RBlock"):
            if (self.PCB & 0x10):
                note += "NAK "
            else:
                note += "ACK "

        # DESEL/WTX for SBlock
        if (self.type == "SBlock"):
            if (self.PCB & 0x30 == 0x00):
                note += "DESEL "
            elif (self.PCB & 0x30 == 0x30):
                note += "WTX"

        # CID
        if (self.CID != None):
            note += "CID:" + hex(self.CID) + " "

        # NAD
        if (self.NAD != None):
            note += "NAD:" + hex(self.NAD) + " "

        # INF
        if (self.INF != None):
            note += self.CardApplicationDecoder(self.INF, self.source)
        # EDC CRC check
        if not self.CRCChecked:
            note += " WRONG CRC "


        return note


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
    "PCB":{
        # IBlock 000X XX1X
        0x02: "IBlock",         # 000X X01X
        0x08: "IBlock",         # 000X X11X
        # RBlock 101X X01X
        0xA2: "RBlock",         # 101X X01X
        # SBlock 11XX X010
        0xC2: "SBlock",         # 110X X010
        0xE2: "SBlock"          # 111X X010
    }
}

CardTrafficTypes = {
    "SAK":{
        0x04: "UID NOT Complete ",
        0x24: "UID NOT Complete, PICC compliant with 14443-4",
        0x20: "UID complete, PICC compliant with 14443-4 ",
        0x00: "UID complete, PICC NOT compliant with 14443-4"
    },
    "FSCI": {
        0x0: "FSC:16 ",
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
    global readerCMD
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
        readerCMD = ReaderCMD.SELECT
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

def parseReader_4(data, Cardtype):
    global readerCMD
    byteCount = len(data)
    note = ""

    # RATS
    if (byteCount == 4 and data[0] == 0xe0 and ((data[1] & 0x0f) < 15) and (
            (data[1] & 0xf0 >> 8) in ReaderTrafficTypes["FSDI"])):
        note += "RATS - "
        note += ReaderTrafficTypes["FSDI"][data[1] >> 8]
        note += "CID:" + hex(data[1] & 0x0f) + " "
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

    # Half-duplex block transmission
    # PCB bit mask: 0b11100110
    elif (BlockData.isBlockData(byteCount, data)):
        blockData = BlockData(byteCount,data, TrafficSource.Reader, Cardtype)
        note = blockData.decode()

    return note

def parseCard_3(data):
    byteCount = len(data)
    note = ""

    # ATQA: RRRR XXXX  XXRX XXXX
    if(byteCount == 2 and (data[0] & 0x20 == 0x00) and (data[1] & 0xf0 == 0x00)):
        note += "ATQA - "
        note += binascii.hexlify(data).decode()
    # SAK
    elif (byteCount == 3 and readerCMD == ReaderCMD.SELECT and ((data[0] & (0x24)) in CardTrafficTypes["SAK"])):
        note += "SAK - "
        note += CardTrafficTypes["SAK"][(data[2] & 0x24)]
        if not CRC_A_check(data):
            note += " WRONG CRC "
    # UID
    elif (byteCount == 5 and (data[0] ^ data[1] ^ data[2] ^ data[3]) == data[4] ):
        note += "UID Resp - CLn "

    return note

def parseCard_4(data, Cardtype):
    global readerCMD
    byteCount = len(data)
    note = ""

    # ATS
    # TL + T0 + TA + TB + TC + T1 ... + CRC
    # TL: length without CRC
    # ATS without data
    if(byteCount == 3 and readerCMD == ReaderCMD.RATS and data[0] == (byteCount-2)):
        note += "ATS - NO DATA"
    # ATS with data mush have T0, T0 b8=0
    elif (byteCount > 3 and readerCMD == ReaderCMD.RATS and data[0] == (byteCount-2)
            and data[1] & 0x80 == 0x00
            and data[1] & 0x0f in CardTrafficTypes["FSCI"]):
        note += "ATS - "
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

        # Check CRC_A
        if not CRC_A_check(data):
            note += " WRONG CRC "
    # Application Data
    elif (BlockData.isBlockData(byteCount, data)):
        blockData = BlockData(byteCount,data, TrafficSource.Card, Cardtype)
        note = blockData.decode()

    readerCMD = ReaderCMD.NONE

    return note

def parseReader(data, Cardtype):
    return parseReader_3(data) + parseReader_4(data, Cardtype)

def parseCard(data, Cardtype):
    return parseCard_3(data) + parseCard_4(data, Cardtype)
