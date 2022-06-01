SHELL := $(if $(SHELL), $(SHELL), /bin/sh)
BASH  := $(if $(shell which bash), $(shell which bash), /bin/bash)

TARGET_CUSTOM_BUILD_NAME     =
TARGET_CUSTOM_BUILD          = $(TARGET)-$(strip $(if $(TARGET_CUSTOM_BUILD_NAME), "CustomBuild_$(TARGET_CUSTOM_BUILD_NAME)", "DefaultBuild"))
DEFAULT_TAG_SUPPORT_BASE     = -DCONFIG_ISO14443A_SNIFF_SUPPORT \
                               -DCONFIG_ISO14443A_READER_SUPPORT
SUPPORTED_TAGS              ?=
SUPPORTED_TAGS_BUILD        := $(SUPPORTED_TAGS)
CUSTOM_CONFIG_SETTINGS_BASE := $(DEFAULT_TAG_SUPPORT) $(SUPPORTED_TAGS_BUILD) -DDEFAULT_CONFIGURATION=CONFIG_NONE

## : Include several standardized custom build target variants:
custom-build: CONFIG_SETTINGS:=$(CUSTOM_CONFIG_SETTINGS_BASE)
custom-build: local-clean $(TARGET).elf $(TARGET).hex $(TARGET).eep $(TARGET).bin check_size
	@cp $(TARGET).hex $(TARGET_CUSTOM_BUILD).hex
	@cp $(TARGET).eep $(TARGET_CUSTOM_BUILD).eep
	@cp $(TARGET).elf $(TARGET_CUSTOM_BUILD).elf
	@cp $(TARGET).bin $(TARGET_CUSTOM_BUILD).bin
	@echo $(MSG_TIDY_ENDSEP)$(MSG_TIDY_ENDSEP)$(MSG_TIDY_ENDSEP)"\n"
	@avr-size -A -x $(TARGET).elf
	@echo $(MSG_TIDY_ENDSEP)"\n"
	@avr-size -B -x $(TARGET).elf
	@echo "\n"$(MSG_TIDY_ENDSEP)"\n"
	@avr-size -C -x $(TARGET).elf
	@echo $(MSG_TIDY_ENDSEP)$(MSG_TIDY_ENDSEP)$(MSG_TIDY_ENDSEP)"\n"
	@echo $(FMT_ANSIC_BOLD)$(FMT_ANSIC_EXCLAIM)"[!!!]"$(FMT_ANSIC_END) \
		$(FMT_ANSIC_BOLD)$(FMT_ANSIC_UNDERLINE)"SUCCESS BUILDING CUSTOM FIRMWARE:"$(FMT_ANSIC_END)
	@echo $(FMT_ANSIC_BOLD)$(FMT_ANSIC_EXCLAIM)"[!!!]"$(FMT_ANSIC_END) \
		$(FMT_ANSIC_BOLD)"$(TARGET_CUSTOM_BUILD).{hex/eep/elf/bin}"$(FMT_ANSIC_END)
	@echo "\n"

default_config_support:   DEFAULT_TAG_SUPPORT:=$(DEFAULT_TAG_SUPPORT_BASE)
nodefault_config_support: DEFAULT_TAG_SUPPORT:= 

mifare: SUPPORTED_TAGS_BUILD:=\
                -DCONFIG_MF_CLASSIC_MINI_4B_SUPPORT \
                -DCONFIG_MF_CLASSIC_1K_SUPPORT \
                -DCONFIG_MF_CLASSIC_1K_7B_SUPPORT \
                -DCONFIG_MF_CLASSIC_4K_SUPPORT \
                -DCONFIG_MF_CLASSIC_4K_7B_SUPPORT \
                -DCONFIG_MF_ULTRALIGHT_SUPPORT
mifare: TARGET_CUSTOM_BUILD_NAME:=MifareDefaultSupport
mifare: custom-build

mifare-classic: nodefault_config_support
mifare-classic: SUPPORTED_TAGS_BUILD:=\
                -DCONFIG_MF_CLASSIC_MINI_4B_SUPPORT \
                -DCONFIG_MF_CLASSIC_1K_SUPPORT \
                -DCONFIG_MF_CLASSIC_1K_7B_SUPPORT \
                -DCONFIG_MF_CLASSIC_4K_SUPPORT \
                -DCONFIG_MF_CLASSIC_4K_7B_SUPPORT
mifare-classic: TARGET_CUSTOM_BUILD_NAME:=MifareClassicSupport
mifare-classic: custom-build

desfire: CONFIG_SETTINGS:=$(DESFIRE_CONFIG_SETTINGS_BASE) \
		-fno-inline-small-functions
desfire: TARGET_CUSTOM_BUILD_NAME:=DESFire
desfire: custom-build

desfire-dev: CONFIG_SETTINGS:=$(DESFIRE_CONFIG_SETTINGS_BASE) \
		-fno-inline-small-functions \
		-DDESFIRE_MIN_OUTGOING_LOGSIZE=0 \
		-DDESFIRE_MIN_INCOMING_LOGSIZE=0 \
		-DDESFIRE_DEFAULT_LOGGING_MODE=DEBUGGING \
		-DDESFIRE_DEFAULT_TESTING_MODE=1
desfire-dev: TARGET_CUSTOM_BUILD_NAME:=DESFire_DEV
desfire-dev: custom-build

iso-modes: nodefault_config_support
iso-modes: SUPPORTED_TAGS_BUILD:=\
                -DCONFIG_ISO14443A_SNIFF_SUPPORT \
                -DCONFIG_ISO14443A_READER_SUPPORT \
                -DCONFIG_ISO15693_SNIFF_SUPPORT
iso-modes: TARGET_CUSTOM_BUILD_NAME:=ISOSniffReaderModeSupport
iso-modes: custom-build

ntag215: default_config_support
ntag215: SUPPORTED_TAGS_BUILD:=-DCONFIG_NTAG215_SUPPORT
ntag215: TARGET_CUSTOM_BUILD_NAME:=NTAG215Support
ntag215: custom-build

vicinity: default_config_support
vicinity: SUPPORTED_TAGS_BUILD:=-DCONFIG_VICINITY_SUPPORT
vicinity: TARGET_CUSTOM_BUILD_NAME:=VicinitySupport
vicinity: custom-build

sl2s2002: default_config_support
sl2s2002: SUPPORTED_TAGS_BUILD:=-DCONFIG_SL2S2002_SUPPORT
sl2s2002: TARGET_CUSTOM_BUILD_NAME:=SL2S2002Support
sl2s2002: custom-build

tagatit: default_config_support
tagatit: SUPPORTED_TAGS_BUILD:=\
                -DCONFIG_TITAGITSTANDARD_SUPPORT \
                -DCONFIG_TITAGITPLUS_SUPPORT
tagatit: TARGET_CUSTOM_BUILD_NAME:=TagatitSupport
tagatit: custom-build

em4233: default_config_support
em4233: SUPPORTED_TAGS_BUILD:=-DCONFIG_EM4233_SUPPORT
em4233: TARGET_CUSTOM_BUILD_NAME:=EM4233Support
em4233: custom-build
