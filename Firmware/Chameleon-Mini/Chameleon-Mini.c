#include "Chameleon-Mini.h"

int main(void) {
    SystemInit();
    SettingsLoad();
    LEDInit();
    PinInit();
    MemoryInit();
    CodecInitCommon();
    ConfigurationInit();
    TerminalInit();
    RandomInit();
    ButtonInit();
    AntennaLevelInit();
    LogInit();
    SystemInterruptInit();

    while (1) {
        if (SystemTick100ms()) {
            LEDTick(); // this has to be the first function called here, since it is time-critical -
            // the functions below may have non-negligible runtimes!
            PinTick();
            RandomTick();
            TerminalTick();
            ButtonTick();
            ApplicationTick();
            LogTick();
            CommandLineTick();
            AntennaLevelTick();
            LEDHook(LED_POWERED, LED_ON);
        }
        ApplicationTask();
        CodecTask();
        LogTask();
        TerminalTask();
    }
}

