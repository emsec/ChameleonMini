/* Config.h */

#ifndef __LOCAL_CONFIG_H__
#define __LOCAL_CONFIG_H__

#define MAX_FRAME_LENGTH             (264)
#define APPLICATION_AID_LENGTH       (3)

static const inline uint8_t MASTER_APPLICATION_AID[] = {
        0x00, 0x00, 0x00 
};

static const inline uint8_t MASTER_KEY_INDEX = 0x00;

static inline uint8_t CRYPTO_RNDB_STATE[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

static inline bool AUTHENTICATED = false;
static inline int AUTHENTICATED_PROTO = 0;

#endif
