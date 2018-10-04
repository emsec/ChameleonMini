#!/usr/bin/env python3
#
# Command line tool to control the Chameleon through command line
# Authors: Simon K. (simon.kueppers@rub.de)

import argparse
import Chameleon
import sys
import datetime

def verboseLog(text):
    formatString = "[{}] {}"
    timeString = datetime.datetime.utcnow()
    print(formatString.format(timeString, text), sys.stderr)

# Command funcs
def cmdInfo(chameleon, arg):
    return "{}".format(chameleon.cmdVersion()['response'])

def cmdSetting(chameleon, arg):
    result = chameleon.cmdSetting(arg)

    if (arg is None):
        return "Current Setting: {}".format(result['response'])
    else:
        if (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "Setting has been changed to {}".format(chameleon.cmdSetting()['response'])
        else:
            return "Change setting to {} failed: {}".format(arg, result['statusText'])
    return

def cmdUID(chameleon, arg):
    result = chameleon.cmdUID(arg)

    if (arg is None):
        return "{}".format(result['response'])
    else:
        if (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "UID has been changed to {}".format(chameleon.cmdUID()['response'])
        else:
            return "Setting UID to {} failed: {}".format(arg, result['statusText'])

def cmdGetUID(chameleon, arg):
    return "{}".format(chameleon.cmdGetUID()['response'])

def cmdIdentify(chameleon, arg):
    return "{}".format(chameleon.cmdIdentify()['response'])

def cmdDumpMFU(chameleon, arg):
    return "{}".format(chameleon.cmdDumpMFU()['response'])

def cmdConfig(chameleon, arg):
    result = chameleon.cmdConfig(arg)

    if (arg is None):
        return "Current configuration: {}".format(result['response'])
    else:
        if (arg == chameleon.SUGGEST_CHAR):
            return "Possible configurations: {}".format(", ".join(result['suggestions']))
        elif (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "Configuration has been changed to {}".format(chameleon.cmdConfig()['response'])
        else:
            return "Changing configuration to {} failed: {}".format(arg, result['statusText'])

def cmdUpload(chameleon, arg):
    with open(arg, 'rb') as fileHandle:
        bytesSent = chameleon.cmdUploadDump(fileHandle)
        return "{} Bytes successfully read from {}".format(bytesSent, arg)

def cmdDownload(chameleon, arg):
    with open(arg, 'wb') as fileHandle:
        bytesReceived = chameleon.cmdDownloadDump(fileHandle)
        return "{} Bytes successfully written to {}".format(bytesReceived, arg)

def cmdLog(chameleon, arg):
    with open(arg, 'wb') as fileHandle:
        bytesReceived = chameleon.cmdDownloadLog(fileHandle)
        return "{} Bytes successfully written to {}".format(bytesReceived, arg)

def cmdLogMode(chameleon, arg):
    result = chameleon.cmdLogMode(arg)

    if (arg is None):
        return "Current logmode is: {}".format(result['response'])
    else:
        if (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "logmode have been set to {}".format(arg)
        else:
            return "Setting logmode failed: {}".format(arg, result['statusText'])

def cmdLButton(chameleon, arg):
    result = chameleon.cmdLButton(arg)

    if (arg is None):
        return "Current left button action: {}".format(result['response'])
    else:
        if (arg == chameleon.SUGGEST_CHAR):
            return "Possible left button actions: {}".format(", ".join(result['suggestions']))
        elif (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "Left button action has been set to {}".format(chameleon.cmdLButton()['response'])
        else:
            return "Setting left button action to {} failed: {}".format(arg, result['statusText'])

def cmdLButtonLong(chameleon, arg):
    result = chameleon.cmdLButtonLong(arg)

    if (arg is None):
        return "Current long press left button action: {}".format(result['response'])
    else:
        if (arg == chameleon.SUGGEST_CHAR):
            return "Possible long press left button actions: {}".format(", ".join(result['suggestions']))
        elif (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "Long press left button action has been set to {}".format(chameleon.cmdLButtonLong()['response'])
        else:
            return "Setting long press left button action to {} failed: {}".format(arg, result['statusText'])

def cmdRButton(chameleon, arg):
    result = chameleon.cmdRButton(arg)

    if (arg is None):
        return "Current right button action: {}".format(result['response'])
    else:
        if (arg == chameleon.SUGGEST_CHAR):
            return "Possible right button actions: {}".format(", ".join(result['suggestions']))
        elif (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "Right button action has been set to {}".format(chameleon.cmdRButton()['response'])
        else:
            return "Setting right button action to {} failed: {}".format(arg, result['statusText'])

def cmdRButtonLong(chameleon, arg):
    result = chameleon.cmdRButtonLong(arg)

    if (arg is None):
        return "Current long press right button action: {}".format(result['response'])
    else:
        if (arg == chameleon.SUGGEST_CHAR):
            return "Possible long press right button actions: {}".format(", ".join(result['suggestions']))
        elif (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "Long press right button action has been set to {}".format(chameleon.cmdRButtonLong()['response'])
        else:
            return "Setting long press right button action to {} failed: {}".format(arg, result['statusText'])

def cmdGreenLED(chameleon, arg):
    result = chameleon.cmdGreenLED(arg)

    if (arg is None):
        return "Current green LED function: {}".format(result['response'])
    else:
        if (arg == chameleon.SUGGEST_CHAR):
            return "Possible green LED functions: {}".format(", ".join(result['suggestions']))
        elif (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "Green LED function has been set to {}".format(chameleon.cmdGreenLED()['response'])
        else:
            return "Setting green LED function to {} failed: {}".format(arg, result['statusText'])

def cmdRedLED(chameleon, arg):
    result = chameleon.cmdRedLED(arg)

    if (arg is None):
        return "Current red LED function: {}".format(result['response'])
    else:
        if (arg == chameleon.SUGGEST_CHAR):
            return "Possible red LED functions: {}".format(", ".join(result['suggestions']))
        elif (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "Red LED function has been set to {}".format(chameleon.cmdRedLED()['response'])
        else:
            return "Setting red LED function to {} failed: {}".format(arg, result['statusText'])

def cmdThreshold(chameleon, arg):
    result = chameleon.cmdThreshold(arg)

    if (arg is None):
        return "Current threshold is: {}".format(result['response'])
    else:
        if (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "Threshold have been set to {}".format(arg)
        else:
            return "Setting threshold failed: {}".format(arg, result['statusText'])

def cmdUpgrade(chameleon, arg):
    if(chameleon.cmdUpgrade() == 0):
        print ("Device changed into Upgrade Mode")
    exit(0)

# Custom class for argparse
class CmdListAction(argparse.Action):
    def __init__(self, option_strings, dest, default=False, required=False,
                 help=None, metavar=None, nargs=None, type=None, choices=None, const=None):
        super(CmdListAction, self).__init__(
            option_strings=option_strings, dest=dest, nargs=nargs, const=const, default=default,
            required=required, help=help, metavar=metavar, type=type, choices=choices)

    def __call__(self, parser, namespace, values, option_string=None):
        # Create new attribute cmdList if not exist and append command to list
        if not hasattr(namespace, "cmdList"):
            setattr(namespace, "cmdList", [])

        namespace.cmdList.append([self.dest, values])

def main():
    argParser = argparse.ArgumentParser(description="Controls the Chameleon through the command line")
    argParser.add_argument("-v",    "--verbose",    dest="verbose",     action="store_true",    default=0,          help="output verbose")
    argParser.add_argument("-p",    "--port",       dest="port",        metavar="COMPORT",                          help="specify device's comport")

    # Add the commands using custom action that populates a list in the order the arguments are given
    cmdArgGroup = argParser.add_argument_group(title="Chameleon commands", description="These arguments can appear multiple times and are executed in the order they are given on the command line. "
                                                                                       "Some of these arguments can be used with '" + Chameleon.Device.SUGGEST_CHAR + "' as parameter to get a list of suggestions.")
    cmdArgGroup.add_argument("-u",  "--upload",      dest="upload",      action=CmdListAction, metavar="DUMPFILE",   help="upload a card dump")
    cmdArgGroup.add_argument("-d",  "--download",    dest="download",    action=CmdListAction, metavar="DUMPFILE",   help="download a card dump")
    cmdArgGroup.add_argument("-l",  "--log",         dest="log",         action=CmdListAction, metavar="LOGFILE",    help="download the device log")
    cmdArgGroup.add_argument("-i",  "--info",        dest="info",        action=CmdListAction, nargs=0,              help="retrieve the version information")
    cmdArgGroup.add_argument("-s",  "--setting",     dest="setting",     action=CmdListAction, nargs='?', type=int, choices=Chameleon.VALID_SETTINGS, help="retrieve or set the current setting")
    cmdArgGroup.add_argument("-U",  "--uid",         dest="uid",         action=CmdListAction, nargs='?',            help="retrieve or set the current UID")
    cmdArgGroup.add_argument("-gu",  "--getuid",         dest="getuid",         action=CmdListAction, nargs='?',            help="retrieve UID of device in range")
    cmdArgGroup.add_argument("-I",  "--identify",         dest="identify",         action=CmdListAction, nargs='?',            help="identify device in range")
    cmdArgGroup.add_argument("-D",  "--dumpmfu",    dest="dumpmfu",    action=CmdListAction, nargs='?',  help="dump information about card in range")
    cmdArgGroup.add_argument("-c",  "--config",      dest="config",      action=CmdListAction, metavar="CFGNAME", nargs='?', help="retrieve or set the current configuration")
    cmdArgGroup.add_argument("-lm",  "--logmode",    dest="logmode",     action=CmdListAction, metavar="LOGMODE", nargs='?', help="retrieve or set the current log mode")
    cmdArgGroup.add_argument("-lb",  "--lbutton",    dest="lbutton",     action=CmdListAction, metavar="ACTION", nargs='?', help="retrieve or set the current left button action")
    cmdArgGroup.add_argument("-lbl",  "--lbuttonlong",    dest="lbutton_long",     action=CmdListAction, metavar="ACTION", nargs='?', help="retrieve or set the current left button long press action")
    cmdArgGroup.add_argument("-rb",  "--rbutton",    dest="rbutton",     action=CmdListAction, metavar="ACTION", nargs='?', help="retrieve or set the current right button action")
    cmdArgGroup.add_argument("-rbl",  "--rbuttonlong",    dest="rbutton_long",     action=CmdListAction, metavar="ACTION", nargs='?', help="retrieve or set the current right button long press action")
    cmdArgGroup.add_argument("-gl",  "--gled",       dest="gled",        action=CmdListAction, metavar="FUNCTION", nargs='?', help="retrieve or set the current green led function")
    cmdArgGroup.add_argument("-rl",  "--rled",       dest="rled",        action=CmdListAction, metavar="FUNCTION", nargs='?', help="retrieve or set the current red led function")
    cmdArgGroup.add_argument("-th",  "--threshold",  dest="threshold",   action=CmdListAction, nargs='?', help="retrieve or set the threshold")
    cmdArgGroup.add_argument("-ug",  "--upgrade",    dest="upgrade",     action=CmdListAction, nargs=0,   help="set the micro Controller to upgrade mode")

    args = argParser.parse_args()

    if (args.verbose):
        verboseFunc = verboseLog
    else:
        verboseFunc = None

    # Instantiate device object and connect
    chameleon = Chameleon.Device(verboseFunc)

    if (args.port):
        if (chameleon.connect(args.port)):
            # Generate a jumptable and execute all commands in the order they are given on the command line
            cmdFuncs = {
                "setting"   : cmdSetting,
                "info"      : cmdInfo,
                "uid"       : cmdUID,
                "getuid"    : cmdGetUID,
                "identify"  : cmdIdentify,
                "dumpmfu"   : cmdDumpMFU,
                "config"    : cmdConfig,
                "upload"    : cmdUpload,
                "download"  : cmdDownload,
                "log"       : cmdLog,
                "logmode"   : cmdLogMode,
                "lbutton"   : cmdLButton,
		        "lbutton_long" : cmdLButtonLong,
		        "rbutton_long" : cmdRButtonLong,
                "rbutton"   : cmdRButton,
                "gled"      : cmdGreenLED,
                "rled"      : cmdRedLED,
                "threshold" : cmdThreshold,
                "upgrade"   : cmdUpgrade,
            }

            if hasattr(args, "cmdList"):
                for (cmd, arg) in args.cmdList:
                    result = cmdFuncs[cmd](chameleon, arg)
                    print("[{}] {}".format(cmd, result))

            # Goodbye
            chameleon.disconnect()
        else:
            print("Unable to establish communication on {}".format(args.port))
            sys.exit(2)
    else:
        #List possible Chameleon ports
        print("Use -p COMPORT to specify the communication port (see help).")
        print("List of potential Chameleons connected to the system:")
        for port in Chameleon.Device.listDevices():
            print(port)


    sys.exit(0)

if __name__ == "__main__":
    main()
