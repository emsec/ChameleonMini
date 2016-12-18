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
import time
import struct
import binascii

def verboseLog(text):
    formatString = "[{}] {}"
    timeString = datetime.datetime.utcnow()
    print(formatString.format(timeString, text), file=sys.stderr)
	
def formatText(log, out):
    formatString  = '{timestamp:0>5d} ms <{deltaTimestamp:>+6d} ms>:'
    formatString += '{eventName:<16} ({dataLength:<3} bytes) [{data}]\n'
    text = io.TextIOWrapper(out)

    for logEntry in log:
        text.write(formatString.format(**logEntry))

    text.close()

def formatJSON(log, out):
    text = io.TextIOWrapper(out)

    text.write(json.dumps(log, sort_keys=True, indent=4))
    
    text.close()

def formatPCAP(log, out):
    headerFormat = "IHHiIII"
    recordFormat = "IIII"
    pseudoHeaderFormat = "!BBH"
    eventTags = { 'CODEC RX': 0xFF, 'CODEC TX': 0xFE }

    tsSec = int(time.time())
    tsmSec = 0

    # Header for Version 2.4 PCAP file with a Snap-Legnth of 65535 and Data Link Type (DLT_ISO_14443)
    out.write(struct.pack(headerFormat, 0xa1b2c3d4, 2, 4, 0, 0, 65535, 264))

    for logEntry in log:
        eventName = logEntry['eventName']
        if eventName in eventTags:

            # The length is the length of the captured packet + the size of the pseudo header
            recordLength = logEntry['dataLength'] + 4

            tsmSec += logEntry['deltaTimestamp']
            tsSec += int(tsmSec / 1000)
            tsmSec = tsmSec % 1000
            tsuSec = tsmSec * 1000
            out.write(struct.pack(recordFormat, tsSec, tsuSec, recordLength, recordLength))

            # Write a pseudo header infront of the data. Refer to http://www.kaiser.cx/pcap-iso14443.html for details.
            out.write(struct.pack(pseudoHeaderFormat, 0, eventTags[eventName], logEntry['dataLength']))
            out.write(binascii.unhexlify(logEntry['data']))

    out.close()

def main():
    outputTypes = {
        'text': formatText,
        'json': formatJSON,
        'pcap': formatPCAP
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
    argParser.add_argument("-o", "--output", dest="outdst", metavar="OUTFILE", help="Write the output to the given file. If not specified, stdout is used.")
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

        # Open output destination (stdout or specified file)
        # The destination is opened in binary mode to allow output
        # of binary and text data.
        if (args.outdst is None):
            outbuffer = sys.stdout.buffer
        else:
            outbuffer = open(args.outdst, "wb")

        # Print to console using chosen output type
        outputTypes[args.type](log, outbuffer)

        outbuffer.close()


if __name__ == "__main__":
    main()
