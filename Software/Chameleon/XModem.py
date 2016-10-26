#!/usr/bin/python
#
# Very lightweight implementation of XModem for Chameleon purposes
# Because the Chameleon uses a CDC over USB, we don't expect any
# retransmissions at all and thus don't implement it

import io
import time

class XModem:
    BYTE_SOH = b'\x01'
    BYTE_NAK = b'\x15'
    BYTE_ACK = b'\x06'
    BYTE_EOT = b'\x04'
    
    def __init__(self, ioStream, verboseFunc = None):
        self.ioStream = ioStream
        self.verboseFunc = verboseFunc

    def verboseLog(self, text):
        if (self.verboseFunc):
            self.verboseFunc(text)
            
    def recvData(self, dataStream):
        packetCounter = 1
        bytesReceived = 0
        startTime = time.time()
        
        self.verboseLog("Starting XMODEM Reception")
            
        # Start transmission by issuing a NAK
        self.ioStream.write(self.BYTE_NAK)
        
        while True:
            pktId = self.ioStream.read(1)

            if (pktId == self.BYTE_SOH):
                currentPacket = self.ioStream.read(2)

                if (currentPacket[0] == (255 - currentPacket[1])):
                    #frame number intact
                    if (currentPacket[0] == packetCounter):
                        #In order packet
                        dataBlock = self.ioStream.read(128)
                        checksum = self.ioStream.read(1)
                        
                        if (int(checksum[0]) == (sum(dataBlock) % 256)):
                            # checksum correct
                            dataStream.write(dataBlock)
                            dataStream.flush()
                            packetCounter = (packetCounter + 1) % 256
                            bytesReceived += 128
                            self.ioStream.write(self.BYTE_ACK)
                        else:
                            self.ioStream.write(self.BYTE_NAK)
            elif (pktId == self.BYTE_EOT):
                # Transmission done
                self.ioStream.write(self.BYTE_ACK)
                break
            else:
                # Unknown pktId
                break

        deltaTime = time.time() - startTime
        self.verboseLog("{} Bytes received in {:.2f} sec. ({:.0f} B/s)".format(bytesReceived, deltaTime, bytesReceived/deltaTime))
                
        return bytesReceived
       
    def sendData(self, dataStream):
        packetCounter = 1
        bytesSent = 0
        startTime = time.time()
        
        self.verboseLog("Waiting for XMODEM Connection")
        
        # Wait for NAK from receiver to start transmission
        if (self.ioStream.read(1) != self.BYTE_NAK):
            # Timeout or different char received
            return None

        lastBlock = False
        while True:
            dataBlock = dataStream.read(128)

            if (len(dataBlock) < 128):
                lastBlock = True
            if (0 < len(dataBlock) < 128):
                # Last part smaller than xmodem block -> pad it
                dataBlock += b'\x00' * (128 - len(dataBlock))
            if (len(dataBlock) == 128):
                # Write SOH, pktId, data and checksum
                self.ioStream.write(self.BYTE_SOH)
                self.ioStream.write(bytes([packetCounter]))
                self.ioStream.write(bytes([255 - packetCounter]))
                self.ioStream.write(dataBlock)
                self.ioStream.write(bytes([sum(dataBlock) % 256]))

                if (self.ioStream.read(1) == self.BYTE_ACK):
                    #Proceed to next packet
                    packetCounter = (packetCounter + 1) % 256
                    bytesSent += 128
            if lastBlock:
                # Write EOT and wait for ACK
                self.ioStream.write(self.BYTE_EOT)
                self.ioStream.read(1)
                break

        deltaTime = time.time() - startTime
        self.verboseLog("{} Bytes sent in {:.2f} sec. ({:.0f} B/s)".format(bytesSent, deltaTime, bytesSent/deltaTime))
        
        return bytesSent
            

