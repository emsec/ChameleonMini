//
// Created by Tom on 27.10.2022.
//

#ifndef CHAMELEON_MINI_DESFIREGALLAGHER_H
#define CHAMELEON_MINI_DESFIREGALLAGHER_H

#include "DESFireCrypto.h"
#include "DESFireApplicationDirectory.h"
#include "Configuration.h"
#include "DESFirePICCControl.h"
#include "DESFireStatusCodes.h"

//Warning - running this function resets the AUTH state!
bool CreateGallagher(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode);

//Warning - running this function resets the AUTH state!
bool CreateGallagherWithAID(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, uint8_t* AID);

bool CreateGallagherAppWithAID(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, DESFireAidType AID);
bool UpdateGallagherFile(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, DESFireAidType AID);
bool UpdateGallagherAppCardID(uint32_t cardId);
void SelectGallagherAID(DESFireAidType AID);

void SetGallagherSiteKey(uint8_t* key);
void ResetGallagherSiteKey();

#endif //CHAMELEON_MINI_DESFIREGALLAGHER_H
