#ifdef CHAMELEON_TINY_UART_MODE

#include "Uart.h"

void ProcessByte(void);

void UartCmdInit(void);
void UartCmdTick(void);
void UartCmdTask(void);

//    Command header ID
#define CMD_HEAD_SIGN 0xA5

//    Command reply ID
#define CMD_SEND_ACK 0x80

//    UART data command 'U'
#define CMD_UART_RXTX 0x75

//    TEST data command 't'
#define CMD_BLE_TEST 0x74

//    Heartbeat command 'a'
#define CMD_TYPE_ALIVE 0x61

//    Power down command 'p'
#define CMD_TYPE_POWERDOWN  0x70



//#pragma pack(push,1)
//    GCC Use another alignment command  __attribute__ ((aligned (1)))
//    Command structure
typedef struct {
    uint8_t bSign;          //    Head sign
    uint8_t bCmd;           //    Command type
    uint8_t bCmdLen;        //    Command length (without header)
    uint8_t bChkSum;        //    Command checksum
} CMD_HEAD, *PCMD_HEAD __attribute__((aligned(1)));

//    Heartbeat command
typedef struct {
    uint8_t bMajVer;        //    main version
    uint8_t bMinVer;        //    Sub version number
    uint8_t bVerStr[32];    //    Version string
} CMD_KEEP_ALIVE, *PCMD_KEEP_ALIVE __attribute__((aligned(1)));

//    Serial data transmission
typedef struct {
    uint16_t wBitCount;     //    Length of data sent, unit: bit
    uint8_t bFlag;          //    Generate check bit, CRC and other flags
} CMD_SEND_DATA, *PCMD_SEND_DATA __attribute__((aligned(1)));


uint8_t GetChkSum(PCMD_HEAD pCmdHead, uint8_t *DataPtr);

//#pragma pack(pop)

#endif
