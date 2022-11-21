/*
This file from this firmware source
is free software written by Tomas Preucil (github.com/tomaspre):
You can redistribute it and/or modify
it under the terms of this license.

This software is intended for demonstration and testing purposes on your own hardware only.
When setting up a Gallagher system, always use a non-default site key!

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

This notice must be retained at the top of all source files where indicated.
*/

/*
 * DESFireGallagher.h
 * Tomas Preucil (github.com/tomaspre)
 */

#ifndef CHAMELEON_MINI_DESFIREGALLAGHER_H
#define CHAMELEON_MINI_DESFIREGALLAGHER_H

#include "DESFireCrypto.h"
#include "DESFireApplicationDirectory.h"
#include "Configuration.h"
#include "DESFirePICCControl.h"
#include "DESFireStatusCodes.h"

//Warning - running this function resets the AUTH state!
bool CreateGallagherCard(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode);

//Warning - running this function resets the AUTH state!
bool CreateGallagherCardWithAID(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, uint8_t* AID);

bool CreateGallagherAppWithAID(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, DESFireAidType AID);
bool UpdateGallagherFile(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, DESFireAidType AID);
bool UpdateGallagherAppCardID(uint32_t cardId);
void SelectGallagherAID(DESFireAidType AID);

void SetGallagherSiteKey(uint8_t* key);
void ResetGallagherSiteKey();

#endif //CHAMELEON_MINI_DESFIREGALLAGHER_H
