from Chameleon.utils import TrafficSource
from binascii import hexlify

lastCMD = 0x00
strFail = "Decode Fail"
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
#########################
# PICC Level Commands
#########################
# Create APP CMD
def decodeCreateAPP(data):
    note = ""
    if len(data) == 6:
        note += "AID:0x"+hexlify(data[1:4]).decode() + " "
        note += "KeySett:"+hex(data[4]) + " "
        note += "NumOfKeys:"+hex(data[5]) + " "
    else:
        note += strFail
    return note

def decodeDelAPP(data):
    if len(data) == 4:
         return "AID:0x"+hexlify(data[1:]).decode()
    else:
        return strFail

def decodeSelectAPP(data):
    if len(data) == 4:
        return "AID:0x"+ hexlify(data[1:4]).decode()
    else:
        return strFail

# Get APP ID CMD
def decodeGetAPPID(data):
    if len(data) == 1:
        return ""
    else:
        return strFail

def decodeRespGetAPPID (data):
    note = "APPIDs: |"
    dataLen = len(data)

    for i in range (0,int((dataLen-1)/3)):
        note += "0x"+hexlify(data[1+3*i: 1+3*(i+1)]).decode()+"|"

    return note
##############################
# Security Related Commands
##############################
# Authenticate AES CMD
def decodeAuthAES(data):
    if len(data) == 2:
        return "KeyNo:"+hex(data[1])
    else:
        return strFail

def decodeRespAuthAES(data):
    if len(data) == 17:
        return "ekNo(RndB):0x" + hexlify(data[1:]).decode()
    else:
        return strFail

def decodeAuthAESAF (data):
    if len(data) == 33:
        return "ekNo(RndA+RndB'):0x" + hexlify(data[1:]).decode()
    else:
        return strFail

def decodeCardAuthAESAF (data):
    if len(data) == 17:
        return "ekNo(RndA'):0x" + hexlify(data[1:]).decode()
    else:
        return strFail

# Authenticate 3DES CMD
def decodeAuth3DES(data):
    if len(data) == 2:
        return "KeyNo:"+hex(data[1])
    else:
        return strFail

def decodeRespAuth3DES(data):
    if len(data) == 9:
        return "ekNo(RndB):0x" + hexlify(data[1:]).decode()
    else:
        return strFail

def decodeAuth3DESAF(data):
    if len(data) == 17:
        return "dkNo(RndA+RndB'):0x" + hexlify(data[1:]).decode()
    else:
        return strFail

def decodeCardAuth3DESAF (data):
    if len(data) == 9:
        return "ekNo(RndA'):0x" + hexlify(data[1:]).decode()
    else:
        return strFail

# ChangeKeySettings CMD
def decodeChangeKeySettings(data):
    if len(data) == 9:
        return "KeySettings:0x" + hexlify(data[1:]).decode()
    else:
        return strFail
# GetKeySettings CMD
def decodeRespGetKeySettings(data):
    if len(data) == 3:
        return "KeySettings:" + hex(data[1]) + " MaxNoKeys:" + hex(data[2])
    else:
        return strFail

# ChangeKey CMD
def decodeChangeKey(data):
    if len(data) == 26:
        return "KeyNo:" + hex(data[1]) + " decipheredKeyData:0x"+hexlify(data[2:]).decode()
    else:
        return strFail

# GetKeyVersion CMD
def decodeGetKeyVersion(data):
    if len(data) == 2:
        return "KeyNo:" + hex(data[1])
    else:
        return strFail
def decodeRespGetKeyVersion(data):
    return "KeyVersion" + hex(data[1]) if len(data) == 2 else strFail

##############################
# Data Manipulation Commands
##############################
# Read Data
def decodeFileNoOffsetLen(data):
    if len(data) == 8:
        fileNo = data[1]
        offSet = data[2:5]
        length = data[5:8]
        return "FileNo:"+hex(fileNo) + " OffSet:0x"+hexlify(offSet).decode() + " len:0x"+hexlify(length).decode()
    else:
        return strFail
