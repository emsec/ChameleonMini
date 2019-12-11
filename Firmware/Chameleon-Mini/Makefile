# This makefile uses sh in order to build the project.
# Note when using AVR-Toolchain, you have to install cygwin and
# append cygwin's bin directory to the PATH environment variable
# because Atmel does not ship sh.exe anymore with the toolchain.
SHELL = /bin/sh

#Supported configurations
SETTINGS    += -DCONFIG_MF_CLASSIC_MINI_4B_SUPPORT
SETTINGS    += -DCONFIG_MF_CLASSIC_1K_SUPPORT
SETTINGS    += -DCONFIG_MF_CLASSIC_1K_7B_SUPPORT
SETTINGS    += -DCONFIG_MF_CLASSIC_4K_SUPPORT
SETTINGS    += -DCONFIG_MF_CLASSIC_4K_7B_SUPPORT
SETTINGS    += -DCONFIG_MF_ULTRALIGHT_SUPPORT
SETTINGS	+= -DCONFIG_ISO14443A_SNIFF_SUPPORT
SETTINGS	+= -DCONFIG_ISO14443A_READER_SUPPORT
SETTINGS	+= -DCONFIG_VICINITY_SUPPORT
SETTINGS	+= -DCONFIG_SL2S2002_SUPPORT
SETTINGS	+= -DCONFIG_TITAGITSTANDARD_SUPPORT
SETTINGS	+= -DCONFIG_ISO15693_SNIFF_SUPPORT
SETTINGS	+= -DCONFIG_EM4233_SUPPORT

#Support magic mode on mifare classic configuration
SETTINGS	+= -DSUPPORT_MF_CLASSIC_MAGIC_MODE

#Don't touch manufacturer byte with BUTTON_ACTION_UID_LEFT_(DE/IN)CREMENT
SETTINGS	+= -DSUPPORT_UID7_FIX_MANUFACTURER_BYTE

#Support activating firmware upgrade mode through command-line
SETTINGS	+= -DSUPPORT_FIRMWARE_UPGRADE

#Default configuration
#SETTINGS   += -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_MINI_4B
#SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_1K
#SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_4K
#SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_MF_ULTRALIGHT
SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_NONE
#SETTINGS	+= -DDEFAULT_CONFIGURATION=CONFIG_ISO14443A_READER

#Default button actions
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_RANDOM
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_LEFT_INCREMENT
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_RIGHT_INCREMENT
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_LEFT_DECREMENT
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_RIGHT_DECREMENT
SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_CYCLE_SETTINGS
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_STORE_MEM
SETTINGS	+= -DDEFAULT_LBUTTON_ACTION=BUTTON_ACTION_RECALL_MEM

#Define if button action setting should be independent of active setting
SETTINGS	+= -DBUTTON_SETTING_GLOBAL

#Default LED functions
SETTINGS	+= -DDEFAULT_RED_LED_ACTION=LED_SETTING_CHANGE
SETTINGS	+= -DDEFAULT_GREEN_LED_ACTION=LED_POWERED

#Define if LED function setting should be independent of active setting
SETTINGS	+= -DLED_SETTING_GLOBAL

#Default logging mode
SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_OFF
#SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_MEMORY
#SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_TERMINAL

#Define if log settings should be global
SETTINGS	+= -DLOG_SETTING_GLOBAL

#Default setting
SETTINGS	+= -DDEFAULT_SETTING=SETTINGS_FIRST

#Default pending task timeout
SETTINGS	+= -DDEFAULT_PENDING_TASK_TIMEOUT=50 #* 100ms

#Default reader threshold
SETTINGS	+= -DDEFAULT_READER_THRESHOLD=400

#Use EEPROM to store settings
SETTINGS	+= -DENABLE_EEPROM_SETTINGS

#Memory definitions and objcopy flags to include sections in binaries
FLASH_DATA_ADDR = 0x10000 #Start of data section in flash
FLASH_DATA_SIZE = 0x10000 #Size of data section in flash
FLASH_DATA_OBJCOPY = --set-section-flags=.flashdata="alloc,load"
SPM_HELPER_ADDR = 0x21FE0 #Start of SPM helper section. Should be last 32Byte in bootloader section
SPM_HELPER_OBJCOPY = --set-section-flags=.spmhelper="alloc,load"

