/* Copyright 2013 Timo Kasper, Simon Küppers, David Oswald ("ORIGINAL
 * AUTHORS"). All rights reserved.
 *
 * DEFINITIONS:
 *
 * "WORK": The material covered by this license includes the schematic
 * diagrams, designs, circuit or circuit board layouts, mechanical
 * drawings, documentation (in electronic or printed form), source code,
 * binary software, data files, assembled devices, and any additional
 * material provided by the ORIGINAL AUTHORS in the ChameleonMini project
 * (https://github.com/skuep/ChameleonMini).
 *
 * LICENSE TERMS:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK are permitted provided that the
 * following conditions are met:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK must include the above copyright
 * notice, this list of conditions, the below disclaimer, and the following
 * attribution:
 *
 * "Based on ChameleonMini an open-source RFID emulator:
 * https://github.com/skuep/ChameleonMini"
 *
 * The attribution must be clearly visible to a user, for example, by being
 * printed on the circuit board and an enclosure, and by being displayed by
 * software (both in binary and source code form).
 *
 * At any time, the majority of the ORIGINAL AUTHORS may decide to give
 * written permission to an entity to use or redistribute the WORK (with or
 * without modification) WITHOUT having to include the above copyright
 * notice, this list of conditions, the below disclaimer, and the above
 * attribution.
 *
 * DISCLAIMER:
 *
 * THIS PRODUCT IS PROVIDED BY THE ORIGINAL AUTHORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE ORIGINAL AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS PRODUCT, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the hardware, software, and
 * documentation should not be interpreted as representing official
 * policies, either expressed or implied, of the ORIGINAL AUTHORS.
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

