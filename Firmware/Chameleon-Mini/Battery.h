/*
 * Battery.h
 *
 *  Created on: 20.08.2014
 *      Author: sk
 */

#ifndef BATTERY_H_
#define BATTERY_H_

#include "Common.h"

#define BATTERY_PORT			PORTD
#define BATTERY_STAT_PIN		PIN0_bm
#define BATTERY_STAT_PINCTRL	PIN0CTRL
#define BATTERY_PORT_MASK		(BATTERY_STAT_PIN)

INLINE void BatteryInit(void) {
    BATTERY_PORT.DIRCLR = BATTERY_PORT_MASK;
    BATTERY_PORT.BATTERY_STAT_PINCTRL = PORT_OPC_PULLUP_gc;
}

INLINE bool BatteryIsCharging(void) {
    if (!(BATTERY_PORT.IN & BATTERY_STAT_PIN)) {
        return true;
    } else {
        return false;
    }
}

#endif /* BATTERY_H_ */
