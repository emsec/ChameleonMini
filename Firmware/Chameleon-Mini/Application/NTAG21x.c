/*
 * NTAG215.c
 *
 *  Created on: 20.02.2019
 *  Author: Giovanni Cammisa (gcammisa)
 *  Still missing support for:
 *      -Still missing the BL bits, that basically are lockbytes for the lockbytes
 *  Thanks to skuser for the MifareUltralight code used as a starting point
 */

#include "ISO14443-3A.h"
#include "../Codec/ISO14443-2A.h"
#include "../Memory.h"
#include "NTAG21x.h"

//DEFINE ATQA and SAK
#define ATQA_VALUE 0x0044
#define SAK_VALUE 0x00

#define SAK_CL1_VALUE           ISO14443A_SAK_INCOMPLETE
#define SAK_CL2_VALUE           ISO14443A_SAK_COMPLETE_NOT_COMPLIANT

//ACK and NACK
#define ACK_VALUE                   0x0A
#define ACK_FRAME_SIZE              4 /* Bits */
#define NAK_INVALID_ARG             0x00
#define NAK_CRC_ERROR               0x01
#define NAK_NOT_AUTHED              0x04
#define NAK_EEPROM_ERROR            0x05
#define NAK_FRAME_SIZE              4

//DEFINING COMMANDS
/* ISO commands */
#define CMD_HALT 0x50

//NTAG COMMANDS
#define CMD_GET_VERSION 0x60
#define CMD_READ 0x30
#define CMD_FAST_READ 0x3A
#define CMD_WRITE 0xA2
#define CMD_COMPAT_WRITE 0xA0
#define CMD_READ_CNT 0x39
#define CMD_PWD_AUTH 0x1B
#define CMD_READ_SIG 0x3C //TODO: CHECK IMPLEMENTATION TO MAKE IT VALID?

//MEMORY LAYOUT STUFF, addresses and sizes in bytes
//UID stuff
#define UID_CL1_ADDRESS         0x00
#define UID_CL1_SIZE            3
#define UID_BCC1_ADDRESS        0x03
#define UID_CL2_ADDRESS         0x04
#define UID_CL2_SIZE            4
#define UID_BCC2_ADDRESS        0x08

//Static Lockbytes (common on all ntag21x variants)
#define STATIC_LOCKBYTE_0_ADDRESS   0x0A
#define STATIC_LOCKBYTE_1_ADDRESS   0x0B 

//Dynamic Lockbytes (different depending on which ntag21x we're working with) //TODO: IMPLEMENT
//There are 3 consecutive lockbytes, just add 0x01 to the address to get the next one
//NTAG213 
#define NTAG213_DYNAMIC_LOCKBYTE_0_ADDRESS NTAG21x_PAGE_SIZE * 0x28

//NTAG215
#define NTAG215_DYNAMIC_LOCKBYTE_0_ADDRESS NTAG21x_PAGE_SIZE * 0x82

//NTAG216
#define NTAG216_DYNAMIC_LOCKBYTE_0_ADDRESS NTAG21x_PAGE_SIZE * 0xE2

//CONFIG stuff
#define CONFIG_AREA_START_ADDRESS_NTAG213   NTAG21x_PAGE_SIZE * 0x29
#define CONFIG_AREA_START_ADDRESS_NTAG215   NTAG21x_PAGE_SIZE * 0x83
#define CONFIG_AREA_START_ADDRESS_NTAG216   NTAG21x_PAGE_SIZE * 0xE3

#define CONFIG_AREA_SIZE        8


//CONFIG offsets, relative to config start address
#define CONF_AUTH0_OFFSET       0x03
#define CONF_ACCESS_OFFSET      0x04
#define CONF_PASSWORD_OFFSET    0x08
#define CONF_PACK_OFFSET        0x0C

//ACCESS bitmasks
#define AUTHLIM_BITMASK         0x7 //0b00000111
#define NFC_CNT_EN_BITMASK      0x10 //0b00010000
#define NFC_CNT_PWD_PROT_BITMASK 0x8 //0b00001000
#define CFGLCK_BITMASK          0x40 //0b01000000
#define PROT_BITMASK            0x80 //0b10000000

