# Chameleon Mini firmware support for DESFire tag emulation

The project began based on a few open source Java-based emulation projects (Android based) and the 
prior initial work to add this support on the Chameleon Mini by **@dev-zzo**. 
The starting point of the current firmware code for this project was compiled from 
[this firmware mod fork](https://github.com/dev-zzo/ChameleonMini/tree/desfire) as 
were the known instruction (command) and status codes from the 
[Android HCE (Java based code)](https://github.com/jekkos/android-hce-desfire) 
repository maintained by **@jekkos**. 
After that point, **@maxieds** reorganized and began work modifying and debugging the 
compiled source base in [this repository](https://github.com/maxieds/ChameleonMiniDESFireStack). 
Most of the preliminary testing of these firmware mods was done using the 
[Chameleon Mini Live Debugger](https://github.com/maxieds/ChameleonMiniLiveDebugger) 
Android logger application, and with ``libnfc`` via a 
USB NFC tag reader (host-based testing code is 
[available here](https://github.com/maxieds/ChameleonMiniDESFireStack/tree/master/Firmware/Chameleon-Mini/Application/DESFire/Testing)). 

Support for DESFire tag emulation on the Chameleon Mini is complicated for at least a couple of reasons:
1. The hardware (onboard AVR) has a small memory footprint available. Thus with all of the new tag support, 
   storage needs for cryptographic data structures, and storage needs for large tables finding space 
   was challenging. Much of this is resolved by storing data needed for operation using a linked, flat file type 
   design for structures where a small amount of buffer space is needed at a time and can be filled by 
   fetching the data from FRAM / settings space. 
2. There is only limited public information available about the precise expectations and runtime 
   conditions needed to write a perfect, standards-compliant DESFire tag implementation. 

The firmware has been tested and known to work with the KAOS manufactured RevG Chameleon devices. 
Unfortunately, formative RevE device support is not available due to the memory requirements to 
run this firmware emulation. The device responds well using the ``libnfc``-based utility 
``nfc-anticol``:
```bash
NFC reader: SCM Micro / SCL3711-NFC&RW opened

Sent bits:     26 (7 bits)
Received bits: 03  44  
Sent bits:     93  20  
Received bits: 88  23  77  00  dc  
Sent bits:     93  70  88  23  77  00  dc  4b  b3  
Received bits: 04  
Sent bits:     95  20  
Received bits: 0b  99  bf  98  b5  
Sent bits:     95  70  0b  99  bf  98  b5  2f  24  
Received bits: 20  
Sent bits:     e0  50  bc  a5  
Received bits: 75  77  81  02  80  
Sent bits:     50  00  57  cd  

Found tag with
 UID: 2377000b99bf98
ATQA: 4403
 SAK: 20
 ATS: 75  77  81  02  80
```
More testing needs to be done to fine tune support for interfacing the Chameleon 
with live, in-the-wild DESFire tag readers in practice. It has been verified to work with the 
Proxmark3 NFC devices: 
```bash
[usb] pm3 --> hf 14a read
[+]  UID: 4A D9 BA 11 B9 97 57
[+] ATQA: 44 03
[+]  SAK: 20 [1]
[+]  ATS: 75 77 81 02 80
[=] field dropped.
```
The DESFire configuration mode has been known to see recognition problems 
using ``libfreefare``. 

## Supported functionality

### Tables of tested support for active commands

#### Native DESFire command support (mixed EV0/EV1/EV2 instruction sets)

| Instruction | Cmd Byte | Description | Testing Status | Implementation Notes |
| :---        |   :----: |     :----:  |    :----:      | :--                  |
| CMD_AUTHENTICATE | 0x0A | Authenticate legacy | :ballot_box_with_check: | Works with the ``-DDESFIRE_QUICK_DES_CRYPTO`` Makefile ``SETTINGS`` compiler flag set to enable "quicker" DES crypto. |
| CMD_AUTHENTICATE_ISO | 0x1A | ISO / 3DES auth | :question: | This implementation is too slow! Need hardware support for 3DES crypto? |
| CMD_AUTHENTICATE_AES | 0xAA | Standard AES auth | :ballot_box_with_check: | |
| CMD_AUTHENTICATE_EV2_FIRST | 0x71 | Newer spec auth variant | :x: | |
| CMD_AUTHENTICATE_EV2_NONFIRST | 0x77 | Newer spec auth variant | :x: | See page 32 of AN12343.pdf |
| CMD_CHANGE_KEY_SETTINGS | 0x54 | | :ballot_box_with_check: | |
| CMD_SET_CONFIGURATION |  0x5C | | :x: | |
| CMD_CHANGE_KEY |  0xC4 | | :ballot_box_with_check: | |
| CMD_GET_KEY_VERSION | 0x64 | | :ballot_box_with_check: | |
| CMD_CREATE_APPLICATION |  0xCA | | :ballot_box_with_check: | |
| CMD_DELETE_APPLICATION |  0xDA | | :ballot_box_with_check: | |
| CMD_GET_APPLICATION_IDS | 0x6A | | :ballot_box_with_check: | |
| CMD_FREE_MEMORY | 0x6E | | :ballot_box_with_check: | |
| CMD_GET_DF_NAMES | 0x6D | | :x: | =Need docs for what this command does! |
| CMD_GET_KEY_SETTINGS | 0x45 | | :ballot_box_with_check: | |
| CMD_SELECT_APPLICATION |  0x5A | | :ballot_box_with_check: | |
| CMD_FORMAT_PICC |  0xFC | | :ballot_box_with_check: | |
| CMD_GET_VERSION | 0x60 | | :ballot_box_with_check: | |
| CMD_GET_CARD_UID | 0x51 | | :ballot_box_with_check: | |
| CMD_GET_FILE_IDS |  0x6F | | :ballot_box_with_check: | |
| CMD_GET_FILE_SETTINGS | 0xF5 | | :ballot_box_with_check: | |
| CMD_CHANGE_FILE_SETTINGS | 0x5F | | :x: | |
| CMD_CREATE_STDDATA_FILE |  0xCD | | :ballot_box_with_check: | |
| CMD_CREATE_BACKUPDATA_FILE |  0xCB | | :ballot_box_with_check: | |
| CMD_CREATE_VALUE_FILE |  0xCC | | :ballot_box_with_check: | |
| CMD_CREATE_LINEAR_RECORD_FILE | 0xC1 | | :wavy_dash: | GetFileSettings still not returning correct data |
| CMD_CREATE_CYCLIC_RECORD_FILE | 0xC0 | | :wavy_dash: | GetFileSettings still not returning correct data |
| CMD_DELETE_FILE | 0xDF | | :ballot_box_with_check: | |
| CMD_GET_ISO_FILE_IDS | 0x61 | | :x: | |
| CMD_READ_DATA |  0xBD | | :ballot_box_with_check: | The data for std/backup files is uninitialized (any bits) until the user sets the data with WriteData |
| CMD_WRITE_DATA |  0x3D | | :ballot_box_with_check: | Only supports write command operations with <= 52 bytes of data at a time. Offset parameters can be used to write lengthier files. |
| CMD_GET_VALUE | 0x6C | | :ballot_box_with_check: | |
| CMD_CREDIT | 0x0C | | :ballot_box_with_check: | |
| CMD_DEBIT | 0xDC | | :ballot_box_with_check: | |
| CMD_LIMITED_CREDIT | 0x1C | | :ballot_box_with_check: | |
| CMD_WRITE_RECORD | 0x3B | | :question: | |
| CMD_READ_RECORDS | 0xBB | | :ballot_box_with_check: :wavy_dash: | |
| CMD_CLEAR_RECORD_FILE | 0xEB | | :question: | |
| CMD_COMMIT_TRANSACTION | 0xC7 | | :ballot_box_with_check: | |
| CMD_ABORT_TRANSACTION | 0xA7 | | :ballot_box_with_check: | |               |

#### ISO7816 command support

| Instruction | Cmd Byte | Description | Testing Status | Implementation Notes |
| :---        |   :----: |     :----:  |    :----:      | :--                  |
| CMD_ISO7816_SELECT | 0xa4 | A more nuanced ISO7816 version of EF/DF selection. | :wavy_dash: :question: | See the implementation notes [in this spec](https://cardwerk.com/smart-card-standard-iso7816-4-section-6-basic-interindustry-commands/#chap6_11). We only support EF selection with ``P1=00000000|000000010`` and DF(AID) with ``P1=00000100``. |
| CMD_ISO7816_GET_CHALLENGE | 0x84 | | :wavy_dash: :question: | |
| CMD_ISO7816_EXTERNAL_AUTHENTICATE | 0x82 | | :x: | |
| CMD_ISO7816_INTERNAL_AUTHENTICATE | 0x88 | | :x: | |
| CMD_ISO7816_READ_BINARY | 0xb0 | | :wavy_dash: :question: | Needs testing. |
| CMD_ISO7816_UPDATE_BINARY | 0xd6 | | :wavy_dash: :question: | Needs testing. |
| CMD_ISO7816_READ_RECORDS | 0xb2 | | :wavy_dash: :question: | Needs testing. |
| CMD_ISO7816_APPEND_RECORD | 0xe2 | | :wavy_dash: :question: | Especially needs testing for corner case checks. |

### TODO list: Suggestions for updates to the code and unsupported functionality 

* Check to figure out where possibly encrypted transfers (AES CMAC) come into play after the initial
  authentication procedure?
* Need to handle encrypted transfer modes invoked after authenticate (term: SAM?)?
* Need to replace the DES/3DES encryption library for something faster?
* When setting the key data via a set command, need to initialize the key addresses, update the
  key count in the AID, and other accounting details ...
* Currently, all of the file transfers (read/write) are done in plaintext ...
* The ``ReadData`` command ``offset`` input has no action at this point (TODO later) ...
* How to accurately handle BackupDataFile types?

```cpp
case DESFIRE_FILE_BACKUP_DATA:
        if (Rollback) {
            CopyBlockBytes(DataAreaBlockId, DataAreaBlockId + File.BackupFile.BlockCount, File.BackupFile.BlockCount);
        }
        else {
            CopyBlockBytes(DataAreaBlockId + File.BackupFile.BlockCount, DataAreaBlockId, File.BackupFile.BlockCount);
        }
        break;
```

* Handle how to write values directly to value files (without credit/debit type actions)? 
* WriteRecordFile only handles/reads off continued chunks in round block sizes. 
* **BIG Q:** Does calling ``MemoryStore()`` to frequently cause problems? This should be rare-ish when 
  data on the tag changes?
* Have an action where a (long) push of a button allows for
     1. A dump of the stored internal logging data to get written LIVE-style to the serial USB
     2. A dump of the pretty-printed DESFire tag layout to get written to the serial USB 
* Store some compile-time randomized system bits (from openssl) to get stored to a special 
  small enough segment within the EEPROM for reference (sort of like a secret UID data, or even
  unique serial number that should get reprogrammed everytime the firmware is re-compiled ...  
  1. ``#define EEPROM_ATTR __attribute__ ((section (".eeprom")))``

### Chameleon Mini terminal addons to support ``CONFIG=MF_DESFIRE`` modes

Note that these commands are only compiled into the firmware when the compiler 
option ``-DALLOW_DESFIRE_TERMINAL_COMMANDS`` and ``-DCONFIG_MF_DESFIRE_SUPPORT`` are 
specified. If you do not at miniumum have a configuration, ``CONFIG=MF_DESFIRE``, then 
these commands will not work. Note that LIVE logging should be disabled before attempting to  
run these commands.

#### Selecting a DESFire configuration

```bash
CONFIG=?
CONFIG=MF_DESFIRE
```

#### DF_SETHDR -- Set PICC header information 

Since the UID and other precious manufacturer data are supposed to be unique to  
tags and sacred ground upon which only manufacturers shall walk, we are going to upheave 
this notion and hack our tag "roll your own" (tag) style. This means we can pick and 
choose the components used to identify the tag by calling a few Chameleon terminal 
command variants. The syntax is varied -- the numbers after the parameter name are 
indicators for the number of bytes to include as arguments: 
```bash
DF_SETHDR?
101:OK WITH TEXT
DF_SETHDR <HardwareVersion-2|SoftwareVersion-2|BatchNumber-5|ProductionDate-2> <HexBytes-N>
```
Likewise, as promised, we can modify the tag header information emulated by the tag as follows:
```bash
DF_SETHDR=ATS xxxxxxxxxx
DF_SETHDR=HardwareVersion xxxx
DF_SETHDR=SoftwareVersion xxxx
DF_SETHDR=BatchNumber xxxxxxxxxx
DF_SETHDR=ProductionDate xxxx
```
For example, to set the ATS bytes reported to emulate a JCOP tag:
```bash
DF_SETHDR=ATS 0675f7b102
```
Note that the UID for the tag can be set using separate Chameleon terminal commands.

#### DF_PPRINT_PICC -- Visualize tag contents

This lets users pretty print the tag layout in several different ways, and with 
a couple of options for verbosity. This helps with visualizing the landscape that 
we are programming. The syntax include: 
```bash
DF_PPRINT_PICC FullImage
DF_PPRINT_PICC HeaderData
```

#### DF_FWINFO -- Print firmware revision information 

Self explanatory and similar to the familiar ``VERSION`` command. Syntax:
```bash 
DF_FWINFO
```

#### DF_LOGMODE -- Sets the depth of (LIVE) logging messages printed at runtime

Syntax -- not guaranteeing that all of these are meaningful or distinct just yet:
```bash
DF_LOGMODE?
DF_LOGMODE=<OFF|NORMAL|VERBOSE|DEBUGGING>
DF_LOGMODE=<0|1|TRUE|FALSE>
```

#### DF_TESTMODE -- Sets whether the firmware emulation is run in testing/debugging mode

Syntax:
```bash
DF_TESTMODE?
DF_TESTMODE=<0|1|TRUE|FALSE|OFF|ON>
```

### Links to public datasheets and online specs 

The following links are the original online resource links are
archived here for documentation on how this firmware operates:
* [ISO/IEC 7816-4 Standard](http://www.unsads.com/specs/ISO/7816/ISO7816-4.pdf)
* [PublicDESFireEV0DatasheetSpecs -- April2004 (M075031_desfire.pdf)](https://web.archive.org/web/20170201031920/http://neteril.org/files/M075031_desfire.pdf)
* [NXP Application Note AN12343](https://www.nxp.com/docs/en/application-note/AN12343.pdf) 
* [TI DESFire EV1 Tag AES Auth Specs (sloa213.pdf)](https://www.ti.com/lit/an/sloa213/sloa213.pdf)
* [NXP Application Note AN10833](https://www.nxp.com/docs/en/application-note/AN10833.pdf)

### Makefile support to enable special functionality

```make
#Whether or not to customize the USB identifier settings in the firmware:
SETTINGS  += -DENABLE_LUFAUSB_CUSTOM_VERSIONS

#Whether or not to allow users Chameleon terminal access to change the DESFire configuration's
#sensitive settings like manufacturer, serial number, etc.
SETTINGS  += -DENABLE_PERMISSIVE_DESFIRE_SETTINGS
SETTINGS  += -DALLOW_DESFIRE_TERMINAL_COMMANDS

#Set a default logging mode for debugging with the DESFire
#emulation code:
SETTINGS  += -DDESFIRE_DEFAULT_LOGGING_MODE=DEBUGGING
#SETTINGS += -DDESFIRE_DEFAULT_LOGGING_MODE=OFF

#Set a default testing mode setting (0 = OFF, non-NULL = ON):
SETTINGS  += -DDESFIRE_DEFAULT_TESTING_MODE=1

#Feature: Use randomized UIDs that mask the actual secret UID until
#the tag has been issued a successful authentication sequence:
SETTINGS  += -DDESFIRE_RANDOMIZE_UIDS_PREAUTH

#Anticipating that the implementation overhead is high with the
#maximum storage allocations for the number of possible keys per
#application directory, and/or the total number of AID numbered
#directory slots, the following options will tweak this limitation:
# -> Set DESFIRE_MEMORY_LIMITED_TESTING to shrink the defaults
# -> Or explicitly define DESFIRE_CUSTOM_MAX_KEYS=##UINT## (per AID),
# -> And/Or define DESFIRE_CUSTOM_MAX_APPS=##UINT##
#    (total number of AID spaces available, not including the master 0x00)
SETTINGS  += -DDESFIRE_MEMORY_LIMITED_TESTING
#SETTINGS += -DDESFIRE_CUSTOM_MAX_APPS=8
#SETTINGS += -DDESFIRE_CUSTOM_MAX_KEYS=6
#SETTINGS += -DDESFIRE_CUSTOM_MAX_FILES=6
#SETTINGS += -DDESFIRE_USE_FACTORY_SIZES
#SETTINGS += -DDESFIRE_MAXIMIZE_SIZES_FOR_STORAGE

#Set a minimum incoming/outgoing log size so we do not spam the
#Chameleon Mini logs to much by logging everything:
SETTINGS  += -DDESFIRE_MIN_INCOMING_LOGSIZE=0
SETTINGS  += -DDESFIRE_MIN_OUTGOING_LOGSIZE=0

#Enable printing of crypto tests when a new DESFire emulation instance is started:
#SETTINGS += -DDESFIRE_RUN_CRYPTO_TESTING_PROCEDURE

#Option to save space with the "Application/Crypto1.c" code by storing large tables
#in PROGMEM. Note that this will slow down the read times when accessing these tables:
SETTINGS  += -DDESFIRE_CRYPTO1_SAVE_SPACE
```

### Hacking the source 

For Chameleon firmware hackers and prospective modders that want to hack on and improve the DESFire 
emulation support sources, the next points are helpful to understanding the local details of 
how data storage is handled, and other implementation notes that should save you some time. 
:smile: 

#### Description of the FRAM storage scheme of the DESFire tag data and file system 

The scheme involves nested or linked pointers to data stored in FRAM. The following
source code snippets are useful to understanding how to load and process this storage:
```cpp 
typedef struct DESFIRE_FIRMWARE_PACKING {
    /* Static data: does not change during the PICC's lifetime.
     * We will add Chameleon Mini terminal commands to enable 
     * resetting this data so tags can be emulated authentically. 
     * This structure is stored verbatim (using memcpy) at the 
     * start of the FRAM setting space for the configuration. 
     */
    uint8_t Uid[DESFIRE_UID_SIZE] DESFIRE_FIRMWARE_ALIGNAT;
    uint8_t StorageSize;
    uint8_t HwVersionMajor;
    uint8_t HwVersionMinor;
    uint8_t SwVersionMajor;
    uint8_t SwVersionMinor;
    uint8_t BatchNumber[5] DESFIRE_FIRMWARE_ALIGNAT;
    uint8_t ProductionWeek;
    uint8_t ProductionYear;
    uint8_t ATSBytes[5];
    /* Dynamic data: changes during the PICC's lifetime */
    uint16_t FirstFreeBlock;
    uint8_t TransactionStarted; // USED ???
    uint8_t Spare[9] DESFIRE_FIRMWARE_ALIGNAT; // USED ???
} DESFirePICCInfoType;

typedef struct DESFIRE_FIRMWARE_PACKING {
    BYTE  Slot;
    BYTE  KeyCount;
    BYTE  MaxKeyCount;
    BYTE  FileCount;
    BYTE  CryptoCommStandard;
    SIZET KeySettings;            /* Block offset in FRAM */
    SIZET FileNumbersArrayMap;    /* Block offset in FRAM */
    SIZET FileCommSettings;       /* Block offset in FRAM */
    SIZET FileAccessRights;       /* Block offset in FRAM */
    SIZET FilesAddress;           /* Block offset in FRAM */
    SIZET KeyVersionsArray;       /* Block offset in FRAM */
    SIZET KeyTypesArray;          /* Block offset in FRAM */
    SIZET KeyAddress;             /* Block offset in FRAM */
    UINT  DirtyFlags; // USED ???
} SelectedAppCacheType;

BYTE SELECTED_APP_CACHE_TYPE_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(sizeof(SelectedAppCacheType));
BYTE APP_CACHE_KEY_SETTINGS_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(DESFIRE_MAX_KEYS);
BYTE APP_CACHE_FILE_NUMBERS_HASHMAP_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(DESFIRE_MAX_FILES);
BYTE APP_CACHE_FILE_COMM_SETTINGS_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(DESFIRE_MAX_FILES);
BYTE APP_CACHE_FILE_ACCESS_RIGHTS_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(DESFIRE_MAX_FILES);
BYTE APP_CACHE_KEY_VERSIONS_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(DESFIRE_MAX_KEYS);
BYTE APP_CACHE_KEY_TYPES_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(DESFIRE_MAX_KEYS);
BYTE APP_CACHE_KEY_BLOCKIDS_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(2 * DESFIRE_MAX_KEYS);
BYTE APP_CACHE_FILE_BLOCKIDS_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(2 * DESFIRE_MAX_KEYS);
BYTE APP_CACHE_MAX_KEY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(CRYPTO_MAX_KEY_SIZE);

/* 
 * Defines the application directory contents.
 * The application directory maps AIDs to application slots:
 * the AID's index in `AppIds` is the slot number.
 * 
 * This is the "global" structure that gets stored in the header 
 * data for the directory. To see the actual byte-for-byte storage 
 * of the accounting data for each instantiated AID slot, refer to the 
 * `DesfireApplicationDataType` structure from above.
 */
typedef struct DESFIRE_FIRMWARE_PACKING {
    BYTE           FirstFreeSlot;
    DESFireAidType AppIds[DESFIRE_MAX_SLOTS] DESFIRE_FIRMWARE_ARRAY_ALIGNAT; 
    SIZET          AppCacheStructBlockOffset[DESFIRE_MAX_SLOTS];
} DESFireAppDirType;

void InitBlockSizes(void) {
     DESFIRE_PICC_INFO_BLOCK_ID = 0;
     DESFIRE_APP_DIR_BLOCK_ID = DESFIRE_PICC_INFO_BLOCK_ID +
                                DESFIRE_BYTES_TO_BLOCKS(sizeof(DESFirePICCInfoType));
     DESFIRE_APP_CACHE_DATA_ARRAY_BLOCK_ID = DESFIRE_APP_DIR_BLOCK_ID +
                                             DESFIRE_BYTES_TO_BLOCKS(sizeof(DESFireAppDirType));
     DESFIRE_FIRST_FREE_BLOCK_ID = DESFIRE_APP_CACHE_DATA_ARRAY_BLOCK_ID;
     DESFIRE_INITIAL_FIRST_FREE_BLOCK_ID = DESFIRE_FIRST_FREE_BLOCK_ID;
}

/* Data about an application's file is currently kept in this structure.
 * The location of these structures is defined by the file index.
 */
typedef struct DESFIRE_FIRMWARE_PACKING {
    uint8_t FileType;
    uint8_t FileNumber;
    uint16_t FileSize;
    uint16_t FileDataAddress; /* FRAM address of the storage of the data for the file */
    union DESFIRE_FIRMWARE_ALIGNAT {
        struct DESFIRE_FIRMWARE_ALIGNAT {
            uint16_t FileSize;
        } StandardFile;
        struct DESFIRE_FIRMWARE_ALIGNAT {
            uint16_t FileSize;
            uint8_t BlockCount;
        } BackupFile;
        struct DESFIRE_FIRMWARE_ALIGNAT {
            int32_t CleanValue;
            int32_t DirtyValue;
            int32_t LowerLimit;
            int32_t UpperLimit;
            uint8_t LimitedCreditEnabled;
            int32_t PreviousDebit;
        } ValueFile;
        struct DESFIRE_FIRMWARE_ALIGNAT {
            uint16_t BlockCount;
            uint16_t RecordPointer;
            //uint8_t ClearPending;  // USED ???
            uint8_t RecordSize[3];
            uint8_t CurrentNumRecords[3];
            uint8_t MaxRecordCount[3];
        } RecordFile;
    };
} DESFireFileTypeSettings;

typedef struct DESFIRE_FIRMWARE_PACKING {
    BYTE Num;
    DESFireFileTypeSettings File;
} SelectedFileCacheType;
```

## Credits 

### Direct funding sources for this project

The author would like to thank the following direct sources of funding and support
for development of this project:
* Professor [Josephine Yu](http://people.math.gatech.edu/~jyu67/) and the
  [School of Mathematics](https://math.gatech.edu) at the
  Georgia Institute of Technology for allowing me to work on this as a secondary
  project as a Ph.D. candidate over the Summer and Fall of 2020.
* The [KAOS manufacturers](https://shop.kasper.it) for providing support in the form of discounted Chameleon RevG
  devices to support my active development on the project.

### Sources of external code and open information about the DESFire specs

The source code for much of this implementation has been directly adapted, or modified, from mostly Java
language open source code for Android using several primary sources. Where possible, the license and credits
for the original sources for this ``avr-gcc``-compatible C language code are as specified in the next
repositories and code bases:
* [Prior DESFire Chameleon Mini Firmware Sourced (dev-zzo)](https://github.com/dev-zzo/ChameleonMini)
* [Android DESFire Host Card Emulation / HCE (jekkos)](https://github.com/jekkos/android-hce-desfire)
* [Android HCE Framework Library (kevinvalk)](https://github.com/kevinvalk/android-hce-framework)
* [AVRCryptoLib in C](https://github.com/cantora/avr-crypto-lib)
* [LibFreefare DESFire Code (mostly as a reference and check point)](https://github.com/nfc-tools/libfreefare/tree/master/libfreefare)

### Clarification: Where the local licenses apply

The code that is not already under direct license (see below) is released according to the normal
[license for the firmware](https://github.com/emsec/ChameleonMini/blob/master/LICENSE.txt).
Additional licenses that apply only to the code used within this DESFire stack implementation,
or to the open source libraries used to derive this code,
are indicated within the local firmware directories.

### DESFire sources header comments 

```
The DESFire stack portion of this firmware source is free software written by Maxie Dion Schmidt (@maxieds): You can redistribute it and/or modify it under the terms of this license.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

The complete source distribution of this firmware is available at the following link: https://github.com/maxieds/ChameleonMiniFirmwareDESFireStack.

Based in part on the original DESFire code created by @dev-zzo (GitHub handle) [Dmitry Janushkevich] available at https://github.com/dev-zzo/ChameleonMini/tree/desfire.

This notice must be retained at the top of all source files where indicated.
```

