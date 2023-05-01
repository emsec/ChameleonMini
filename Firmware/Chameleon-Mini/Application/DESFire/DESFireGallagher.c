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
 * DESFireGallagher.c
 * Tomas Preucil (github.com/tomaspre)
 */

#include <string.h>
#include "DESFireGallagher.h"
#include "DESFireGallagherTools.h"
#include "DESFireLogging.h"
#include "DESFireUtils.h"
//SelectedApp
//Authenticated, wthkey, wthmasterkey
//CRYPTO_AES_KEY_SIZE

#define CAD_BLOCK_LEN 0x24
#define GALL_BLOCK_LEN 0x10

uint32_t lastCardId = 0xFFFFFFFF;
uint16_t lastFacilityId = 0xFFFF;
uint8_t lastIssueLevel = 0xFF;
uint8_t lastRegionCode = 0xFF;
DESFireAidType selectedGallagherAID = {0xFF, 0xFF, 0xFF};


//Defaults to the default gallagher site key
uint8_t GallagherSiteKey[16] = {
        0x31, 0x12, 0xB7, 0x38, 0xD8, 0x86, 0x2C, 0xCD,
        0x34, 0x30, 0x2E, 0xB2, 0x99, 0xAA, 0xB4, 0x56,
};

//Warning - running this function resets the AUTH state!
bool CreateGallagherCard(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode) {

    //TODO: Find a suitable AID
    DESFireAidType AID = {0xF4, 0x81, 0x20};

    return CreateGallagherCardWithAID(cardId, facilityId, issueLevel, regionCode, AID);
}

//Warning - running this function resets the AUTH state!
bool CreateGallagherCardWithAID(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, DESFireAidType AID) {
    uint8_t Status = 0;
    ConfigurationUidType UID_GALL;
    GetPiccUid(UID_GALL);
    InvalidateAuthState(false);

    //TODO: Is this needed?
    Authenticated = true;
    AuthenticatedWithKey = 0;
    AuthenticatedWithPICCMasterKey = true;

    //Create card app directory app
    DESFireAidType CADAid;
    CADAid[0] = 0xF4;
    CADAid[1] = 0x81;
    CADAid[2] = 0x2F;//0x2F81F4
    uint8_t KeyCount = 1;
    uint8_t KeySettings = 0x0B;
    Status = CreateApp(CADAid, KeyCount, KeySettings);

    if (Status != STATUS_OPERATION_OK) {
        DEBUG_PRINT_P(PSTR("Err: %u"), Status);
        return false;
    }

    //Difersify key for app directory app
    uint8_t CADKeyZero[CRYPTO_AES_KEY_SIZE];
    hfgal_diversify_key(GallagherSiteKey, UID_GALL, 7, 0, CADAid, CADKeyZero);

    //Select the app direcory app
    SelectApp(CADAid);

    //Channge key
    uint8_t nextKeyVersion = ReadKeyVersion(SelectedApp.Slot, 0) + 1;
    WriteAppKey(SelectedApp.Slot, 0, CADKeyZero, CRYPTO_AES_KEY_SIZE);
    WriteKeyVersion(SelectedApp.Slot, 0, nextKeyVersion);
    WriteKeyCryptoType(SelectedApp.Slot, 0, CRYPTO_TYPE_AES128);

    //Select the app direcory app
    SelectApp(CADAid);

    //Create file in CAD
    Status = CreateStandardFile(0x00, 0x00, 0xE000, CAD_BLOCK_LEN);

    if (Status != STATUS_OPERATION_OK) {
        return false;
    }

    //TODO: This needs to be rewritten to support more than one Gallagher app on the card
    //Get CAD block to write
    uint8_t CADBlock[CAD_BLOCK_LEN];
    CADBlock[0] = regionCode;
    CADBlock[1] = (facilityId >> 8) & 0xFF;
    CADBlock[2] = facilityId & 0xFF;
    CADBlock[3] = AID[2];
    CADBlock[4] = AID[1];
    CADBlock[5] = AID[0];
    memset(CADBlock+6, 0, CAD_BLOCK_LEN-6);

    //Update file in CAD
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, 0);
    uint8_t fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    Status = WriteDataFileSetup(fileIndex, fileType, 0x00, 0, CAD_BLOCK_LEN);

    if (Status != STATUS_OPERATION_OK) {
        return false;
    }

    Status = WriteDataFileIterator(CADBlock, CAD_BLOCK_LEN);

    if (Status != STATUS_OPERATION_OK) {
        return false;
    }

    return CreateGallagherAppWithAID(cardId, facilityId, issueLevel, regionCode, AID);
}

