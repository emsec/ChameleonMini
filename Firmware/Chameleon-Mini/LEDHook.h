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

    if (GlobalSettings.ActiveSettingPtr->LEDGreenFunction == Func) {
        LEDGreenAction = Action;
    }

    if (GlobalSettings.ActiveSettingPtr->LEDRedFunction == Func) {
        LEDRedAction = Action;
    }
}

#endif /* LEDHOOK_H_ */
