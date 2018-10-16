#!/usr/bin/python3

import serial
import serial.tools.list_ports
import sys
import datetime
import time
import Chameleon

class Device:
    COMMAND_VERSION = "VERSION"
    COMMAND_UPLOAD = "UPLOAD"
    COMMAND_DOWNLOAD = "DOWNLOAD"
    COMMAND_SETTING = "SETTING"
    COMMAND_UID = "UID"
    COMMAND_GETUID = "GETUID"
    COMMAND_IDENTIFY = "IDENTIFY"
    COMMAND_DUMPMFU = "DUMP_MFU"
    COMMAND_CONFIG = "CONFIG"
    COMMAND_LOG_DOWNLOAD = "LOGDOWNLOAD"
    COMMAND_LOG_CLEAR = "LOGCLEAR"
    COMMAND_LOGMODE = "LOGMODE"
    COMMAND_LBUTTON = "LBUTTON"
    COMMAND_LBUTTONLONG = "LBUTTON_LONG"
    COMMAND_RBUTTON = "RBUTTON"
    COMMAND_RBUTTONLONG = "RBUTTON_LONG"
    COMMAND_GREEN_LED = "LEDGREEN"
    COMMAND_RED_LED = "LEDRED"
    COMMAND_THRESHOLD = "THRESHOLD"
    COMMAND_UPGRADE = "upgrade"

    STATUS_CODE_OK = 100
    STATUS_CODE_OK_WITH_TEXT = 101
    STATUS_CODE_WAITING_FOR_XMODEM = 110
    STATUS_CODE_FALSE = 120
    STATUS_CODE_TRUE = 121
    STATUS_CODE_UNKNOWN_COMMAND = 200
    STATUS_CODE_UNKNOWN_COMMAND_USAGE = 201
    STATUS_CODE_INVALID_PARAMETER = 202

    STATUS_CODES_SUCCESS = [
        STATUS_CODE_OK,
        STATUS_CODE_OK_WITH_TEXT,
        STATUS_CODE_WAITING_FOR_XMODEM,
        STATUS_CODE_FALSE,
        STATUS_CODE_TRUE
    ]

    STATUS_CODES_FAILURE = [
        STATUS_CODE_UNKNOWN_COMMAND,
        STATUS_CODE_UNKNOWN_COMMAND_USAGE,
        STATUS_CODE_INVALID_PARAMETER
    ]

    LINE_ENDING = "\r"
    SUGGEST_CHAR = "?"
    SET_CHAR = "="
    GET_CHAR = "?"

    def __init__(self, verboseFunc = None):
        self.verboseFunc = verboseFunc
        self.serial = serial.Serial(None, 9600, timeout=5.0)
        self.versionString = ""
        self.supportedConfs = []

    def verboseLog(self, text):
        if (self.verboseFunc):
            self.verboseFunc(text)

    def listDevices():
        devices = []

        for port in serial.tools.list_ports.grep("({0:04x}:{1:04x})|({0:04X}:{1:04X})".format(Chameleon.USB_VID, Chameleon.USB_PID)):
            devices.append(port[0])

        return devices

    def connect(self, comport):
        self.serial.port = comport
        try:
            self.serial.open()
        except:
            pass

        if (self.serial.isOpen()):
            # Send escape key to force clearing the Chameleon's input buffer
            self.serial.write(b"\x1B")
            self.verboseLog("Opening serial port {} succeeded".format(comport))
        else:
            self.verboseLog("Opening serial port {} failed".format(comport))
            return False

        # Try to retrieve chameleons version information and supported confs
        result = self.getSetCmd(self.COMMAND_VERSION)

        if (result is not None):
            if (result['statusCode'] == self.STATUS_CODE_OK_WITH_TEXT):
                self.versionString = result['response']
            else:
                return False

            result = self.getCmdSuggestions(self.COMMAND_CONFIG)

            if (result['statusCode'] == self.STATUS_CODE_OK_WITH_TEXT):
                self.supportedConfs = result['response'].split(",")
            else:
                return False
        else:
            return False

        return True

    def disconnect(self):
        self.verboseLog("Closing serial port")
        self.serial.close()

    def isConnected(self):
        return self.serial.isOpen()

    def read(self, size=1024, timeout=0.01):
        self.serial.timeout = timeout
        data = self.serial.read(size)
        self.serial.timeout = 5.0
        return data

    def writeCmd(self, cmd):
        # Execute command
        cmdLine = cmd + self.LINE_ENDING
        self.serial.write(cmdLine.encode('ascii'))

        # Get status response
        status = self.serial.readline().decode('ascii').rstrip()

        if (len(status) == 0):
            self.verboseLog("Executing <{}>: Timeout".format(cmd))
            return None
        else:
            self.verboseLog("Executing <{}>: {}".format(cmd, status))

        statusCode, statusText = status.split(":")
        statusCode = int(statusCode)

        result = {'statusCode': statusCode, 'statusText': statusText, 'response': None}

        if (statusCode == self.STATUS_CODE_OK_WITH_TEXT):
            result['response'] =  self.readResponse()
        elif (statusCode == self.STATUS_CODE_TRUE):
            result['response'] = True
        elif (statusCode == self.STATUS_CODE_FALSE):
            result['response'] = False

        return result

    def readResponse(self):
        # Read response to command, if any
        response = self.serial.readline().decode('ascii').rstrip()
        self.verboseLog("Response: {}".format(response))
        return response

    def execCmd(self, cmd, args=None):
        if (args is None):
            return self.writeCmd("{}".format(cmd))
        else:
            return self.writeCmd("{} {}".format(cmd, args))

    def getSetCmd(self, cmd, arg=None):
        # Determine if set or get mode
        if (arg is None):
            return self.writeCmd("{}{}".format(cmd, self.GET_CHAR))
        else:
            return self.writeCmd("{}{}{}".format(cmd, self.SET_CHAR, arg))

    def returnCmd(self, cmd, arg=None):
        return self.writeCmd("{}".format(cmd))

    def getCmdSuggestions(self, cmd):
        result = self.getSetCmd(cmd, self.SUGGEST_CHAR)
        if (result['response'] is not None):
            result['suggestions'] = result['response'].split(",")

        return result

    def cmdUploadDump(self, dataStream):
        if (self.execCmd(self.COMMAND_UPLOAD)['statusCode'] == self.STATUS_CODE_WAITING_FOR_XMODEM):
            # XMODEM started
            xmodem = Chameleon.XModem(self.serial, self.verboseFunc)
            bytesSent = xmodem.sendData(dataStream)
            return bytesSent
        else:
            return None

    def cmdDownloadDump(self, dataStream):
        if (self.execCmd(self.COMMAND_DOWNLOAD)['statusCode'] == self.STATUS_CODE_WAITING_FOR_XMODEM):
            # XMODEM started
            xmodem = Chameleon.XModem(self.serial, self.verboseFunc)
            return xmodem.recvData(dataStream)
        else:
            return None

    def cmdDownloadLog(self, dataStream):
        if (self.execCmd(self.COMMAND_LOG_DOWNLOAD)['statusCode'] == self.STATUS_CODE_WAITING_FOR_XMODEM):
            # XMODEM started
            xmodem = Chameleon.XModem(self.serial, self.verboseFunc)
            return xmodem.recvData(dataStream)
        else:
            return None

    def cmdClearLog(self):
        return self.execCmd(self.COMMAND_LOG_CLEAR)

    def cmdLogMode(self, newLogMode):
        return self.getSetCmd(self.COMMAND_LOGMODE, newLogMode)

    def cmdVersion(self):
        return self.getSetCmd(self.COMMAND_VERSION)

    def cmdSetting(self, newSetting = None):
        return self.getSetCmd(self.COMMAND_SETTING, newSetting)

    def cmdUID(self, newUID = None):
        return self.getSetCmd(self.COMMAND_UID, newUID)

    def cmdGetUID(self):
        return self.returnCmd(self.COMMAND_GETUID)

    def cmdIdentify(self):
        return self.returnCmd(self.COMMAND_IDENTIFY)

    def cmdDumpMFU(self):
        return self.returnCmd(self.COMMAND_DUMPMFU)

    def cmdConfig(self, newConfig = None):
        if (newConfig == self.SUGGEST_CHAR):
            return self.getCmdSuggestions(self.COMMAND_CONFIG)
        else:
            return self.getSetCmd(self.COMMAND_CONFIG, newConfig)

    def cmdLButton(self, newAction = None):
        if (newAction == self.SUGGEST_CHAR):
            return self.getCmdSuggestions(self.COMMAND_LBUTTON)
        else:
            return self.getSetCmd(self.COMMAND_LBUTTON, newAction)

    def cmdLButtonLong(self, newAction = None):
        if (newAction == self.SUGGEST_CHAR):
            return self.getCmdSuggestions(self.COMMAND_LBUTTONLONG)
        else:
            return self.getSetCmd(self.COMMAND_LBUTTONLONG, newAction)

    def cmdRButton(self, newAction = None):
        if (newAction == self.SUGGEST_CHAR):
            return self.getCmdSuggestions(self.COMMAND_RBUTTON)
        else:
            return self.getSetCmd(self.COMMAND_RBUTTON, newAction)

    def cmdRButtonLong(self, newAction = None):
        if (newAction == self.SUGGEST_CHAR):
            return self.getCmdSuggestions(self.COMMAND_RBUTTONLONG)
        else:
            return self.getSetCmd(self.COMMAND_RBUTTONLONG, newAction)

    def cmdGreenLED(self, newFunction = None):
        if (newFunction == self.SUGGEST_CHAR):
            return self.getCmdSuggestions(self.COMMAND_GREEN_LED)
        else:
            return self.getSetCmd(self.COMMAND_GREEN_LED, newFunction)

    def cmdRedLED(self, newFunction = None):
        if (newFunction == self.SUGGEST_CHAR):
            return self.getCmdSuggestions(self.COMMAND_RED_LED)
        else:
            return self.getSetCmd(self.COMMAND_RED_LED, newFunction)

    def cmdThreshold(self, value):
        if(value == self.SUGGEST_CHAR):
            return self.getCmdSuggestions(self.COMMAND_THRESHOLD)
        else:
            return self.getSetCmd(self.COMMAND_THRESHOLD, value)

    def cmdUpgrade(self):
        # Execute command
        cmdLine = self.COMMAND_UPGRADE + self.LINE_ENDING
        self.serial.write(cmdLine.encode('ascii'))
        return 0