//WRITE STUFF
#define BYTES_PER_WRITE         4
#define PAGE_WRITE_MIN          0x02

//CONFIG masks to check individual needed bits

#define VERSION_INFO_LENGTH 8 //8 bytes info lenght + crc

#define BYTES_PER_READ NTAG21x_PAGE_SIZE * 4

//SIGNATURE Lenght
#define SIGNATURE_LENGTH        32

//ADDRESSES FOR CONFIGS AFTER THE DUMP
#define AUTHLIM_COUNTER_AFTER_DUMP_OFFSET 0x00
#define NFC_CNT_AFTER_DUMP_OFFSET 0x01


static enum {
    NTAG213,
    NTAG215,
    NTAG216,
} Ntag_type;

static enum {
    STATE_HALT,
    STATE_IDLE,
    STATE_READY1,
    STATE_READY2,
    STATE_ACTIVE
} State;

static bool FromHalt = false;
static uint8_t PageCount;
static bool ArmedForCompatWrite;
static uint8_t CompatWritePageAddress;
static bool Authenticated;
static uint8_t FirstAuthenticatedPage;
static bool ReadAccessProtected; //CHECK IF THIS CRAP IS ACTUALLY INVERTED
static uint8_t Access;
static bool NfcCntEnabled;
static bool NfcCntPwdProt;

static bool IsFirstReadAfterReset;

/* TAG INIT FUNCTIONS */
//NTAG21x common
void NTAG21xAppReset(void) {
    State = STATE_IDLE;
    switch(Ntag_type){
        case NTAG213:
            NTAG213AppInit();
            break;
        case NTAG215:
            NTAG215AppInit();
            break;
        case NTAG216:
            NTAG216AppInit();
            break;
    }
}

void NTAG21xAppInit(void) {
    State = STATE_IDLE;
    FromHalt = false;
    ArmedForCompatWrite = false;
    Authenticated = false;
    IsFirstReadAfterReset = true;
}

void NTAG21xAppTask(void) {

}

//NTAG213
void NTAG213AppInit(void) {
    PageCount = NTAG213_PAGES;
    Ntag_type = NTAG213;
    NTAG21xAppInit();
    /* Fetch some of the configuration into RAM */
    MemoryReadBlock(&FirstAuthenticatedPage, CONFIG_AREA_START_ADDRESS_NTAG213 + CONF_AUTH0_OFFSET, 1);
    MemoryReadBlock(&Access, CONFIG_AREA_START_ADDRESS_NTAG213 + CONF_ACCESS_OFFSET, 1);
    ReadAccessProtected = !!(Access & PROT_BITMASK);
    NfcCntEnabled = !!(Access & NFC_CNT_EN_BITMASK);
    NfcCntPwdProt = !!(Access & NFC_CNT_PWD_PROT_BITMASK);
}

//NTAG215
void NTAG215AppInit(void) {
    PageCount = NTAG215_PAGES;
    Ntag_type = NTAG215;
    NTAG21xAppInit();
    /* Fetch some of the configuration into RAM */
    MemoryReadBlock(&FirstAuthenticatedPage, CONFIG_AREA_START_ADDRESS_NTAG215 + CONF_AUTH0_OFFSET, 1);
    MemoryReadBlock(&Access, CONFIG_AREA_START_ADDRESS_NTAG215 + CONF_ACCESS_OFFSET, 1);
    ReadAccessProtected = !!(Access & PROT_BITMASK);
    NfcCntEnabled = !!(Access & NFC_CNT_EN_BITMASK);
    NfcCntPwdProt = !!(Access & NFC_CNT_PWD_PROT_BITMASK);
}

//NTAG216
void NTAG216AppInit(void) {
    PageCount = NTAG216_PAGES;
    Ntag_type = NTAG216;
    NTAG21xAppInit();
    /* Fetch some of the configuration into RAM */
    MemoryReadBlock(&FirstAuthenticatedPage, CONFIG_AREA_START_ADDRESS_NTAG216 + CONF_AUTH0_OFFSET, 1);
    MemoryReadBlock(&Access, CONFIG_AREA_START_ADDRESS_NTAG216 + CONF_ACCESS_OFFSET, 1);
    ReadAccessProtected = !!(Access & PROT_BITMASK);
    NfcCntEnabled = !!(Access & NFC_CNT_EN_BITMASK);
    NfcCntPwdProt = !!(Access & NFC_CNT_PWD_PROT_BITMASK);
}

