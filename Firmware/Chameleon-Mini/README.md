Compiling the firmware
======================

Windows
-------
For compiling under Windows you need to do the following:
1. Install AVR GNU Toolchain from Atmel
2. Install Cygwin for sh.exe (base installation is sufficient)
3. Add the bin path of cygwin (containing sh.exe) to the PATH environment variable for MAKE to find it

Linux
-----
For compiling under Linux you need to execute the following:

`sudo apt-get install gcc-avr binutils-avr gdb-avr avr-libc avrdude`

Then you should be good to go to build the firmware by typing in the following:

`make`