void SetGallagherSiteKey(uint8_t* key) {
    for (uint8_t i = 0; i < 16; ++i) {
        GallagherSiteKey[i] = key[i];
    }
}

void ResetGallagherSiteKey() {
    uint8_t key[16] = {
            0x31, 0x12, 0xB7, 0x38, 0xD8, 0x86, 0x2C, 0xCD,
            0x34, 0x30, 0x2E, 0xB2, 0x99, 0xAA, 0xB4, 0x56,
    };
    SetGallagherSiteKey(key);
}

bool CreateGallagherAppWithAID(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, DESFireAidType AID) {
    //Create Gall app
    uint8_t KeyCount = 3;
    uint8_t KeySettings = 0x0B;
    uint8_t Status = CreateApp(AID, KeyCount, KeySettings);

    ConfigurationUidType UID_GALL;
    GetPiccUid(UID_GALL);

    SelectApp(AID);

    //Diversify and change key 2
    uint8_t GallAppKeyTwo[CRYPTO_AES_KEY_SIZE];
    hfgal_diversify_key(GallagherSiteKey, UID_GALL, 7, 2, AID, GallAppKeyTwo);

    uint8_t nextKeyVersion = ReadKeyVersion(SelectedApp.Slot, 2) + 1;
    WriteAppKey(SelectedApp.Slot, 2, GallAppKeyTwo, CRYPTO_AES_KEY_SIZE);
    WriteKeyVersion(SelectedApp.Slot, 2, nextKeyVersion);
    WriteKeyCryptoType(SelectedApp.Slot, 2, CRYPTO_TYPE_AES128);

    //Diversify and change key 0
    uint8_t GallAppKeyZero[CRYPTO_AES_KEY_SIZE];
    hfgal_diversify_key(GallagherSiteKey, UID_GALL, 7, 0, AID, GallAppKeyZero);

    nextKeyVersion = ReadKeyVersion(SelectedApp.Slot, 0) + 1;
    WriteAppKey(SelectedApp.Slot, 0, GallAppKeyZero, CRYPTO_AES_KEY_SIZE);
    WriteKeyVersion(SelectedApp.Slot, 0, nextKeyVersion);
    WriteKeyCryptoType(SelectedApp.Slot, 0, CRYPTO_TYPE_AES128);

    //Create file
    Status = CreateStandardFile(0x00, 0x03, 0x2000, GALL_BLOCK_LEN);

    if (Status != STATUS_OPERATION_OK) {
        DEBUG_PRINT_P(PSTR("Err: %u"), Status);
        return false;
    }

    selectedGallagherAID[0] = AID[0];
    selectedGallagherAID[1] = AID[1];
    selectedGallagherAID[2] = AID[2];

    return UpdateGallagherFile(cardId, facilityId, issueLevel, regionCode, AID);

}

bool UpdateGallagherFile(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, DESFireAidType AID) {
    SelectApp(AID);

    //Get Gallagher block to write
    uint8_t GallBlock[GALL_BLOCK_LEN];
    gallagher_encode_creds(GallBlock, regionCode, facilityId, cardId, issueLevel);
    for (int i = 0; i < 8; i++) {
        GallBlock[i + 8] = GallBlock[i] ^ 0xFF;
    }

    //Update file with Gall access data
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, 0);
    uint8_t fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    uint8_t Status = WriteDataFileSetup(fileIndex, fileType, 0x03, 0, GALL_BLOCK_LEN);

    if (Status != STATUS_OPERATION_OK) {
        return false;
    }

    Status = WriteDataFileIterator(GallBlock, GALL_BLOCK_LEN);

    if (Status != STATUS_OPERATION_OK) {
        return false;
    }

    lastCardId = cardId;
    lastFacilityId = facilityId;
    lastIssueLevel = issueLevel;
    lastRegionCode = regionCode;

    return true;
}

bool UpdateGallagherAppCardID(uint32_t cardId) {
    if ((lastCardId == 0xFFFFFFFF && lastFacilityId == 0xFFFF && lastIssueLevel == 0xFF && lastRegionCode == 0xFF) ||
            (selectedGallagherAID[0] == 0xFF && selectedGallagherAID[1] == 0xFF && selectedGallagherAID[2] == 0xFF)) {
        return false;
    }

    return UpdateGallagherFile(cardId, lastFacilityId, lastIssueLevel, lastRegionCode, selectedGallagherAID);
}

void SelectGallagherAID(DESFireAidType AID) {
    selectedGallagherAID[0] = AID[0];
    selectedGallagherAID[1] = AID[1];
    selectedGallagherAID[2] = AID[2];
}
