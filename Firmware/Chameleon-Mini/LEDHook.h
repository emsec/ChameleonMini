/*
 * LEDHook.h
 *
 *  Created on: 02.12.2014
 *      Author: sk
 */

#ifndef LEDHOOK_H_
#define LEDHOOK_H_

#include "Settings.h"

INLINE void LEDHook(LEDHookEnum Func, LEDActionEnum Action) {
    extern LEDActionEnum LEDGreenAction;
    extern LEDActionEnum LEDRedAction;
#ifdef	USER_INTERFACE_MODE_CONFIGURABLE
if ( GlobalSettings.UserInterfaceMode == USER_INTERFACE_INDIVIDUAL) {	
	if (GlobalSettings.ActiveSettingPtr->LEDGreenFunction == Func) {
			LEDGreenAction = Action;
		}

		if (GlobalSettings.ActiveSettingPtr->LEDRedFunction == Func) {
			LEDRedAction = Action;
		}
	}
	else {	
		if (GLOBAL_UI_STORAGE.LEDGreenFunction == Func) {
			LEDGreenAction = Action;
		}

		if (GLOBAL_UI_STORAGE.LEDRedFunction == Func) {
			LEDRedAction = Action;
		}
	}
#else
    if (GlobalSettings.ActiveSettingPtr->LEDGreenFunction == Func) {
        LEDGreenAction = Action;
    }

    if (GlobalSettings.ActiveSettingPtr->LEDRedFunction == Func) {
        LEDRedAction = Action;
    }
#endif
}

#endif /* LEDHOOK_H_ */
