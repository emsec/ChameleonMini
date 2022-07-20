## : This makefile uses sh in order to build the project.
## : Note when using AVR-Toolchain, you have to install cygwin and
## : append cygwin's bin directory to the PATH environment variable
## : because Atmel does not ship sh.exe anymore with the toolchain.
SHELL            = /bin/sh
CONFIG_SETTINGS ?=
SETTINGS        ?=

## : Supported configurations:
## : CAUTION: Enabling too many configurations in the firmware
## :          can/will/has been known to eat up too much memory
## :          on the Chameleon integrated AVR and lead to stability
## :          problems. Proceed carefully, or use one of the
## :          custom build targets found in the BuildScripts/* directory
CONFIG_SETTINGS  += -DCONFIG_MF_CLASSIC_MINI_4B_SUPPORT
CONFIG_SETTINGS  += -DCONFIG_MF_CLASSIC_1K_SUPPORT
CONFIG_SETTINGS += -DCONFIG_MF_CLASSIC_1K_7B_SUPPORT
CONFIG_SETTINGS += -DCONFIG_MF_CLASSIC_4K_SUPPORT
CONFIG_SETTINGS += -DCONFIG_MF_CLASSIC_4K_7B_SUPPORT
CONFIG_SETTINGS  += -DCONFIG_MF_ULTRALIGHT_SUPPORT
CONFIG_SETTINGS  += -DCONFIG_ISO14443A_SNIFF_SUPPORT
CONFIG_SETTINGS  += -DCONFIG_ISO14443A_READER_SUPPORT
CONFIG_SETTINGS += -DCONFIG_NTAG215_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_VICINITY_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_SL2S2002_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_TITAGITSTANDARD_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_TITAGITPLUS_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_ISO15693_SNIFF_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_EM4233_SUPPORT
#CONFIG_SETTINGS += -DCONFIG_MF_DESFIRE_SUPPORT

## : Default configuration
#CONFIG_SETTINGS    += -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_MINI_4B
#CONFIG_SETTINGS    += -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_1K
#CONFIG_SETTINGS    += -DDEFAULT_CONFIGURATION=CONFIG_MF_CLASSIC_4K
#CONFIG_SETTINGS    += -DDEFAULT_CONFIGURATION=CONFIG_MF_ULTRALIGHT
#CONFIG_SETTINGS    += -DDEFAULT_CONFIGURATION=CONFIG_ISO14443A_READER
CONFIG_SETTINGS     += -DDEFAULT_CONFIGURATION=CONFIG_NONE

## : Support magic mode on mifare classic configuration
SETTINGS	+= -DSUPPORT_MF_CLASSIC_MAGIC_MODE

## : Don't touch manufacturer byte with BUTTON_ACTION_UID_LEFT_(DE/IN)CREMENT
SETTINGS	+= -DSUPPORT_UID7_FIX_MANUFACTURER_BYTE

## : Support activating firmware upgrade mode through command-line
SETTINGS	+= -DSUPPORT_FIRMWARE_UPGRADE

## : Default button actions
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_RANDOM
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_LEFT_INCREMENT
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_RIGHT_INCREMENT
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_LEFT_DECREMENT
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_UID_RIGHT_DECREMENT
#SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_CYCLE_SETTINGS
SETTINGS	+= -DDEFAULT_RBUTTON_ACTION=BUTTON_ACTION_STORE_MEM
SETTINGS	+= -DDEFAULT_LBUTTON_ACTION=BUTTON_ACTION_RECALL_MEM

## : Define if button action setting should be independent of active setting
SETTINGS	+= -DBUTTON_SETTING_GLOBAL

## : Default LED functions
SETTINGS	+= -DDEFAULT_RED_LED_ACTION=LED_SETTING_CHANGE
SETTINGS	+= -DDEFAULT_GREEN_LED_ACTION=LED_POWERED

## : Define if LED function setting should be independent of active setting
SETTINGS	+= -DLED_SETTING_GLOBAL

## : Default logging mode
SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_OFF
#SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_MEMORY
#SETTINGS	+= -DDEFAULT_LOG_MODE=LOG_MODE_LIVE

