#!/usr/bin/env python3
#
# Command line tool to analyze binary dump files from the Chameleon
# Authors: Simon K. (simon.kueppers@rub.de)

from __future__ import print_function

import argparse
import sys
import json
import Chameleon
import io
import datetime

def verboseLog(text):
    formatString = "[{}] {}"
    timeString = datetime.datetime.utcnow()
    print(formatString.format(timeString, text), file=sys.stderr)
	
def formatText(log):
    formatString  = '{timestamp:0>5d} ms <{deltaTimestamp:>+6d} ms>:'
    formatString += '{eventName:<16} ({dataLength:<3} bytes) [{data}]\n'
    text = ''
    
    for logEntry in log:
        text += formatString.format(**logEntry)

    return text

def formatJSON(log):
    text = json.dumps(log, sort_keys=True, indent=4)
    
    return text

def main():
    outputTypes = {
        'text': formatText,
        'json': formatJSON
    }

    argParser = argparse.ArgumentParser(description="Analyzes binary Chameleon logfiles")

    group = argParser.add_mutually_exclusive_group(required=True)
    group.add_argument("-f", "--file", dest="logfile", metavar="LOGFILE")
    group.add_argument("-p", "--port", dest="port", metavar="COMPORT")
    
    argParser.add_argument("-t", "--type", choices=outputTypes.keys(), default='text',
                            help="specifies output type")
    argParser.add_argument("-l", "--live", dest="live", action='store_true', help="Use live logging capabilities of Chameleon")
    argParser.add_argument("-c", "--clear", dest="clear", action='store_true', help="Clear Chameleon's log memory when using -p")
    argParser.add_argument("-m", "--mode", dest="mode", metavar="LOGMODE", help="Additionally set Chameleon's log mode after reading it's memory")
    argParser.add_argument("-v", "--verbose", dest="verbose", action='store_true', default=0)

    args = argParser.parse_args()
	
    if (args.verbose):
        verboseFunc = verboseLog
    else:
        verboseFunc = None

    if (args.live):
        # Live logging mode
        if (args.port is not None):
            chameleon = Chameleon.Device(verboseFunc)

            if (chameleon.connect(args.port)):
                chameleon.cmdLogMode("LIVE")

                while True:
                    stream = io.BytesIO(chameleon.read())
                    log = Chameleon.Log.parseBinary(stream)
                    if (len(log) > 0):
                        print(outputTypes[args.type](log))
      
    else:
        if (args.logfile is not None):
            handle = open(args.logfile, "rb")
        elif (args.port is not None):
            chameleon = Chameleon.Device(verboseFunc)

            if (chameleon.connect(args.port)):
                handle = io.BytesIO()
                chameleon.cmdDownloadLog(handle)
                handle.seek(0)
                
                if (args.clear):
                    chameleon.cmdClearLog()

                if (args.mode is not None):
                    chameleon.cmdLogMode(args.mode)
                        
                chameleon.disconnect()
            else:
                sys.exit(2)
                
        # Parse actual logfile
        log = Chameleon.Log.parseBinary(handle)

        # Print to console using chosen output type
        print(outputTypes[args.type](log))


if __name__ == "__main__":
    main()
