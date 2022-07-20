#!/usr/bin/python

import struct
import binascii
import math
import Chameleon.ISO14443 as iso14443_3

def checkParityBit(data):
    byteCount = len(data)
    # Short frame, no parityBit
    if (byteCount == 1):
        return (True, data)

    # 9 bit is a group, validate bit count is calculated below
    bitCount = int((byteCount*8)/9) * 9
    parsedData = bytearray(int(bitCount/9))

    oneCounter = 0            # Counter for count ones in a byte
    for i in range(0, bitCount):
        # Get bit i in data
        byteIndex = math.floor(i/8)
        bitIndex = i % 8
        bit = (data[byteIndex] >> bitIndex) & 0x01

        # Check parityBit
        # Current bit is parityBit
        if(i % 9 == 8):
            # Even number of ones in current byte
            if(oneCounter % 2 and bit == 1):
                return (False, data)
            # Odd number of ones in current byte
            elif((not oneCounter % 2) and bit == 0):
                return (False, data)
            oneCounter = 0
        # Current bit is normal bit
        else:
            oneCounter += bit
            parsedData[int(i/9)] |= bit << (i%9)
    return (True, parsedData)

def noDecoder(data):
    return ""

def textDecoder(data):
    return data.decode('ascii')

def binaryDecoder(data):
    return binascii.hexlify(data).decode()

def binaryParityDecoder(data):
    isValid, checkedData = checkParityBit(data)
    if(isValid):
        return binascii.hexlify(checkedData).decode()
    else:
        return binascii.hexlify(checkedData).decode()+"!"

