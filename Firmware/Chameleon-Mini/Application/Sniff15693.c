/*
 * SniffISO15693.h
 *
 *  Created on: 05.11.2019
 *      Author: ceres-c
 */

#ifdef CONFIG_ISO15693_SNIFF_SUPPORT

#include "../Codec/SniffISO15693.h"
#include "Sniff15693.h"

/* Incrementing in steps of 50 was considered as sufficiently */
/* accurate during tests */
#define ISO15693_TH_CALIBRATE_STEPS    50
//#define ISO15693_DEBUG_LOG
typedef enum {
    AUTOCALIB_INIT,
    AUTOCALIB_READER_DATA,
    AUTOCALIB_CARD_DATA,
} Sniff15693State;

Sniff15693Command Sniff15693CurrentCommand;
static Sniff15693State autocalib_state = AUTOCALIB_INIT;

/* The Autocalib shall work as following: */
/* Calls to AppProcess shall always alternate b/w ReaderData and CardData */
/* If there are 2 successive calls to to AppProcess with ReaderData, it means */
/*   we have not received card data -> bad case */
/* So, we need always two calls to AppProcess to evaluate b/w good & bad case */
/* two calls are called a cycle further on */
/* we do increment the threshold after each cycle */
/* We store the first successfull card data as min_succ_th (means the threshold when we have received */
/* card data the first time */
/* And now we increment the threshold further, until we dont get card data anymore */
/* This will be stored in 'max_succ_th */
/* We do assume now, that the optimal threshold is between min_succ_th and max_succ_th */
/* With this approach we do avoid, to maintain a large array of possible thresholds */
/* The serach range is defined by the DemodFloorNoiseLevel */
/* We search b/w: */
/*   0.5 * DemodFloorNoiseLevel --> 1.5 * DemodFloorNoiseLevel */


/*
                _________________________ scanning range ______________________________________
               /                                                                               \
    -|---------|-----------------------------------------threshold-----------------------------|------------>
               |                                                                               |
typical pattern|                                                                               |
- bad case     |------------------------+-++++-+-+---------------------------------------------|
+ good case    |                        |        |                                             |
               |                   min_succ_th   |                                             |
               |                               max_succ_th                                     |
             min_th                                                                           max_th

   And finally the desired threshold is: (min_succ_th + max_succ_th) >> 1
*/
static uint16_t curr_th = 0;
static uint16_t min_th = 0;
static uint16_t max_th = 0xFFF;
static uint16_t min_succ_th = 0;
static uint16_t max_succ_th = 0xFFF;

/* to store the threshold which was used before we started AUTOCALIBRATION */
static uint16_t recent_th = 0;

static bool last_cycle_successful = false;

void SniffISO15693AppTimeout(void) {
    CodecThresholdSet(recent_th);
    SniffISO15693AppReset();
}

void SniffISO15693AppInit(void) {
    Sniff15693CurrentCommand = Sniff15693_Do_Nothing;
    autocalib_state = AUTOCALIB_INIT;
}

void SniffISO15693AppReset(void) {
    SniffISO15693AppInit();
}


void SniffISO15693AppTask(void) {

}

void SniffISO15693AppTick(void) {

}

INLINE void SniffISO15693InitAutocalib(void) {
    uint16_t floor_noise = SniffISO15693GetFloorNoise();
    /* We search b/w: */
    /*   0.5 * DemodFloorNoiseLevel --> 1.5 * DemodFloorNoiseLevel */
    min_th = floor_noise >> 1;
    max_th = floor_noise + (floor_noise >> 1);
    if (min_th >= max_th) {
        min_th = 0;
        max_th = 0xFFF;
    }

    min_succ_th = 0;
    max_succ_th = 0xFFF;


    /* store current threshold before we started AUTOCALIBRATION */
    recent_th = GlobalSettings.ActiveSettingPtr->ReaderThreshold;

    last_cycle_successful = false;
#ifdef ISO15693_DEBUG_LOG
    {
        char str[64];
        sprintf(str, "Sniff15693: Start Autocalibration %d - %d ", min_th, max_th);
        LogEntry(LOG_INFO_GENERIC, str, strlen(str));
    }
#endif /*#ifdef ISO15693_DEBUG_LOG*/
    curr_th = min_th;
    CodecThresholdSet(curr_th);
    return;
}

INLINE void SniffISO15693FinishAutocalib(void) {
    uint16_t new_th;
    CommandStatusIdType ReturnStatusID = COMMAND_ERR_INVALID_USAGE_ID;
    if (min_succ_th > 0) {
        /* In this case AUTOCALIBRATION was successfull */
        new_th = (min_succ_th + max_succ_th) >> 1;
        CodecThresholdSet(new_th);
#ifdef ISO15693_DEBUG_LOG
        {
            char str[64];
            sprintf(str, "Sniff15693: Finished Autocalibration %d - %d --> % d", min_succ_th, max_succ_th, new_th);
            LogEntry(LOG_INFO_GENERIC, str, strlen(str));
        }
#endif /*#ifdef ISO15693_DEBUG_LOG*/
        ReturnStatusID = COMMAND_INFO_OK_WITH_TEXT_ID;
    } else {
        /* This means we never received a valid frame - Error*/
        CodecThresholdSet(recent_th);
        /* ReturnStatusID already set to error code */
    }
    SniffISO15693AppInit();
    CommandLinePendingTaskFinished(ReturnStatusID, NULL);
}