## : Define if log settings should be global
SETTINGS	+= -DLOG_SETTING_GLOBAL

## : Default setting
SETTINGS	+= -DDEFAULT_SETTING=SETTINGS_FIRST

## : Default pending task timeout
SETTINGS	+= -DDEFAULT_PENDING_TASK_TIMEOUT=65

## : Default reader threshold
SETTINGS	+= -DDEFAULT_READER_THRESHOLD=400

## : Use EEPROM to store settings
SETTINGS	+= -DENABLE_EEPROM_SETTINGS

## : Enable tests for DES/2KTDEA/3DES/AES128 crypto schemes:
#SETTINGS  += -DENABLE_CRYPTO_TESTS
#SETTINGS  += -DENABLE_CRYPTO_TDEA_TESTS
#SETTINGS  += -DENABLE_CRYPTO_3DES_TESTS
#SETTINGS  += -DENABLE_CRYPTO_AES_TESTS

## : Enable a command to run any tests added by developers, e.g., the
## : crypto scheme tests that can be enabled above:
#SETTINGS  += -DENABLE_RUNTESTS_TERMINAL_COMMAND

## : Whether or not to allow users Chameleon terminal access to change the DESFire configuration's
## : sensitive settings like manufacturer, serial number, etc.
#SETTINGS += -DDISABLE_PERMISSIVE_DESFIRE_SETTINGS
#SETTINGS += -DDISABLE_DESFIRE_TERMINAL_COMMANDS

## : Enable extra verbose logging messages for debugging:
#CONFIG_SETTINGS  += -DDESFIRE_DEBUGGING=1

## : Feature: Use randomized UIDs that mask the actual secret UID until
## : the tag has been issued a successful authentication sequence:
#SETTINGS += -DDESFIRE_RANDOMIZE_UIDS_PREAUTH

## : Anticipating that the implementation overhead is high with the
## : maximum storage allocations for the number of possible keys per
## : application directory, and/or the total number of AID numbered
## : directory slots, the following options will tweak this limitation:
## :  -> Set MEMORY_LIMITED_TESTING to shrink the defaults
## :  -> Or explicitly define DESFIRE_CUSTOM_MAX_KEYS=##UINT## : (per AID),
## :  -> And/Or define DESFIRE_CUSTOM_MAX_APPS=##UINT##
## :     (total number of AID spaces available, not including the master 0x00)
#SETTINGS += -DMEMORY_LIMITED_TESTING
#SETTINGS += -DDESFIRE_CUSTOM_MAX_APPS=8
#SETTINGS += -DDESFIRE_CUSTOM_MAX_KEYS=6
#SETTINGS += -DDESFIRE_CUSTOM_MAX_FILES=6
#SETTINGS += -DDESFIRE_USE_FACTORY_SIZES
#SETTINGS += -DDESFIRE_MAXIMIZE_SIZES_FOR_STORAGE

## : Set a minimum incoming/outgoing log size so we do not spam the
## : Chameleon Mini logs to much by logging everything:
CONFIG_SETTINGS  += -DDESFIRE_MIN_INCOMING_LOGSIZE=0
CONFIG_SETTINGS  += -DDESFIRE_MIN_OUTGOING_LOGSIZE=0

## : Enable printing of crypto tests when a new DESFire emulation instance is started:
#SETTINGS += -DDESFIRE_RUN_CRYPTO_TESTING_PROCEDURE

## : Option to save space with the "Application/Crypto1.c" code by storing large tables
## : in PROGMEM. Note that this will slow down the read times when accessing these tables:
#SETTINGS  += -DDESFIRE_CRYPTO1_SAVE_SPACE

## : Fix some issues with standard Makefile targets on MacOS
## : where non-GNU versions of coreutils (and Unix commands like
## : grep, sed, awk) lead to unexpected behavior:
CHECK_SIZE_GREP_ARGS=$(if $(filter "$(shell uname -s)", "Darwin"), -Eo "[[:digit:]]+", -oP "\d+")

## Support 'fancy' ANSI terminal colors and unicode emojis
## (idea from the PM3 command line utilities):
ECHO = echo $(ECHOFLAGS)
ifeq ("$(shell uname -s)", "Darwin")
     ECHOFLAGS=