eventTypes = {
    0x00: { 'name': 'EMPTY',          'decoder': noDecoder },
    0x10: { 'name': 'GENERIC',        'decoder': textDecoder },
    0x11: { 'name': 'CONFIG SET',     'decoder': textDecoder },
    0x12: { 'name': 'SETTING SET',    'decoder': textDecoder },
    0x13: { 'name': 'UID SET',        'decoder': binaryDecoder },
    0x20: { 'name': 'RESET APP',      'decoder': noDecoder },

    0x40: { 'name': 'CODEC RX',          'decoder': binaryDecoder },
    0x41: { 'name': 'CODEC TX',          'decoder': binaryDecoder },
    0x42: { 'name': 'CODEC RX W/PARITY', 'decoder': binaryDecoder },
    0x43: { 'name': 'CODEC TX W/PARITY', 'decoder': binaryDecoder },

    0x44: { 'name': 'CODEC RX SNI READER',                  'decoder': binaryDecoder },
    0x45: { 'name': 'CODEC RX SNI READER W/PARITY',         'decoder': binaryParityDecoder },
    0x46: { 'name': 'CODEC RX SNI CARD',                    'decoder': binaryDecoder },
    0x47: { 'name': 'CODEC RX SNI CARD W/PARITY',           'decoder': binaryParityDecoder },
    0x48: { 'name': 'CODEC RX SNI READER FIELD DETECTED',   'decoder': noDecoder },
   
    0x53: { 'name': 'ISO14443A (DESFIRE) STATE',       'decoder': binaryDecoder },
    0x54: { 'name': 'ISO144443-4 (DESFIRE) STATE',     'decoder': binaryDecoder },
    0x55: { 'name': 'ISO14443A (DESFIRE) APP NO RESP', 'decoder': binaryDecoder },

    0x80: { 'name': 'APP READ',       'decoder': binaryDecoder },
    0x81: { 'name': 'APP WRITE',      'decoder': binaryDecoder },
    0x84: { 'name': 'APP INC',        'decoder': binaryDecoder },
    0x85: { 'name': 'APP DEC',        'decoder': binaryDecoder },
    0x86: { 'name': 'APP TRANSFER',   'decoder': binaryDecoder },
    0x87: { 'name': 'APP RESTORE',    'decoder': binaryDecoder },
    
    0x90: { 'name': 'APP AUTH',       'decoder': binaryDecoder },
    0x91: { 'name': 'APP HALT',       'decoder': binaryDecoder },
    0x92: { 'name': 'APP UNKNOWN',    'decoder': binaryDecoder },
    0x93: { 'name': 'APP REQA',       'decoder': binaryDecoder },
    0x94: { 'name': 'APP WUPA',       'decoder': binaryDecoder },
    0x95: { 'name': 'APP DESELECT',   'decoder': binaryDecoder },

    0xA0: { 'name': 'APP AUTHING' ,    'decoder': binaryDecoder },
    0xA1: { 'name': 'APP AUTHED',      'decoder': binaryDecoder },

    0xC0: { 'name': 'APP AUTH FAILED', 'decoder': binaryDecoder },
    0xC1: { 'name': 'APP CSUM FAILED', 'decoder': binaryDecoder },
    0xC2: { 'name': 'APP NOT AUTHED',  'decoder': binaryDecoder },
 
    0xD0: { 'name': 'APP DESFIRE AUTH KEY',  'decoder': binaryDecoder },
    0xD1: { 'name': 'APP DESFIRE NONCE B',   'decoder': binaryDecoder },
    0xD2: { 'name': 'APP DESFIRE NONCE AB',  'decoder': binaryDecoder },
    0xD3: { 'name': 'APP DESFIRE SESION IV', 'decoder': binaryDecoder },

    0xE0: { 'name': 'APP DESFIRE GENERIC ERROR',         'decoder': textDecoder },
    0xE1: { 'name': 'APP DESFIRE STATUS INFO',           'decoder': binaryDecoder },
    0xE2: { 'name': 'APP DESFIRE DEBUG OUTPUT',          'decoder': binaryDecoder },
    0xE3: { 'name': 'APP DESFIRE INCOMING',              'decoder': binaryDecoder },
    0xE4: { 'name': 'APP DESFIRE INCOMING ENC',          'decoder': binaryDecoder },
    0xE5: { 'name': 'APP DESFIRE OUTGOING',              'decoder': binaryDecoder },
    0xE6: { 'name': 'APP DESFIRE OUTGOING ENC',          'decoder': binaryDecoder },
    0xE7: { 'name': 'APP DESFIRE NATIVE CMD',            'decoder': binaryDecoder },
    0xE8: { 'name': 'APP DESFIRE ISO14443 CMD',          'decoder': binaryDecoder },
    0xE9: { 'name': 'APP DESFIRE ISO7816 CMD',           'decoder': binaryDecoder },
    0xEA: { 'name': 'APP DESFIRE PICC RESET',            'decoder': binaryDecoder },
    0xEB: { 'name': 'APP DESFIRE PICC RESET FROM MEM',   'decoder': binaryDecoder },
    0xEC: { 'name': 'APP DESFIRE PROT DATA SET',         'decoder': binaryDecoder },
    0xED: { 'name': 'APP DESFIRE PROT DATA SET VERBOSE', 'decoder': binaryDecoder },

    0xFF: { 'name': 'BOOT',            'decoder': binaryDecoder },
}

TIMESTAMP_MAX = 65536
eventTypes = { i : ({'name': f'UNKNOWN {hex(i)}', 'decoder': binaryDecoder} if i not in eventTypes.keys() else eventTypes[i]) for i in range(256) }

def parseBinary(binaryStream, decoder=None):
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
        deltaTimestamp = timestamp - lastTimestamp
        lastTimestamp = timestamp
        
        if (deltaTimestamp < 0):
            deltaTimestamp += TIMESTAMP_MAX

        note = ""
        # If we need to decode the data and paritybit check success
        if (decoder!=None and len(logData) >0 and logData[-1] != '!'):
            # Decode the data from Reader
            if(event == 0x44 or event == 0x45):
                note = iso14443_3.parseReader(binascii.a2b_hex(logData), decoder)
            elif (event == 0x46 or event == 0x47):
                note = iso14443_3.parseCard(binascii.a2b_hex(logData), decoder)

        # Create log entry as dict and append it to event list
        logEntry = {
            'eventName': eventTypes[event]['name'],
            'dataLength': dataLength,
            'timestamp': timestamp,
            'deltaTimestamp': deltaTimestamp,
            'data': logData,
            'note': note
        }
        
        log.append(logEntry)

    return log


        