# Write Data
def decodeFileNoOffsetLenData(data):
    if len(data) > 8:
        fileNo = data[1]
        offSet = data[2:5]
        length = data[5:8]
        dataWrite = data[8:]
        return "FileNo:"+hex(fileNo) + " OffSet:0x"+hexlify(offSet).decode() + " len:0x"+hexlify(length).decode() + \
               " Data:0x" + hexlify(dataWrite).decode()
    else:
        return strFail

# Get Value
def decodeFileNo(data):
    if len(data) == 2:
        return "FileNo:"+hex(data[1])
    else:
        return strFail
# Credit
def decodeFileNoData(data):
    if len(data) >2:
        return "FileNo:" + hex(data[1]) + " Data:0x" + hexlify(data[2:]).decode()
    else:
        return strFail

def decodeData(data):
    return "Data:0x"+hexlify(data[1:]).decode() if len(data) >1 else strFail

###########################
# Application Level Commands
###########################
def decodeRespGetFileIDs(data):
    if len(data) > 0:
        return "FIDs:0x"+hexlify(data[1:]).decode()
    else:
        return strFail

def decodeRespGetFileSettings(data):
    lenData = len(data)
    note = ""
    if lenData >5:
        note += "FileType:"+hex(data[1])+" ComSett:"+hex(data[2]) + "AccessRight:0x"+hexlify(data[3:5]).decode() + " "
        if lenData == 8:
            note += "FileSize:0x"+hexlify(data[5:]).decode()
        elif lenData == 18:
            note += "LowLimit:0x"+hexlify(data[5:9]).decode() + " "
            note += "UpLimit:0x" + hexlify(data[9:13]).decode() + " "
            note += "LimitedCreditVal:0x"+hexlify(data[13:17]).decode() + " "
            note += "LimitedCreditEn:" + hex(data[17])
        elif lenData == 14:
            note += "RecordSize:0x"+ hexlify(data[5:8]).decode() + " "
            note += "MaxNumRecords:0x"+hexlify(data[8:11]).decode() + " "
            note += "CurrentNumRecords:0x"+hexlify(data[11:]).decode()
        return note
    else:
        return strFail

def decodeFNComSetAccessRightsFileSize(data):
    if len(data) == 8:
        note = "FileNo:" + hex(data[1]) + " ComSet:" + hex(data[2]) + "AccessRight:0x" + hexlify(
            data[3:5]).decode() + " "
        note += "FileSize:0x" + hexlify(data[5:]).decode()
        return note
    else:
        return strFail

def decodeChangeFileSettings(data):
    lenData = len(data)
    note =""
    if lenData > 2:
        note += "FileNo:"+hex(data[1]) + " "
        if lenData == 5:
            note += "ComSet:"+hex(data[2]) + " "
            note += "AccessRights:0x"+hexlify(data[3:]).decode()
        elif lenData == 10:
            note += "NewSettings:0x"+hexlify(data[2:]).decode()
        return note
    else:
        return strFail

def decodeCreateValueFile(data):
    if len(data) == 18:
        note = "FileNo:"+hex(data[1])+" ComSet:"+hex(data[2]) + "AccessRight:0x"+hexlify(data[3:5]).decode() + " "
        note += "LowLimit:0x"+hexlify(data[5:9]).decode() + " "
        note += "UpLimit:0x" + hexlify(data[9:13]).decode() + " "
        note += "Val:0x"+hexlify(data[13:17]).decode() + " "
        note += "LimitedCreditEn:" + hex(data[17])
        return note
    else:
        return strFail

def decodeCreateRecordFile(data):
    if len(data) == 11:
        note = "FileNo:"+hex(data[1])+" ComSet:"+hex(data[2]) + "AccessRight:0x"+hexlify(data[3:5]).decode() + " "
        note += "RecordSize:0x"+hexlify(data[5:8]).decode()+" "
        note += "MaxNumRecords:0x"+hexlify(data[8:]).decode()
        return note
    else:
        return strFail

