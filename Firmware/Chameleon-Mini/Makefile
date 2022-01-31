# This makefile uses sh in order to build the project.
# Note when using AVR-Toolchain, you have to install cygwin and
# append cygwin's bin directory to the PATH environment variable
# because Atmel does not ship sh.exe anymore with the toolchain.
SHELL = /bin/sh

#Supported configurations
CONFIG_SETTINGS  += -DCONFIG_MF_CLASSIC_MINI_4B_SUPPORT
CONFIG_SETTINGS  += -DCONFIG_MF_CLASSIC_1K_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_MF_CLASSIC_1K_7B_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_MF_CLASSIC_4K_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_MF_CLASSIC_4K_7B_SUPPORT
CONFIG_SETTINGS  += -DCONFIG_MF_ULTRALIGHT_SUPPORT
#CONFIG_SETTINGS  += -DCONFIG_ISO14443A_SNIFF_SUPPORT
CONFIG_SETTINGS  += -DCONFIG_ISO14443A_READER_SUPPORT
CONFIG_SETTINGS  += -DCONFIG_NTAG21x_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_VICINITY_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_SL2S2002_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_TITAGITSTANDARD_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_TITAGITPLUS_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_ISO15693_SNIFF_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_EM4233_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_MF_DESFIRE_SUPPORT

#Default configuration
#CONFIG_SETTINGS    += -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_MINI_4B
#CONFIG_SETTINGS    += -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_1K
#CONFIG_SETTINGS    += -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_4K
#CONFIG_SETTINGS    += -DDEFAULT_CONFIGURATION=CONFIG_MF_ULTRALIGHT
#CONFIG_SETTINGS    += -DDEFAULT_CONFIGURATION=CONFIG_ISO14443A_READER
CONFIG_SETTINGS     += -DDEFAULT_CONFIGURATION=CONFIG_NONE

#Support magic mode on mifare classic configuration
SETTINGS	+= -DSUPPORT_MF_CLASSIC_MAGIC_MODE

#Don't touch manufacturer byte with BUTTON_ACTION_UID_LEFT_(DE/IN)CREMENT
SETTINGS	+= -DSUPPORT_UID7_FIX_MANUFACTURER_BYTE

#Support activating firmware upgrade mode through command-line
SETTINGS	+= -DSUPPORT_FIRMWARE_UPGRADE

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
#SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_LIVE

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

#Enable tests for DES/2KTDEA/3DES/AES128 crypto schemes:
#SETTINGS  += -DENABLE_CRYPTO_TESTS

#Enable a command to run any tests added by developers, e.g., the
#crypto scheme tests that can be enabled above:
#SETTINGS  += -DENABLE_RUNTESTS_TERMINAL_COMMAND

#Whether or not to allow users Chameleon terminal access to change the DESFire configuration's
#sensitive settings like manufacturer, serial number, etc.
#SETTINGS += -DDISABLE_PERMISSIVE_DESFIRE_SETTINGS
#SETTINGS += -DDISABLE_DESFIRE_TERMINAL_COMMANDS

#Set a default logging mode for debugging with the DESFire
#emulation code:
SETTINGS  += -DDESFIRE_DEFAULT_LOGGING_MODE=DEBUGGING
#SETTINGS += -DDESFIRE_DEFAULT_LOGGING_MODE=OFF

#Set a default testing mode setting (0 = OFF, non-NULL = ON):
SETTINGS  += -DDESFIRE_DEFAULT_TESTING_MODE=0
#SETTINGS  += -DDESFIRE_DEFAULT_TESTING_MODE=1

#Feature: Use randomized UIDs that mask the actual secret UID until
#the tag has been issued a successful authentication sequence:
#SETTINGS += -DDESFIRE_RANDOMIZE_UIDS_PREAUTH

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
SETTINGS  += -DDESFIRE_MIN_INCOMING_LOGSIZE=1
SETTINGS  += -DDESFIRE_MIN_OUTGOING_LOGSIZE=1
#SETTINGS  += -DDESFIRE_MIN_OUTGOING_LOGSIZE=0

#Enable printing of crypto tests when a new DESFire emulation instance is started:
#SETTINGS += -DDESFIRE_RUN_CRYPTO_TESTING_PROCEDURE

