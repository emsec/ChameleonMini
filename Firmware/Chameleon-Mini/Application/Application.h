/*
 * Application.h
 *
 *  Created on: 18.02.2013
 *      Author: skuser
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "../Common.h"
#include "../Configuration.h"
#include "../Log.h"

/* Applications */
#include "MifareUltralight.h"
#include "MifareClassic.h"
#include "Reader14443A.h"
#include "Vicinity.h"
#include "Sl2s2002.h"
#include "TITagitstandard.h"
#include "TITagitplus.h"
#include "Sniff14443A.h"
#include "NTAG21x.h"
#include "EM4233.h"
#include "Sniff15693.h"

/* Function wrappers */
INLINE void ApplicationInit(void) {
    ActiveConfiguration.ApplicationInitFunc();
}

INLINE void ApplicationTask(void) {
    ActiveConfiguration.ApplicationTaskFunc();
}

INLINE void ApplicationTick(void) {
    ActiveConfiguration.ApplicationTickFunc();
}

INLINE uint16_t ApplicationProcess(uint8_t *ByteBuffer, uint16_t ByteCount) {
    return ActiveConfiguration.ApplicationProcessFunc(ByteBuffer, ByteCount);
}

INLINE void ApplicationReset(void) {
    ActiveConfiguration.ApplicationResetFunc();
    //LogEntry(LOG_INFO_RESET_APP, NULL, 0);
}

INLINE void ApplicationGetUid(ConfigurationUidType Uid) {
    ActiveConfiguration.ApplicationGetUidFunc(Uid);
}

INLINE void ApplicationSetUid(ConfigurationUidType Uid) {
    ActiveConfiguration.ApplicationSetUidFunc(Uid);
    LogEntry(LOG_INFO_UID_SET, Uid, ActiveConfiguration.UidSize);
}

#endif /* APPLICATION_H_ */
