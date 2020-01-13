#include "Settings.h"
#include <avr/eeprom.h>
#include "Configuration.h"
#include "Log.h"
#include "Memory.h"
#include "LEDHook.h"
#include "Terminal/CommandLine.h"

#include "System.h"

#define SETTING_TO_INDEX(S) (S - SETTINGS_FIRST)
#define INDEX_TO_SETTING(I) (I + SETTINGS_FIRST)

SettingsType GlobalSettings;
SettingsType EEMEM StoredSettings = {
    .ActiveSettingIdx = SETTING_TO_INDEX(DEFAULT_SETTING),
    .ActiveSettingPtr = &GlobalSettings.Settings[SETTING_TO_INDEX(DEFAULT_SETTING)],

    .Settings = {
        [0 ...(SETTINGS_COUNT - 1)] =	{
            .Configuration = DEFAULT_CONFIGURATION,
            .ButtonActions = {
                [BUTTON_L_PRESS_SHORT] = DEFAULT_LBUTTON_ACTION, [BUTTON_R_PRESS_SHORT] = DEFAULT_RBUTTON_ACTION,
                [BUTTON_L_PRESS_LONG]  = DEFAULT_LBUTTON_ACTION, [BUTTON_R_PRESS_LONG]  = DEFAULT_RBUTTON_ACTION
            },
            .LogMode = DEFAULT_LOG_MODE,
            .LEDRedFunction = DEFAULT_RED_LED_ACTION,
            .LEDGreenFunction = DEFAULT_GREEN_LED_ACTION,
            .PendingTaskTimeout = DEFAULT_PENDING_TASK_TIMEOUT,
            .ReaderThreshold = DEFAULT_READER_THRESHOLD
        }
    }
};

void SettingsLoad(void) {
    ReadEEPBlock((uint16_t) &StoredSettings, &GlobalSettings, sizeof(SettingsType));
}

void SettingsSave(void) {
#if ENABLE_EEPROM_SETTINGS
    WriteEEPBlock((uint16_t) &StoredSettings, &GlobalSettings, sizeof(SettingsType));
#endif
}

void SettingsCycle(void) {
    uint8_t i = SETTINGS_COUNT;
    uint8_t SettingIdx = GlobalSettings.ActiveSettingIdx;

    while (i-- > 0) {
        /* Try to set one of the SETTINGS_COUNT following settings.
         * But only set if it is not CONFIG_NONE. */
        SettingIdx = (SettingIdx + 1) % SETTINGS_COUNT;

        if (GlobalSettings.Settings[SettingIdx].Configuration != CONFIG_NONE) {
            SettingsSetActiveById(INDEX_TO_SETTING(SettingIdx));
            break;
        }
    }
}

bool SettingsSetActiveById(uint8_t Setting) {
    if ((Setting >= SETTINGS_FIRST) && (Setting <= SETTINGS_LAST)) {
        uint8_t SettingIdx = SETTING_TO_INDEX(Setting);

        /* Break potentially pending timeout task (manual timeout) */
        CommandLinePendingTaskBreak();

        if (SettingIdx != GlobalSettings.ActiveSettingIdx) {
            /* Store current memory contents permanently */
            MemoryStore();

            GlobalSettings.ActiveSettingIdx = SettingIdx;
            GlobalSettings.ActiveSettingPtr =
                &GlobalSettings.Settings[SettingIdx];

            /* Recall new memory contents ( Moved this to allow for Access to new Memory in Application init())*/
            MemoryRecall();

            /* Settings have changed. Progress changes through system */
            ConfigurationSetById(GlobalSettings.ActiveSettingPtr->Configuration);
            LogSetModeById(GlobalSettings.ActiveSettingPtr->LogMode);


            SETTING_UPDATE(GlobalSettings.ActiveSettingIdx);
            SETTING_UPDATE(GlobalSettings.ActiveSettingPtr);
        }

        /* Notify LED. blink according to current setting */
        LEDHook(LED_SETTING_CHANGE, LED_BLINK + SettingIdx);

        return true;
    } else {
        return false;
    }
}

uint8_t SettingsGetActiveById(void) {
    return INDEX_TO_SETTING(GlobalSettings.ActiveSettingIdx);
}

void SettingsGetActiveByName(char *SettingOut, uint16_t BufferSize) {
    SettingOut[0] = SettingsGetActiveById() + '0';
    SettingOut[1] = '\0';
}

bool SettingsSetActiveByName(const char *Setting) {
    uint8_t SettingNr = Setting[0] - '0';

    if (Setting[1] == '\0') {
        LogEntry(LOG_INFO_SETTING_SET, Setting, 1);
        return SettingsSetActiveById(SettingNr);
    } else {
        return false;
    }
}

