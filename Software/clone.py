#!/usr/bin/env python3
#
# A quick and simple card cloner for use with the ChameleonMini
# Currently only copies UID of cards
#
# Author: J-Xander (j.alexander@tpidg.us)
# Based on code from Simon K. (simon.kueppers@rub.de)

import argparse
import Chameleon
import sys
import datetime

def verboseLog(text):
    formatString = "[{}] {}"
    timeString = datetime.datetime.utcnow()
    print(formatString.format(timeString, text), sys.stderr)

def cmdSetting(chameleon, arg):
    if(arg is None):
        result = chameleon.cmdSetting(2)

        if (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "Setting has been changed to {}".format(chameleon.cmdSetting()['response'])
        else:
            return "Change setting to {} failed: {}".format(arg, result['statusText'])
    else:
        result = chameleon.cmdSetting(arg)

        if (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "Setting has been changed to {}".format(chameleon.cmdSetting()['response'])
        else:
            return "Change setting to {} failed: {}".format(arg, result['statusText'])

def cmdUID(chameleon, arg):
    result = chameleon.cmdUID(arg)

    if (arg is None):
        return "{}".format(result['response'])
    else:
        if (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "UID has been changed to {}".format(chameleon.cmdUID()['response'])
        else:
            return 0

def cmdGetUID(chameleon, arg):
    return "{}".format(chameleon.cmdGetUID()['response'])

def cmdConfig(chameleon, arg):
    result = chameleon.cmdConfig(arg)

    if (arg is None):
        return result['response']
    else:
        if (arg == chameleon.SUGGEST_CHAR):
            return "Possible configurations: {}".format(", ".join(result['suggestions']))
        elif (result['statusCode'] in chameleon.STATUS_CODES_SUCCESS):
            return "Configuration has been changed to {}".format(chameleon.cmdConfig()['response'])
        else:
            return "Changing configuration to {} failed: {}".format(arg, result['statusText'])


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
    argParser.add_argument("-s",  "--setting",     dest="setting",      nargs='?', type=int, choices=Chameleon.VALID_SETTINGS, help="slot to clone to")
    args = argParser.parse_args()

    if (args.verbose):
        verboseFunc = verboseLog
    else:
        verboseFunc = None

    # Instantiate device object and connect
    chameleon = Chameleon.Device(verboseFunc)

    if (args.port):
        if (chameleon.connect(args.port)):

            ##commands here
            currentConfig = cmdConfig(chameleon, None)
            print("[{}] {}".format("Current Config:", currentConfig))

            if currentConfig == "ISO14443A_READER":
                print("config: OK")

                cardUID = cmdGetUID(chameleon, None)
                print("[{}] {}".format("UID of card in range:", cardUID))

                currentSetting = cmdSetting(chameleon, args.setting)
                print("[{}] {}".format("Changing to slot:", currentSetting))

                currentUID = cmdUID(chameleon, cardUID)

                if currentUID == 0:
                    print("UID not compatible")
                    print("cloning: failed")
                else:
                    print("[{}] {}".format("UID of chameleon changed to:", currentUID))
                    print("cloning: successful")
            else:
                print("Chameleon not in reader mode. Aborting...")

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