def decodeDelFile(data):
    if len(data) == 2:
        return "FileNo:"+hex(data[1])
    else:
        return strFail

def decodeAdiFrame(data):
    return "Data:0x"+hexlify(data[1:]).decode() if len(data) >1 else strFail

def decodeRespAdiFrame(data):
    return "Data:0x"+hexlify(data[1:]).decode() if len(data) >1 else strFail

def decodeDummy(data):
    return ""





MFDESFireCMDTypes = {
    # Application Level Commands
    0x6F : {"name": "GetFileIDs",           "CMDdecoder":decodeDummy,                       "RespDecoder": decodeRespGetFileIDs},
    0xF5 : {"name": "GetFileSettings",      "CMDdecoder":decodeFileNo,                      "RespDecoder": decodeRespGetFileSettings},
    0x5F : {"name": "ChangeFileSettings",   "CMDdecoder":decodeChangeFileSettings,          "RespDecoder": decodeDummy},
    0xCD : {"name": "CreateStdDataFile",    "CMDdecoder":decodeFNComSetAccessRightsFileSize,"RespDecoder": decodeDummy},
    0xCB : {"name": "CreateBackupDataFile", "CMDdecoder":decodeFNComSetAccessRightsFileSize,"RespDecoder": decodeDummy},
    0xCC : {"name": "CreateValueFile",      "CMDdecoder":decodeCreateValueFile,             "RespDecoder": decodeDummy},
    0xC1 : {"name": "CreateLinearRecordFile","CMDdecoder":decodeCreateRecordFile,           "RespDecoder": decodeDummy},
    0xC0 : {"name": "CreateCyclicRecordFile","CMDdecoder":decodeCreateRecordFile,           "RespDecoder": decodeDummy},
    0xDF: {"name": "DelFile",               "CMDdecoder":decodeDelFile,                     "RespDecoder": decodeDummy},

    # PICC Level Commands, GetVersion not decoded, please refer to datasheet for the meaning of resp
    0xCA : {"name": "CreateApp ",       "CMDdecoder":decodeCreateAPP,       "RespDecoder":   decodeDummy},
    0xDA : {"name": "DelApp ",          "CMDdecoder":decodeDelAPP,          "RespDecoder":   decodeDummy},
    0x5A : {"name": "SelectApp ",       "CMDdecoder":decodeSelectAPP,       "RespDecoder":   decodeDummy},
    0x6A : {"name": "GetAPPID ",        "CMDdecoder":decodeGetAPPID,        "RespDecoder":   decodeRespGetAPPID},
    0xFC : {"name": "FormatPICC ",      "CMDdecoder":decodeDummy,           "RespDecoder":   decodeDummy},
    0x60 : {"name": "GetVersion ",      "CMDdecoder":decodeDummy,           "RespDecoder":   decodeDummy},
    # Data Manipulation Commands
    0xBD : {"name": "ReadData ",        "CMDdecoder":decodeFileNoOffsetLen, "RespDecoder":   decodeData},
    0x3D : {"name": "WriteData ",       "CMDdecoder":decodeFileNoOffsetLenData,     "RespDecoder":   decodeDummy},
    0x6C : {"name": "GetValue ",        "CMDdecoder":decodeFileNo,          "RespDecoder":   decodeData},
    0x0C : {"name": "Credit ",          "CMDdecoder":decodeFileNoData,      "RespDecoder":   decodeDummy},
    0xDC : {"name": "Debit ",           "CMDdecoder":decodeFileNoData,      "RespDecoder":   decodeDummy},
    0x1C : {"name": "LimitedCredit ",   "CMDdecoder":decodeFileNoData,      "RespDecoder":   decodeDummy},
    0x3B : {"name": "WriteRecord ",     "CMDdecoder":decodeFileNoOffsetLenData,     "RespDecoder":  decodeDummy},
    0xBB : {"name": "ReadRecord ",      "CMDdecoder":decodeFileNoOffsetLen, "RespDecoder":   decodeData},
    0xEB : {"name": "ClearRecordFile ", "CMDdecoder":decodeFileNo,          "RespDecoder":   decodeDummy},
    0xC7 : {"name": "CommitTransaction ","CMDdecoder":decodeDummy,          "RespDecoder":   decodeDummy},
    0xA7 : {"name": "AbortTransaction ","CMDdecoder":decodeDummy,           "RespDecoder":   decodeDummy},
    # Security Related CMD
    0xAA : {"name": "AuthAES ",         "CMDdecoder": decodeAuthAES,        "RespDecoder":   decodeRespAuthAES},
    0x0A : {"name": "Auth3DES ",        "CMDdecoder": decodeAuth3DES,       "RespDecoder":   decodeRespAuth3DES},
    0x54 : {"name": "ChangeKeySettings ","CMDdecoder":decodeChangeKeySettings,"RespDecoder": decodeDummy},
    0x45 : {"name": "GetKeySettings ",  "CMDdecoder":decodeDummy,           "RespDecoder":   decodeRespGetKeySettings},
    0xC4 : {"name": "ChangeKey ",       "CMDdecoder":decodeChangeKey,       "RespDecoder":   decodeDummy},
    0x64 : {"name": "GetKeyVersion ",   "CMDdecoder":decodeGetKeyVersion,   "RespDecoder":   decodeRespGetKeyVersion},

    # Additional Frame
    0xAF: {"name": "AdditionalFrame ",  "CMDdecoder": decodeAdiFrame,       "RespDecoder": decodeRespAdiFrame},

}

