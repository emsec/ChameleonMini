//
// Created by Tom on 27.10.2022.
//

#ifndef CHAMELEON_MINI_DESFIREGALLAGHER_H
#define CHAMELEON_MINI_DESFIREGALLAGHER_H

//Warning - running this function resets the AUTH state!
bool CreateGallagherApp(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode);

//Warning - running this function resets the AUTH state!
bool CreateGallagherAppWithAID(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, uint8_t* AID);

void SetGallagherSiteKey(uint8_t* key);
void ResetGallagherSiteKey();

#endif //CHAMELEON_MINI_DESFIREGALLAGHER_H