#Option to save space with the "Application/Crypto1.c" code by storing large tables
#in PROGMEM. Note that this will slow down the read times when accessing these tables:
#SETTINGS  += -DDESFIRE_CRYPTO1_SAVE_SPACE

DESFIRE_MAINSRC = Application/DESFire

DESFIRE_CONFIG_SETTINGS_BASE = $(SETTINGS) -DCONFIG_MF_DESFIRE_SUPPORT -DDESFIRE_CRYPTO1_SAVE_SPACE -UDEFAULT_CONFIGURATION

#Memory definitions and objcopy flags to include sections in binaries
FLASH_DATA_ADDR = 0x10000  #Start of data section in flash
FLASH_DATA_SIZE = 0x10000 #Size of data section in flash
#FLASH_DATA_SIZE = 0x0FFFF  #Size of data section in flash
FLASH_DATA_OBJCOPY = --set-section-flags=.flashdata="alloc,load"
SPM_HELPER_ADDR = 0x21FE0 #Start of SPM helper section. Should be last 32Byte in bootloader section
SPM_HELPER_OBJCOPY = --set-section-flags=.spmhelper="alloc,load"

#Build configuration
ifeq ($(OS),Windows_NT)
BUILD_DATE   = "\"$(shell date /t)\""
else
BUILD_DATE   = $(shell date +'\"%y%m%d\"')
endif
COMMIT_ID    = $(shell git rev-parse --short HEAD)
MCU          = atxmega128a4u
ARCH         = XMEGA
BOARD        = NONE
F_CPU        = 27120000
F_USB        = 48000000
TARGET       = Chameleon-Mini
OPTIMIZATION = s
SRC         += $(TARGET).c LUFADescriptors.c System.c ISRSharing.S Configuration.c Random.c Common.c \
			Memory.c MemoryAsm.S Button.c Log.c Settings.c LED.c Pin.c Map.c AntennaLevel.c
SRC         += Terminal/Terminal.c Terminal/Commands.c Terminal/XModem.c Terminal/CommandLine.c
SRC         += Codec/Codec.c Codec/ISO14443-2A.c Codec/Reader14443-2A.c Codec/SniffISO14443-2A.c Codec/Reader14443-ISR.S
SRC         += Application/MifareUltralight.c Application/MifareClassic.c Application/ISO14443-3A.c \
			Application/Crypto1.c Application/Reader14443A.c Application/Sniff14443A.c \
			Application/CryptoTDEA-HWAccelerated.S Application/CryptoTDEA.c Application/CryptoAES128.c \
			Tests/CryptoTests.c Tests/ChameleonTerminal.c
SRC		      += Application/NTAG21x.c
SRC         += Codec/ISO15693.c Codec/SniffISO15693.c
SRC         += Application/Vicinity.c Application/Sl2s2002.c Application/TITagitstandard.c Application/TITagitplus.c Application/ISO15693-A.c Application/EM4233.c Application/Sniff15693.c
SRC	       += $(DESFIRE_MAINSRC)/../MifareDESFire.c \
               $(DESFIRE_MAINSRC)/DESFireApplicationDirectory.c \
               $(DESFIRE_MAINSRC)/DESFireChameleonTerminal.c \
               $(DESFIRE_MAINSRC)/DESFireCrypto.c \
               $(DESFIRE_MAINSRC)/DESFireFile.c \
               $(DESFIRE_MAINSRC)/DESFireISO14443Support.c \
               $(DESFIRE_MAINSRC)/DESFireISO7816Support.c \
               $(DESFIRE_MAINSRC)/DESFireInstructions.c \
               $(DESFIRE_MAINSRC)/DESFireLogging.c \
               $(DESFIRE_MAINSRC)/DESFireMemoryOperations.c \
               $(DESFIRE_MAINSRC)/DESFirePICCControl.c \
               $(DESFIRE_MAINSRC)/DESFirePICCHeaderLayout.c \
               $(DESFIRE_MAINSRC)/DESFireUtils.c
