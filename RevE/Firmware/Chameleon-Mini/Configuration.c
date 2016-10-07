/*
 * Standards.c
 *
 *  Created on: 15.02.2013
 *      Author: skuser
 */

#include "Configuration.h"
#include "Settings.h"
#include <avr/pgmspace.h>

/* Include all Codecs and Applications */
#include "Codec/Codec.h"
#include "Application/Application.h"

static void CodecInitDummy(void) { }
static void CodecTaskDummy(void) { }
static void ApplicationInitDummy(void) {}
static void ApplicationResetDummy(void) {}
static void ApplicationTaskDummy(void) {}
static uint16_t ApplicationProcessDummy(uint8_t* ByteBuffer, uint16_t ByteCount) { return 0; }
static void ApplicationGetUidDummy(ConfigurationUidType Uid) { }
static void ApplicationSetUidDummy(ConfigurationUidType Uid) { }

static const PROGMEM ConfigurationType ConfigurationTable[] = {
    [CONFIG_NONE] = {
        .ConfigurationID = CONFIG_NONE,
        .ConfigurationName = "NONE",
        .CodecInitFunc = CodecInitDummy,
        .CodecTaskFunc = CodecTaskDummy,
        .ApplicationInitFunc = ApplicationInitDummy,
        .ApplicationResetFunc = ApplicationResetDummy,
        .ApplicationTaskFunc = ApplicationTaskDummy,
        .ApplicationProcessFunc = ApplicationProcessDummy,
        .ApplicationGetUidFunc = ApplicationGetUidDummy,
        .ApplicationSetUidFunc = ApplicationSetUidDummy,
        .UidSize = 0,
        .MemorySize = 0,
        .ReadOnly = true
    },
#ifdef CONFIG_MF_ULTRALIGHT_SUPPORT
    [CONFIG_MF_ULTRALIGHT] = {
        .ConfigurationID = CONFIG_MF_ULTRALIGHT,
        .ConfigurationName = "MF_ULTRALIGHT",
        .CodecInitFunc = ISO14443ACodecInit,
        .CodecTaskFunc = ISO14443ACodecTask,
        .ApplicationInitFunc = MifareUltralightAppInit,
        .ApplicationResetFunc = MifareUltralightAppReset,
        .ApplicationTaskFunc = MifareUltralightAppTask,
        .ApplicationProcessFunc = MifareUltralightAppProcess,
        .ApplicationGetUidFunc = MifareUltralightGetUid,
        .ApplicationSetUidFunc = MifareUltralightSetUid,
        .UidSize = MIFARE_ULTRALIGHT_UID_SIZE,
        .MemorySize = MIFARE_ULTRALIGHT_MEM_SIZE,
        .ReadOnly = false
    },
#endif
#ifdef CONFIG_MF_CLASSIC_1K_SUPPORT
    [CONFIG_MF_CLASSIC_1K] = {
        .ConfigurationID = CONFIG_MF_CLASSIC_1K,
        .ConfigurationName = "MF_CLASSIC_1K",
        .CodecInitFunc = ISO14443ACodecInit,
        .CodecTaskFunc = ISO14443ACodecTask,
        .ApplicationInitFunc = MifareClassicAppInit1K,
        .ApplicationResetFunc = MifareClassicAppReset,
        .ApplicationTaskFunc = MifareClassicAppTask,
        .ApplicationProcessFunc = MifareClassicAppProcess,
        .ApplicationGetUidFunc = MifareClassicGetUid,
        .ApplicationSetUidFunc = MifareClassicSetUid,
        .UidSize = MIFARE_CLASSIC_UID_SIZE,
        .MemorySize = MIFARE_CLASSIC_1K_MEM_SIZE,
        .ReadOnly = false
    },
#endif
#ifdef CONFIG_MF_Plus1k_7B_SUPPORT
[CONFIG_MF_Plus1k_7B] = {
	.ConfigurationID = CONFIG_MF_Plus1k_7B,
	.ConfigurationName = "MF_PLUS1K_7B",
	.CodecInitFunc = ISO14443ACodecInit,
	.CodecTaskFunc = ISO14443ACodecTask,
	.ApplicationInitFunc = MifarePlus1kAppInit_7B,
	.ApplicationResetFunc = MifareClassicAppReset,
	.ApplicationTaskFunc = MifareClassicAppTask,
	.ApplicationProcessFunc = MifareClassicAppProcess,
	.ApplicationGetUidFunc = MifareClassicGetUid,
	.ApplicationSetUidFunc = MifareClassicSetUid,
	.UidSize = ISO14443A_UID_SIZE_DOUBLE,
	.MemorySize = MIFARE_CLASSIC_1K_MEM_SIZE,
	.ReadOnly = false
},
#endif
#ifdef CONFIG_MF_CLASSIC_4K_SUPPORT
    [CONFIG_MF_CLASSIC_4K] = {
        .ConfigurationID = CONFIG_MF_CLASSIC_4K,
        .ConfigurationName = "MF_CLASSIC_4K",
        .CodecInitFunc = ISO14443ACodecInit,
        .CodecTaskFunc = ISO14443ACodecTask,
        .ApplicationInitFunc = MifareClassicAppInit4K,
        .ApplicationResetFunc = MifareClassicAppReset,
        .ApplicationTaskFunc = MifareClassicAppTask,
        .ApplicationProcessFunc = MifareClassicAppProcess,
        .ApplicationGetUidFunc = MifareClassicGetUid,
        .ApplicationSetUidFunc = MifareClassicSetUid,
        .UidSize = MIFARE_CLASSIC_UID_SIZE,
        .MemorySize = MIFARE_CLASSIC_4K_MEM_SIZE,
        .ReadOnly = false
    },
#endif
#ifdef CONFIG_ISO14443A_SNIFF_SUPPORT
    [CONFIG_ISO14443A_SNIFF] = {
    	.ConfigurationID = CONFIG_ISO14443A_SNIFF,
    	.ConfigurationName = "ISO14443A_SNIFF",
    	.CodecInitFunc = ISO14443ACodecInit,
		.CodecTaskFunc = ISO14443ACodecTask,
		.ApplicationInitFunc = ApplicationInitDummy,
		.ApplicationResetFunc = ApplicationResetDummy,
		.ApplicationTaskFunc = ApplicationTaskDummy,
		.ApplicationProcessFunc = ApplicationProcessDummy,
		.ApplicationGetUidFunc = ApplicationGetUidDummy,
		.ApplicationSetUidFunc = ApplicationSetUidDummy,
		.UidSize = 0,
		.MemorySize = 0,
		.ReadOnly = true
    },
#endif

};