static uint16_t GetNFCCNTAddress() {
    uint16_t CounterAddr;
    switch(Ntag_type) {
        case NTAG213:
            CounterAddr = NTAG213_MEM_SIZE + NFC_CNT_AFTER_DUMP_OFFSET;
            break;
        case NTAG215:
            CounterAddr = NTAG215_MEM_SIZE + NFC_CNT_AFTER_DUMP_OFFSET;
            break;
        case NTAG216:
            CounterAddr = NTAG215_MEM_SIZE + NFC_CNT_AFTER_DUMP_OFFSET;
            break;
        }
    return CounterAddr;
}

//Verify authentication
static bool VerifyAuthentication(uint8_t PageAddress) {
    /* If authenticated, no verification needed */
    if (Authenticated) {
        return true;
    }
    /* Otherwise, verify the accessed page is below the limit */
    return PageAddress < FirstAuthenticatedPage;
}

static int GetNthBit(uint8_t value, uint8_t n) {
    return (value & ( 1 << n )) >> n;
}

//Verify lockbytes: returns true if authorized, false if not authorized
static bool VerifyDynamicLockbytes(uint8_t PageAddress) { //TODO: CHECK IF THE IMPLEMENTATION IS CORRECT
    if (PageAddress >= 16) {
        uint8_t DynLockByte0;
        uint8_t DynLockByte1;
        uint8_t DynLockByte2; //TODO: IMPLEMENT BL

        switch(Ntag_type) {
            case NTAG213:
                if (PageAddress > 39) { //After page 39 we can't lock stuff, so we're always authorized
                    return true;
                }
                else {
                    if (PageAddress <=31) { //Pages 0-31 refer to lockbyte 0
                        MemoryReadBlock(&DynLockByte0, NTAG213_DYNAMIC_LOCKBYTE_0_ADDRESS, 1);
                        if (GetNthBit(DynLockByte0, (PageAddress>>1)-8) == 1){
                            return false;
                        }
                    }
                    else {  //Pages 32-39 refer to lockbyte 1
                        MemoryReadBlock(&DynLockByte1, NTAG213_DYNAMIC_LOCKBYTE_0_ADDRESS + 1, 1);
                        if (GetNthBit(DynLockByte1, (PageAddress>>1)-16) == 1){
                            return false;
                        }
                    }
                }
                return true;
        
            case NTAG215:
                if (PageAddress > 129) { //After page 129 we can't lock stuff, so we're always authorized
                    return true;
                } else { //On the NTAG215 only the DynLockbyte0 is used
                    MemoryReadBlock(&DynLockByte0, NTAG215_DYNAMIC_LOCKBYTE_0_ADDRESS, 1);
                    if (GetNthBit(DynLockByte0, (PageAddress>>4)-1) == 1){
                        return false;
                    }
                }
                return true;
            
            case NTAG216:
                if (PageAddress > 225) { //After page 225 we can't lock stuff, so we're always authorized
                    return true;
                }
                else {
                    if (PageAddress <=143) { //Pages 0-143 refer to lockbyte 0
                    MemoryReadBlock(&DynLockByte0, NTAG216_DYNAMIC_LOCKBYTE_0_ADDRESS, 1);
                        if (GetNthBit(DynLockByte0, (PageAddress>>4)-1) == 1){
                            return false;
                        }
                    }
                    else { //Pages 144-225 refer to lockbyte 1
                    MemoryReadBlock(&DynLockByte1, NTAG216_DYNAMIC_LOCKBYTE_0_ADDRESS + 1, 1);
                        if (GetNthBit(DynLockByte1, (PageAddress>>4)-9) == 1){
                            return false;
                        }
                    }
                }
                return true;
            
        }
    }
}