SRC         += $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
LUFA_PATH    = ../LUFA
CC_FLAGS     = -g0 -DUSE_LUFA_CONFIG_HEADER -DFLASH_DATA_ADDR=$(FLASH_DATA_ADDR) -DFLASH_DATA_SIZE=$(FLASH_DATA_SIZE) \
			   -DSPM_HELPER_ADDR=$(SPM_HELPER_ADDR) -DBUILD_DATE=$(BUILD_DATE) -DCOMMIT_ID=\"$(COMMIT_ID)\" $(SETTINGS) \
			   $(CONFIG_SETTINGS) $(SETTINGS) \
			   -D__AVR_ATxmega128A4U__ -D__PROG_TYPES_COMPAT__ -DMAX_ENDPOINT_INDEX=4 \
			   -std=gnu99 -Werror=implicit-function-declaration \
			   -fno-inline-small-functions -fshort-enums -fpack-struct \
                  	   -ffunction-sections -Wl,--gc-sections --data-sections -ffunction-sections \
                           -Wl,-relax -fno-split-wide-types -fno-tree-scev-cprop \
			   -fno-aggressive-loop-optimizations
LD_FLAGS     = $(CC_FLAGS) -Wl,--section-start=.flashdata=$(FLASH_DATA_ADDR) -Wl,--section-start=.spmhelper=$(SPM_HELPER_ADDR)
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

.PHONY: program program-latest dfu-flip dfu-prog dfu-reset check_size style

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
program: $(TARGET).hex $(TARGET).eep check_size
	avrdude $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_APP) $(AVRDUDE_WRITE_EEPROM)

# Program the device using avrdude with the latest official firmware
program-latest: check_size
	avrdude $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_APP_LATEST) $(AVRDUDE_WRITE_EEPROM_LATEST)

# Program the device using batchisp and the DFU bootloader
# Note that the device has to be in bootloader mode already
dfu-flip: $(TARGET).hex $(TARGET).eep check_size
	cp $(TARGET).eep EEPROM.hex
	batchisp -hardware usb -device $(MCU) -operation erase f memory FLASH loadbuffer $(TARGET).hex program verify memory EEPROM loadbuffer EEPROM.hex program verify start reset 0
	rm EEPROM.hex

# Program the device using dfu-programmer
dfu-prog: $(TARGET).hex $(TARGET).eep check_size
	dfu-programmer $(MCU) erase
	dfu-programmer $(MCU) flash-eeprom $(TARGET).eep
	dfu-programmer $(MCU) flash $(TARGET).hex
	dfu-programmer $(MCU) reset

# Reset the device after firmware upgrade
dfu-reset:
	dfu-programmer $(MCU) reset

check_size:
	@{ \
	set -e; \
	if [ ! -f $(TARGET).elf ]; then \
		exit 0; \
	fi; \
	PROGMEM_SIZE=$$(avr-size $(TARGET).elf | grep -oP "\d+" | sed -n 4p); \
	MAX_PRGMEM_SIZE=$$(printf "%d\n" $(FLASH_DATA_ADDR)); \
	if [ $$PROGMEM_SIZE -gt $$MAX_PRGMEM_SIZE ]; then \
		echo "make: *** $(TARGET).elf Application Section size $$PROGMEM_SIZE excedes maximum allowed $$MAX_PRGMEM_SIZE. Please disable some features in Makefile"; \
		exit 1; \
	fi; \
	}

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

local-clean:
	@rm -f $(TARGET)*.{elf,hex,eep,bin,lss,map}
	@rm -rf $(OBJDIR)
	@mkdir $(OBJDIR)

git-add-dev:
	@git add Makefile ./*.{c,h} ./*/*.{c,h} ./*/*/*.{c,h}

## Defining custom targets for the DESFire build (normal/user mode) and
## developer mode for use with the Android CMLD application that enables
## the printing of LIVE logs to the phone's console by default:
desfire-build: local-clean $(TARGET).elf $(TARGET).hex $(TARGET).eep
	@cp $(TARGET).hex $(TARGET)-DESFire.hex
	@cp $(TARGET).eep $(TARGET)-DESFire.eep
	@avr-size $(TARGET).elf
desfire: CONFIG_SETTINGS:=$(DESFIRE_CONFIG_SETTINGS_BASE) -DDEFAULT_CONFIGURATION=CONFIG_NONE
desfire: desfire-build
desfire-dev: CONFIG_SETTINGS:=$(DESFIRE_CONFIG_SETTINGS_BASE) -DDEFAULT_CONFIGURATION=CONFIG_MF_DESFIRE_SUPPORT
desfire-dev: desfire-build

