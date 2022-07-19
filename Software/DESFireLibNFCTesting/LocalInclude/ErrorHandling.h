/* ErrorHandling.h */

#ifndef __LOCAL_ERROR_HANDLING_H__
#define __LOCAL_ERROR_HANDLING_H__

#include <string.h>
#include <stdint.h>
#include <errno.h>

#define GetSourceFileLoggingData(outputDataBuf, maxBufSize)      ({    \
        char *strBuffer;                                               \
        do {                                                           \
        snprintf(outputDataBuf, maxBufSize,                            \
                 "@@ LINE #%d in *%s @@",                              \
                 __LINE__, __FILE__);                                  \
        outputDataBuf[maxBufSize - 1] = '\0';                          \
        } while(0);                                                    \
        strBuffer = outputDataBuf;                                     \
        strBuffer;                                                     \
        })

#define GetSymbolNameString(symbolName, outputDataBuf, maxBufSize)  ({ \
        char *strBuffer = NULL;                                        \
        do {                                                           \
        strncpy(outputDataBuf, #symbolName, maxBufSize);               \
        outputDataBuf[maxBufSize - 1] = '\0';                          \
        } while(0);                                                    \
        strBuffer = outputDataBuf;                                     \
        strBuffer;                                                     \
        })

#define AppendSymbolNameString(symbolName, outputBuf, maxBufSize)   ({     \
        size_t bufLength = strlen(outputBuf);                              \
        GetSymbolNameString(symbolName, outputBuf + bufLength, maxBufSize) \
        })

typedef enum {
    NO_ERROR = 0,
    LIBC_ERROR,
    LIBNFC_ERROR,
    GENERIC_OTHER_ERROR,
    INVALID_PARAMS_ERROR,
    AES_AUTH_FAILED,
    DATA_LENGTH_ERROR,
    LAST_ERROR,
} ErrorType_t;

static const char *LOCAL_ERROR_MSGS[] = {
    [NO_ERROR]                    = "No error",
    [LIBC_ERROR]                  = "Libc function error",
    [GENERIC_OTHER_ERROR]         = "Unspecified (generic) error",
    [INVALID_PARAMS_ERROR]        = "Invalid parameters",
    [AES_AUTH_FAILED]             = "AES auth procedure failed (generic)",
    [DATA_LENGTH_ERROR]           = "Data length error (buffer size too large)",
    [LAST_ERROR]                  = NULL,
};

static bool RUNTIME_QUIET_MODE = false;
static bool RUNTIME_VERBOSE_MODE = true;
static bool PRINT_STATUS_EXCHANGE_MESSAGES = true;

#define STATUS_OK                       (0)

#endif
