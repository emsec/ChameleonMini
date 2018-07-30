from Chameleon.utils import TrafficSource
from binascii import hexlify

lastCMD = 0x00

StatusCode = {
    0x00 : "OPERATION_OK",
    0x0C : "NO_CHANGES",
    0x0E : "ERR_OUT_OF_EEPROM",
    0x1C : "ILLEGAL_CMD_CODE",
    0x1E : "ERR_INTEGRITY",
    0x40 : "NO_SUCH_KEY",
    0x7E : "ERR_LENGTH",
    0x9D : "PERMISSION_DENIED",
    0x9E : "ERR_PARAMETER",
    0xA0 : "APP_NOT_FOUND",
    0xA1 : "ERR_APP_INTEGRITY",
    0xAE : "ERR_AUTH",
    0xAF : "ADDITIONAL_FRAME",
    0xBE : "ERR_BOUNDARY",
    0xC1 : "ERR_PICC_INTEGRITY",
    0xCA : "CMD_ABORTED",
    0xCD : "ERR_PICC_DISABLED",
    0xCE : "ERR_COUNT",
    0xDE : "ERR_DUPLICATE",
    0xEE : "ERR_EEPROM",
    0xF0 : "FILE_NOT_FOUND",
    0xF1 : "ERR_FILE_INTEGRITY"
}


def decodeSelectAPP(data):
    if len(data) == 4:
        return "AID: 0x"+ hexlify(data[1:4]).decode()
    else:
        return "Decode Fail"

def decodeGetAPPID(data):
    if len(data) == 1:
        return ""
    else:
        return "Decode Fail"

def decodeRespGetAPPID (data):
    note = "APPIDs: |"
    dataLen = len(data)

    for i in range (0,int((dataLen-1)/3)):
        note += "0x"+hexlify(data[3*i: 3*(i+1)]).decode()+"|"

    return note


def decodeAuthAES(data):
    if len(data) == 2:
        return "KeyNo:"+hex(data[1])
    else:
        return "Decode Fail"

def decodeRespAuthAES(data):
    return ""


def decodeReadData(data):
    if len(data) == 8:
        fileNo = data[1]
        offSet = data[2:5]
        length = data[5:8]
        return "FileNo:"+hex(fileNo) + " OffSet:0x"+hexlify(offSet).decode() + " len:0x"+hexlify(length).decode()
    else:
        return "Decode Fail"

def decodeRespReadData(data):
    return "Data:0x"+hexlify(data[1:]).decode()

def decodeAdiFrame(data):
    return ""

def decodeRespAdiFrame(data):
    return ""

def decodeDummy(data):
    return ""

MFDESFireCMDTypes = {
    0x5A : {"name": "SelectApp ",       "CMDdecoder":decodeSelectAPP,       "RespDecoder":   decodeDummy},
    0x6A : {"name": "GetAPPID ",        "CMDdecoder":decodeGetAPPID,        "RespDecoder":   decodeRespGetAPPID},
    0xAA : {"name": "AuthAES ",         "CMDdecoder":decodeAuthAES,         "RespDecoder":   decodeRespAuthAES},
    0xBD : {"name": "ReadData ",        "CMDdecoder":decodeReadData,        "RespDecoder":   decodeRespReadData},
    0xAF : {"name": "AdditionalFrame",  "CMDdecoder":decodeAdiFrame,        "RespDecoder":   decodeRespAdiFrame}
}


def MFDESFireDecode(data, source):
    note = ""
    global lastCMD
    if (source == TrafficSource.Reader):
        cmd = data[0]
        if cmd in MFDESFireCMDTypes :
            lastCMD = cmd
            note += "CMD:" + MFDESFireCMDTypes[cmd]["name"]
            note += MFDESFireCMDTypes[cmd]["CMDdecoder"](data)

    elif (source == TrafficSource.Card):
        status = data[0]
        # Decode status code
        if status in StatusCode:
            note += StatusCode[status] + " "
        # If status Ok, decode data
        if status == 0x00 and lastCMD in MFDESFireCMDTypes:
            note += MFDESFireCMDTypes[lastCMD]["RespDecoder"](data)

    return note