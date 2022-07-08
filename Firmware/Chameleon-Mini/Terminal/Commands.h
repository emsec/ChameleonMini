

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
#define COMMAND_INFO_FALSE_ID			120
#define COMMAND_INFO_FALSE			"FALSE"
#define COMMAND_INFO_TRUE_ID			121
#define COMMAND_INFO_TRUE			"TRUE"
#define COMMAND_ERR_UNKNOWN_CMD_ID      200
#define COMMAND_ERR_UNKNOWN_CMD         "UNKNOWN COMMAND"
#define COMMAND_ERR_INVALID_USAGE_ID    201
#define COMMAND_ERR_INVALID_USAGE       "INVALID COMMAND USAGE"
#define COMMAND_ERR_INVALID_PARAM_ID    202
#define COMMAND_ERR_INVALID_PARAM       "INVALID PARAMETER"
#define COMMAND_ERR_TIMEOUT_ID		203
#define COMMAND_ERR_TIMEOUT			"TIMEOUT"
#define TIMEOUT_COMMAND				255 // this is just for the CommandLine module to know that this is a timeout command


#define COMMAND_CHAR_TRUE           '1'
#define COMMAND_CHAR_FALSE          '0'
#define COMMAND_CHAR_SUGGEST		 '?' /* <CMD>=? for help */

#define COMMAND_UID_BUFSIZE         32

#define COMMAND_IS_SUGGEST_STRING(x) ( ((x)[0] == COMMAND_CHAR_SUGGEST) && ((x)[1] == '\0') )

typedef uint8_t CommandStatusIdType;
typedef const char CommandStatusMessageType[MAX_STATUS_LENGTH];

typedef CommandStatusIdType(*CommandExecFuncType)(char *OutMessage);
typedef CommandStatusIdType(*CommandExecParamFuncType)(char *OutMessage, const char *InParams);
typedef CommandStatusIdType(*CommandSetFuncType)(char *OutMessage, const char *InParam);
typedef CommandStatusIdType(*CommandGetFuncType)(char *OutParam);

typedef struct {
    char Command[MAX_COMMAND_LENGTH];
    CommandExecFuncType ExecFunc;
    CommandExecParamFuncType ExecParamFunc;
    CommandSetFuncType SetFunc;
    CommandGetFuncType GetFunc;
} CommandEntryType;

#define COMMAND_VERSION       "VERSION"
CommandStatusIdType CommandGetVersion(char *OutParam);

#define COMMAND_CONFIG        "CONFIG"
CommandStatusIdType CommandGetConfig(char *OutParam);
CommandStatusIdType CommandSetConfig(char *OutMessage, const char *InParam);

#define COMMAND_UID           "UID"
#define COMMAND_UID_RANDOM    "RANDOM"
CommandStatusIdType CommandGetUid(char *OutParam);
CommandStatusIdType CommandSetUid(char *OutMessage, const char *InParam);

#define COMMAND_READONLY      "READONLY"
CommandStatusIdType CommandGetReadOnly(char *OutParam);
CommandStatusIdType CommandSetReadOnly(char *OutMessage, const char *InParam);

#define COMMAND_UPLOAD        "UPLOAD"
CommandStatusIdType CommandExecUpload(char *OutMessage);

#define COMMAND_DOWNLOAD      "DOWNLOAD"
CommandStatusIdType CommandExecDownload(char *OutMessage);

#define COMMAND_RESET         "RESET"
CommandStatusIdType CommandExecReset(char *OutMessage);

#define COMMAND_UPGRADE       "UPGRADE"
CommandStatusIdType CommandExecUpgrade(char *OutMessage);

#define COMMAND_MEMSIZE       "MEMSIZE"
CommandStatusIdType CommandGetMemSize(char *OutParam);

#define COMMAND_UIDSIZE       "UIDSIZE"
CommandStatusIdType CommandGetUidSize(char *OutParam);

#define COMMAND_RBUTTON       "RBUTTON"
CommandStatusIdType CommandGetRButton(char *OutParam);
CommandStatusIdType CommandSetRButton(char *OutMessage, const char *InParam);

#define COMMAND_RBUTTON_LONG  "RBUTTON_LONG"
CommandStatusIdType CommandGetRButtonLong(char *OutParam);
CommandStatusIdType CommandSetRButtonLong(char *OutMessage, const char *InParam);

#define COMMAND_LBUTTON       "LBUTTON"
CommandStatusIdType CommandGetLButton(char *OutParam);
CommandStatusIdType CommandSetLButton(char *OutMessage, const char *InParam);

#define COMMAND_LBUTTON_LONG  "LBUTTON_LONG"
CommandStatusIdType CommandGetLButtonLong(char *OutParam);
CommandStatusIdType CommandSetLButtonLong(char *OutMessage, const char *InParam);


#define COMMAND_LEDGREEN      "LEDGREEN"
CommandStatusIdType CommandGetLedGreen(char *OutParam);
CommandStatusIdType CommandSetLedGreen(char *OutMessage, const char *InParam);

