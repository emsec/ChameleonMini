#include "Chameleon-Mini.h"
#include "LEDHook.h"

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


    while(1) 
	{
        if (SystemTick100ms()) 
		{
			RandomTick();
			TerminalTick();
			ButtonTick();
			LogTick();
			LEDTick();
			ApplicationTick();

			LEDHook(LED_POWERED, LED_ON);
        }

		TerminalTask();
		ApplicationTask();
		LogTask();
		CodecTask();
    }
}