static bool VerifyStaticLockbytes(uint8_t PageAddress) { //TODO: CHECK IF THE IMPLEMENTATION IS CORRECT
    uint8_t StaticLockByte0;
    uint8_t StaticLockByte1; //TODO: IMPLEMENT BL

    if(PageAddress<3 || PageAddress>15) { //Static lockbytes only deal with pages 3-15
        return true;
    }

    if (PageAddress <=7) { //Pages 3-7 refer to lockbyte 0
        MemoryReadBlock(&StaticLockByte0, STATIC_LOCKBYTE_0_ADDRESS, 1);
        if (GetNthBit(StaticLockByte0, PageAddress) == 1){
            return false;
        }
    }
    else { //Pages 8-15 refer to lockbyte 1
        MemoryReadBlock(&StaticLockByte1, STATIC_LOCKBYTE_1_ADDRESS, 1);
        if (GetNthBit(StaticLockByte1, (PageAddress-8)) == 1){
            return false;
        }
    }
}

//Writes a page
static uint8_t AppWritePage(uint8_t PageAddress, uint8_t *const Buffer) {
    if (!ActiveConfiguration.ReadOnly) {
        MemoryWriteBlock(Buffer, PageAddress * NTAG21x_PAGE_SIZE, NTAG21x_PAGE_SIZE);
    } else {
        /* If the chameleon is in read only mode, it silently
        * ignores any attempt to write data. */
    }
    return 0;
}

