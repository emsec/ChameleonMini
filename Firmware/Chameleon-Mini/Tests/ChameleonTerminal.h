/* ChameleonTerminal.h */

#ifndef __TESTS_CHAMELEON_TERMINAL_H__
#define __TESTS_CHAMELEON_TERMINAL_H__

#include "../Terminal/Commands.h"
#include "../Terminal/CommandLine.h"
#include "../Terminal/Terminal.h"

typedef bool (*ChameleonTestType)(char*, uint16_t);

#define COMMAND_RUNTESTS                 "RUNTESTS"
CommandStatusIdType CommandRunTests(char *OutParam);

#endif
