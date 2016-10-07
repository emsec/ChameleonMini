#include "Chameleon-Mini.h"

int main(void)
{
    SystemInit();
    SettingsLoad();
    LogInit();
    LEDInit();
    MemoryInit();
    ConfigurationInit();
    TerminalInit();
    RandomInit();
    ButtonInit();
    AntennaLevelInit();

    SystemInterruptInit();
    SystemSleepEnable();

    while(1) {
        if (SystemTick100ms()) {
            RandomTick();
            TerminalTick();
            ButtonTick();
            LogTick();
            LEDTick();
        }

		TerminalTask();
		ApplicationTask();
		LogTask();
		CodecTask();
    }
}