#Build configuration
ifeq ($(OS),Windows_NT)
BUILD_DATE   = "\"$(shell date /t)\""
else
BUILD_DATE   = $(shell date +'\"%y%m%d\"')
endif
COMMIT_ID	 = $(shell git rev-parse --short HEAD)
MCU          = atxmega128a4u
ARCH         = XMEGA
BOARD        = NONE
F_CPU        = 27120000
F_USB        = 48000000
TARGET       = Chameleon-Mini
OPTIMIZATION = s
SRC         += $(TARGET).c LUFADescriptors.c System.c ISRSharing.S Configuration.c Random.c Common.c Memory.c MemoryAsm.S Button.c Log.c Settings.c LED.c Map.c AntennaLevel.c
SRC         += Terminal/Terminal.c Terminal/Commands.c Terminal/XModem.c Terminal/CommandLine.c
SRC         += Codec/Codec.c Codec/ISO14443-2A.c Codec/Reader14443-2A.c Codec/SniffISO14443-2A.c Codec/Reader14443-ISR.S
SRC         += Application/MifareUltralight.c Application/MifareClassic.c Application/ISO14443-3A.c Application/Crypto1.c Application/Reader14443A.c Application/Sniff14443A.c Application/CryptoTDEA.S
SRC         += Codec/ISO15693.c
SRC         += Application/Vicinity.c Application/Sl2s2002.c Application/TITagitstandard.c Application/ISO15693-A.c Application/EM4233.c
SRC         += $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
LUFA_PATH    = ../LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -DFLASH_DATA_ADDR=$(FLASH_DATA_ADDR) -DFLASH_DATA_SIZE=$(FLASH_DATA_SIZE) -DSPM_HELPER_ADDR=$(SPM_HELPER_ADDR) -DBUILD_DATE=$(BUILD_DATE) -DCOMMIT_ID=\"$(COMMIT_ID)\" $(SETTINGS)
LD_FLAGS     = -Wl,--section-start=.flashdata=$(FLASH_DATA_ADDR) -Wl,--section-start=.spmhelper=$(SPM_HELPER_ADDR)
OBJDIR       = Bin
OBJECT_FILES =


#AVRDUDE settings
AVRDUDE_PROGRAMMER = flip2
AVRDUDE_MCU = atxmega128a4u
AVRDUDE_PORT = usb
AVRDUDE_WRITE_APP = -U application:w:$(TARGET).hex
AVRDUDE_WRITE_EEPROM = -U eeprom:w:$(TARGET).eep
AVRDUDE_FLAGS = -p $(AVRDUDE_MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)

#AVRDUDE settings to program the precompiled firmware
AVRDUDE_WRITE_APP_LATEST = -U application:w:Latest/Chameleon-Mini.hex
AVRDUDE_WRITE_EEPROM_LATEST = -U eeprom:w:Latest/Chameleon-Mini.eep


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

#Overwrite the LUFA versions of hex/bin file generation to include spmhelper and flashdata sections
%.hex: %.elf
	@echo $(MSG_OBJCPY_CMD) Extracting HEX file data from \"$<\"
	$(CROSS)-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature $(FLASH_DATA_OBJCOPY) $< $@

%.bin: %.elf
	@echo $(MSG_OBJCPY_CMD) Extracting BIN file data from \"$<\"
	$(CROSS)-objcopy -O binary -R .eeprom -R .fuse -R .lock -R .signature $(FLASH_DATA_OBJCOPY) $< $@

# Extract SPMHelper in the last 32 Byte of the bootloader section to externally combine it with any bootloader
spmhelper: $(TARGET).elf
	@echo $(MSG_OBJCPY_CMD) Extracting SPM helper HEX file from $(TARGET).elf
	$(CROSS)-objcopy -O ihex -j .spmhelper $(SPM_HELPER_OBJCOPY) $(TARGET).elf $(TARGET).hex

# Program the device using avrdude
program: $(TARGET).hex $(TARGET).eep
	avrdude $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_APP) $(AVRDUDE_WRITE_EEPROM)

# Program the device using avrdude with the latest official firmware
program-latest:
	avrdude $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_APP_LATEST) $(AVRDUDE_WRITE_EEPROM_LATEST)

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

style:
	# Make sure astyle is installed
	@which astyle >/dev/null || ( echo "Please install 'astyle' package first" ; exit 1 )
	# Remove spaces & tabs at EOL, add LF at EOF if needed on *.c, *.h, Makefile
	find . \( -name "*.[ch]" -or -name "Makefile" \) \
	    -exec perl -pi -e 's/[ \t]+$$//' {} \; \
	    -exec sh -c "tail -c1 {} | xxd -p | tail -1 | grep -q -v 0a$$" \; \
	    -exec sh -c "echo >> {}" \;
	# Apply astyle on *.c, *.h
	find . -name "*.[ch]" -exec astyle --formatted --mode=c --suffix=none \
	    --indent=spaces=4 --indent-switches \
	    --keep-one-line-blocks --max-instatement-indent=60 \
	    --style=google --pad-oper --unpad-paren --pad-header \
	    --align-pointer=name {} \;
