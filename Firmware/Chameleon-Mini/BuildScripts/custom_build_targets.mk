## : Include several standardized custom build target variants:
custom-build: local-clean $(TARGET).elf $(TARGET).hex $(TARGET).eep $(TARGET).bin check_size
	@cp $(TARGET).hex $(TARGET)-$(TARGET_CUSTOM_BUILD).hex
	@cp $(TARGET).eep $(TARGET)-$(TARGET_CUSTOM_BUILD).eep
	@cp $(TARGET).bin $(TARGET)-$(TARGET_CUSTOM_BUILD).bin
	@echo ""
	@avr-size -A -x $(TARGET).elf
	@avr-size -B -x $(TARGET).elf
	@echo ""
	@avr-size -C -x $(TARGET).elf
	@echo ""
	@echo "   ==== SUCCESS BUILDING CUSTOM FIRMWARE -- $(TARGET)-$(TARGET_CUSTOM_BUILD) ===="
	@echo ""

default_config_support: DEFAULT_TAG_SUPPORT:= \
                        -DCONFIG_ISO14443A_SNIFF_SUPPORT \
                        -DCONFIG_ISO14443A_READER_SUPPORT
nodefault_config_support: DEFAULT_TAG_SUPPORT:= 

## : Define a few other useful custom build targets:
mifare: SUPPORTED_TAGS:=$(DEFAULT_TAG_SUPPORT) \
                -DCONFIG_MF_CLASSIC_MINI_4B_SUPPORT \
                -DCONFIG_MF_CLASSIC_1K_SUPPORT \
                -DCONFIG_MF_CLASSIC_1K_7B_SUPPORT \
                -DCONFIG_MF_CLASSIC_4K_SUPPORT \
                -DCONFIG_MF_CLASSIC_4K_7B_SUPPORT \
                -DCONFIG_MF_ULTRALIGHT_SUPPORTmifare: CONFIG_SETTINGS:= $(SUPPORTED_TAGS) -DDEFAULT_CONFIGURATION=CONFIG_NONE
mifare: TARGET_CUSTOM_BUILD:=CustomBuild_MifareDefaultSupport
mifare: custom-build

mifare-classic: nodefault_config_support
mifare-classic: SUPPORTED_TAGS:=$(DEFAULT_TAG_SUPPORT) \
                -DCONFIG_MF_CLASSIC_MINI_4B_SUPPORT \
                -DCONFIG_MF_CLASSIC_1K_SUPPORT \
                -DCONFIG_MF_CLASSIC_1K_7B_SUPPORT \
                -DCONFIG_MF_CLASSIC_4K_SUPPORT \
                -DCONFIG_MF_CLASSIC_4K_7B_SUPPORT
mifare-classic: CONFIG_SETTINGS:= $(SUPPORTED_TAGS) -DDEFAULT_CONFIGURATION=CONFIG_NONE
mifare-classic: TARGET_CUSTOM_BUILD:=CustomBuild_MifareClassicSupport
mifare-classic: custom-build

iso-modes: nodefault_config_support
iso-modes: SUPPORTED_TAGS:=$(DEFAULT_TAG_SUPPORT) \
                -DCONFIG_ISO14443A_SNIFF_SUPPORT \
                -DCONFIG_ISO14443A_READER_SUPPORT \
                -DCONFIG_ISO15693_SNIFF_SUPPORT
iso-modes: CONFIG_SETTINGS:= $(SUPPORTED_TAGS) -DDEFAULT_CONFIGURATION=CONFIG_NONE
iso-modes: TARGET_CUSTOM_BUILD:=CustomBuild_ISOSniffReaderModeSupport
iso-modes: custom-build

ntag215: default_config_support
ntag215: SUPPORTED_TAGS:=$(DEFAULT_TAG_SUPPORT) \
                -DCONFIG_NTAG215_SUPPORT
ntag215: CONFIG_SETTINGS:= $(SUPPORTED_TAGS) -DDEFAULT_CONFIGURATION=CONFIG_NONE
ntag215: TARGET_CUSTOM_BUILD:=CustomBuild_NTAG215Support
ntag215: custom-build

vicinity: default_config_support
vicinity: SUPPORTED_TAGS:=$(DEFAULT_TAG_SUPPORT) \
                -DCONFIG_VICINITY_SUPPORT
vicinity: CONFIG_SETTINGS:= $(SUPPORTED_TAGS) -DDEFAULT_CONFIGURATION=CONFIG_NONE
vicinity: TARGET_CUSTOM_BUILD:=CustomBuild_VicinitySupport
vicinity: custom-build

sl2s2002: default_config_support
sl2s2002: SUPPORTED_TAGS:=$(DEFAULT_TAG_SUPPORT) \
                -DCONFIG_SL2S2002_SUPPORT
sl2s2002: CONFIG_SETTINGS:= $(SUPPORTED_TAGS) -DDEFAULT_CONFIGURATION=CONFIG_NONE
sl2s2002: TARGET_CUSTOM_BUILD:=CustomBuild_SL2S2002Support
sl2s2002: custom-build

tagatit: default_config_support
tagatit: SUPPORTED_TAGS:=$(DEFAULT_TAG_SUPPORT) \
                -DCONFIG_TITAGITSTANDARD_SUPPORT \
                -DCONFIG_TITAGITPLUS_SUPPORT
tagatit: CONFIG_SETTINGS:= $(SUPPORTED_TAGS) -DDEFAULT_CONFIGURATION=CONFIG_NONE
tagatit: TARGET_CUSTOM_BUILD:=CustomBuild_TagatitSupport
tagatit: custom-build

em4233: default_config_support
em4233: SUPPORTED_TAGS:=$(DEFAULT_TAG_SUPPORT) \
                -DCONFIG_EM4233_SUPPORT
em4233: CONFIG_SETTINGS:= $(SUPPORTED_TAGS) -DDEFAULT_CONFIGURATION=CONFIG_NONE
em4233: TARGET_CUSTOM_BUILD:=CustomBuild_EM4233Support
em4233: custom-build

## : Define custom targets for the DESFire build (normal/user mode) and 
## : developer mode for use with the Android CMLD application that enables
## : the printing of LIVE logs to the phone's console by default:
desfire-build: local-clean $(TARGET).elf $(TARGET).hex $(TARGET).eep $(TARGET).bin check_size
	@cp $(TARGET).hex $(TARGET)-DESFire.hex
	@cp $(TARGET).eep $(TARGET)-DESFire.eep
	@cp $(TARGET).bin $(TARGET)-DESFire.bin
	@echo ""
	@avr-size -A -x $(TARGET).elf
	@avr-size -B -x $(TARGET).elf
	@echo ""
	@avr-size -C -x $(TARGET).elf
desfire: CONFIG_SETTINGS:=$(DESFIRE_CONFIG_SETTINGS_BASE) -fno-inline-small-functions
desfire: desfire-build
desfire-dev: CONFIG_SETTINGS:=$(DESFIRE_CONFIG_SETTINGS_BASE) -fno-inline-small-functions \
	-DDESFIRE_MIN_OUTGOING_LOGSIZE=0 \
	-DDESFIRE_MIN_INCOMING_LOGSIZE=0 \
	-DDESFIRE_DEFAULT_LOGGING_MODE=DEBUGGING \
	-DDESFIRE_DEFAULT_TESTING_MODE=1
desfire-dev: desfire-build
