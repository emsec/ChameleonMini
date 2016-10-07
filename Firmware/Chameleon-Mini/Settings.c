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

	.Settings = { [0 ... (SETTINGS_COUNT-1)] =	{
		.Configuration = DEFAULT_CONFIGURATION,
		.ButtonActions = {
				[BUTTON_L_PRESS_SHORT] = DEFAULT_LBUTTON_ACTION, [BUTTON_R_PRESS_SHORT] = DEFAULT_RBUTTON_ACTION,
				[BUTTON_L_PRESS_LONG]  = DEFAULT_LBUTTON_ACTION, [BUTTON_R_PRESS_LONG]  = DEFAULT_RBUTTON_ACTION
		},
		.LogMode = DEFAULT_LOG_MODE,
		.LEDRedFunction = DEFAULT_RED_LED_ACTION,
		.LEDGreenFunction = DEFAULT_GREEN_LED_ACTION,
		.PendingTaskTimeout = DEFAULT_PENDING_TASK_TIMEOUT
	}}
};

static inline void NVM_EXEC(void)
{
	void *z = (void *)&NVM_CTRLA;

	__asm__ volatile("out %[ccp], %[ioreg]"  "\n\t"
	"st z, %[cmdex]"
	:
	: [ccp] "I" (_SFR_IO_ADDR(CCP)),
	[ioreg] "d" (CCP_IOREG_gc),
				 [cmdex] "r" (NVM_CMDEX_bm),
				 [z] "z" (z)
				 );
}

void WaitForNVM(void)
{
        while (NVM.STATUS & NVM_NVMBUSY_bm) { };
}

void FlushNVMBuffer(void)
{
	WaitForNVM();

	if ((NVM.STATUS & NVM_EELOAD_bm) != 0) {
		NVM.CMD = NVM_CMD_ERASE_EEPROM_BUFFER_gc;
		NVM_EXEC();
	}
}

uint16_t ReadEEPBlock(uint16_t Address, void *DestPtr, uint16_t ByteCount)
{
	uint16_t BytesRead = 0;
	uint8_t* BytePtr = (uint8_t*) DestPtr;
	NVM.ADDR2 = 0;

	WaitForNVM();

	while (ByteCount > 0)
	{
			NVM.ADDR0 = Address & 0xFF;
			NVM.ADDR1 = (Address >> 8) & 0x1F;

			NVM.CMD = NVM_CMD_READ_EEPROM_gc;
			NVM_EXEC();

			*BytePtr++ = NVM.DATA0;
			Address++;

			ByteCount--;
			BytesRead++;
	}

	return BytesRead;
}


uint16_t WriteEEPBlock(uint16_t Address, const void *SrcPtr, uint16_t ByteCount)
{
	const uint8_t* BytePtr = (const uint8_t*) SrcPtr;
	uint8_t ByteAddress = Address % EEPROM_PAGE_SIZE;
	uint16_t PageAddress = Address - ByteAddress;
	uint16_t BytesWritten = 0;

	FlushNVMBuffer();
	WaitForNVM();
	NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;

	NVM.ADDR1 = 0;
	NVM.ADDR2 = 0;

	while (ByteCount > 0)
	{
		NVM.ADDR0 = ByteAddress;

		NVM.DATA0 = *BytePtr++;

		ByteAddress++;
		ByteCount--;

		if (ByteCount == 0 || ByteAddress >= EEPROM_PAGE_SIZE)
		{
			NVM.ADDR0 = PageAddress & 0xFF;
			NVM.ADDR1 = (PageAddress >> 8) & 0x1F;

			NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
			NVM_EXEC();

			PageAddress += EEPROM_PAGE_SIZE;
			ByteAddress = 0;

			WaitForNVM();

			NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;
		}

		BytesWritten++;
	}

	return BytesWritten;
}


void SettingsLoad(void) {
	ReadEEPBlock(0, &GlobalSettings, sizeof(SettingsType));
}

void SettingsSave(void) {
#if ENABLE_EEPROM_SETTINGS
	WriteEEPBlock(0, &GlobalSettings, sizeof(SettingsType));
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
	if ( (Setting >= SETTINGS_FIRST) && (Setting <= SETTINGS_LAST) ) {
		uint8_t SettingIdx = SETTING_TO_INDEX(Setting);

		/* Break potentially pending timeout task (manual timeout) */
		CommandLinePendingTaskBreak();

		/* Store current memory contents permanently */
		MemoryStore();

		GlobalSettings.ActiveSettingIdx = SettingIdx;
		GlobalSettings.ActiveSettingPtr =
				&GlobalSettings.Settings[SettingIdx];

		/* Settings have changed. Progress changes through system */
		ConfigurationSetById(GlobalSettings.ActiveSettingPtr->Configuration);
		LogSetModeById(GlobalSettings.ActiveSettingPtr->LogMode);

		/* Recall new memory contents */
		MemoryRecall();

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

void SettingsGetActiveByName(char* SettingOut, uint16_t BufferSize) {
	SettingOut[0] = SettingsGetActiveById() + '0';
	SettingOut[1] = '\0';
}

bool SettingsSetActiveByName(const char* Setting) {
	uint8_t SettingNr = Setting[0] - '0';

	if (Setting[1] == '\0') {
		LogEntry(LOG_INFO_SETTING_SET, Setting, 1);
		return SettingsSetActiveById(SettingNr);
	} else {
		return false;
	}
}

