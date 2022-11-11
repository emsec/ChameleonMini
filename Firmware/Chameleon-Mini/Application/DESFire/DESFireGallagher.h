//
// Created by Tom on 27.10.2022.
//

#ifndef CHAMELEON_MINI_DESFIREGALLAGHER_H
#define CHAMELEON_MINI_DESFIREGALLAGHER_H

bool CreateGallagherApp(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode);
bool CreateGallagherAppWithAID(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, uint8_t* AID);

#endif //CHAMELEON_MINI_DESFIREGALLAGHER_H
