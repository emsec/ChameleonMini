//
// Created by Zitai Chen on 05/07/2018.
//

#include "SniffISO14443-2A.h"

#include "Reader14443-2A.h"
#include "Codec.h"
#include "../System.h"
#include "../Application/Application.h"
#include "LEDHook.h"
#include "Terminal/Terminal.h"
#include <util/delay.h>


bool SniffEnable;
enum RCTraffic TrafficSource;

void Sniff14443ACodecInit(void)
{
    SniffEnable = true;
    TrafficSource = TRAFFIC_READER;
    // Start with sniffing Reader->Card direction traffic
    // Card -> Reader traffic sniffing interrupt will be enbaled
    // when Reader-> Card direction sniffing finished
    // See ISO14443-2A.c ISO14443ACodecTask()
    ISO14443ACodecInit();

    // Also configure Interrupt settings for Reader function
//    Reader14443ACodecInit();


}

void Sniff14443ACodecDeInit(void)
{
    ISO14443ACodecDeInit();
    Reader14443ACodecDeInit();
}
void Sniff14443ACodecTask(void)
{
    if (TrafficSource == TRAFFIC_READER){
        ISO14443ACodecTask();
    } else{
        Reader14443ACodecTask();
    }
//    ISO14443ACodecDeInit();
//    Reader14443ACodecTask();
}