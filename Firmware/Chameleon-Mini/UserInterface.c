#include "UserInterface.h"
#include "Random.h"
#include "Common.h"
#include "Settings.h"
#include "Memory.h"
#include "Map.h"
#include "Terminal/CommandLine.h"
#include "Application/Application.h"



static const MapEntryType PROGMEM UserInterfaceModeMap[] = {
    { .Id = USER_INTERFACE_GLOBAL,					.Text = "GLOBAL" },
    { .Id = USER_INTERFACE_INDIVIDUAL,			.Text = "INDIVIDUAL" },
};

void UserInterfaceModeInit(void)
{
    ConfigurationSetById(GlobalSettings.UserInterfaceMode);
}

void UserInferfaceModeSetById( UserInterfaceModeEnum Mode )
{
     CommandLinePendingTaskBreak(); // break possibly pending task

    GlobalSettings.UserInterfaceMode = Mode;

}

void UserInterfaceGetModeByName(char* Mode, uint16_t BufferSize)
{
    MapIdToText(UserInterfaceModeMap, ARRAY_COUNT(UserInterfaceModeMap), GlobalSettings.UserInterfaceMode, Mode, BufferSize);
}

bool UserInterfaceSetModeByName(const char* Mode)
{
    MapIdType Id;

    if (MapTextToId(UserInterfaceModeMap, ARRAY_COUNT(UserInterfaceModeMap), Mode, &Id)) {
        UserInferfaceModeSetById(Id);
        LogEntry(LOG_INFO_CONFIG_SET, Mode, StringLength(Mode, CONFIGURATION_NAME_LENGTH_MAX-1));
        return true;
    } else {
        return false;
    }
}

void UserInterfaceModeList(char* List, uint16_t BufferSize)
{
    MapToString(UserInterfaceModeMap, ARRAY_COUNT(UserInterfaceModeMap), List, BufferSize);
}

