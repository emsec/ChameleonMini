# This makefile uses sh in order to build the project.
# Note when using AVR-Toolchain, you have to install cygwin and 
# append cygwin's bin directory to the PATH environment variable 
# because Atmel does not ship sh.exe anymore with the toolchain. 
SHELL = /bin/sh

#Supported configurations
SETTINGS    += -DCONFIG_MF_CLASSIC_1K_SUPPORT
SETTINGS    += -DCONFIG_MF_Plus1k_7B_SUPPORT
SETTINGS    += -DCONFIG_MF_CLASSIC_4K_SUPPORT
SETTINGS    += -DCONFIG_MF_ULTRALIGHT_SUPPORT 
SETTINGS	+= -DCONFIG_ISO14443A_SNIFF_SUPPORT

#Support magic mode on mifare classic configuration
SETTINGS	+= -DSUPPORT_MF_CLASSIC_MAGIC_MODE

#Support activating firmware upgrade mode through command-line
SETTINGS	+= -DSUPPORT_FIRMWARE_UPGRADE

#Default configuration
#SETTINGS += -DDEFAULT_CONFIGURATION=CONFIG_MF_Plus1k_7B
#SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_1K
#SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_4K
#SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_MF_ULTRALIGHT
SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_NONE

#Default button actions
#SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_UID_RANDOM
#SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_UID_LEFT_INCREMENT
#SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_UID_RIGHT_INCREMENT
#SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_UID_LEFT_DECREMENT
#SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_UID_RIGHT_DECREMENT
SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_CYCLE_SETTINGS
#SETTINGS	+= -DDEFAULT_BUTTON_ACTION=BUTTON_ACTION_STORE_MEM
SETTINGS	+= -DDEFAULT_BUTTON_LONG_ACTION=BUTTON_ACTION_RECALL_MEM

#Define if button action setting should be independent of active setting 
SETTINGS	+= -DBUTTON_SETTING_GLOBAL

#Default LED functions
	
#SETTINGS	+= -DDEFAULT_RED_LED_ACTION=LED_TERMINAL_CONN 
#SETTINGS	+= -DDEFAULT_RED_LED_ACTION=LED_TERMINAL_RXTX
SETTINGS	+= -DDEFAULT_RED_LED_ACTION=LED_SETTING_CHANGE
#SETTINGS	+= -DDEFAULT_RED_LED_ACTION=LED_MEMORY_STORED
#SETTINGS	+= -DDEFAULT_RED_LED_ACTION=LED_MEMORY_CHANGED
#SETTINGS	+= -DDEFAULT_RED_LED_ACTION=LED_CODEC_RX
#SETTINGS	+= -DDEFAULT_RED_LED_ACTION=LED_CODEC_TX	

SETTINGS	+= -DDEFAULT_GREEN_LED_ACTION=LED_TERMINAL_CONN 
#SETTINGS	+= -DDEFAULT_GREEN_LED_ACTION=LED_TERMINAL_RXTX
#SETTINGS	+= -DDEFAULT_GREEN_LED_ACTION=LED_SETTING_CHANGE
#SETTINGS	+= -DDEFAULT_GREEN_LED_ACTION=LED_MEMORY_STORED
#SETTINGS	+= -DDEFAULT_GREEN_LED_ACTION=LED_MEMORY_CHANGED
#SETTINGS	+= -DDEFAULT_GREEN_LED_ACTION=LED_CODEC_RX
#SETTINGS	+= -DDEFAULT_GREEN_LED_ACTION=LED_CODEC_TX	


#Define if LED function setting should be independent of active setting
SETTINGS	+= -DLED_SETTING_GLOBAL

#Default logging mode
SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_OFF
#SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_MEMORY
#SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_TERMINAL

#Default setting
SETTINGS	+= -DDEFAULT_SETTING=SETTINGS_FIRST

#Use EEPROM to store settings
SETTINGS	+= -DENABLE_EEPROM_SETTINGS

#Build configuration
BUILD_DATE	 = $(shell date +'\"%y%m%d\"')
MCU          = atxmega32a4u #atxmega128a4u
ARCH         = XMEGA
BOARD        = NONE
F_CPU        = 32000000
F_USB        = 48000000
TARGET       = Chameleon-Mini
OPTIMIZATION = 3
SRC         += $(TARGET).c LUFADescriptors.c System.c Configuration.c Random.c Common.c Memory.c Button.c Log.c Settings.c LED.c
SRC         += Terminal/Terminal.c Terminal/Commands.c Terminal/XModem.c Terminal/CommandLine.c
SRC         += Codec/Codec.c Codec/ISO14443-2A.c
SRC         += Application/MifareUltralight.c Application/MifareClassic.c Application/ISO14443-3A.c Application/Crypto1.c
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
