# Copyright 2013 Timo Kasper, Simon Küppers, David Oswald ("ORIGINAL
# AUTHORS"). All rights reserved.
# 
# DEFINITIONS:
# 
# "WORK": The material covered by this license includes the schematic
# diagrams, designs, circuit or circuit board layouts, mechanical
# drawings, documentation (in electronic or printed form), source code,
# binary software, data files, assembled devices, and any additional
# material provided by the ORIGINAL AUTHORS in the ChameleonMini project
# (https://github.com/skuep/ChameleonMini).
# 
# LICENSE TERMS:
# 
# Redistributions and use of this WORK, with or without modification, or
# of substantial portions of this WORK are permitted provided that the
# following conditions are met:
# 
# Redistributions and use of this WORK, with or without modification, or
# of substantial portions of this WORK must include the above copyright
# notice, this list of conditions, the below disclaimer, and the following
# attribution:
# 
# "Based on ChameleonMini an open-source RFID emulator:
# https://github.com/skuep/ChameleonMini"
# 
# The attribution must be clearly visible to a user, for example, by being
# printed on the circuit board and an enclosure, and by being displayed by
# software (both in binary and source code form).
# 
# At any time, the majority of the ORIGINAL AUTHORS may decide to give
# written permission to an entity to use or redistribute the WORK (with or
# without modification) WITHOUT having to include the above copyright
# notice, this list of conditions, the below disclaimer, and the above
# attribution.
# 
# DISCLAIMER:
# 
# THIS PRODUCT IS PROVIDED BY THE ORIGINAL AUTHORS "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE ORIGINAL AUTHORS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS PRODUCT, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
# 
# The views and conclusions contained in the hardware, software, and
# documentation should not be interpreted as representing official
# policies, either expressed or implied, of the ORIGINAL AUTHORS.

#Supported configurations
SETTINGS    += -DCONFIG_MF_CLASSIC_1K_SUPPORT
SETTINGS    += -DCONFIG_MF_CLASSIC_4K_SUPPORT

#Support activating firmware upgrade mode through command-line
SETTINGS	+= -DSUPPORT_FIRMWARE_UPGRADE

#Default configuration
SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_NONE

#Default button action
#SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_UID_RANDOM
#SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_UID_LEFT_INCREMENT
#SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_UID_RIGHT_INCREMENT
#SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_UID_LEFT_DECREMENT
#SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_UID_RIGHT_DECREMENT
SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_CYCLE_SETTINGS

#Default setting
SETTINGS	+= -DDEFAULT_SETTING=0

#Use EEPROM to store settings
SETTINGS	+= -DENABLE_EEPROM_SETTINGS

#Build configuration
BUILD_DATE	 = $(shell date +'\"%y%m%d\"')
MCU          = atxmega32a4u
ARCH         = XMEGA
BOARD        = NONE
F_CPU        = 32000000
F_USB        = 48000000
TARGET       = Chameleon-Mini
OPTIMIZATION = 3
SRC         += $(TARGET).c LUFADescriptors.c System.c Configuration.c Random.c Common.c Memory.c Button.c Settings.c LED.c
SRC         += Terminal/Terminal.c Terminal/Commands.c Terminal/XModem.c Terminal/CommandLine.c
SRC         += Codec/Codec.c Codec/ISO14443-2A.c
SRC         += AApplication/MifareClassic.c Application/ISO14443-3A.c Application/Crypto1.c
SRC         += $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
LUFA_PATH    = ../LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -DBUILD_DATE=$(BUILD_DATE) $(SETTINGS)
LD_FLAGS     =
OBJDIR       = Bin
OBJECT_FILES = 


#AVRDUDE settings
AVRDUDE_PROGRAMMER = jtag2pdi #avrisp2
AVRDUDE_MCU = atxmega32a4 #atxmega128a4
AVRDUDE_PORT = usb
AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex
AVRDUDE_WRITE_EEPROM = -U eeprom:w:$(TARGET).eep
AVRDUDE_FLAGS = -p $(AVRDUDE_MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)

# Default target
all:

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
# include $(LUFA_PATH)/Build/lufa_doxygen.mk
# include $(LUFA_PATH)/Build/lufa_dfu.mk
# include $(LUFA_PATH)/Build/lufa_hid.mk
# include $(LUFA_PATH)/Build/lufa_avrdude.mk
# include $(LUFA_PATH)/Build/lufa_atprogram.mk

# Program the device using avrdude  
program: $(TARGET).hex $(TARGET).eep
	avrdude $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM)

# Program the device using batchisp and the DFU bootloader
# Note that the device has to be in bootloader mode already
dfu-flip: $(TARGET).hex $(TARGET).eep
	cp $(TARGET).eep EEPROM.hex
	batchisp -hardware usb -device $(MCU) -operation erase f memory FLASH loadbuffer $(TARGET).hex program verify memory EEPROM loadbuffer EEPROM.hex program verify start reset 0
	rm EEPROM.hex

# Program the device using dfu-programmer
dfu-prog: $(TARGET).hex $(TARGET).eep
	dfu-programmer $(MCU) erase
	dfu-programmer $(MCU) flash-eeprom $(TARGET).eep
	dfu-programmer $(MCU) flash $(TARGET).hex
	dfu-programmer $(MCU) reset
