#include "Chameleon-Mini.h"

int main(void)
{
    SystemInit();
    SettingsLoad();
    LEDInit();
    MemoryInit();
    CodecInitCommon();
    ConfigurationInit();
    TerminalInit();
    RandomInit();
    ButtonInit();
    AntennaLevelInit();
    LogInit();
    SystemInterruptInit();

    while(1) {
		if (SystemTick100ms()) {
			RandomTick();
			TerminalTick();
			ButtonTick();
			LogTick();
			LEDTick();
			ApplicationTick();
			CommandLineTick();
			AntennaLevelTick();

			LEDHook(LED_POWERED, LED_ON);
		}

		TerminalTask();
		LogTask();
		ApplicationTask();
		CodecTask();
    }
}

