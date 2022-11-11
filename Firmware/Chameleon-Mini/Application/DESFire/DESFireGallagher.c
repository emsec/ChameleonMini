//
// Created by Tom on 27.10.2022.
//

#include "DESFireGallagher.h"


static const uint32_t CAD_AID = 0x2F81F4;

//Defaults to the default gallagher site key
uint8_t GallagherSiteKey[] = {
        0x31, 0x12, 0xB7, 0x38, 0xD8, 0x86, 0x2C, 0xCD,
        0x34, 0x30, 0x2E, 0xB2, 0x99, 0xAA, 0xB4, 0x56,
};

bool CreateGallagherApp(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode) {

    //Determine AID

    return CreateGallagherAppWithAID(cardId, facilityId, issueLevel, regionCode, AID);
}

bool CreateGallagherAppWithAID(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, uint8_t* AID) {

}

void SetGallagherSiteKey(uint8_t* key) {
    for (uint8_t i = 0; i < 16; ++i) {
        GallagherSiteKey[i] = key[i];
    }
}

void ResetGallagherSiteKey() {
    key[16] = {
            0x31, 0x12, 0xB7, 0x38, 0xD8, 0x86, 0x2C, 0xCD,
            0x34, 0x30, 0x2E, 0xB2, 0x99, 0xAA, 0xB4, 0x56,
    };
    SetGallagherSiteKey(key);
}
