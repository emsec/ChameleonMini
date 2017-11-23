/*
 * Standards.h
 *
 *  Created on: 15.02.2013
 *      Author: skuser
 */

#ifndef STANDARDS_H_
#define STANDARDS_H_

#include <stdint.h>
#include <stdbool.h>

#define CONFIGURATION_NAME_LENGTH_MAX   32
#define CONFIGURATION_UID_SIZE_MAX      16

typedef uint8_t ConfigurationUidType[CONFIGURATION_UID_SIZE_MAX];

typedef enum  {
    /* This HAS to be the first element */
    CONFIG_NONE = 0,

#ifdef CONFIG_MF_ULTRALIGHT_SUPPORT
    CONFIG_MF_ULTRALIGHT,
#endif
#ifdef CONFIG_MF_CLASSIC_1K_SUPPORT
    CONFIG_MF_CLASSIC_1K,
#endif
#ifdef CONFIG_MF_CLASSIC_4K_SUPPORT
    CONFIG_MF_CLASSIC_4K,
#endif
#ifdef CONFIG_ISO14443A_SNIFF_SUPPORT
    CONFIG_ISO14443A_SNIFF,
#endif

    /* This HAS to be the last element */
    CONFIG_COUNT
} ConfigurationEnum;

typedef struct {
    /* Codec used for this configuration */
    void (*CodecInitFunc) (void);
    void (*CodecDeInitFunc) (void);
    void (*CodecTaskFunc) (void);

    /* Application used for this configuration */
    void (*ApplicationInitFunc) (void);
    void (*ApplicationResetFunc) (void);
    void (*ApplicationTaskFunc) (void);
    void (*ApplicationTickFunc) (void);
    uint16_t (*ApplicationProcessFunc) (uint8_t* ByteBuffer, uint16_t ByteCount);
    void (*ApplicationGetUidFunc) (ConfigurationUidType Uid);
    void (*ApplicationSetUidFunc) (ConfigurationUidType Uid);

    uint16_t MemorySize;
    uint8_t UidSize;
    bool ReadOnly;

} ConfigurationType;

extern ConfigurationType ActiveConfiguration;

void ConfigurationInit(void);
void ConfigurationSetById(ConfigurationEnum Configuration);
void ConfigurationGetByName(char* Configuration, uint16_t BufferSize);
bool ConfigurationSetByName(const char* Configuration);
void ConfigurationGetList(char* ConfigurationList, uint16_t BufferSize);

#endif /* STANDARDS_H_ */