//Basic sketch of the command handling stuff
static uint16_t AppProcess(uint8_t *const Buffer, uint16_t ByteCount) {
    uint8_t Cmd = Buffer[0];

    /* Handle the compatibility write command */
    if (ArmedForCompatWrite) {
        ArmedForCompatWrite = false;

        AppWritePage(CompatWritePageAddress, &Buffer[2]);
        Buffer[0] = ACK_VALUE;
        return ACK_FRAME_SIZE;
    }

    switch (Cmd) {
        case CMD_GET_VERSION: {
            /* Provide hardcoded version response depending on what NTAG21x is selected */
            switch (Ntag_type) {
                case NTAG213:
                    //VERSION RESPONSE FOR NTAG 213
                    Buffer[0] = 0x00;
                    Buffer[1] = 0x04;
                    Buffer[2] = 0x04;
                    Buffer[3] = 0x02;
                    Buffer[4] = 0x01;
                    Buffer[5] = 0x00;
                    Buffer[6] = 0x0F;
                    Buffer[7] = 0x03;
                    break;

                case NTAG215:
                    //VERSION RESPONSE FOR NTAG 215
                    Buffer[0] = 0x00;
                    Buffer[1] = 0x04;
                    Buffer[2] = 0x04;
                    Buffer[3] = 0x02;
                    Buffer[4] = 0x01;
                    Buffer[5] = 0x00;
                    Buffer[6] = 0x11;
                    Buffer[7] = 0x03;
                    break;

                case NTAG216:
                    //VERSION RESPONSE FOR NTAG 216
                    Buffer[0] = 0x00;
                    Buffer[1] = 0x04;
                    Buffer[2] = 0x04;
                    Buffer[3] = 0x02;
                    Buffer[4] = 0x01;
                    Buffer[5] = 0x00;
                    Buffer[6] = 0x13;
                    Buffer[7] = 0x03;
                    break;

                default:
                    //TODO: REMOVE, THIS IS FOR DEBUG PURPOSE, WE SHOULD NEVER GET HERE
                    Buffer[0] = 0x00;
                    Buffer[1] = 0x04;
                    Buffer[2] = 0x04;
                    Buffer[3] = 0x02;
                    Buffer[4] = 0x01;
                    Buffer[5] = 0x00;
                    Buffer[6] = 0x00;
                    Buffer[7] = 0x00;
                    break;
            }
            ISO14443AAppendCRCA(Buffer, VERSION_INFO_LENGTH);
            return (VERSION_INFO_LENGTH + ISO14443A_CRCA_SIZE) * 8;
        }

        case CMD_READ: {
            //We're incrementing the counter at the first read after a reset
            if (IsFirstReadAfterReset && NfcCntEnabled) {
                    uint16_t CounterAddr = GetNFCCNTAddress();
                    uint16_t CurrentCounterValue; //TODO: The counter is 24 bits long, but I'm currently using only 16 bits for testing. Might need to define a new data type (uint24_t ???) or do some hacks with a uint32_t. Also keep in mind little endian / big endian stuff
                    MemoryReadBlock(&CurrentCounterValue, CounterAddr, 3);
                    CurrentCounterValue++;
                    MemoryWriteBlock(&CurrentCounterValue, CounterAddr, 3);
                    IsFirstReadAfterReset = false;
            }

            uint8_t PageAddress = Buffer[1];
            uint8_t PageLimit;
            uint8_t Offset;

            PageLimit = PageCount;

            /* if protected and not autenticated, ensure the wraparound is at the first protected page */
            if (!ReadAccessProtected && !Authenticated) {
                PageLimit = FirstAuthenticatedPage;
            } else {
                PageLimit = PageCount;
            }

            /* Validation */
            if (PageAddress >= PageLimit) {
                Buffer[0] = NAK_INVALID_ARG;
                return NAK_FRAME_SIZE;
            }

            /* Read out, emulating the wraparound */
            for (Offset = 0; Offset < BYTES_PER_READ; Offset += 4) {
                MemoryReadBlock(&Buffer[Offset], PageAddress * NTAG21x_PAGE_SIZE, NTAG21x_PAGE_SIZE);
                PageAddress++;
                if (PageAddress == PageLimit) { // if arrived at the last page, start reading from page 0
                    PageAddress = 0;
                }
            }
            ISO14443AAppendCRCA(Buffer, BYTES_PER_READ);
            return (BYTES_PER_READ + ISO14443A_CRCA_SIZE) * 8;
        }

        case CMD_FAST_READ: {
            //We're incrementing the counter at the first read after a reset
            if (IsFirstReadAfterReset && NfcCntEnabled) {
                    uint16_t CounterAddr = GetNFCCNTAddress();
                    uint16_t CurrentCounterValue; //TODO: The counter is 24 bits long, but I'm currently using only 16 bits for testing. Might need to define a new data type (uint24_t ???) or do some hacks with a uint32_t. Also keep in mind little endian / big endian stuff
                    MemoryReadBlock(&CurrentCounterValue, CounterAddr, 3);
                    CurrentCounterValue++;
                    MemoryWriteBlock(&CurrentCounterValue, CounterAddr, 3);
                    IsFirstReadAfterReset = false;
            }

            uint8_t StartPageAddress = Buffer[1];
            uint8_t EndPageAddress = Buffer[2];
            /* Validation */
            if ((StartPageAddress > EndPageAddress) || (StartPageAddress >= PageCount) || (EndPageAddress >= PageCount)) {
                Buffer[0] = NAK_INVALID_ARG;
                return NAK_FRAME_SIZE;
            }

            /* Check authentication only if protection is read&write (instead of only write protection) */
            if (!ReadAccessProtected) {
                if (!VerifyAuthentication(StartPageAddress) || !VerifyAuthentication(EndPageAddress)) {
                    Buffer[0] = NAK_NOT_AUTHED;
                    return NAK_FRAME_SIZE;
                }
            }

            ByteCount = (EndPageAddress - StartPageAddress + 1) * NTAG21x_PAGE_SIZE;
            MemoryReadBlock(Buffer, StartPageAddress * NTAG21x_PAGE_SIZE, ByteCount);
            ISO14443AAppendCRCA(Buffer, ByteCount);
            return (ByteCount + ISO14443A_CRCA_SIZE) * 8;
        }

        case CMD_PWD_AUTH: { //TODO: Check if the implementation is correct
            uint8_t Password[4];
            
            uint8_t AuthLim = Access & AUTHLIM_BITMASK;
            uint8_t CurrentAuthLimCounterValue;

            uint16_t PwdAddr;
            uint16_t AuthLimCounterAddr;
            uint16_t PACKAddr;

            uint8_t ZeroValue = 0x00;

            //Get all the addresses that are dependant on the type of tag at once, makes the code a bit more readable
            switch(Ntag_type) {
                case NTAG213:
                    PwdAddr = CONFIG_AREA_START_ADDRESS_NTAG213 + CONF_PASSWORD_OFFSET;
                    AuthLimCounterAddr = NTAG213_MEM_SIZE + AUTHLIM_COUNTER_AFTER_DUMP_OFFSET;
                    PACKAddr = CONFIG_AREA_START_ADDRESS_NTAG213 + CONF_PACK_OFFSET;
                    break;
                case NTAG215:
                    PwdAddr = CONFIG_AREA_START_ADDRESS_NTAG215 + CONF_PASSWORD_OFFSET;
                    AuthLimCounterAddr = NTAG215_MEM_SIZE + AUTHLIM_COUNTER_AFTER_DUMP_OFFSET;
                    PACKAddr = CONFIG_AREA_START_ADDRESS_NTAG215 + CONF_PACK_OFFSET;
                    break;
                case NTAG216:
                    PwdAddr = CONFIG_AREA_START_ADDRESS_NTAG215 + CONF_PASSWORD_OFFSET;
                    AuthLimCounterAddr = NTAG215_MEM_SIZE + AUTHLIM_COUNTER_AFTER_DUMP_OFFSET;
                    PACKAddr = CONFIG_AREA_START_ADDRESS_NTAG215 + CONF_PACK_OFFSET;
                    break;
            }

            /* Read and compare the password */
            MemoryReadBlock(&CurrentAuthLimCounterValue, AuthLimCounterAddr, 1);
            MemoryReadBlock(&Password, PwdAddr, 4);
         
            if (CurrentAuthLimCounterValue > AuthLim){ //If the counter is bigger than the limit, we should not be able to authenticate ever again
                Buffer[0] = NAK_NOT_AUTHED;
                return NAK_FRAME_SIZE;
            }

            if (Password[0] != Buffer[1] || Password[1] != Buffer[2] || Password[2] != Buffer[3] || Password[3] != Buffer[4]) {
                CurrentAuthLimCounterValue++;
                MemoryWriteBlock(&CurrentAuthLimCounterValue,AuthLimCounterAddr,1); //?& TODO: CHECK THIS
                Buffer[0] = NAK_NOT_AUTHED;
                return NAK_FRAME_SIZE;
            }
            /* Authenticate the user */
            Authenticated = true;

            //after a successful authentication, we need to reset the counter
            MemoryWriteBlock(&ZeroValue,AuthLimCounterAddr,1);

            /* Send the PACK value back */ 
            MemoryReadBlock(Buffer, PACKAddr, 2);
            ISO14443AAppendCRCA(Buffer, 2);
            return (2 + ISO14443A_CRCA_SIZE) * 8;
        }

        case CMD_WRITE: {
            /* This is a write command containing 4 bytes of data that
            * should be written to the given page address. */
            uint8_t PageAddress = Buffer[1];
            /* Validation */
            if ((PageAddress < PAGE_WRITE_MIN) || (PageAddress >= PageCount)) {
                Buffer[0] = NAK_INVALID_ARG;
                return NAK_FRAME_SIZE;
            }
            if (!VerifyAuthentication(PageAddress)) {
                Buffer[0] = NAK_NOT_AUTHED;
                return NAK_FRAME_SIZE;
            }
            if(!VerifyStaticLockbytes(PageAddress)) { //If false the static lockbytes are set in such a way that does not allow writing to this page
                Buffer[0] = NAK_NOT_AUTHED;
                return NAK_FRAME_SIZE;
            }
            if (!VerifyDynamicLockbytes(PageAddress)) { //If false the dynamic lockbytes are set in such a way that does not allow writing to this page
                Buffer[0] = NAK_NOT_AUTHED;
                return NAK_FRAME_SIZE;
            }
            

            AppWritePage(PageAddress, &Buffer[2]);
            Buffer[0] = ACK_VALUE;
            return ACK_FRAME_SIZE;
        }

        case CMD_COMPAT_WRITE: {
            uint8_t PageAddress = Buffer[1];
            /* Validation */
            if ((PageAddress < PAGE_WRITE_MIN) || (PageAddress >= PageCount)) {
                Buffer[0] = NAK_INVALID_ARG;
                return NAK_FRAME_SIZE;
            }
            if (!VerifyAuthentication(PageAddress)) {
                Buffer[0] = NAK_NOT_AUTHED;
                return NAK_FRAME_SIZE;
            }
            if (!VerifyStaticLockbytes(PageAddress)) { //If false the static lockbytes are set in such a way that does not allow writing to this page
                Buffer[0] = NAK_NOT_AUTHED;
                return NAK_FRAME_SIZE;
            }
            if (!VerifyDynamicLockbytes(PageAddress)) { //If false the dynamic lockbytes are set in such a way that does not allow writing to this page
                Buffer[0] = NAK_NOT_AUTHED;
                return NAK_FRAME_SIZE;
            }

            //TODO: check for static lock bytes

            /* CRC check passed and page-address is within bounds.
            * Store address and proceed to receiving the data. */
            CompatWritePageAddress = PageAddress;
            ArmedForCompatWrite = true; //TODO:IMPLEMENT ARMED COMPAT WRITE
            Buffer[0] = ACK_VALUE;
            return ACK_FRAME_SIZE;
        }

        case CMD_READ_SIG: { //TODO: IMPLEMENT SOME KIND OF ABILITY TO WRITE SIGNATURE, MIGHT NEED FOR A BIGGER DUMP FILE TO BE UPLOADED WITH THE SIGNATURE APPENDED
            /* Hardcoded response */
            memset(Buffer, 0xCA, SIGNATURE_LENGTH); //replace this with code to read signature from extended memory dump
            ISO14443AAppendCRCA(Buffer, SIGNATURE_LENGTH);
            return (SIGNATURE_LENGTH + ISO14443A_CRCA_SIZE) * 8;
        }

        case CMD_READ_CNT: {
            
            //If the NFCCNT is not protected, OR if the NFCCNT is protected BUT we're authenticated, then we can read the NFCCNT
            if (!NfcCntPwdProt || (NfcCntPwdProt && Authenticated)) {
                uint16_t CounterAddr = GetNFCCNTAddress();
                MemoryReadBlock(&Buffer[0], CounterAddr, 3);

                ISO14443AAppendCRCA(Buffer, 3);
                return (3 + ISO14443A_CRCA_SIZE) * 8;
            }

            //If we don't have authentication and the NFCCNT is protected, we can't read the NFCCNT so we return NAK_NOT_AUTHED
            Buffer[0] = NAK_NOT_AUTHED;
            return NAK_FRAME_SIZE;
        }


        //PART OF ISO STANDARD, NOT OF NTAG DATASHEET
        case CMD_HALT: {
            /* Halts the tag. According to the ISO14443, the second
            * byte is supposed to be 0. */
            if (Buffer[1] == 0) {
                /* According to ISO14443, we must not send anything
                * in order to acknowledge the HALT command. */
                State = STATE_HALT;
                return ISO14443A_APP_NO_RESPONSE;
            } else {
                Buffer[0] = NAK_INVALID_ARG;
                return NAK_FRAME_SIZE;
            }
        }

        default: {
            break;
        }

    }
    /* Command not handled. Switch to idle. */

    State = STATE_IDLE;
    return ISO14443A_APP_NO_RESPONSE;

}