else
     ECHOFLAGS=-e
endif

## : Memory definitions and objcopy flags to include sections in binaries
## ++ Start of data section in flash
FLASH_DATA_ADDR         = 0x10000
## ++ Size of data section in flash
## ++ Each setting is 8192 bytes ... This gives eight total settings to store tag config data
## ++ Conservative settings of FLASH_DATA_SIZE:=0x0E000 yield seven total settings
## ++ Unused area in the .flashdata section can be commandered to store secret data on the
##    Chameleon that can be accessed via the flash memory functions for firmware mods
FLASH_DATA_SIZE         = 0x10000
FLASH_DATA_SECTION_NAME = .flashdata
FLASH_DATA_OBJCOPY      = --set-section-flags=$(FLASH_DATA_SECTION_NAME)="alloc,load"
## ++ Start of SPM helper section.
## ++ Should be last 32 bytes in bootloader section
SPM_HELPER_ADDR         = 0x21FE0
SPM_HELPER_OBJCOPY      = --set-section-flags=.spmhelper="alloc,load"

## : Build configuration
COMMIT_ID    = $(shell git rev-parse --short HEAD)
BUILD_DATE   = $(strip $(if $(filter $(OS), Windows_NT), "\"$(shell date /t)\"", $(shell date +'\"%Y-%m-%d\"')))

## : Adding the '?=' before the `MCU` variable definition lets it be reset by an environment variable,
## : so that the following works (may be useful for those users working with a RevE generation
## : Chameleon device):
## : $ TARGET=atxmega32a4u make
## : ... OR ...
## : $ TARGET=atxmega32a4u make <custom-target-name>
MCU          ?= atxmega128a4u

ARCH         =  XMEGA
BOARD        =  NONE
F_CPU        =  27120000
F_USB        =  48000000
TARGET       ?= Chameleon-Mini
OPTIMIZATION =  s
SRC          =
SRC         +=  $(TARGET).c \
		LUFADescriptors.c \
		System.c \
		ISRSharing.S \
		Configuration.c \
		Random.c \
		Common.c \
		Memory.c \
		MemoryAsm.S \
		Button.c \
		Log.c \
		Settings.c \
		LED.c \
		Pin.c \
		Map.c \
		AntennaLevel.c
SRC         +=  Terminal/Terminal.c \
		Terminal/Commands.c \
		Terminal/XModem.c \
		Terminal/CommandLine.c
SRC         +=  Codec/Codec.c \
		Codec/ISO14443-2A.c \
		Codec/Reader14443-2A.c \
		Codec/SniffISO14443-2A.c \
		Codec/Reader14443-ISR.S \
		Codec/ISO15693.c \
		Codec/SniffISO15693.c
SRC         +=  Application/MifareUltralight.c \
		Application/MifareClassic.c \
		Application/ISO14443-3A.c \
		Application/Crypto1.c \
		Application/Reader14443A.c \
		Application/Sniff14443A.c \
		Application/CryptoTDEA-HWAccelerated.S \
		Application/CryptoTDEA.c \
		Application/CryptoAES128.c \
		Application/CryptoCMAC.c
SRC         +=  Application/NTAG215.c
SRC         +=  Application/Vicinity.c \
		Application/Sl2s2002.c \
		Application/TITagitstandard.c \
		Application/TITagitplus.c \
		Application/ISO15693-A.c \
		Application/EM4233.c \
		Application/Sniff15693.c
SRC	    +=  Application/DESFire/../MifareDESFire.c \
                Application/DESFire/DESFireApplicationDirectory.c \
                Application/DESFire/DESFireChameleonTerminal.c \
                Application/DESFire/DESFireCrypto.c \
                Application/DESFire/DESFireFile.c \
                Application/DESFire/DESFireISO14443Support.c \
                Application/DESFire/DESFireISO7816Support.c \
                Application/DESFire/DESFireInstructions.c \
                Application/DESFire/DESFireLogging.c \
                Application/DESFire/DESFireMemoryOperations.c \
                Application/DESFire/DESFirePICCControl.c \
                Application/DESFire/DESFireUtils.c