#define COMMAND_LEDRED     	"LEDRED"
CommandStatusIdType CommandGetLedRed(char *OutParam);
CommandStatusIdType CommandSetLedRed(char *OutMessage, const char *InParam);

#define COMMAND_PIN           "PIN"
CommandStatusIdType CommandGetPin(char *OutParam);
CommandStatusIdType CommandSetPin(char *OutMessage, const char *InParam);

#define COMMAND_LOGMODE       "LOGMODE"
CommandStatusIdType CommandGetLogMode(char *OutParam);
CommandStatusIdType CommandSetLogMode(char *OutMessage, const char *InParam);

#define COMMAND_LOGMEM        "LOGMEM"
CommandStatusIdType CommandGetLogMem(char *OutParam);

#define COMMAND_LOGDOWNLOAD	"LOGDOWNLOAD"
CommandStatusIdType CommandExecLogDownload(char *OutMessage);

#define COMMAND_STORELOG	     "LOGSTORE"
CommandStatusIdType CommandExecStoreLog(char *OutMessage);

#define COMMAND_LOGCLEAR	     "LOGCLEAR"
CommandStatusIdType CommandExecLogClear(char *OutMessage);

#define COMMAND_SETTING		"SETTING"
CommandStatusIdType CommandGetSetting(char *OutParam);
CommandStatusIdType CommandSetSetting(char *OutMessage, const char *InParam);

#define COMMAND_CLEAR		"CLEAR"
CommandStatusIdType CommandExecClear(char *OutMessage);

#define COMMAND_STORE		"STORE"
CommandStatusIdType CommandExecStore(char *OutMessage);

#define COMMAND_RECALL		"RECALL"
CommandStatusIdType CommandExecRecall(char *OutMessage);

#define COMMAND_CHARGING 	"CHARGING"
CommandStatusIdType CommandGetCharging(char *OutParam);

#define COMMAND_HELP          "HELP"
CommandStatusIdType CommandExecHelp(char *OutMessage);

#define COMMAND_RSSI		"RSSI"
CommandStatusIdType CommandGetRssi(char *OutParam);

#define COMMAND_SYSTICK		"SYSTICK"
CommandStatusIdType CommandGetSysTick(char *OutParam);

#define COMMAND_SEND_RAW	     "SEND_RAW"
CommandStatusIdType CommandExecParamSendRaw(char *OutMessage, const char *InParams);

#define COMMAND_SEND		"SEND"
CommandStatusIdType CommandExecParamSend(char *OutMessage, const char *InParams);

#define COMMAND_GETUID		"GETUID"
CommandStatusIdType CommandExecGetUid(char *OutMessage);

#define COMMAND_DUMP_MFU	     "DUMP_MFU"
CommandStatusIdType CommandExecDumpMFU(char *OutMessage);

#define COMMAND_CLONE_MFU	"CLONE_MFU"
CommandStatusIdType CommandExecCloneMFU(char *OutMessage);

#define COMMAND_IDENTIFY_CARD	"IDENTIFY"
CommandStatusIdType CommandExecIdentifyCard(char *OutMessage);

#define COMMAND_TIMEOUT		"TIMEOUT"
CommandStatusIdType	CommandGetTimeout(char *OutMessage);
CommandStatusIdType	CommandSetTimeout(char *OutMessage, const char *InParam);

#define COMMAND_THRESHOLD	"THRESHOLD"
CommandStatusIdType CommandGetThreshold(char *OutParam);
CommandStatusIdType CommandSetThreshold(char *OutMessage, const char *InParam);

#define COMMAND_AUTOCALIBRATE "AUTOCALIBRATE"
CommandStatusIdType CommandExecAutocalibrate(char *OutMessage);

#define COMMAND_FIELD	     "FIELD"
CommandStatusIdType CommandSetField(char *OutMessage, const char *InParam);
CommandStatusIdType CommandGetField(char *OutMessage);

#define COMMAND_CLONE         "CLONE"
CommandStatusIdType CommandExecClone(char *OutMessage);

#ifdef CONFIG_ISO15693_SNIFF_SUPPORT
#define COMMAND_AUTOTHRESHOLD "AUTOTHRESHOLD"
CommandStatusIdType CommandGetAutoThreshold(char *OutParam);
CommandStatusIdType CommandSetAutoThreshold(char *OutMessage, const char *InParam);
#endif /*#ifdef CONFIG_ISO15693_SNIFF_SUPPORT*/

#ifdef ENABLE_RUNTESTS_TERMINAL_COMMAND
#include "../Tests/ChameleonTerminal.h"
#endif

#if defined(CONFIG_MF_DESFIRE_SUPPORT) && !defined(DISABLE_DESFIRE_TERMINAL_COMMANDS)
#include "../Application/DESFire/DESFireChameleonTerminal.h"
#endif

#define COMMAND_LIST_END    ""
/* Defines the end of command list. This is no actual command */

#endif /* COMMANDS_H_ */
