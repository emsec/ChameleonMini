//
// Created by Tom on 27.10.2022.
//

#include "DESFireGallagher.h"
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
    uint8_t Status;
    InvalidateAuthState(false);

    //TODO: Is this needed?
    Authenticated = true;
    AuthenticatedWithKey = 0;
    AuthenticatedWithPICCMasterKey = true;

    //Create card app directory app
    const DESFireAidType CADAid = {0x2F, 0x81, 0xF4}; //0x2F81F4
    Status = CreateApp(CADAid, 0x0B, 1);

    //TODO: Difersify key for app directory app
    uint8_t[CRYPTO_AES_KEY_SIZE] CADKeyZero;

    //Select the app direcory app
    SelectApp(CADAid);

    //Channge key
    uint8_t nextKeyVersion = ReadKeyVersion(SelectedApp.Slot, KeyId) + 1;
    WriteAppKey(SelectedApp.Slot, 0, CADKeyZero, CRYPTO_AES_KEY_SIZE);
    WriteKeyVersion(SelectedApp.Slot, 0, nextKeyVersion);
    WriteKeyCryptoType(SelectedApp.Slot, 0, CRYPTO_TYPE_AES128);

    //Select the app direcory app
    SelectApp(CADAid);

    //Create file in CAD
    Status = CreateStandardFile(0x00, 0x00, 0xE000, CAD_BLOCK_LEN);

    //TODO: Get CAD block to write
    uint8_t[CAD_BLOCK_LEN] CADBlock;

    //Update file in CAD
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, 0);
    uint8_t fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    Status = WriteDataFileSetup(fileIndex, fileType, 0x00, 0, CAD_BLOCK_LEN);
    Status = WriteDataFileIterator(CADBlock, CAD_BLOCK_LEN);

    //Create Gall app
    Status = CreateApp(AID, 0x0B, 3);

    //Diversify and change key 2
    uint8_t[CRYPTO_AES_KEY_SIZE] GallAppKeyTwo;
    //TODO: Diversify

    nextKeyVersion = ReadKeyVersion(SelectedApp.Slot, KeyId) + 1;
    WriteAppKey(SelectedApp.Slot, 2, GallAppKeyTwo, CRYPTO_AES_KEY_SIZE);
    WriteKeyVersion(SelectedApp.Slot, 0, nextKeyVersion);
    WriteKeyCryptoType(SelectedApp.Slot, 0, CRYPTO_TYPE_AES128);

    //Diversify and change key 0
    uint8_t[CRYPTO_AES_KEY_SIZE] GallAppKeyZero;
    //TODO: Diversify

    nextKeyVersion = ReadKeyVersion(SelectedApp.Slot, KeyId) + 1;
    WriteAppKey(SelectedApp.Slot, 0, GallAppKeyZero, CRYPTO_AES_KEY_SIZE);
    WriteKeyVersion(SelectedApp.Slot, 0, nextKeyVersion);
    WriteKeyCryptoType(SelectedApp.Slot, 0, CRYPTO_TYPE_AES128);

    //Create file
    Status = CreateStandardFile(0x00, 0x03, 0x2000, GALL_BLOCK_LEN);

    //TODO: Get CAD block to write
    uint8_t[GALL_BLOCK_LEN] GallBlock;

    //Update file with Gall access data
    fileIndex = LookupFileNumberIndex(SelectedApp.Slot, 0);
    fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    Status = WriteDataFileSetup(fileIndex, fileType, 0x03, 0, GALL_BLOCK_LEN);
    Status = WriteDataFileIterator(GallBlock, GALL_BLOCK_LEN);


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
