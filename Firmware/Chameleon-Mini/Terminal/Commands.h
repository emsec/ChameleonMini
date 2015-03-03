

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "../Common.h"

#define MAX_COMMAND_LENGTH          16
#define MAX_STATUS_LENGTH           32


#define COMMAND_INFO_OK_ID              100
#define COMMAND_INFO_OK                 "OK"
#define COMMAND_INFO_OK_WITH_TEXT_ID    101
#define COMMAND_INFO_OK_WITH_TEXT       "OK WITH TEXT"
#define COMMAND_INFO_XMODEM_WAIT_ID     110
#define COMMAND_INFO_XMODEM_WAIT        "WAITING FOR XMODEM"
#define COMMAND_ERR_UNKNOWN_CMD_ID      200
#define COMMAND_ERR_UNKNOWN_CMD         "UNKNOWN COMMAND"
#define COMMAND_ERR_INVALID_USAGE_ID    201
#define COMMAND_ERR_INVALID_USAGE       "INVALID COMMAND USAGE"
#define COMMAND_ERR_INVALID_PARAM_ID    202
#define COMMAND_ERR_INVALID_PARAM       "INVALID PARAMETER"


#define COMMAND_CHAR_TRUE           '1'
#define COMMAND_CHAR_FALSE          '0'

#define COMMAND_UID_BUFSIZE         32

typedef uint8_t CommandStatusIdType;
typedef const char CommandStatusMessageType[MAX_STATUS_LENGTH];

typedef CommandStatusIdType (*CommandExecFuncType) (char* OutMessage);
typedef CommandStatusIdType (*CommandSetFuncType) (const char* InParam);
typedef CommandStatusIdType (*CommandGetFuncType) (char* OutParam);

typedef struct {
  char Command[MAX_COMMAND_LENGTH];
  CommandExecFuncType ExecFunc;
  CommandSetFuncType SetFunc;
  CommandGetFuncType GetFunc;
} CommandEntryType;

#define COMMAND_VERSION     "VERSION"
CommandStatusIdType CommandGetVersion(char* OutParam);

#define COMMAND_CONFIG      "CONFIG"
CommandStatusIdType CommandExecConfig(char* OutMessage);
CommandStatusIdType CommandGetConfig(char* OutParam);
CommandStatusIdType CommandSetConfig(const char* InParam);

#define COMMAND_UID         "UID"
#define COMMAND_UID_RANDOM  "RANDOM"
CommandStatusIdType CommandGetUid(char* OutParam);
CommandStatusIdType CommandSetUid(const char* InParam);

#define COMMAND_READONLY    "READONLY"
CommandStatusIdType CommandGetReadOnly(char* OutParam);
CommandStatusIdType CommandSetReadOnly(const char* InParam);

#define COMMAND_UPLOAD      "UPLOAD"
CommandStatusIdType CommandExecUpload(char* OutMessage);

#define COMMAND_DOWNLOAD    "DOWNLOAD"
CommandStatusIdType CommandExecDownload(char* OutMessage);

#define COMMAND_RESET       "RESET"
CommandStatusIdType CommandExecReset(char* OutMessage);

#define COMMAND_UPGRADE     "UPGRADE"
CommandStatusIdType CommandExecUpgrade(char* OutMessage);

#define COMMAND_MEMSIZE     "MEMSIZE"
CommandStatusIdType CommandGetMemSize(char* OutParam);

#define COMMAND_UIDSIZE     "UIDSIZE"
CommandStatusIdType CommandGetUidSize(char* OutParam);

#define COMMAND_BUTTON      "BUTTON"
CommandStatusIdType CommandExecButton(char* OutMessage);
CommandStatusIdType CommandGetButton(char* OutParam);
CommandStatusIdType CommandSetButton(const char* InParam);

#define COMMAND_BUTTON_LONG "BUTTON_LONG"
CommandStatusIdType CommandExecButtonLong(char* OutMessage);
CommandStatusIdType CommandGetButtonLong(char* OutParam);
CommandStatusIdType CommandSetButtonLong(const char* InParam);

#define COMMAND_LEDGREEN     "LEDGREEN"
CommandStatusIdType CommandExecLedGreen(char* OutMessage);
CommandStatusIdType CommandGetLedGreen(char* OutParam);
CommandStatusIdType CommandSetLedGreen(const char* InParam);

#define COMMAND_LEDRED     	"LEDRED"
CommandStatusIdType CommandExecLedRed(char* OutMessage);
CommandStatusIdType CommandGetLedRed(char* OutParam);
CommandStatusIdType CommandSetLedRed(const char* InParam);

#define COMMAND_LOGMODE     "LOGMODE"
CommandStatusIdType CommandExecLogMode(char* OutMessage);
CommandStatusIdType CommandGetLogMode(char* OutParam);
CommandStatusIdType CommandSetLogMode(const char* InParam);

#define COMMAND_LOGMEM      "LOGMEM"
#define COMMAND_LOGMEM_LOADBIN "LOADBIN"
#define COMMAND_LOGMEM_CLEAR "CLEAR"
CommandStatusIdType CommandExecLogMem(char* OutMessage);
CommandStatusIdType CommandGetLogMem(char* OutParam);
CommandStatusIdType CommandSetLogMem(const char* InParam);

#define COMMAND_SETTING		"SETTING"
CommandStatusIdType CommandGetSetting(char* OutParam);
CommandStatusIdType CommandSetSetting(const char* InParam);

#define COMMAND_CLEAR		"CLEAR"
CommandStatusIdType CommandExecClear(char* OutParam);

#define COMMAND_HELP        "HELP"
CommandStatusIdType CommandExecHelp(char* OutMessage);

#define COMMAND_RSSI		"RSSI"
CommandStatusIdType CommandGetRssi(char* OutParam);

#define COMMAND_LIST_END    ""
/* Defines the end of command list. This is no actual command */

#endif /* COMMANDS_H_ */
