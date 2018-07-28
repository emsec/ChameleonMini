import binascii
import crcmod

# Parameters for CRC_A
CRC_INIT = 0x6363
POLY = 0x11021
CRC_A_func = crcmod.mkCrcFun(POLY, initCrc=CRC_INIT, xorOut=0)

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
        0x20: "UID complete, PICC compliant with 14443-4 ",
        0x00: "UID complete, PICC NOT compliant with 14443-4"
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

def parseReader(data):
    byteCount = len(data)
    note = ""

    # short frame commands
    if (byteCount == 1 and data[0] in ReaderTrafficTypes["SHORTFRAME"]):
            note += ReaderTrafficTypes["SHORTFRAME"][data[0]]

    # ANTICOLLISION command
    elif (byteCount<9 and byteCount > 1 and data[0] in ReaderTrafficTypes["SEL"] and data[1] & 0x88 == 0 ):
        note += "ATCOLI - "
        note += ReaderTrafficTypes["SEL"][data[0]]
        # note += "UID_CLn:" + binascii.hexlify(data[2:7]).decode() + " "
        # note += str((data[1] >> 4) & 0x0f) + "bytes + " + str(data[1] & 0x0f) + "bits "

    # SELECT Command
    elif (byteCount== 9 and data[0] in ReaderTrafficTypes["SEL"] and data[1] == 0x70):
        # TODO: distinguish CT+uid012+BCC and uid0123+BCC
        note += "SELECT - "
        note += ReaderTrafficTypes["SEL"][data[0]]
        note += "UID_CLn:" + binascii.hexlify(data[2:6]).decode() + " "
        # Check CRC for SELECT
        if(not CRC_A_check(data)):
            note+=" WRONG CRC "
        # note += "7bytes + 0bits "
        # note += "CRC_A:"+data[7:9]
    # HALT Command
    elif (byteCount == 4 and data[0] == 0x50 and data[1] == 0x00):
        note += "HALT"
    # RATS
    elif (byteCount == 4 and data[0] == 0xe0 and ((data[1] & 0x0f) < 15) and ((data[1] & 0xf0 >> 8) in ReaderTrafficTypes["FSDI"])):
        note += "RATS - "
        note += ReaderTrafficTypes["FSDI"][data[1]>>8]
        note += "CID:" + str(data[1]&0x0f) + " "
        if(not CRC_A_check(data)):
            note+=" WRONG CRC "
    # PPS Protocol and parameter selection request
    # PSS0 only
    elif (byteCount == 4 and (data[0]&0xf0 == 0xd0) and (data[1] == 0x01)):
        note += "PSS0 - "
        note += "CID:"+str(data[1]&0x0f) + " "
    # PSS0+1
    elif (byteCount == 5 and (data[0]&0xf0 == 0xd0) and (data[1] == 0x11) and (data[2]&0xf0 == 0x00)):
        note += "PSS0+1 - "
        note += "CID:"+str(data[1]&0x0f) + " "
        note += "DSI:"+str(pow(2,(data[2]>>2)&0x03)) + " "
        note += "DRI:"+str(pow(2,data[2]&0x03)) + " "
    # BLOCK S-block PCB DESELECT 
    # Without CID
    elif (byteCount == 3 and data[0] == 0xc2):
        note += "DESEL"
    # With CID
    elif (byteCount == 4 and data[0] == 0xca and data[1]&0x30 == 0x00):
        note += "DESEL - "
        note += "CID:"+ str(data[1]&0x0f) + " "
        
    return note

def parseCard(data):

    byteCount = len(data)
    note = ""

    # ATQA: RRRR XXXX  XXRX XXXX
    if(byteCount == 2 and (data[0] & 0x20 == 0x00) and (data[1] & 0xf0 == 0x00)):
        note += "ATQA - "
        note += binascii.hexlify(data).decode()
    # SAK
    elif (byteCount == 3 and (data[0] & (~0x24)) == 0x00):
        note += "SAK - "
        note += CardTrafficTypes["SAK"][(data[2] & 0x24)]
        if not CRC_A_check(data):
            note += " WRONG CRC "
    # UID
    elif (byteCount == 5 and (data[0] ^ data[1] ^ data[2] ^ data[3]) == data[4] ):
        note += "UID Resp - CLn "

    return note

    pass