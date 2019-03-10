/*
 * UserInterface.h
 *
 *  Created on: 30.11.2018
 *      Author: vrumfondel
 */

#ifndef USERINTERFACE_H_
#define USERINTERFACE_H_

#include "Common.h"

typedef enum {
    USER_INTERFACE_GLOBAL = 0,
    USER_INTERFACE_INDIVIDUAL,
    /* Must be last element */
    USER_INTERFACE_COUNT
} UserInterfaceModeEnum;

void UserInterfaceModeList(char* List, uint16_t BufferSize);
void UserInterfaceModeById(UserInterfaceModeEnum Mode);
void UserInterfaceGetModeByName(char* Mode, uint16_t BufferSize);
bool UserInterfaceSetModeByName(const char* Mode);


#endif /* USERINTERFACE_H_ */
