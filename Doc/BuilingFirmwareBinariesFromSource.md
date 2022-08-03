# Building the Chameleon Mini RevG firmware from source

Users that are preparing to reflash their older generation RevE devices should 
see [this software](https://github.com/iceman1001/ChameleonMini-rebooted) 
for the latest sources to compile fresh binaries to flash onto their Chameleon Mini. 
Because the AVR chips on the Chameleon Mini RevE devices have less memory, the configurations 
such as those for 
[DESFire tags](https://github.com/emsec/ChameleonMini/blob/master/Doc/DESFireSupportReadme.md) 
will (most likely) not work if the firmware binaries are built 
using the source code in this repository. 

## Prerequisites

Users need to install ``avr-gcc`` developer packages and GNU ``make`` to compile the sources.
To flash the firmware onto the Chameleon over USB, users will need to install software such as 
``avrdude``. On Linux and Unix systems, users will likely need to setup extra ``udev`` rules on their 
system so the Chameleon Mini is recognized as a USB device in bootloader (flash) and runtime modes. 
Details on this configuration are found on the 
[getting started WIKI](https://rawgit.com/emsec/ChameleonMini/master/Doc/Doxygen/html/_page__getting_started.html).

## Cloning

```bash
$ git clone ttps://@github.com/emsec/ChameleonMini.git
$ cd ChameleonMini/Firmware/ChameleonMini
```
If you are working from an older cloned source, make sure to update to the latest by running 
```bash
$ git pull
```

## Customizing the build

The complexity and memory requirements needed to have all possible 
[configurations](https://rawgit.com/emsec/ChameleonMini/master/Doc/Doxygen/html/_page__configurations.html) 
(list not comprehensive) 
enabled for use on the Chameleon are too demanding for the onboard AVR chip. 
Users will have to choose a subset that includes support for only a few configurations at 
a time (reflash to use firmware built with others). 
There are several custom targets that make building specialized firmware possible. 
They include one of the following strings (henceforth ``BUILD_TARGET``):
```bash
mifare, mifare-classic, desfire, desfire-dev, iso-modes, ntag215, vicinity, sls2s2002, titagit, em4233, misc-tags
```
Precise up-to-date information about which configurations are supported by each build variant are found by reviewing the 
[build script source](https://github.com/emsec/ChameleonMini/blob/master/Firmware/Chameleon-Mini/BuildScripts/custom_build_targets.mk). 
The per-build configuration lists are currently as follows:

* ``mifare``: ``MF_CLASSIC_MINI_4B``, ``MF_CLASSIC_1K``, ``MF_CLASSIC_1K_7B``, ``MF_CLASSIC_4K``, ``MF_CLASSIC_4K_7B`` and ``MF_ULTRALIGHT``
* ``mifare-classic``: ``MF_CLASSIC_MINI_4B``, ``MF_CLASSIC_1K``, ``MF_CLASSIC_1K_7B``, ``MF_CLASSIC_4K`` and ``MF_CLASSIC_4K_7B``
* ``desfire``, ``desfire-dev``: ``MF_DESFIRE``, ``MF_DESFIRE_2KEV1``, ``MF_DESFIRE_4KEV1`` and ``MF_DESFIRE_4KEV2``
* ``iso-modes``: ``ISO14443A_SNIFF``, ``ISO14443A_READER``, ``ISO15693_SNIFF`` and ``MF_ULTRALIGHT``
* ``ntag215``: ``NTAG215``
* ``vicinity``: ``VICINITY``
* ``sl2s2002``: ``SL2S2002``
* ``titagit``: ``TITAGITSTANDARD`` and ``TITAGITPLUS``
* ``em4233``: ``EM4233``
* ``misc-tags``: ``NTAG215``, ``VICINITY``, ``SL2S2002``, ``TITAGITSTANDARD``, ``TITAGITPLUS`` and ``EM4233``

### Choosing prepacakaged firmware binaries

Latest builds supporting ISO14443, ISO1593 and DESFire (non development) are generated automatically on the 
main Chameleon Mini firmware repository 
(see [this listing](https://github.com/emsec/ChameleonMini/actions) and the [active latest firmware builds here](https://github.com/emsec/ChameleonMini/releases)). 

### More customized builds

Users that wish to build a hybrid of any of the above ``make`` targets may edit the 
``Makefile`` in the current working directory (as set in the cloning step above) and set the 
``BUILD_TARGET`` varaible to empty (i.e., build the source by just running ``make`` below). 
*Caveat emptor*: the warning is again that enabling too much functionality in the firmware build may cause 
errors due to memory restrictions. 

## Compiling the source

Build the source by running
```bash
$ make $BUILD_TARGET
```
For example, to build the firmware with DESFire support and extra printing of debugging information that 
can be printed with ``LOGMODE=LIVE`` and viewed with the 
[Chameleon Mini Live Debugger](https://github.com/maxieds/ChameleonMiniLiveDebugger) 
application for Android phones, we run
```bash
$ make desfire-dev
```

## Flashing the firmware 

See the [getting started documentation](https://rawgit.com/emsec/ChameleonMini/master/Doc/Doxygen/html/_page__getting_started.html) 
for more information. The flash command using ``avrdude`` on Linux is the following:
```bash
$ export FIRMWARE_TARGET=Chameleon-Mini
$ sudo avrdude -c flip2 -p ATXMega128A4U -B 60 -P usb -U application:w:$FIRMWARE_TARGET.hex:i -U eeprom:w:$FIRMWARE_TARGET.eep:i
```
More information about flashing Chameleon devices on odd platforms and hardware setups is 
[found here (Linux/Unix)](https://github.com/iceman1001/ChameleonMini-rebooted/wiki/Flashing-Linux-(Unix)) and [here (Mac OSX)](https://github.com/iceman1001/ChameleonMini-rebooted/wiki/Flashing-OSX).

## Getting up and running with the Chameleon Mini over serial USB

Users can install ``minicom`` to interface to the Chameleon Mini. 
Configuration details are OS specific and are found elsewhere. 
Alternately, if users wish to use a portable interface and log viewer on their 
Android device with Google Play Store, see the 
[CMLD application WIKI](https://github.com/maxieds/ChameleonMiniLiveDebugger/wiki/GettingStarted).
Python-based software to download and view the logs on the Chameleon is located 
[in this directory](https://github.com/emsec/ChameleonMini/tree/master/Software/ChamTool). 
Sample dumps for several configuration types that can be uploaded onto the running 
Chameleon device are [located here](https://github.com/emsec/ChameleonMini/tree/master/Dumps).
