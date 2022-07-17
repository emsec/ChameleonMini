# Chameleon Mini firmware support for DESFire tag emulation

## Quick configuration of cloned DESFire tags

### Chameleon Mini terminal addons to support DESFire tag configurations

#### Selecting a DESFire configuration

```bash
CONFIG=?
CONFIG=MF_DESFIRE
CONFIG=MF_DESFIRE_2KEV1
CONFIG=MF_DESFIRE_4KEV1
CONFIG=MF_DESFIRE_4KEV2
```

#### DF_SETHDR -- Set PICC header information 

The UID for the tag can be set using separate Chameleon terminal commands as 
usual for all other configurations.
We can modify the remaining tag header information emulated by the tag as follows:
```bash
DF_SETHDR=ATS xxxxxxxxxx
DF_SETHDR=ATQA xxxx
DF_SETHDR=ManuID xx
DF_SETHDR=HardwareVersion mmMM
DF_SETHDR=SoftwareVersion mmMM
DF_SETHDR=BatchNumber xxxxxxxxxx
DF_SETHDR=ProductionDate WWYY
```

##### Examples:

The default ATS bytes for a DESFire tag are the same as specifying:
```bash
DF_SETHDR=ATS 067577810280
```
To set the ATS bytes reported to emulate a JCOP tag:
```bash
DF_SETHDR=ATS 0675f7b102
```
To reset the ATQA value returned in the anticollision loop handshaking:
```
DF_SETHDR=ATQA 0344
DF_SETHDR=ATQA 2838
```

##### Documentation for cloning specific tag types

