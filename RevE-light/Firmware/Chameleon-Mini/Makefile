# This makefile uses sh in order to build the project.
# Note when using AVR-Toolchain, you have to install cygwin and 
# append cygwin's bin directory to the PATH environment variable 
# because Atmel does not ship sh.exe anymore with the toolchain. 
SHELL = /bin/sh

# Supported configurations
SETTINGS    += -DCONFIG_MF_CLASSIC_1K_SUPPORT
SETTINGS    += -DCONFIG_MF_Plus1k_7B_SUPPORT
SETTINGS    += -DCONFIG_MF_CLASSIC_4K_SUPPORT
SETTINGS    += -DCONFIG_MF_ULTRALIGHT_SUPPORT 
SETTINGS	+= -DCONFIG_ISO14443A_SNIFF_SUPPORT

# Support magic mode on mifare classic configuration
SETTINGS	+= -DSUPPORT_MF_CLASSIC_MAGIC_MODE

# Support activating firmware upgrade mode through command-line
SETTINGS	+= -DSUPPORT_FIRMWARE_UPGRADE

# Default configuration
#SETTINGS += -DDEFAULT_CONFIGURATION=CONFIG_MF_Plus1k_7B
#SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_1K
#SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_4K
#SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_MF_ULTRALIGHT
SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_1K

# Default button actions
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_RANDOM
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_LEFT_INCREMENT
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_RIGHT_INCREMENT
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_LEFT_DECREMENT
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_RIGHT_DECREMENT
SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_CYCLE_SETTINGS
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_STORE_MEM
SETTINGS	+= -DDEFAULT_LBUTTON_ACTION=BUTTON_ACTION_STORE_MEM

# Define if button action setting should be independent of active setting 
SETTINGS	+= -DBUTTON_SETTING_GLOBAL

# Default LED functions
	
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


# Define if LED function setting should be independent of active setting
SETTINGS	+= -DLED_SETTING_GLOBAL

# Default logging mode
SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_OFF
#SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_MEMORY
#SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_TERMINAL

# Default setting
SETTINGS	+= -DDEFAULT_SETTING=SETTINGS_FIRST

# Use EEPROM to store settings
SETTINGS	+= -DENABLE_EEPROM_SETTINGS


# Memory definitions and objcopy flags to include sections in binaries
FLASH_DATA_ADDR = 0x10000 # Start of data section in flash
FLASH_DATA_SIZE = 0x8000 # Size of data section in flash
FLASH_DATA_OBJCOPY = --set-section-flags=.flashdata="alloc,load"
SPM_HELPER_ADDR = 0x21FE0 # Start of SPM helper section. Should be last 32Byte in bootloader section
SPM_HELPER_OBJCOPY = --set-section-flags=.spmhelper="alloc,load"


# Build configuration
BUILD_DATE	 = $(shell date +'\"%y%m%d\"')
MCU          = atxmega128a4u
ARCH         = XMEGA
BOARD        = NONE
F_CPU        = 32000000
F_USB        = 48000000
TARGET       = Chameleon-Mini
OPTIMIZATION = 3

# Core files
SRC         += $(TARGET).c 
SRC         += Button.c 
SRC         += Common.c 
SRC         += Configuration.c 
SRC         += LED.c
SRC         += LUFADescriptors.c 
SRC         += Log.c 
SRC         += Map.c 
SRC         += Memory.c 
SRC         += MemoryAsm.S 
SRC         += Random.c 
SRC         += Settings.c 
SRC         += System.c 

# Terminal
SRC         += Terminal/Terminal.c 
SRC         += Terminal/Commands.c 
SRC         += Terminal/XModem.c 
SRC         += Terminal/CommandLine.c

# Codecs
SRC         += Codec/Codec.c 
SRC         += Codec/ISO14443-2A.c

# Applications
SRC         += Application/MifareUltralight.c 
SRC         += Application/MifareClassic.c 
SRC         += Application/ISO14443-3A.c 
SRC         += Application/Crypto1.c

# LUFA
SRC         += $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
LUFA_PATH    = ../../../Firmware/LUFA

# General linker / compiler settings
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -DFLASH_DATA_ADDR=$(FLASH_DATA_ADDR) -DFLASH_DATA_SIZE=$(FLASH_DATA_SIZE) -DSPM_HELPER_ADDR=$(SPM_HELPER_ADDR) -DBUILD_DATE=$(BUILD_DATE) $(SETTINGS) -I$(LUFA_PATH)
LD_FLAGS     = -Wl,--section-start=.flashdata=$(FLASH_DATA_ADDR) -Wl,--section-start=.spmhelper=$(SPM_HELPER_ADDR)
OBJDIR       = Bin
OBJECT_FILES = 

#AVRDUDE settings
AVRDUDE_PROGRAMMER = jtag2pdi #avrisp2
AVRDUDE_MCU = atxmega128a4
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

# Overwrite the LUFA versions of hex/bin file generation to include spmhelper and flashdata sections
%.hex: %.elf
	@echo $(MSG_OBJCPY_CMD) Chameleon: Extracting HEX file data from \"$<\"
	@$(CROSS)-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature $(FLASH_DATA_OBJCOPY) $< $@

%.bin: %.elf
	@echo $(MSG_OBJCPY_CMD) Chameleon: Extracting BIN file data from \"$<\"
	@$(CROSS)-objcopy -O binary -R .eeprom -R .fuse -R .lock -R .signature $(FLASH_DATA_OBJCOPY) $< $@

# Extract SPMHelper in the last 32 Byte of the bootloader section to externally combine it with any bootloader
spmhelper: $(TARGET).elf
	@echo $(MSG_OBJCPY_CMD) Chameleon: Extracting SPM helper HEX file from $(TARGET).elf
	@$(CROSS)-objcopy -O ihex -j .spmhelper $(SPM_HELPER_OBJCOPY) $(TARGET).elf $(TARGET).hex

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