SRC         +=  Tests/CryptoTests.c \
		Tests/ChameleonTerminal.c
LUFA_SRC     =  $(LUFA_SRC_USB) \
		$(LUFA_SRC_USBCLASS)
LUFA_PATH    =  ../LUFA
CC_FLAGS     =  -g0 \
		-DUSE_LUFA_CONFIG_HEADER \
		-DFLASH_DATA_ADDR=$(FLASH_DATA_ADDR) \
		-DFLASH_DATA_SIZE=$(FLASH_DATA_SIZE) \
		-DSPM_HELPER_ADDR=$(SPM_HELPER_ADDR) \
		-DBUILD_DATE=$(BUILD_DATE) \
		-DCOMMIT_ID=\"$(COMMIT_ID)\" \
		$(SETTINGS) \
		$(CONFIG_SETTINGS) \
		-D__AVR_ATxmega128A4U__ \
		-D__PROG_TYPES_COMPAT__ \
		-DMAX_ENDPOINT_INDEX=4 \
		-std=gnu99 \
		-Werror=implicit-function-declaration \
		-fno-inline-small-functions \
		-fshort-enums \
		-fpack-struct \
		-ffunction-sections \
		-fdata-sections \
		-fvisibility=hidden \
		-Wl,--gc-sections \
		--data-sections \
                -Wl,-relax \
		-fno-split-wide-types \
		-fno-tree-scev-cprop \
		-fno-aggressive-loop-optimizations \
		-fno-jump-tables \
		-fno-common
LD_FLAGS     = $(CC_FLAGS) \
		-Wl,--section-start=.flashdata=$(FLASH_DATA_ADDR) \
		-Wl,--section-start=.spmhelper=$(SPM_HELPER_ADDR)
OBJDIR       = Bin
TEMPDIR      = $(OBJDIR)/Temp
OBJECT_FILES =
FWROOT       = ../..
BUILD_SCR    = ./BuildScripts

## : AVRDUDE settings
AVRDUDE_PROGRAMMER = flip2
AVRDUDE_MCU = $(MCU)
AVRDUDE_PORT = usb
AVRDUDE_WRITE_APP = -U application:w:$(TARGET).hex
AVRDUDE_WRITE_EEPROM = -U eeprom:w:$(TARGET).eep
AVRDUDE_FLAGS = -p $(AVRDUDE_MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)

## : AVRDUDE settings to program the precompiled firmware
AVRDUDE_WRITE_APP_LATEST = -U application:w:Latest/$(TARGET).hex
AVRDUDE_WRITE_EEPROM_LATEST = -U eeprom:w:Latest/$(TARGET).eep

.PHONY: clean program program-latest dfu-flip dfu-prog check_size style

## : Default target
.DEFAULT all:

## : Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk

## : Define custom targets and standard build variants
include $(BUILD_SCR)/lufa_build_extra.mk
include $(BUILD_SCR)/custom_build_targets.mk

## : Overwrite the LUFA versions of hex/bin file generation to include spmhelper and flashdata sections
%.hex: %.elf
	@$(ECHO) $(MSG_TIDY_PRE_FORMATTING)
	@$(ECHO) $(MSG_OBJCPY_CMD) Extracting $(FMT_ANSIC_LIGHTRED)HEX$(FMT_ANSIC_END) file data from $(FMT_FILENAME_BEGIN)\"$<\"$(MSG_NEWLINE)
	@$(ECHO) $(MSG_TIDY_INTERMED_FORMATTING)
	$(CROSS)-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature $(FLASH_DATA_OBJCOPY) $< $@
	@$(ECHO) $(MSG_TIDY_POST_FORMATTING)

%.bin: %.elf
	@$(ECHO) $(MSG_TIDY_PRE_FORMATTING)
	@$(ECHO) $(MSG_OBJCPY_CMD) Extracting $(FMT_ANSIC_LIGHTRED)BIN$(FMT_ANSIC_END) file data from $(FMT_FILENAME_BEGIN)\"$<\"$(MSG_NEWLINE)
	@$(ECHO) $(MSG_TIDY_INTERMED_FORMATTING)
	$(CROSS)-objcopy -O binary -R .eeprom -R .fuse -R .lock -R .signature $(FLASH_DATA_OBJCOPY) $< $@
	@$(ECHO) $(MSG_TIDY_POST_FORMATTING)