INLINE void SniffISO15693IncrementThreshold(void) {

    /* So increment the threshold */
    /* Steps of 3*16=48 are sufficient for us */
    curr_th += ISO15693_TH_CALIBRATE_STEPS;
    CodecThresholdSet(curr_th);
#ifdef ISO15693_DEBUG_LOG
    {
        char str[64];
        sprintf(str, "Sniff15693: Try next th=%d", curr_th);
        LogEntry(LOG_INFO_GENERIC, str, strlen(str));
    }
#endif /*#ifdef ISO15693_DEBUG_LOG*/
    if (curr_th >= max_th) {
        /* We are finished now */
        /* Evaluate the results */
        SniffISO15693FinishAutocalib();
    }
}

uint16_t SniffISO15693AppProcess(uint8_t *FrameBuf, uint16_t FrameBytes) {

    bool crc_chk = ISO15693CheckCRC(FrameBuf, FrameBytes - ISO15693_CRC16_SIZE);

#ifdef ISO15693_DEBUG_LOG
    char str[64];
    sprintf(str, "Sniff15693: CrcCheck result=%d", crc_chk);
    LogEntry(LOG_INFO_GENERIC, str, strlen(str));
#endif /*#ifdef ISO15693_DEBUG_LOG*/

    switch (Sniff15693CurrentCommand) {
        case Sniff15693_Do_Nothing: {
            return 0;
        }
        case Sniff15693_Autocalibrate: {
            switch (autocalib_state) {
                case (AUTOCALIB_INIT):
                    SniffISO15693InitAutocalib();
                    if (SniffTrafficSource == TRAFFIC_CARD)
                        autocalib_state = AUTOCALIB_CARD_DATA;
                    else
                        autocalib_state = AUTOCALIB_READER_DATA;
                    break;
                case (AUTOCALIB_READER_DATA): /* Means last time we have received reader data */
                    if (SniffTrafficSource == TRAFFIC_READER) {
                        /* Second time Reader Data received */
                        /* This means no card data received */
                        /* If we had successful tries before */
                        /* we need to record it as threshold max here */
                        if (last_cycle_successful == true) {
                            max_succ_th = GlobalSettings.ActiveSettingPtr->ReaderThreshold -
                                          CODEC_THRESHOLD_CALIBRATE_STEPS;
#ifdef ISO15693_DEBUG_LOG
                            sprintf(str, "Sniff15693: Updated max_succ_th (%d) ", max_succ_th);
                            LogEntry(LOG_INFO_GENERIC, str, strlen(str));
#endif /*#ifdef ISO15693_DEBUG_LOG*/
                        }
                        last_cycle_successful = false;
                    } else {
                        if (crc_chk) {
                            /* We have received card data now */
                            /* Threshold is in a good range */
                            /* if min_succ_th was never set ( == 0) */
                            /* Set it now to the current threshold */
                            if (min_succ_th == 0)
                                min_succ_th = GlobalSettings.ActiveSettingPtr->ReaderThreshold;
                            last_cycle_successful = true;
                        }
#ifdef ISO15693_DEBUG_LOG
                        sprintf(str, "Sniff15693: Found card data (%d) - crc=%d", min_succ_th, crc_chk);
                        LogEntry(LOG_INFO_GENERIC, str, strlen(str));
#endif /*#ifdef ISO15693_DEBUG_LOG*/
                        autocalib_state = AUTOCALIB_CARD_DATA;
                    }
                    SniffISO15693IncrementThreshold();
                    break;
                case (AUTOCALIB_CARD_DATA): /* Means last time we have received card data */
                    if (SniffTrafficSource == TRAFFIC_CARD) {
                        if (crc_chk) {
                            /* We have received card data now */
                            /* Threshold is in a good range */
                            /* if min_succ_th was never set ( == 0) */
                            /* Set it now to the current threshold */
                            if (min_succ_th == 0)
                                min_succ_th = GlobalSettings.ActiveSettingPtr->ReaderThreshold;
                            last_cycle_successful = true;
                        }
#ifdef ISO15693_DEBUG_LOG
                        sprintf(str, "Sniff15693: Found card data (%d) - crc=%d", min_succ_th, crc_chk);
                        LogEntry(LOG_INFO_GENERIC, str, strlen(str));
#endif /*#ifdef ISO15693_DEBUG_LOG*/
                        SniffISO15693IncrementThreshold();
                    } else {
                        autocalib_state = AUTOCALIB_READER_DATA;
                    }
                    break;
                default:
                    break;
            }
            return 0;
        }
    }
    return 0;
}

#endif /* CONFIG_ISO15693_SNIFF_SUPPORT */
