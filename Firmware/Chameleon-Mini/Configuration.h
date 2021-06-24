/*
 * Standards.h
 *
 *  Created on: 15.02.2013
 *      Author: skuser
 */
/** \file */
#ifndef STANDARDS_H_
#define STANDARDS_H_

#include <stdint.h>
#include <stdbool.h>

#include "Map.h"

#define CONFIGURATION_NAME_LENGTH_MAX   32
#define CONFIGURATION_UID_SIZE_MAX      16

typedef uint8_t ConfigurationUidType[CONFIGURATION_UID_SIZE_MAX];

typedef enum  {
    /* This HAS to be the first element */
    CONFIG_NONE = 0,

#ifdef CONFIG_MF_ULTRALIGHT_SUPPORT
    CONFIG_MF_ULTRALIGHT,
    CONFIG_MF_ULTRALIGHT_C,
    CONFIG_MF_ULTRALIGHT_EV1_80B,
    CONFIG_MF_ULTRALIGHT_EV1_164B,
#endif
#ifdef CONFIG_MF_CLASSIC_MINI_4B_SUPPORT
    CONFIG_MF_CLASSIC_MINI_4B,
#endif
#ifdef CONFIG_MF_CLASSIC_1K_SUPPORT
    CONFIG_MF_CLASSIC_1K,
#endif
#ifdef CONFIG_MF_CLASSIC_1K_7B_SUPPORT
    CONFIG_MF_CLASSIC_1K_7B,
#endif
#ifdef CONFIG_MF_CLASSIC_4K_SUPPORT
    CONFIG_MF_CLASSIC_4K,
#endif
#ifdef CONFIG_MF_CLASSIC_4K_7B_SUPPORT
    CONFIG_MF_CLASSIC_4K_7B,
#endif
#ifdef CONFIG_ISO14443A_SNIFF_SUPPORT
    CONFIG_ISO14443A_SNIFF,
#endif
#ifdef CONFIG_ISO14443A_READER_SUPPORT
    CONFIG_ISO14443A_READER,
#endif
#ifdef CONFIG_NTAG21x_SUPPORT
    CONFIG_NTAG213,
    CONFIG_NTAG215,
    CONFIG_NTAG216,
#endif
#ifdef CONFIG_VICINITY_SUPPORT
    CONFIG_VICINITY,
#endif
#ifdef CONFIG_ISO15693_SNIFF_SUPPORT
    CONFIG_ISO15693_SNIFF,
#endif
#ifdef CONFIG_SL2S2002_SUPPORT
    CONFIG_SL2S2002,
#endif
#ifdef CONFIG_TITAGITSTANDARD_SUPPORT
    CONFIG_TITAGITSTANDARD,
#endif
#ifdef CONFIG_TITAGITPLUS_SUPPORT
    CONFIG_TITAGITPLUS,
#endif
#ifdef CONFIG_EM4233_SUPPORT
    CONFIG_EM4233,
#endif
#ifdef CONFIG_MF_DESFIRE_SUPPORT
    CONFIG_MF_DESFIRE,
#endif
    /* This HAS to be the last element */
    CONFIG_COUNT
} ConfigurationEnum;

/** Tag Family definitions **/
#define TAG_FAMILY_NONE      0
#define TAG_FAMILY_ISO14443A 1
#define TAG_FAMILY_ISO14443B 2
#define TAG_FAMILY_ISO15693  5


/** With this `struct` the behavior of a configuration is defined. */
typedef struct {
    /**
     * \defgroup
     * \name Codec
     * Codec related methods.
     * @{
     */
    /** Function that initializes the codec. */
    void (*CodecInitFunc)(void);
    /** Function that deinitializes the codec. */
    void (*CodecDeInitFunc)(void);
    /** Function that is called on every iteration of the main loop.
     * Within this function the essential codec work is done.
     */
    void (*CodecTaskFunc)(void);
    /**
     * @}
     */

    /**
     * \defgroup
     * \name Application
     * Application related functions.
     * @{
     */
    /** Function that initializes the application. */
    void (*ApplicationInitFunc)(void);
    /** Function that resets the application. */
    void (*ApplicationResetFunc)(void);
    /** Function that is called on every iteration of the main loop. Application work that is independent from the codec layer can be done here. */
    void (*ApplicationTaskFunc)(void);
    /** Function that is called roughly every 100ms. This can be used for parallel tasks of the application, that is independent of the codec module. */
    void (*ApplicationTickFunc)(void);
    /** This function does two important things. It gets called by the codec.
     *  The first task is to deliver data that have been received by the codec module to
     *  the application module. The application then can decide how to answer to these data and return
     *  the response to the codec module, which will process it according to the configured codec.
     *
     * \param ByteBuffer	Pointer to the start of the buffer, where the received data are and where the
     * 						application can put the response data.
     * \param BitCount		Number of bits that have been received.
     *
     * \return				Number of bits of the response.
     */
    uint16_t (*ApplicationProcessFunc)(uint8_t *ByteBuffer, uint16_t BitCount);
    /**
     * Writes the UID for the current configuration to the given buffer.
     * \param Uid	The target buffer.
     */
    void (*ApplicationGetUidFunc)(ConfigurationUidType Uid);
    /**
     * Writes a given UID to the current configuration.
     * \param Uid	The source buffer.
     */
    void (*ApplicationSetUidFunc)(ConfigurationUidType Uid);
    /**
     * @}
     */

    /**
     * Defines how many space the configuration needs. For emulating configurations this is the memory space of
     * the emulated card.
     *
     * \note For reader or sniff configurations this is set to zero.
     */
    uint16_t MemorySize;
    /**
     * Defines the size of the UID for emulating configurations.
     *
     * \note For reader or sniff configurations this is set to zero.
     */
    uint8_t UidSize;
    /**
     * Implies whether the Memory can be changed.
     */
    bool ReadOnly;
    /**
     * Specify tag family - see the defines above.
     */
    uint8_t TagFamily;

} ConfigurationType;

extern ConfigurationType ActiveConfiguration;

void ConfigurationInit(void);
void ConfigurationSetById(ConfigurationEnum Configuration);
MapIdType ConfigurationCheckByName(const char *Configuration);
void ConfigurationGetByName(char *Configuration, uint16_t BufferSize);
bool ConfigurationSetByName(const char *Configuration);
void ConfigurationGetList(char *ConfigurationList, uint16_t BufferSize);

#endif /* STANDARDS_H_ */