## : Extract SPMHelper in the last 32 Byte of the bootloader section to externally combine it with any bootloader
spmhelper: $(TARGET).elf
	@$(ECHO) $(MSG_TIDY_PRE_FORMATTING)
	@$(ECHO) $(MSG_OBJCPY_CMD) Extracting $(FMT_ANSIC_LIGHTRED)SPM$(FMT_ANSIC_END) helper \
		$(FMT_ANSIC_LIGHTRED)HEX$(FMT_ANSIC_END) file \
		from $(FMT_FILENAME_BEGIN)$(TARGET).elf$(MSG_NEWLINE)
	@$(ECHO) $(MSG_TIDY_INTERMED_FORMATTING)
	$(CROSS)-objcopy -O ihex -j .spmhelper $(SPM_HELPER_OBJCOPY) $(TARGET).elf $(TARGET).hex
	@$(ECHO) $(MSG_TIDY_POST_FORMATTING)

## : Program the device using avrdude
program: $(TARGET).hex $(TARGET).eep check_size
	avrdude $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_APP) $(AVRDUDE_WRITE_EEPROM)

## : Program the device using avrdude with the latest official firmware
program-latest: check_size
	avrdude $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_APP_LATEST) $(AVRDUDE_WRITE_EEPROM_LATEST)

## : Program the device using batchisp and the DFU bootloader
## : Note that the device has to be in bootloader mode already-ffunction-sections -fdata-sections
dfu-flip: $(TARGET).hex $(TARGET).eep check_size
	cp $(TARGET).eep EEPROM.hex
	batchisp -hardware usb -device $(MCU) -operation erase f memory FLASH loadbuffer $(TARGET).hex \
		program verify memory EEPROM loadbuffer EEPROM.hex \
		program verify start reset 0
	rm EEPROM.hex

## : Program the device using dfu-programmer
dfu-prog: $(TARGET).hex $(TARGET).eep check_size
	dfu-programmer $(MCU) erase
	dfu-programmer $(MCU) flash-eeprom $(TARGET).eep
	dfu-programmer $(MCU) flash $(TARGET).hex
	dfu-programmer $(MCU) reset

check_size: BASH:=$(if $(shell which bash), $(shell which bash), /bin/bash)
check_size: BASH_SCRIPT_EXEC_LINES:='                                                                            \
	set -e;                                                                                                  \
	if [[ ! -f $(TARGET).elf ]]; then                                                                        \
		exit 0;                                                                                          \
	fi;                                                                                                      \
	PROGMEM_SIZE=$$(avr-size $(TARGET).elf | grep $(CHECK_SIZE_GREP_ARGS) | sed -n 4p);                      \
	MAX_PRGMEM_SIZE=$$(printf "%d\n" $(FLASH_DATA_ADDR));                                                    \
	if [[ $$PROGMEM_SIZE -gt $$MAX_PRGMEM_SIZE ]]; then                                                      \
	     $(ECHO) \"make: *** $(TARGET).elf Application Section size $$PROGMEM_SIZE \"                        \
		$(ECHO) \"excedes maximum allowed $$MAX_PRGMEM_SIZE. Please disable some features in Makefile\"; \
		exit 1;                                                                                          \
	fi;                                                                                                      \
	'
check_size:
	@$(BASH) -c $(BASH_SCRIPT_EXEC_LINES) || $(SHELL) -c $(BASH_SCRIPT_EXEC_LINES)

