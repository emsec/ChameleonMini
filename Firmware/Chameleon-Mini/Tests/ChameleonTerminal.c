/* ChameleonTerminal.c */

#ifdef ENABLE_RUNTESTS_TERMINAL_COMMAND

#include "ChameleonTerminal.h"
#include "CryptoTests.h"

CommandStatusIdType CommandRunTests(char *OutParam) {
    const ChameleonTestType testCases[] = {
#ifdef ENABLE_CRYPTO_TESTS
        &CryptoTDEATestCase1,
        &CryptoTDEATestCase2,
        &CryptoAESTestCase1,
        &CryptoAESTestCase2,
#endif
    };
    uint32_t t;
    uint16_t maxOutputChars = TERMINAL_BUFFER_SIZE, charCount = 0, testsFailedCount = 0;
    bool statusPassed = true;
    for (t = 0; t < ARRAY_COUNT(testCases); t++) {
        if (!testCases[t](OutParam, maxOutputChars)) {
            size_t opLength = StringLength(OutParam, maxOutputChars);
            OutParam += opLength;
            maxOutputChars -= opLength;
            charCount = snprintf_P(OutParam, maxOutputChars, PSTR("> Test #% 2d ... [X]\r\n"), t + 1);
            maxOutputChars = maxOutputChars < charCount ? 0 : maxOutputChars - charCount;
            OutParam += charCount;
            statusPassed = false;
            ++testsFailedCount;
        }
    }
    if (statusPassed) {
        snprintf_P(OutParam, maxOutputChars, PSTR("All tests passed: %d / %d."),
                   ARRAY_COUNT(testCases), ARRAY_COUNT(testCases));
    } else {
        snprintf_P(OutParam, maxOutputChars, PSTR("Tests failed: %d / %d."),
                   testsFailedCount, ARRAY_COUNT(testCases));
    }
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

#endif /* ENABLE_RUNTESTS_TERMINAL_COMMAND */
