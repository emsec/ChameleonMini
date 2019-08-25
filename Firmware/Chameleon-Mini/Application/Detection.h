#ifndef DETECTION_H
#define DETECTION_H

#include "Application.h"
#include "ISO14443-3A.h"

 void MifareDetectionInit(void);
 void MifareDetectionReset(void);
 uint16_t MifareDetectionAppProcess(uint8_t* Buffer, uint16_t BitCount);
#endif