```cpp
/* Other HW product types for DESFire tags: See page 7 of
 * https://www.nxp.com/docs/en/application-note/AN12343.pdf
 */
// typedef enum DESFIRE_FIRMWARE_ENUM_PACKING {
//     NATIVEIC_PHYS_CARD                 = 0x01,
//     LIGHT_NATIVEIC_PHYS_CARD           = 0x08,
//     MICROCONTROLLER_PHYS_CARDI         = 0x81,
//     MICROCONTROLLER_PHYS_CARDII        = 0x83,
//     JAVACARD_SECURE_ELEMENT_PHYS_CARD  = 0x91,
//     HCE_MIFARE_2GO                     = 0xa1,
// } DESFireHWProductCodes;
```
An up-to-date listing of bytes that indicate the tag manufacturer ID is 
found in the [Proxmark3 client source](https://github.com/RfidResearchGroup/proxmark3/blob/65b9a9fb769541f5d3e255ccf2c17d1cb77ac126/client/src/cmdhf14a.c#L48):
```cpp
static const manufactureName_t manufactureMapping[] = {
    // ID,  "Vendor Country"
    { 0x01, "Motorola UK" },
    { 0x02, "ST Microelectronics SA France" },
    { 0x03, "Hitachi, Ltd Japan" },
    { 0x04, "NXP Semiconductors Germany" },
    { 0x05, "Infineon Technologies AG Germany" },
    { 0x06, "Cylink USA" },
    { 0x07, "Texas Instrument France" },
    { 0x08, "Fujitsu Limited Japan" },
    { 0x09, "Matsushita Electronics Corporation, Semiconductor Company Japan" },
    { 0x0A, "NEC Japan" },
    { 0x0B, "Oki Electric Industry Co. Ltd Japan" },
    { 0x0C, "Toshiba Corp. Japan" },
    { 0x0D, "Mitsubishi Electric Corp. Japan" },
    { 0x0E, "Samsung Electronics Co. Ltd Korea" },
    { 0x0F, "Hynix / Hyundai, Korea" },
    { 0x10, "LG-Semiconductors Co. Ltd Korea" },
    { 0x11, "Emosyn-EM Microelectronics USA" },
    { 0x12, "INSIDE Technology France" },
    { 0x13, "ORGA Kartensysteme GmbH Germany" },
    { 0x14, "SHARP Corporation Japan" },
    { 0x15, "ATMEL France" },
    { 0x16, "EM Microelectronic-Marin SA Switzerland" },
    { 0x17, "KSW Microtec GmbH Germany" },
    { 0x18, "ZMD AG Germany" },
    { 0x19, "XICOR, Inc. USA" },
    { 0x1A, "Sony Corporation Japan" },
    { 0x1B, "Malaysia Microelectronic Solutions Sdn. Bhd Malaysia" },
    { 0x1C, "Emosyn USA" },
    { 0x1D, "Shanghai Fudan Microelectronics Co. Ltd. P.R. China" },
    { 0x1E, "Magellan Technology Pty Limited Australia" },
    { 0x1F, "Melexis NV BO Switzerland" },
    { 0x20, "Renesas Technology Corp. Japan" },
    { 0x21, "TAGSYS France" },
    { 0x22, "Transcore USA" },
    { 0x23, "Shanghai belling corp., ltd. China" },
    { 0x24, "Masktech Germany Gmbh Germany" },
    { 0x25, "Innovision Research and Technology Plc UK" },
    { 0x26, "Hitachi ULSI Systems Co., Ltd. Japan" },
    { 0x27, "Cypak AB Sweden" },
    { 0x28, "Ricoh Japan" },
    { 0x29, "ASK France" },
    { 0x2A, "Unicore Microsystems, LLC Russian Federation" },
    { 0x2B, "Dallas Semiconductor/Maxim USA" },
    { 0x2C, "Impinj, Inc. USA" },
    { 0x2D, "RightPlug Alliance USA" },
    { 0x2E, "Broadcom Corporation USA" },
    { 0x2F, "MStar Semiconductor, Inc Taiwan, ROC" },
    { 0x30, "BeeDar Technology Inc. USA" },
    { 0x31, "RFIDsec Denmark" },
    { 0x32, "Schweizer Electronic AG Germany" },
    { 0x33, "AMIC Technology Corp Taiwan" },
    { 0x34, "Mikron JSC Russia" },
    { 0x35, "Fraunhofer Institute for Photonic Microsystems Germany" },
    { 0x36, "IDS Microchip AG Switzerland" },
    { 0x37, "Thinfilm - Kovio USA" },
    { 0x38, "HMT Microelectronic Ltd Switzerland" },
    { 0x39, "Silicon Craft Technology Thailand" },
    { 0x3A, "Advanced Film Device Inc. Japan" },
    { 0x3B, "Nitecrest Ltd UK" },
    { 0x3C, "Verayo Inc. USA" },
    { 0x3D, "HID Global USA" },
    { 0x3E, "Productivity Engineering Gmbh Germany" },
    { 0x3F, "Austriamicrosystems AG (reserved) Austria" },
    { 0x40, "Gemalto SA France" },
    { 0x41, "Renesas Electronics Corporation Japan" },
    { 0x42, "3Alogics Inc Korea" },
    { 0x43, "Top TroniQ Asia Limited Hong Kong" },
    { 0x44, "Gentag Inc. USA" },
    { 0x45, "Invengo Information Technology Co.Ltd China" },
    { 0x46, "Guangzhou Sysur Microelectronics, Inc China" },
    { 0x47, "CEITEC S.A. Brazil" },
    { 0x48, "Shanghai Quanray Electronics Co. Ltd. China" },
    { 0x49, "MediaTek Inc Taiwan" },
    { 0x4A, "Angstrem PJSC Russia" },
    { 0x4B, "Celisic Semiconductor (Hong Kong) Limited China" },
    { 0x4C, "LEGIC Identsystems AG Switzerland" },
    { 0x4D, "Balluff GmbH Germany" },
    { 0x4E, "Oberthur Technologies France" },
    { 0x4F, "Silterra Malaysia Sdn. Bhd. Malaysia" },
    { 0x50, "DELTA Danish Electronics, Light & Acoustics Denmark" },
    { 0x51, "Giesecke & Devrient GmbH Germany" },
    { 0x52, "Shenzhen China Vision Microelectronics Co., Ltd. China" },
    { 0x53, "Shanghai Feiju Microelectronics Co. Ltd. China" },
    { 0x54, "Intel Corporation USA" },
    { 0x55, "Microsensys GmbH Germany" },
    { 0x56, "Sonix Technology Co., Ltd. Taiwan" },
    { 0x57, "Qualcomm Technologies Inc USA" },
    { 0x58, "Realtek Semiconductor Corp Taiwan" },
    { 0x59, "Freevision Technologies Co. Ltd China" },
    { 0x5A, "Giantec Semiconductor Inc. China" },
    { 0x5B, "JSC Angstrem-T Russia" },
    { 0x5C, "STARCHIP France" },
    { 0x5D, "SPIRTECH France" },
    { 0x5E, "GANTNER Electronic GmbH Austria" },
    { 0x5F, "Nordic Semiconductor Norway" },
    { 0x60, "Verisiti Inc USA" },
    { 0x61, "Wearlinks Technology Inc. China" },
    { 0x62, "Userstar Information Systems Co., Ltd Taiwan" },
    { 0x63, "Pragmatic Printing Ltd. UK" },
    { 0x64, "Associacao do Laboratorio de Sistemas Integraveis Tecnologico - LSI-TEC Brazil" },
    { 0x65, "Tendyron Corporation China" },
    { 0x66, "MUTO Smart Co., Ltd. Korea" },
    { 0x67, "ON Semiconductor USA" },
    { 0x68, "TUBITAK BILGEM Turkey" },
    { 0x69, "Huada Semiconductor Co., Ltd China" },
    { 0x6A, "SEVENEY France" },
    { 0x6B, "ISSM France" },
    { 0x6C, "Wisesec Ltd Israel" },
    { 0x7C, "DB HiTek Co Ltd Korea" },
    { 0x7D, "SATO Vicinity Australia" },
    { 0x7E, "Holtek Taiwan" },
    { 0x00, "no tag-info available" } // must be the last entry
};
```
Similarly, the PM3 source maintains the authoritative way to 
fingerprint the DESFire tag subtype in the 
[client source files](https://github.com/RfidResearchGroup/proxmark3/blob/65b9a9fb769541f5d3e255ccf2c17d1cb77ac126/client/src/cmdhfmfp.c#L92):
```cpp
    if (major == 0x00)
        snprintf(retStr, sizeof(buf), "%x.%x (" _GREEN_("DESFire MF3ICD40") ")", major, minor);
    else if (major == 0x01 && minor == 0x00)
        snprintf(retStr, sizeof(buf), "%x.%x (" _GREEN_("DESFire EV1") ")", major, minor);
    else if (major == 0x12 && minor == 0x00)
        snprintf(retStr, sizeof(buf), "%x.%x (" _GREEN_("DESFire EV2") ")", major, minor);
    else if (major == 0x33 && minor == 0x00)
        snprintf(retStr, sizeof(buf), "%x.%x (" _GREEN_("DESFire EV3") ")", major, minor);
    else if (major == 0x30 && minor == 0x00)
        snprintf(retStr, sizeof(buf), "%x.%x (" _GREEN_("DESFire Light") ")", major, minor);
    else if (major == 0x11 && minor == 0x00)
        snprintf(retStr, sizeof(buf), "%x.%x (" _GREEN_("Plus EV1") ")", major, minor);
    else
        snprintf(retStr, sizeof(buf), "%x.%x (" _YELLOW_("Unknown") ")", major, minor);
```

#### DF_COMM_MODE -- Manually sets the communication mode of the current session

The supported (work in progress) DESFire communication modes include: 
PLAINTEXT, PLAINTEXT-MAC, ENCIPHERED-CMAC-3DES, and ENCIPHERED-CMAC-AES128. 
It should be clear from the prior commands issued in the session which ``CommMode`` 
congiguration we are supposed to be working within. This command let's the user 
reset it intentionally at will for testing and debugging purposes. 

The syntax is as follows:
```bash
DF_COMM_MODE=Plaintext
DF_COMM_MODE=Plaintext:MAC
DF_COMM_MODE=Enciphered:3K3DES
DF_COMM_MODE=Enciphered:AES128
```
Use of this experimental command may cause unexpected results, vulnerabilities exposing 
your keys and sensitive (a priori) protected data to hackers and sniffers, and is 
discouraged unless you know what you are doing :) Try not to report bugs with the 
DESFire emulation if things suddenly fail after a call to this terminal command. 
Putting the Chameleon through a full power recycle (battery off) should reset the setting 
to the defaults. 

#### DF_COMM_MODE -- Manually sets the communication mode of the current session

This commanf sets the encryption mode for cryptographic operations. 
The two supported modes are ECB and CBC. 
The default mode for AES and DES (all types) of encryption is ECB mode. 
This is the supported mode for DESFire tags using the latest Proxmark3 software. 

The syntax is demonstrated by the following examples:
```bash
DF_ENCMODE=ECB
DF_ENCMODE=DES:ECB
DF_ENCMODE=AES:ECB
DF_ENCMODE=CBC
DF_ENCMODE=DES:CBC
DF_ENCMODE=AES:CBC
```

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

### Proxmark3 (PM3) compatibility and support 

The next PM3 commands are known to work with the Chameleon DESFire tag emulation (using both the RDV4 and Easy device types). 
The sample outputs obtained running the ``pm3`` command line utility below may vary by usage and proximity to the PM3 hardware.

#### Getting a summary of tag information

```bash
TODO
```

#### ISODES authentication with the PICC and PICC master key

```bash
TODO
```

### Compatibility with external USB readers and LibNFC

The DESFire configurations are known to work with the anticollision and RATS handshaking utility ``nfc-anticol`` from LibNFC. 
The Mifare DESFire commands installed by LibFreeFare have not been tested nor confirmed to work with the Chameleon Mini. 
The developers are actively working to ensure compatibility of the Chameleon DESFire emulation with external USB readers used 
running ``pcscd`` and ``pcsc_spy``. This support is not yet functional with tests using ACR-122 and HID Omnikey 5022CL readers. 
The DESFire support for the Chameleon Mini is tested with the LibNFC-based source code 
[developed in this directory]() with 
[sample dumps and output here]().

### Links to public datasheets and online specs 

The following links are the original online resource links are
archived here for documentation on how this firmware operates:
* [ISO/IEC 7816-4 Standard](http://www.unsads.com/specs/ISO/7816/ISO7816-4.pdf)
* [PublicDESFireEV0DatasheetSpecs -- April2004 (M075031_desfire.pdf)](https://web.archive.org/web/20170201031920/http://neteril.org/files/M075031_desfire.pdf)
* [NXP Application Note AN12343](https://www.nxp.com/docs/en/application-note/AN12343.pdf) 
* [TI DESFire EV1 Tag AES Auth Specs (sloa213.pdf)](https://www.ti.com/lit/an/sloa213/sloa213.pdf)
* [NXP Application Note AN10833](https://www.nxp.com/docs/en/application-note/AN10833.pdf)
* My favorite conference submission in grad school is (by far) about this project -- even though I did not present my talk that year.
  In rare form, the [presentation slides (tentative; see uploads)](https://archive.org/details/@maxiedschmidt) and the 
  [accepted manuscript](https://archive.org/download/ftc2021-presentation-slides-with-notes/schmidt-ftc2021-submission.pdf) (published in print form by Springer) 
  effectively document the scarce details of the DESFire spec and command sets gleaned while working on this project as a conference proceedings article.
  Grace Hopper would have approved :)

## Credits 

### Direct funding sources for this project

The author would like to thank the following direct sources of funding and support
for development of this project:
* Professor [Josephine Yu](http://people.math.gatech.edu/~jyu67/) and the
  [School of Mathematics](https://math.gatech.edu) at the
  Georgia Institute of Technology for allowing me to work on this as a secondary
  project as a Ph.D. candidate over the Summer and Fall of 2020. 
* More work to improve and add compatibility with the PM3 devices over the Spring of 2022 was supported by
  Georgia Tech to work as a RA through the university COVID-19 relief funding.
* The [KAOS manufacturers](https://shop.kasper.it) for providing support in the form of discounted Chameleon RevG
  devices to support my active development on the project.

### Sources of external code and open information about the DESFire specs

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

The source code for much of this implementation has been directly adapted, or modified, from mostly Java
language open source code for Android using several primary sources. Where possible, the license and credits
for the original sources for this ``avr-gcc``-compatible C language code are as specified in the next
repositories and code bases:
* [Prior DESFire Chameleon Mini Firmware Sourced (dev-zzo)](https://github.com/dev-zzo/ChameleonMini)
* [Android DESFire Host Card Emulation / HCE (jekkos)](https://github.com/jekkos/android-hce-desfire)
* [Android HCE Framework Library (kevinvalk)](https://github.com/kevinvalk/android-hce-framework)
* [AVRCryptoLib in C](https://github.com/cantora/avr-crypto-lib)
* [LibFreefare DESFire Code (mostly as a reference and check point)](https://github.com/nfc-tools/libfreefare/tree/master/libfreefare)

## New development sources of DESFire support for the Chameleon Mini

David Oswald has added a [DESFire emulation project](https://github.com/orgs/emsec/projects?type=classic) to organize tasks in 
progress for DESFire emulation support on the Chameleon Mini. The 
[original development sources](https://github.com/maxieds/ChameleonMiniDESFireStack/releases) are now archived and 
not kept up to date with the latest firmware pull requests and development sources. There are development sources for pull request projects in 
progress written by **@maxieds** are [located here](https://github.com/maxieds/ChameleonMini). 
For example, a newer branch can be built by running
```bash
$ git clone https://github.com/maxieds/ChameleonMini.git
$ cd ChameleonMini
$ git checkout DESFireNFCExternalUSBReaderPatches-LibNFCTestCode
$ cd Firmware/ChameleonMini
$ make desfire-dev
```
Other GitHub users are developing modifications of the main firmware sources for projects that include Mifare DESFire Plus support elsewhere.
