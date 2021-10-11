#include "Chameleon-Mini.h"
#ifdef CHAMELEON_TINY_UART_MODE
#include "Uart.h"
#include "UartCmd.h"
#endif

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
#ifdef CHAMELEON_TINY_UART_MODE
    UartInit();
    UartCmdInit();
#endif

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
#ifdef CHAMELEON_TINY_UART_MODE
	    UartCmdTick();
#endif
            LEDHook(LED_POWERED, LED_ON);
        }
#ifdef CHAMELEON_TINY_UART_MODE
	UartTask();
	UartCmdTask();
#endif
        ApplicationTask();
        CodecTask();
        LogTask();
        TerminalTask();
    }
}
