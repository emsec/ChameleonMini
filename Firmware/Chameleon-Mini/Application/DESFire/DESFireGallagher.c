//
// Created by Tom on 27.10.2022.
//

#include <string.h>
#include "DESFireGallagher.h"
#include "DESFireGallagherTools.h"
//SelectedApp
//Authenticated, wthkey, wthmasterkey
//CRYPTO_AES_KEY_SIZE

#define CAD_BLOCK_LEN 0x24
#define GALL_BLOCK_LEN 0x10


//Defaults to the default gallagher site key
uint8_t GallagherSiteKey[] = {
        0x31, 0x12, 0xB7, 0x38, 0xD8, 0x86, 0x2C, 0xCD,
        0x34, 0x30, 0x2E, 0xB2, 0x99, 0xAA, 0xB4, 0x56,
};

//Warning - running this function resets the AUTH state!
bool CreateGallagherApp(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode) {

    //TODO: Find a suitable AID
    DESFireAidType AID = {0x20, 0x81, 0xF4};

    return CreateGallagherAppWithAID(cardId, facilityId, issueLevel, regionCode, AID);
}

//Warning - running this function resets the AUTH state!
bool CreateGallagherAppWithAID(uint32_t cardId, uint16_t facilityId, uint8_t issueLevel, uint8_t regionCode, DESFireAidType AID) {
    uint8_t Status = 0;
    ConfigurationUidType UID_GALL;
    GetPiccUid(UID_GALL);
    InvalidateAuthState(false);

    //TODO: Is this needed?
    Authenticated = true;
    AuthenticatedWithKey = 0;
    AuthenticatedWithPICCMasterKey = true;

    //Create card app directory app
    const DESFireAidType CADAid = {0x2F, 0x81, 0xF4}; //0x2F81F4
    Status = CreateApp(CADAid, 0x0B, 1);

    if (Status != STATUS_OPERATION_OK) {
        return false;
    }

    //Difersify key for app directory app
    uint8_t CADKeyZero[CRYPTO_AES_KEY_SIZE];
    hfgal_diversify_key(GallagherSiteKey, UID_GALL, 7, 0, CADAid[0]*256*256 + CADAid[1]*256 + CADAid[2], CADKeyZero);

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
    CADBlock[3] = CADAid[0];
    CADBlock[4] = CADAid[1];
    CADBlock[5] = CADAid[2];
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

    //Create Gall app
    Status = CreateApp(AID, 0x0B, 3);

    //Diversify and change key 2
    uint8_t GallAppKeyTwo[CRYPTO_AES_KEY_SIZE];
    hfgal_diversify_key(GallagherSiteKey, UID_GALL, 7, 2, AID[0]*256*256 + AID[1]*256 + AID[2], GallAppKeyTwo);

    nextKeyVersion = ReadKeyVersion(SelectedApp.Slot, 2) + 1;
    WriteAppKey(SelectedApp.Slot, 2, GallAppKeyTwo, CRYPTO_AES_KEY_SIZE);
    WriteKeyVersion(SelectedApp.Slot, 0, nextKeyVersion);
    WriteKeyCryptoType(SelectedApp.Slot, 0, CRYPTO_TYPE_AES128);

    //Diversify and change key 0
    uint8_t GallAppKeyZero[CRYPTO_AES_KEY_SIZE];
    hfgal_diversify_key(GallagherSiteKey, UID_GALL, 7, 0, AID[0]*256*256 + AID[1]*256 + AID[2], GallAppKeyZero);

    nextKeyVersion = ReadKeyVersion(SelectedApp.Slot, 0) + 1;
    WriteAppKey(SelectedApp.Slot, 0, GallAppKeyZero, CRYPTO_AES_KEY_SIZE);
    WriteKeyVersion(SelectedApp.Slot, 0, nextKeyVersion);
    WriteKeyCryptoType(SelectedApp.Slot, 0, CRYPTO_TYPE_AES128);

    //Create file
    Status = CreateStandardFile(0x00, 0x03, 0x2000, GALL_BLOCK_LEN);

    if (Status != STATUS_OPERATION_OK) {
        return false;
    }

    //Get Gallagher block to write
    uint8_t GallBlock[GALL_BLOCK_LEN];
    gallagher_encode_creds(GallBlock, regionCode, facilityId, cardId, issueLevel);
    for (int i = 0; i < 8; i++) {
        GallBlock[i + 8] = GallBlock[i] ^ 0xFF;
    }

    //Update file with Gall access data
    fileIndex = LookupFileNumberIndex(SelectedApp.Slot, 0);
    fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    Status = WriteDataFileSetup(fileIndex, fileType, 0x03, 0, GALL_BLOCK_LEN);

    if (Status != STATUS_OPERATION_OK) {
        return false;
    }

    Status = WriteDataFileIterator(GallBlock, GALL_BLOCK_LEN);

    if (Status != STATUS_OPERATION_OK) {
        return false;
    }

    return true;
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