style:
	## : Make sure astyle is installed
	@which astyle >/dev/null || ( echo "Please install 'astyle' package first" ; exit 1 )
	## : Remove spaces & tabs at EOL, add LF at EOF if needed on *.c, *.h, Makefile
	find . \( -name "*.[ch]" -or -name "Makefile" \)                      \
	    -exec perl -pi -e 's/[ \t]+$$//' {} \;                            \
	    -exec sh -c "tail -c1 {} | xxd -p | tail -1 | grep -q -v 0a$$" \; \
	    -exec sh -c "echo >> {}" \;
	## : Apply astyle on *.c, *.h
	find . -name "*.[ch]" -exec astyle --formatted --mode=c --suffix=none \
	    --indent=spaces=4 --indent-switches                               \
	    --keep-one-line-blocks --max-instatement-indent=60                \
	    --style=google --pad-oper --unpad-paren --pad-header              \
	    --align-pointer=name {} \;

local-clean:
	@rm -f  $(TARGET)*.{elf,hex,eep,bin,lss,map}
	## :BlockCount = Count / CRYPTO_DES_BLOCK_SIZE; Try to avoid rebuilding the LUFA objects that rarely change:
	@rm -f $(CHAMELEON_OBJECT_FILES)
	@mkdir -p $(OBJDIR)
clean: local-clean

git-add-dev: LOCALFW_SOURCE_FILES:=Makefile ./*.{c,h,S} ./*/*.{c,h} ./Application/*.S ./*/*/*.{c,h}
git-add-dev: LOCALFW_BUILD_SCRIPT_FILES:=.gitignore custom_build_targets.mk lufa_build_extra.mk
git-add-dev: TESTING_SOURCE_FILES:=Makefile LocalInclude/*.h Source/*.c SampleOutputDumps/*.dump
git-add-dev:
	@make style && git add $(LOCALFW_SOURCE_FILES)
	@cd $(BUILD_SCR) && git add -f $(LOCALFW_BUILD_SCRIPT_FILES)
	@cd $(FWROOT)/Software/DESFireLibNFCTesting && make style && git add $(TESTING_SOURCE_FILES)
	@git add $(FWROOT)/Doc/*.md
	@cd $(FWROOT)/Software/ChamTool && git add ./*.py ./*/*.py
	@cd $(FWROOT)/Dumps && git add ./*

configure-chameleon-usb: UDEV_RULES_CONFIG_LINES:='\n                                                \
	\# ChameleonMini (RevG)\n                                                                    \
	SUBSYSTEMS==\"usb\", ATTRS{idVendor}==\"16d0\", ATTRS{idProduct}==\"04b2\", GROUP=\"users\", \
	MODE=\"0666\", SYMLINK+=\"ChameleonMini\", ENV{ID_MM_DEVICE_IGNORE}=\"1\"\n                  \
	\# ChameleonMini (RevE)\n                                                                    \
	SUBSYSTEMS==\"usb\", ATTRS{idVendor}==\"03eb\", ATTRS{idProduct}==\"2044\", GROUP=\"users\", \
	MODE=\"0666\", SYMLINK+=\"ChameleonMiniE\", ENV{ID_MM_DEVICE_IGNORE}=\"1\"\n                 \
	\# ChameleonMini DFU mode\n                                                                  \
	SUBSYSTEMS==\"usb\", ATTRS{idVendor}==\"03eb\", ATTRS{idProduct}==\"2fde\", GROUP=\"users\", \
	MODE=\"0666\"                                                                                \
	'
configure-chameleon-usb:
	@mkdir -p $(TEMPDIR) && echo $(UDEV_RULES_CONFIG_LINES) > $(TEMPDIR)/98-ChameleonMini.rules
	@$(ECHO) "TO CONFIGURE THE CHAMELEON OVER USB, RUN THE FOLLOWING COMMANDS:"
	@$(ECHO) "    $$ sudo cp $(TEMPDIR)/98-ChameleonMini.rules /etc/udev/rules.d/"
	@$(ECHO) "    $$ sudo service udev restart"
	@$(ECHO) "    $$ sudo udevadm control --reload"
	@$(ECHO) ""
	@$(ECHO) "FOR HELP TROUBLESHOOTING THE CHAMELEON USB CONNECTION, CONSIDER THE FOLLOWING OUTPUT:"
	@$(ECHO) "    $$ dmesg | tail | grep -i dev"
	@$(ECHO) "    $$ dmesg | tail | grep -i usb"
	@$(ECHO) "    $$ sudo udevadm monitor"
