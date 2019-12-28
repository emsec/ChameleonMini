# Compiling the firmware

## Windows
For compiling under windows you need to do the following:
1. Install AVR GNU Toolchain from Atmel
2. Install Cygwin for sh.exe (base installation is sufficient)
3. Add the bin path of cygwin (containing sh.exe) to the PATH environment variable for MAKE to find it

Then you should be good to go to build the firmware by typing in the following:
> make

## Linux
### Arch
In Arch use your favorite AUR-Helper and install
* avr-gcc
* avr-libc

Afterwards go to this directory and run
> make

** Advanced** 
If you already know how to set your Chameleon into Bootloader mode, you can also use
> make program
to automatically upload the new firmware to the Chameleon.

# Where to go to from here
Afterwards you can start flashing following [this](https://rawgit.com/emsec/ChameleonMini/master/Doc/Doxygen/html/_page__getting_started.html) tutorial.
