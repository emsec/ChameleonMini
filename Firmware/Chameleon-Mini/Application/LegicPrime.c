/*
 * LegicPrime.c
 *
 *  Created on: 5.7.2024
 *      Author: Ladislav Marko
 *  Inspired by MifareClassic.c
 */
#if defined(CONFIG_LEGIC_PRIME_SUPPORT)

#include "LegicPrime.h"

char legic_log_str[64];

#include "../Codec/ISO14443-2F.h"
#include "../Memory.h"

#define MEM_UID_ADDRESS         0x00
#define MEM_UID_CRC_ADDRESS     0x04
#define MEM_DCF_LOW_ADDRESS     0x05
#define MEM_DCF_HIGH_ADDRESS    0x06
#define MEM_BACKUP_ADDRESS      0x0D
#define MEM_BACKUP_CRC_ADDRESS  0x13

#define MEM_REPLAY_ADDRESS      0x1000

/* LEGIC prime card layout
 * UID [4 Bytes]
 * UID CRC [1 Byte]
 * Decremental filed  low byte (DCF) [1 byte]
 * Decremental filed  high byte (DCF) [1 byte]
 * 0x9F 0xFF 0x00 0x00 0x00 0x11 [6 bytes]
 * Backup [6 bytes]
 * Backup CRC [1 byte]
 * 0x00 0x00 [2 bytes]
 * additional segments
 * */
uint8_t response_index;

uint16_t LegicPrimeAppProcess(uint8_t *Buffer, uint16_t BitCount) {

    uint8_t tmpbf[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    switch(BitCount){
        case 0:
            return ISO14443F_APP_NO_RESPONSE;
        case 7:
            // Probably start of setup phase
            MemoryReadBlock(tmpbf, MEM_REPLAY_ADDRESS, 1);
            memcpy(Buffer, tmpbf, 1);
            return 6;
        case 6:
            // Probably end of setup phase
            return ISO14443F_APP_NO_RESPONSE;
        case 9:
            // Probably reading MIM256 card
            // Fallthrough to case 11
        case 11:
            // Probably reading MIM1024 card
            switch(response_index){
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                    MemoryReadBlock(tmpbf, MEM_REPLAY_ADDRESS + 1  + (response_index * 2), 2);
                    response_index++;
                    break;
                default:
                    sprintf(legic_log_str, "Legic APP Processing response index that is too high");
                    LogEntry(LOG_INFO_GENERIC, legic_log_str, strlen(legic_log_str));
                    return ISO14443F_APP_NO_RESPONSE;
            }

            memcpy(Buffer, tmpbf, 2);
            return 12;
        default:
            sprintf(legic_log_str, "Legic APP Processing unknown response");
            LogEntry(LOG_INFO_GENERIC, legic_log_str, strlen(legic_log_str));
            return ISO14443F_APP_NO_RESPONSE; //TODO: die horribly here?
    }
}

void LegicPrimeGetUid(ConfigurationUidType Uid) {
    sprintf(legic_log_str, "LEGIC GET UID");
    LogEntry(LOG_INFO_GENERIC, legic_log_str, strlen(legic_log_str));
    MemoryReadBlock(Uid, MEM_UID_ADDRESS, LEGIC_PRIME_UID_SIZE);
}

void LegicPrimeSetUid(ConfigurationUidType Uid) {
    sprintf(legic_log_str, "LEGIC SET UID");
    MemoryWriteBlock(Uid, MEM_UID_ADDRESS, LEGIC_PRIME_UID_SIZE);
    //TODO: Write also the LEGIC prime UID CRC to MEM_UID_CRC_ADDRESS
    LogEntry(LOG_INFO_GENERIC, legic_log_str, strlen(legic_log_str));
}

void LegicPrimeAppInit(void) {
    response_index = 0;
    sprintf(legic_log_str, "LEGIC APP INIT");
    LogEntry(LOG_INFO_GENERIC, legic_log_str, strlen(legic_log_str));

    // Prepare some captured communication beforehand or upload through the SEND/UPLOAD terminal functionality
    // Card ID: 0x57 0x46 0x5f 0x85
    uint8_t capture[] =  {0x39, 0x3e, 0x5, 0x45, 0x5, 0xfb, 0x2, 0x41, 0x0, 0xac, 0xc};
    MemoryWriteBlock(capture, MEM_REPLAY_ADDRESS, 11);
    // Card ID: 0x81 0xAB 0xB8 0x4A
    //    uint8_t capture2[] = {0x19, 0x27, 0xb, 0x9b, 0x1, 0xa1, 0x1, 0x6d, 0xa, 0x52, 0x3};
    //    MemoryWriteBlock(capture2, MEM_REPLAY_ADDRESS, 11);
}

void LegicPrimeAppReset(void) {
    response_index = 0;
}
#endif