# Commands need to use additional frame
MFDESFireAFCMD = {
    0xBD : {"name": "ReadData ",         "AFReaderDecoder": decodeDummy,         "AFCardDecoder": decodeData},
    0xAA : {"name": "AuthAES ",          "AFReaderDecoder":decodeAuthAESAF,     "AFCardDecoder": decodeCardAuthAESAF},
    0x0A : {"name": "Auth3DES ",         "AFReaderDecoder":decodeAuth3DESAF,    "AFCardDecoder": decodeCardAuth3DESAF},

    0x3D : {"name": "WriteData ",        "AFReaderDecoder":decodeData,          "AFCardDecoder": decodeDummy},
    0xBB : {"name": "ReadRecord ",       "AFReaderDecoder":decodeDummy,         "AFCardDecoder": decodeData},

    0x3B: {"name": "WriteRecord ",       "AFReaderDecoder": decodeData,         "AFCardDecoder": decodeDummy}

}

def MFDESFireDecode(data, source):
    note = ""
    global lastCMD
    if (source == TrafficSource.Reader):
        cmd = data[0]
        if cmd in MFDESFireCMDTypes :
            # If current frame is Additional Frame
            # And the previous cmd need Additional Frame
            if lastCMD in MFDESFireAFCMD and cmd == 0xAF:
                note += "CMD:AdditionalFrame "
                note += MFDESFireAFCMD[lastCMD]["AFReaderDecoder"](data)
            # Current command not need Additional Frame
            # or last cmd doesn't need AF
            else:
                lastCMD = cmd
                note += "CMD:" + MFDESFireCMDTypes[cmd]["name"]
                note += MFDESFireCMDTypes[cmd]["CMDdecoder"](data)

    elif (source == TrafficSource.Card):
        status = data[0]
        # Decode status code
        if status in StatusCode:
            note += StatusCode[status] + " "
        # If status Ok, decode data
        if (status == 0x00 or status == 0xAF) and lastCMD in MFDESFireCMDTypes:
            # If last cmd need additional frame and this is the last frame
            if status == 0x00 and lastCMD in MFDESFireAFCMD:
                note += MFDESFireAFCMD[lastCMD]["AFCardDecoder"](data)
            else:
                note += MFDESFireCMDTypes[lastCMD]["RespDecoder"](data)

    return note
