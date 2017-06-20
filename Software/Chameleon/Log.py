#!/usr/bin/python

import struct
import binascii

def noDecoder(data):
    return ""

def textDecoder(data):
    return data.decode('ascii')

def binaryDecoder(data):
    return binascii.hexlify(data).decode()

eventTypes = {
    0x00: { 'name': 'EMPTY',          'decoder': noDecoder },
    0x10: { 'name': 'GENERIC',        'decoder': textDecoder },
    0x11: { 'name': 'CONFIG SET',     'decoder': textDecoder },
    0x12: { 'name': 'SETTING SET',    'decoder': textDecoder },
    0x13: { 'name': 'UID SET',        'decoder': binaryDecoder },
    0x20: { 'name': 'RESET APP',      'decoder': noDecoder },

    0x40: { 'name': 'CODEC RX',       'decoder': binaryDecoder },
    0x41: { 'name': 'CODEC TX',       'decoder': binaryDecoder },
    0x42: { 'name': 'CODEC RX W/PARITY', 'decoder': binaryDecoder },
    0x43: { 'name': 'CODEC TX W/PARITY', 'decoder': binaryDecoder },

    0x80: { 'name': 'APP READ',       'decoder': binaryDecoder },
    0x81: { 'name': 'APP WRITE',      'decoder': binaryDecoder },
    0x84: { 'name': 'APP INC',        'decoder': binaryDecoder },
    0x85: { 'name': 'APP DEC',        'decoder': binaryDecoder },
    0x86: { 'name': 'APP TRANSFER',   'decoder': binaryDecoder },
    0x87: { 'name': 'APP RESTORE',    'decoder': binaryDecoder },
    
    0x90: { 'name': 'APP AUTH',       'decoder': binaryDecoder },
    0x91: { 'name': 'APP HALT',       'decoder': binaryDecoder },
    0x92: { 'name': 'APP UNKNOWN',    'decoder': binaryDecoder },
    
    0xA0: { 'name': 'APP AUTHING' ,    'decoder': binaryDecoder },
    0xA1: { 'name': 'APP AUTHED',      'decoder': binaryDecoder },

    0xC0: { 'name': 'APP AUTH FAILED', 'decoder': binaryDecoder },
    0xC1: { 'name': 'APP CSUM FAILED', 'decoder': binaryDecoder },
    0xC2: { 'name': 'APP NOT AUTHED',  'decoder': binaryDecoder },
    
    0xFF: { 'name': 'BOOT',            'decoder': binaryDecoder },
}

TIMESTAMP_MAX = 65536
eventTypes = { i : ({'name': 'UNKNOWN', 'decoder': binaryDecoder} if i not in eventTypes.keys() else eventTypes[i]) for i in range(256) }

def parseBinary(binaryStream):
    log = []
    
    # Completely read file contents and process them byte by byte
    # logFile = fileHandle.read()
    # fileIdx = 0
    lastTimestamp = 0
    
    while True:
        # Read log entry header from file
        header = binaryStream.read(struct.calcsize('<BBH'))

        if (header is None):
            # No more data available
            break

        if (len(header) < struct.calcsize('<BBH')):
            # No more data available
            break
        
        (event, dataLength, timestamp) = struct.unpack_from('>BBH', header)
    
        # Break if there are no more events
        if (eventTypes[event]['name'] == 'EMPTY'):
            break

        # Read data from file
        logData = binaryStream.read(dataLength)

        # Decode data
        logData = eventTypes[event]['decoder'](logData)
        
        # Calculate delta timestamp respecting 16 bit overflow
        deltaTimestamp = timestamp - lastTimestamp;
        lastTimestamp = timestamp
        
        if (deltaTimestamp < 0):
            deltaTimestamp += TIMESTAMP_MAX;

        # Create log entry as dict and append it to event list
        logEntry = {
            'eventName': eventTypes[event]['name'],
            'dataLength': dataLength,
            'timestamp': timestamp,
            'deltaTimestamp': deltaTimestamp,
            'data': logData
        }
        
        log.append(logEntry)

    return log


        
