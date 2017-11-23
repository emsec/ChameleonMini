/*
 * Settings.h
 *
 *  Created on: 21.12.2013
 *      Author: skuser
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "Button.h"
#include "Configuration.h"
#include "Log.h"
#include "LED.h"
#include "Memory.h"

#define SETTINGS_COUNT		(MEMORY_SIZE / MEMORY_SIZE_PER_SETTING)
#define SETTINGS_FIRST		1
#define SETTINGS_LAST		(SETTINGS_FIRST + SETTINGS_COUNT - 1)


typedef struct {
	ButtonActionEnum ButtonActions[BUTTON_TYPE_COUNT];
	LogModeEnum LogMode;
	ConfigurationEnum Configuration;
	LEDHookEnum LEDRedFunction;
	LEDHookEnum LEDGreenFunction;
} SettingsEntryType;

typedef struct {
	uint8_t ActiveSettingIdx;
	SettingsEntryType* ActiveSettingPtr;
	SettingsEntryType Settings[SETTINGS_COUNT];
} SettingsType;

extern SettingsType GlobalSettings;

void SettingsLoad(void);
void SettingsSave(void);

void SettingsCycle(void);
bool SettingsSetActiveById(uint8_t Setting);
uint8_t SettingsGetActiveById(void);
void SettingsGetActiveByName(char* SettingOut, uint16_t BufferSize);
bool SettingsSetActiveByName(const char* Setting);

#endif /* SETTINGS_H_ */
