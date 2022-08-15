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
DF_SETHDR=HwType xx
DF_SETHDR=HwSubtype xx
DF_SETHDR=HwProtoType xx 
DF_SETHDR=HwVers mmMM
DF_SETHDR=SwType xx
DF_SETHDR=SwSubtype xx
DF_SETHDR=SwProtoType xx
DF_SETHDR=SwVers mmMM
DF_SETHDR=BatchNo xxxxxxxxxx
DF_SETHDR=ProdDate WWYY
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
[client source files](https://github.com/RfidResearchGroup/proxmark3/blob/65b9a9fb769541f5d3e255ccf2c17d1cb77ac126/client/src/cmdhfmfdes.c#L278):
```cpp
    if (major == 0x00)
        return DESFIRE_MF3ICD40;
    if (major == 0x01 && minor == 0x00)
        return DESFIRE_EV1;
    if (major == 0x12 && minor == 0x00)
        return DESFIRE_EV2;
    if (major == 0x22 && minor == 0x00)
        return DESFIRE_EV2_XL;
    if (major == 0x42 && minor == 0x00)
        return DESFIRE_EV2;
    if (major == 0x33 && minor == 0x00)
        return DESFIRE_EV3;
    if (major == 0x30 && minor == 0x00)
        return DESFIRE_LIGHT;
    if (major == 0x11 && minor == 0x00)
        return PLUS_EV1;
    if (major == 0x10 && minor == 0x00)
        return NTAG413DNA;
    return DESFIRE_UNKNOWN;
```
Table 2 in section 2.1 of [NXP AN10833](https://www.nxp.com/docs/en/application-note/AN10833.pdf) (page 5) lists
standard Mifare tag identifications for several tags. This byte is represented by setting 
``Picc.HwType`` using the Chameleon terminal command ``DF_SETHDR=HwType xx``. The default setting for the 
Chameleon DESFire tags is ``0x01`` (*MIFARE DESFire*). The table in the application note is reproduced 
below for reference. The NXP documentation says: "*The upper nibble [X] defines if the
device is a native MIFARE IC (``0x0``), an implementation (``0x8``), an applet on a Java Card
(``0x9``) or MIFARE 2GO (``0xA``).*"

| Second Byte of GetVersion Response (``Picc.HwType``) | NXP Type Tag |
| :---: | :-- |
| ``0xX1`` | *MIFARE DESFire* |
| ``0xX2`` | *MIFARE Plus* |
| ``0xX3`` | *MIFARE Ultralight* |
| ``0xX4`` | *NTAG* |
| ``0xX5`` | *RFU* |
| ``0xX6`` | *RFU* |
| ``0xX7`` | *NTAG I2C* |
| ``0xX8`` | *MIFARE DESFire Light* |

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

### Proxmark3 (PM3) compatibility and support 

The next PM3 commands are known to work with the Chameleon DESFire tag emulation (using both the RDV4 and Easy device types). 
The sample outputs obtained running the ``pm3`` command line utility below may vary by usage and proximity to the PM3 hardware.

#### PM3 logging and debugging setup script (run this first)

```bash
hw dbg -4
prefs set clientdebug --full
data setdebugmode -2
```

#### Listing initial tag response

```bash
[usb] pm3 --> hf mfdes list
[=] downloading tracelog data from device
[+] Recorded activity (trace len = 146 bytes)
[=] start = start of start frame end = end of frame. src = source of transfer
[=] ISO14443A - all times are in carrier periods (1/13.56MHz)

      Start |        End | Src | Data (! denotes parity error)                                           | CRC | Annotation
------------+------------+-----+-------------------------------------------------------------------------+-----+--------------------
          0 |        992 | Rdr |52                                                                       |     | WUPA
       2116 |       4484 | Tag |04  03                                                                   |     | 
       7040 |       9504 | Rdr |93  20                                                                   |     | ANTICOLL
      10580 |      16404 | Tag |88  08  e2  38  5a                                                       |     | 
      19072 |      29600 | Rdr |93  70  88  08  e2  38  5a  95  d5                                       |  ok | SELECT_UID
      30660 |      34180 | Tag |24  d8  36                                                               |     | 
      35584 |      38048 | Rdr |95  20                                                                   |     | ANTICOLL-2
      39124 |      44948 | Tag |f6  12  53  42  f5                                                       |     | 
      47616 |      58080 | Rdr |95  70  f6  12  53  42  f5  cb  66                                       |  ok | SELECT_UID-2
      59220 |      62804 | Tag |20  fc  70                                                               |     | 
      64512 |      69280 | Rdr |e0  80  31  73                                                           |  ok | RATS
```

#### Getting a summary of tag information

The output of this command will change significantly if the header and 
manufacturer bytes are changed using the Chameleon terminal commands above. 
The tag type reported will also vary depending on which EV0/EV1/EV2 generation of the
DESFire configuration is used:
```bash
[usb] pm3 --> hf mfdes info
[#] BCC2 incorrect, got 0xf5, expected 0x12
[#] Aborting
[#] Can't select card
[#] switch_off
[!] ⚠️  Can't select card
[usb] pm3 --> hf mfdes info
[#] pcb_blocknum 0 == 2 
[#] [WCMD <--: : 08/08] 02 90 60 00 00 00 14 98 
[#] pcb_blocknum 1 == 3 
[#] [WCMD <--: : 08/08] 03 90 af 00 00 00 1f 15 
[#] pcb_blocknum 0 == 2 
[#] [WCMD <--: : 08/08] 02 90 af 00 00 00 34 11 
[#] halt warning. response len: 4
[#] Halt error
[#] switch_off

[=] ---------------------------------- Tag Information ----------------------------------
[+]               UID: 08 E2 38 F6 12 53 42 
[+]      Batch number: BB 27 CB 35 08 
[+]   Production date: week 01 / 2005

[=] --- Hardware Information
[=]    raw: 04010100011A05
[=]      Vendor Id: NXP Semiconductors Germany
[=]           Type: 0x01
[=]        Subtype: 0x01
[=]        Version: 0.1 ( DESFire MF3ICD40 )
[=]   Storage size: 0x1A ( 8192 bytes )
[=]       Protocol: 0x05 ( ISO 14443-2, 14443-3 )

[=] --- Software Information
[=]    raw: 04010100011805
[=]      Vendor Id: NXP Semiconductors Germany
[=]           Type: 0x01
[=]        Subtype: 0x01
[=]        Version: 0.1
[=]   Storage size: 0x18 ( 4096 bytes )
[=]       Protocol: 0x05 ( ISO 14443-3, 14443-4 )

[=] --------------------------------- Card capabilities ---------------------------------
[#] error DESFIRESendRaw Current configuration/status does not allow the requested command
[#] error DESFIRESendRaw Unknown error
[#] error DESFIRESendRaw Current configuration/status does not allow the requested command
[#] error DESFIRESendApdu Command code not supported
[#] error DESFIRESendApdu Command code not supported
[+] ------------------------------------ PICC level -------------------------------------
[+] Applications count: 0 free memory n/a
[+] PICC level auth commands: 
[+]    Auth.............. YES
[+]    Auth ISO.......... YES
[+]    Auth AES.......... YES
[+]    Auth Ev2.......... NO
[+]    Auth ISO Native... NO
[+]    Auth LRP.......... NO

[=] --- Free memory
[+]    Card doesn't support 'free mem' cmd
```

#### ISODES authentication with the PICC and PICC master key

```bash
[usb] pm3 --> hf mfdes auth -n 0 -t 3tdea -k 000000000000000000000000000000000000000000000000 -v -c native -a
[=] Key num: 0 Key algo: 3tdea Key[24]: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
[=] Secure channel: n/a Command set: native Communication mode: plain
[+] Setting ISODEP -> inactive
[+] Setting ISODEP -> NFC-A
[=] AID 000000 is selected
[=] Auth: cmd: 0x1a keynum: 0x00
[+] raw>> 1A 00 
[+] raw<< AF EE 91 30 1E E8 F5 84 D6 C7 85 1D 05 65 13 90 A6 C6 D5 
[#] encRndB: EE 91 30 1E E8 F5 84 D6 
[#] RndB: CA FE BA BE 00 11 22 33 
[#] rotRndB: FE BA BE 00 11 22 33 CA FE BA BE 00 11 22 33 CA 
[#] Both   : 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 FE BA BE 00 11 22 33 CA FE BA BE 00 11 22 33 CA 
[+] raw>> AF 30 EB 55 F3 29 39 04 96 77 88 CE EF 33 A3 C8 7B 18 66 1A F1 62 78 A0 28 53 84 67 98 7C BB DB 03 
[+] raw<< 00 9B 71 57 8F FB DF 80 A8 F6 EF 33 4A C6 CD F9 7A 7D BE 
[=] Session key : 01 02 03 04 CA FE BA BE 07 08 09 10 22 33 CA FE 13 14 15 16 00 11 22 33 
[=] Desfire  authenticated
[+] PICC selected and authenticated succesfully
[+] Context: 
[=] Key num: 0 Key algo: 3tdea Key[24]: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
[=] Secure channel: ev1 Command set: native Communication mode: plain
[=] Session key [24]: 01 02 03 04 CA FE BA BE 07 08 09 10 22 33 CA FE 13 14 15 16 00 11 22 33  
[=]     IV [8]: 00 00 00 00 00 00 00 00 
[+] Setting ISODEP -> inactive
```

#### AES (128-bit) authentication with the PICC and PICC master key

```bash
[usb] pm3 --> hf mfdes auth -n 0 -t aes -k 00000000000000000000000000000000 -v -c native -a
[=] Key num: 0 Key algo: aes Key[16]: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
[=] Secure channel: n/a Command set: native Communication mode: plain
[+] Setting ISODEP -> inactive
[+] Setting ISODEP -> NFC-A
[=] AID 000000 is selected
[=] Auth: cmd: 0xaa keynum: 0x00
[+] raw>> AA 00 
[+] raw<< AF EA 8C 8F 55 42 BB 7B 81 7C 26 44 EC EC 73 85 AB 8B AF 
[#] encRndB: EA 8C 8F 55 42 BB 7B 81 
[#] RndB: CA FE BA BE 00 11 22 33 
[#] rotRndB: FE BA BE 00 11 22 33 CA FE BA BE 00 11 22 33 CA 
[#] Both   : 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 FE BA BE 00 11 22 33 CA FE BA BE 00 11 22 33 CA 
[+] raw>> AF 04 25 9E 8B C4 49 26 DD 5D 9F 1E 84 1F 2F 13 E4 F1 BD 8E 58 72 AD A6 29 D3 CC 93 91 52 99 BC 71 
[+] raw<< 00 59 2D 75 D8 BE 6A 4B C1 25 E9 9D 95 D4 B1 B0 D2 D1 5D 
[=] Session key : 01 02 03 04 CA FE BA BE 13 14 15 16 00 11 22 33 
[=] Desfire  authenticated
[+] PICC selected and authenticated succesfully
[+] Context: 
[=] Key num: 0 Key algo: aes Key[16]: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
[=] Secure channel: ev1 Command set: native Communication mode: plain
[=] Session key [16]: 01 02 03 04 CA FE BA BE 13 14 15 16 00 11 22 33  
[=]     IV [16]: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
[+] Setting ISODEP -> inactive
```

### Compatibility with external USB readers and LibNFC

The DESFire configurations are known to work with the anticollision and RATS handshaking utility ``nfc-anticol`` 
from [LibNFC](https://github.com/nfc-tools/libnfc). The following is the output of this utility using the 
ACS ACR-122U reader over USB (with the ``pcscd`` dameon not running):
```bash
$ sudo nfc-anticol -f
NFC reader: ACS / ACR122U PICC Interface opened

Sent bits:     26 (7 bits)
Received bits: 04  03  
Sent bits:     93  20  
Received bits: 88  08  c7  df  98  
Sent bits:     93  70  88  08  c7  df  98  9c  ae  
Received bits: 24  d8  36  
Sent bits:     95  20  
Received bits: f1  02  fc  6c  63  
Sent bits:     95  70  f1  02  fc  6c  63  3a  98  
Received bits: 20  fc  70  
Sent bits:     e0  50  bc  a5  
Received bits: 06  75  00  81  02  80  66  fd  
Sent bits:     50  00  57  cd  
Received bits: 50  00  57  cd  

Found tag with
 UID: 08c7dff102fc6c
ATQA: 0304
 SAK: 20
 ATS: 06  75  00  81  02  80  66  fd
```
The DESFire support for the Chameleon Mini is tested with the LibNFC-based source code 
[developed in this directory](https://github.com/emsec/ChameleonMini/tree/master/Software/DESFireLibNFCTesting) with 
[sample dumps and output here](https://github.com/emsec/ChameleonMini/tree/master/Software/DESFireLibNFCTesting/SampleOutputDumps).
The Mifare DESFire commands installed by [LibFreefare](https://github.com/nfc-tools/libfreefare) 
do not work with the Chameleon Mini. 

The developers are actively working to ensure compatibility of the Chameleon DESFire emulation with external USB readers used 
running ``pcscd`` and ``pcsc_spy``. This support does not work with the HID Omnikey 5022CL reader. 
The ACS ACR-122U reader recognizes the Chameleon running the vanilla ``CONFIG=MF_DESFIRE`` over PCSC (driver ``pcscd``) 
as shown in the output of the ``pcsc_spy -v`` command:
```bash
$ sudo pcsc_scan -v
Using reader plug'n play mechanism
Scanning present readers...
Waiting for the first reader...found one
Scanning present readers...
0: ACS ACR122U PICC Interface 00 00
 
Mon Jul 25 19:26:28 2022
 Reader 0: ACS ACR122U PICC Interface 00 00
  Event number: 3
  Card state: Card removed, 
   
Mon Jul 25 19:26:37 2022
 Reader 0: ACS ACR122U PICC Interface 00 00
  Event number: 4
  Card state: Card inserted, 
  ATR: 3B 81 80 01 80 80

ATR: 3B 81 80 01 80 80
+ TS = 3B --> Direct Convention
+ T0 = 81, Y(1): 1000, K: 1 (historical bytes)
  TD(1) = 80 --> Y(i+1) = 1000, Protocol T = 0 
-----
  TD(2) = 01 --> Y(i+1) = 0000, Protocol T = 1 
-----
+ Historical bytes: 80
  Category indicator byte: 80 (compact TLV data object)
+ TCK = 80 (correct checksum)

Possibly identified card (using /usr/share/pcsc/smartcard_list.txt):
3B 81 80 01 80 80
	RFID - ISO 14443 Type A - NXP DESFire or DESFire EV1 or EV2
	"Reiner LoginCard" (or "OWOK", how they name it) - they have been distributed by a german computer magazine ("Computer BILD")
	https://cardlogin.reiner-sct.com/
	Belgium A-kaart (Antwerp citycard)
	Oyster card - Transport for London (second-gen "D")
	https://en.wikipedia.org/wiki/Oyster_card
	Kaba Legic Advant 4k
	Sydney Opal card public transport ticket (Transport)
	https://www.opal.com.au
	TH Köln (University of Applied Sciences Cologne) - Student Identity Card
	https://www.th-koeln.de/en/academics/multica_5893.php
	German red cross blood donation service
	http://www.blutspende-nordost.de/
	Greater Toronto/Hamilton/Ottawa PRESTO contactless fare card
	http://en.wikipedia.org/wiki/Presto_card
	Electic vehicle charging card of the EMSP EnBW Energie Baden-Württemberg AG, Tarif ADAC e-Charge, Germany
   
Mon Jul 25 19:26:37 2022
 Reader 0: ACS ACR122U PICC Interface 00 00
  Event number: 5
  Card state: Card removed, 
```

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

### Links to public datasheets and online specs 

The following links are the original online resource links are
archived here for documentation on how this firmware operates:
* [ISO/IEC 7816-4 Standard](http://www.unsads.com/specs/ISO/7816/ISO7816-4.pdf)
* [DESFire Functional specification (MF3ICD81, November 2008)](https://web.archive.org/web/20201115030854/https://marvin.blogreen.org/~romain/nfc/MF3ICD81%20-%20MIFARE%20DESFire%20-%20Functional%20specification%20-%20Rev.%203.5%20-%2028%20November%202008.pdf)
* [DESFire EV0 Datasheet (M075031, April 2004)](https://web.archive.org/web/20170201031920/http://neteril.org/files/M075031_desfire.pdf)
* [NXP Application Note AN12343](https://www.nxp.com/docs/en/application-note/AN12343.pdf) 
* [TI DESFire EV1 Tag AES Auth Specs (sloa213.pdf)](https://www.ti.com/lit/an/sloa213/sloa213.pdf)
* [NXP Application Note AN10833](https://www.nxp.com/docs/en/application-note/AN10833.pdf)
* My favorite conference submission in grad school is (by far) about this project -- even though I did not present my talk that year.
  In rare form, the [presentation slides (tentative; see uploads)](https://archive.org/details/@maxiedschmidt) and the 
  [accepted manuscript](https://archive.org/download/ftc2021-presentation-slides-with-notes/schmidt-ftc2021-submission.pdf) 
  (published in print form by Springer) document the scarce details of the DESFire spec and command sets gleaned while working 
  on this project as a conference proceedings article.
  Grace Hopper would have approved :)

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