ConfigurationType ActiveConfiguration;

void ConfigurationInit(void)
{
    ConfigurationSetById(GlobalSettings.ActiveSettingPtr->Configuration);
}

void ConfigurationSetById( ConfigurationEnum Configuration )
{
	GlobalSettings.ActiveSettingPtr->Configuration = Configuration;

    /* Copy struct from PROGMEM to RAM */
    memcpy_P(&ActiveConfiguration,
            &ConfigurationTable[Configuration], sizeof(ConfigurationType));


    ApplicationInit();
    CodecInit();
}

bool ConfigurationSetByName(const char* ConfigurationName)
{
    uint8_t i;

    /* Loop through table trying to find the configuration */
    for (i=0; i<(sizeof(ConfigurationTable) / sizeof(*ConfigurationTable)); i++) {
        const char* pTableConfigName = ConfigurationTable[i].ConfigurationName;
        const char* pRequestedConfigName = ConfigurationName;
        bool StringMismatch = false;
        char c = pgm_read_byte(pTableConfigName);

        /* Try to keep running until both strings end at the same point */
        while ( !(c == '\0' && *pRequestedConfigName == '\0') ) {
            if ( (c == '\0') || (*pRequestedConfigName == '\0') ) {
                /* One String ended before the other did -> unequal length */
                StringMismatch = true;
                break;
            }

            if (c != *pRequestedConfigName) {
                /* Character mismatch */
                StringMismatch = true;
                break;
            }

            /* Proceed to next character */
            pTableConfigName++;
            pRequestedConfigName++;

            c = pgm_read_byte(pTableConfigName);
        }

        if (!StringMismatch) {
            /* Configuration found */
            ConfigurationSetById(i);
            return true;
        }
    }

    return false;
}

void ConfigurationGetList(char* ConfigListOut, uint16_t ByteCount)
{
    uint8_t i;

    /* Account for '\0' */
    ByteCount--;

    for (i=0; i<CONFIG_COUNT; i++) {
        const char* ConfigName = ConfigurationTable[i].ConfigurationName;
        char c;

        while( (c = pgm_read_byte(ConfigName)) != '\0' && ByteCount > CONFIGURATION_NAME_LENGTH_MAX) {
            /* While not end-of-string and enough buffer to
            * put a complete configuration name */
            *ConfigListOut++ = c;
            ConfigName++;
            ByteCount--;
        }

        if ( i < (CONFIG_COUNT - 1) ) {
            /* No comma on last configuration */
            *ConfigListOut++ = ',';
            ByteCount--;
        }
    }

    *ConfigListOut = '\0';
}