//FINITE STATE MACHINE STUFF, SHOULD BE THE VERY SIMILAR TO Mifare Ultralight
uint16_t NTAG21xAppProcess(uint8_t *Buffer, uint16_t BitCount) {
    uint8_t Cmd = Buffer[0];
    uint16_t ByteCount;

    switch (State) {
        case STATE_IDLE:
        case STATE_HALT:
            FromHalt = State == STATE_HALT;
            if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE, FromHalt)) {
                /* We received a REQA or WUPA command, so wake up. */
                State = STATE_READY1;
                return BitCount;
            }
            break;

        case STATE_READY1:
            if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE, FromHalt)) {
                State = FromHalt ? STATE_HALT : STATE_IDLE;
                return ISO14443A_APP_NO_RESPONSE;
            } else if (Cmd == ISO14443A_CMD_SELECT_CL1) {
                /* Load UID CL1 and perform anticollision. Since
                * MF Ultralight use a double-sized UID, the first byte
                * of CL1 has to be the cascade-tag byte. */
                uint8_t UidCL1[ISO14443A_CL_UID_SIZE] = { [0] = ISO14443A_UID0_CT };

                MemoryReadBlock(&UidCL1[1], UID_CL1_ADDRESS, UID_CL1_SIZE);

                if (ISO14443ASelect(Buffer, &BitCount, UidCL1, SAK_CL1_VALUE)) {
                    /* CL1 stage has ended successfully */
                    State = STATE_READY2;
                }

                return BitCount;
            } else {
                /* Unknown command. Enter halt state */
                State = STATE_IDLE;
            }
            break;

        case STATE_READY2:
            if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE, FromHalt)) {
                State = FromHalt ? STATE_HALT : STATE_IDLE;
                return ISO14443A_APP_NO_RESPONSE;
            } else if (Cmd == ISO14443A_CMD_SELECT_CL2) {
                /* Load UID CL2 and perform anticollision */
                uint8_t UidCL2[ISO14443A_CL_UID_SIZE];

                MemoryReadBlock(UidCL2, UID_CL2_ADDRESS, UID_CL2_SIZE);

                if (ISO14443ASelect(Buffer, &BitCount, UidCL2, SAK_CL2_VALUE)) {
                    /* CL2 stage has ended successfully. This means
                    * our complete UID has been sent to the reader. */
                    State = STATE_ACTIVE;
                }

                return BitCount;
            } else {
                /* Unknown command. Enter halt state */
                State = STATE_IDLE;
            }
            break;

        //Only ACTIVE state, no AUTHENTICATED state, PWD_AUTH is handled in commands.
        case STATE_ACTIVE:
            /* Preserve incoming data length */
            ByteCount = (BitCount + 7) >> 3;
            if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE, FromHalt)) {
                State = FromHalt ? STATE_HALT : STATE_IDLE;
                return ISO14443A_APP_NO_RESPONSE;
            }
            /* At the very least, there should be 3 bytes in the buffer. */
            if (ByteCount < (1 + ISO14443A_CRCA_SIZE)) {
                State = STATE_IDLE;
                return ISO14443A_APP_NO_RESPONSE;
            }
            /* All commands here have CRCA appended; verify it right away */
            ByteCount -= 2;
            if (!ISO14443ACheckCRCA(Buffer, ByteCount)) {
                Buffer[0] = NAK_CRC_ERROR;
                return NAK_FRAME_SIZE;
            }
            return AppProcess(Buffer, ByteCount);

        default:
            /* Unknown state? Should never happen. */
            break;
    }

    /* No response has been sent, when we reach here */
    return ISO14443A_APP_NO_RESPONSE;
}

//HELPER FUNCTIONS
void NTAG21xGetUid(ConfigurationUidType Uid) {
    /* Read UID from memory */
    MemoryReadBlock(&Uid[0], UID_CL1_ADDRESS, UID_CL1_SIZE);
    MemoryReadBlock(&Uid[UID_CL1_SIZE], UID_CL2_ADDRESS, UID_CL2_SIZE);
}

void NTAG21xSetUid(ConfigurationUidType Uid) {
    /* Calculate check bytes and write everything into memory */
    uint8_t BCC1 = ISO14443A_UID0_CT ^ Uid[0] ^ Uid[1] ^ Uid[2];
    uint8_t BCC2 = Uid[3] ^ Uid[4] ^ Uid[5] ^ Uid[6];

    MemoryWriteBlock(&Uid[0], UID_CL1_ADDRESS, UID_CL1_SIZE);
    MemoryWriteBlock(&BCC1, UID_BCC1_ADDRESS, ISO14443A_CL_BCC_SIZE);
    MemoryWriteBlock(&Uid[UID_CL1_SIZE], UID_CL2_ADDRESS, UID_CL2_SIZE);
    MemoryWriteBlock(&BCC2, UID_BCC2_ADDRESS, ISO14443A_CL_BCC_SIZE);
}