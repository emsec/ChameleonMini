/*
 * ISO15693.c
 *
 *  Created on: 25.01.2017
 *      Author: Phillip Nash
 */

#include "ISO15693.h"
#include "../System.h"
#include "../Application/Application.h"
#include "LEDHook.h"
#include "AntennaLevel.h"
#include "Terminal/Terminal.h"
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>


#define SOC_1_OF_4_CODE         0x7B
#define SOC_1_OF_256_CODE       0x7E
#define EOC_CODE                0xDF
#define ISO15693_SAMPLE_CLK     TC_CLKSEL_DIV2_gc // 13.56MHz
#define ISO15693_SAMPLE_PERIOD  128 // 9.4us

#define WRITE_GRID_CYCLES       4096
#define SUBCARRIER_1            32
#define SUBCARRIER_2            28
#define SUBCARRIER_OFF          0
#define SOF_PATTERN             0x1D // 0001 1101 
#define EOF_PATTERN             0xB8 // 1011 1000

// These registers provide quick access but are limited
// so global vars will be necessary
#define DataRegister            Codec8Reg0
#define StateRegister           Codec8Reg1
#define ModulationPauseCount    Codec8Reg2
#define SampleRegister          Codec8Reg3
#define BitSent                 CodecCount16Register1
#define BitSampleCount          CodecCount16Register2
#define CodecBufferPtr          CodecPtrRegister1

static volatile struct {
    volatile bool DemodFinished;
    volatile bool LoadmodFinished;
    volatile bool DemodAborted;
} Flags = { 0 };

typedef enum {
    DEMOD_SOC_STATE,
    DEMOD_1_OUT_OF_4_STATE,
    DEMOD_1_OUT_OF_256_STATE
} DemodStateType;

static volatile enum {
    LOADMOD_WAIT,

    /* Single subcarrier */
    LOADMOD_START_SINGLE,
    LOADMOD_SOF_SINGLE,
    LOADMOD_BIT0_SINGLE,
    LOADMOD_BIT1_SINGLE,
    LOADMOD_EOF_SINGLE,

    /* Dual subcarrier */
    LOADMOD_START_DUAL,
    LOADMOD_SOF_DUAL,
    LOADMOD_BIT0_DUAL,
    LOADMOD_BIT1_DUAL,
    LOADMOD_EOF_DUAL,

    LOADMOD_FINISHED
} LoadModState;

static volatile DemodStateType DemodState;
static volatile uint8_t ShiftRegister;
static volatile uint8_t ByteCount;
static volatile uint16_t BitRate1;
static volatile uint16_t BitRate2;
static volatile uint16_t SampleDataCount;
static volatile uint16_t ReadCommandFromReader = 0;

#ifdef CONFIG_VICINITY_SUPPORT

// Started when a single pulse has been detected
// ISR(CODEC_DEMOD_IN_INT0_VECT) 
void isr_ISO15693_CODEC_DEMOD_IN_INT0_VECT(void)
{
    // Start sample timer
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_HI_gc;
    /* Disable this interrupt */
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;
}

INLINE void ISO15693_EOC(void)
{
    BitRate1 = 256 * 4; // 256 * 4 - 1
    if (CodecBuffer[0] & ISO15693_REQ_DATARATE_HIGH)
        BitRate1 = 256;

    if (CodecBuffer[0] & ISO15693_REQ_SUBCARRIER_DUAL)
    {
        BitRate2 = 252 * 4; // 252 * 4 - 3
        if (CodecBuffer[0] & ISO15693_REQ_DATARATE_HIGH)
            BitRate2 = 252;
    } else {
        BitRate2 = BitRate1;
    }

    CODEC_TIMER_LOADMOD.CTRLD = 0;
    CODEC_TIMER_LOADMOD.INTFLAGS = TC0_CCBIF_bm;
    CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCBINTLVL_HI_gc;
    CODEC_TIMER_LOADMOD.PERBUF = BitRate1 - 1;

    Flags.DemodFinished = 1;
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_SAMPLING.INTCTRLB = 0;
}

// ISR(CODEC_TIMER_SAMPLING_CCC_VECT) // Reading data sent from the reader
void isr_ISO15693_CODEC_TIMER_SAMPLING_CCC_VECT(void)  
{
    /* Shift demod data */
    SampleRegister = (SampleRegister << 1) | (!(CODEC_DEMOD_IN_PORT.IN & CODEC_DEMOD_IN_MASK) ? 0x01 : 0x00);

    if (++BitSampleCount == 8)
    {
        BitSampleCount = 0;
        switch (DemodState)
        {
            case DEMOD_SOC_STATE:
                if (SampleRegister == SOC_1_OF_4_CODE)
                {
                    DemodState = DEMOD_1_OUT_OF_4_STATE;
                    SampleDataCount = 0;
                    ModulationPauseCount = 0;
                } else if (SampleRegister == SOC_1_OF_256_CODE) {
                    DemodState = DEMOD_1_OUT_OF_256_STATE;
                    SampleDataCount = 0;
                } else { // No SOC. Restart and try again
                    Flags.DemodFinished = 1;
                    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
                    CODEC_TIMER_SAMPLING.INTCTRLB = 0;
                }
                break;

            case DEMOD_1_OUT_OF_4_STATE:
                if (SampleRegister == EOC_CODE)
                {
                    ISO15693_EOC();
                } else {
                    uint8_t SampleData = ~SampleRegister;
                    if (SampleData == (0x01 << 6))
                    {
                        /* ^_^^^^^^ -> 00 */
                        ModulationPauseCount++;
                        DataRegister >>= 2;

                    } else if (SampleData == (0x01 << 4)) {
                        /* ^^^_^^^^ -> 01 */
                        ModulationPauseCount++;
                        DataRegister >>= 2;
                        DataRegister |= 0b01 << 6;

                    } else if (SampleData == (0x01 << 2)) {
                        /* ^^^^^_^^ -> 10 */
                        ModulationPauseCount++;
                        DataRegister >>= 2;
                        DataRegister |= 0b10 << 6;

                    } else if (SampleData == (0x01 << 0)) {
                        /* ^^^^^^^_ -> 11 */
                        ModulationPauseCount++;
                        DataRegister >>= 2;
                        DataRegister |= 0b11 << 6;
                    }

                    if (ModulationPauseCount == 4)
                    {
                        ModulationPauseCount = 0;
                        *CodecBufferPtr = DataRegister;
                        ++CodecBufferPtr;
                        ++ByteCount;
                    }
                }
                break;

            case DEMOD_1_OUT_OF_256_STATE:
                if (SampleRegister == EOC_CODE)
                {
                    ISO15693_EOC();
                } else {
                    uint8_t Position = ((SampleDataCount / 2) % 256) - 1;
                    uint8_t SampleData = ~SampleRegister;

                    if (SampleData == (0x01 << 6))
                    {
                        /* ^_^^^^^^ -> N-3 */
                        DataRegister = Position - 3;
                        ModulationPauseCount++;

                    } else if (SampleData == (0x01 << 4)) {
                        /* ^^^_^^^^ -> N-2 */
                        DataRegister = Position - 2;
                        ModulationPauseCount++;

                    } else if (SampleData == (0x01 << 2)) {
                        /* ^^^^^_^^ -> N-1 */
                        DataRegister = Position - 1;
                        ModulationPauseCount++;

                    } else if (SampleData == (0x01 << 0)) {
                        /* ^^^^^^^_ -> N-0 */
                        DataRegister = Position - 0;
                        ModulationPauseCount++;
                    } 

                    if (ModulationPauseCount == 1)
                    {
                        ModulationPauseCount = 0;
                        *CodecBufferPtr = DataRegister;
                        ++CodecBufferPtr;
                        ++ByteCount;
                    } 
                }
                break;
        }
    }
    SampleDataCount++;
}


//ISR(CODEC_TIMER_LOADMOD_CCB_VECT)
void isr_ISO15693_CODEC_TIMER_LOADMOD_CCB_VECT(void)
{
    static void* JumpTable[] = {
        [LOADMOD_START_SINGLE] = &&LOADMOD_START_SINGLE_LABEL,
        [LOADMOD_SOF_SINGLE] = &&LOADMOD_SOF_SINGLE_LABEL,
        [LOADMOD_BIT0_SINGLE] = &&LOADMOD_BIT0_SINGLE_LABEL,
        [LOADMOD_BIT1_SINGLE] = &&LOADMOD_BIT1_SINGLE_LABEL,
        [LOADMOD_EOF_SINGLE] = &&LOADMOD_EOF_SINGLE_LABEL,
        [LOADMOD_START_DUAL] = &&LOADMOD_START_DUAL_LABEL,
        [LOADMOD_SOF_DUAL] = &&LOADMOD_SOF_DUAL_LABEL,
        [LOADMOD_BIT0_DUAL] = &&LOADMOD_BIT0_DUAL_LABEL,
        [LOADMOD_BIT1_DUAL] = &&LOADMOD_BIT1_DUAL_LABEL,
        [LOADMOD_EOF_DUAL] = &&LOADMOD_EOF_DUAL_LABEL,
        [LOADMOD_FINISHED] = &&LOADMOD_FINISHED_LABEL
    };

    if ( (StateRegister >= LOADMOD_START_SINGLE) && (StateRegister <= LOADMOD_FINISHED) ) {
        goto *JumpTable[StateRegister];
    } else {
        return;
    }

    LOADMOD_START_SINGLE_LABEL:
        /* Application produced data. With this interrupt we are aligned to the bit-grid. */
        ShiftRegister = SOF_PATTERN;  
        BitSent = 0;
        /* Fallthrough */
    LOADMOD_SOF_SINGLE_LABEL:
        if (ShiftRegister & 0x80) {
            CodecSetLoadmodState(true);
        } else {
            CodecSetLoadmodState(false);
        }

        CodecStartSubcarrier();

        ShiftRegister <<= 1;
        BitSent++;

        if ( (BitSent % 8) == 0) {
            /* Last SOF bit has been put out. Start sending out data */
            StateRegister = LOADMOD_BIT0_SINGLE;
            ShiftRegister = (*CodecBufferPtr++);
        } else {
            StateRegister = LOADMOD_SOF_SINGLE;
        }
    return;

    LOADMOD_BIT0_SINGLE_LABEL: //Manchester encoding
        if (ShiftRegister & 0x01) {
            /* Deactivate carrier */
            CodecSetLoadmodState(false);
        } else {
            /* Activate carrier */
            CodecSetLoadmodState(true);
        }

        StateRegister = LOADMOD_BIT1_SINGLE;
    return;

    LOADMOD_BIT1_SINGLE_LABEL: //Manchester encoding
        if (ShiftRegister & 0x01) {
            CodecSetLoadmodState(true);
        } else {
            CodecSetLoadmodState(false);
        }

        ShiftRegister >>= 1;
        BitSent++;

        StateRegister = LOADMOD_BIT0_SINGLE;

        if ( (BitSent % 8) == 0 ) {
            /* Byte boundary */
            if (--ByteCount == 0) {
                /* No more data left */
                ShiftRegister = EOF_PATTERN;
                StateRegister = LOADMOD_EOF_SINGLE;
            } else {
                ShiftRegister = (*CodecBufferPtr++);
            }
        }
    return;

    LOADMOD_EOF_SINGLE_LABEL: //End of Manchester encoding
        /* Output EOF */
        if (ShiftRegister & 0x80) {
            CodecSetLoadmodState(true);
        } else {
            CodecSetLoadmodState(false);
        }

        ShiftRegister <<= 1;
        BitSent++;

        if ( (BitSent % 8) == 0) {
            /* Last bit has been put out */
            StateRegister = LOADMOD_FINISHED;
        } else {
            StateRegister = LOADMOD_EOF_SINGLE;
        }
    return;

   // ------------------------------------------------------------- 

   LOADMOD_START_DUAL_LABEL:
        ShiftRegister = SOF_PATTERN;
        BitSent = 0;
        CodecSetLoadmodState(true);
        CodecStartSubcarrier();
        // fallthrough
    LOADMOD_SOF_DUAL_LABEL:
        if (ShiftRegister & 0x80) {
            CodecChangeDivider(SUBCARRIER_1);
            CODEC_TIMER_LOADMOD.PER = BitRate1 - 1;
        } else {
            CodecChangeDivider(SUBCARRIER_2);
            CODEC_TIMER_LOADMOD.PER = BitRate2 - 1;
        }

        ShiftRegister <<= 1;
        BitSent++;

        if ( (BitSent % 8) == 0) {
            /* Last SOF bit has been put out. Start sending out data */
            StateRegister = LOADMOD_BIT0_DUAL;
            ShiftRegister = (*CodecBufferPtr++);
        } else {
            StateRegister = LOADMOD_SOF_DUAL;
        }
    return;

    LOADMOD_BIT0_DUAL_LABEL: //Manchester encoding
        if (ShiftRegister & 0x01) {
            CodecChangeDivider(SUBCARRIER_2);
            CODEC_TIMER_LOADMOD.PER = BitRate2 - 1;
        } else {
            CodecChangeDivider(SUBCARRIER_1);
            CODEC_TIMER_LOADMOD.PER = BitRate1 - 1;
        }

        StateRegister = LOADMOD_BIT1_DUAL;
    return;

    LOADMOD_BIT1_DUAL_LABEL: //Manchester encoding
        if (ShiftRegister & 0x01) {
            /* fc / 32 */
            CodecChangeDivider(SUBCARRIER_1);
            CODEC_TIMER_LOADMOD.PER = BitRate1 - 1;
        } else {
            /* fc / 28 */
            CodecChangeDivider(SUBCARRIER_2);
            CODEC_TIMER_LOADMOD.PER = BitRate2 - 1;
        }

        ShiftRegister >>= 1;
        BitSent++;

        StateRegister = LOADMOD_BIT0_DUAL;

        if ( (BitSent % 8) == 0 ) {
            /* Byte boundary */
            if (--ByteCount == 0) {
                /* No more data left */
                ShiftRegister = EOF_PATTERN;
                StateRegister = LOADMOD_EOF_DUAL;
            } else {
                ShiftRegister = (*CodecBufferPtr++);
            }
        }
    return;

    LOADMOD_EOF_DUAL_LABEL: //End of Manchester encoding
        /* Output EOF */
        if (ShiftRegister & 0x80) {
            CodecChangeDivider(SUBCARRIER_1);
            CODEC_TIMER_LOADMOD.PER = BitRate1 - 1;
        } else {
            CodecChangeDivider(SUBCARRIER_2);
            CODEC_TIMER_LOADMOD.PER = BitRate2 - 1;
        }

        ShiftRegister <<= 1;
        BitSent++;

        if ( (BitSent % 8) == 0) {
            /* Last bit has been put out */
            StateRegister = LOADMOD_FINISHED;
        } else {
            StateRegister = LOADMOD_EOF_DUAL;
        }
    return;

    LOADMOD_FINISHED_LABEL:
        CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
        CODEC_TIMER_LOADMOD.INTCTRLB = 0;
        CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OFF, SUBCARRIER_1);
        Flags.LoadmodFinished = 1;
    return;
}

#endif /* CONFIG_VICINITY_SUPPORT */

void StartISO15693Demod(void) {

    CodecBufferPtr = CodecBuffer;
    Flags.DemodFinished = 0;
    Flags.LoadmodFinished = 0;
    DemodState = DEMOD_SOC_STATE;
    DataRegister = 0;
    SampleRegister = 0;
    BitSampleCount = 0;
    SampleDataCount = 0;
    ModulationPauseCount = 0;
    ByteCount = 0;
    ShiftRegister = 0;

    /* Activate Power for demodulator */
    CodecSetDemodPower(true);

    /* Configure sampling-timer free running and sync to first modulation-pause. */
    CODEC_TIMER_SAMPLING.CNT = 0;
    CODEC_TIMER_SAMPLING.PER = ISO15693_SAMPLE_PERIOD - 1; 
    CODEC_TIMER_SAMPLING.CCC = ISO15693_SAMPLE_PERIOD / 2 - 14 - 1; /* Half bit. ISR compensate*/
    CODEC_TIMER_SAMPLING.CTRLA = ISO15693_SAMPLE_CLK;
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_RESTART_gc | CODEC_TIMER_MODSTART_EVSEL;
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_OFF_gc;

    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_RESTART_gc | CODEC_TIMER_MODSTART_EVSEL;
    CODEC_TIMER_LOADMOD.PER = 4192 + 128 + 128 - 1;
    CODEC_TIMER_LOADMOD.INTCTRLA = 0;
    CODEC_TIMER_LOADMOD.INTCTRLB = 0;
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_EVCH6_gc;

    /* Start looking out for modulation pause via interrupt. */
    CODEC_DEMOD_IN_PORT.INTFLAGS = 0x03;
    CODEC_DEMOD_IN_PORT.INT0MASK = CODEC_DEMOD_IN_MASK0;
}

void ISO15693CodecInit(void) 
{
    CodecInitCommon();
    isr_func_TCD0_CCC_vect = &isr_ISO15693_CODEC_TIMER_SAMPLING_CCC_VECT;
    isr_func_CODEC_DEMOD_IN_INT0_VECT = &isr_ISO15693_CODEC_DEMOD_IN_INT0_VECT;
    isr_func_CODEC_TIMER_LOADMOD_CCB_VECT = &isr_ISO15693_CODEC_TIMER_LOADMOD_CCB_VECT;
    StartISO15693Demod();
}

void ISO15693CodecDeInit(void) 
{
    /* Gracefully shutdown codec */
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;

    CodecBufferPtr = CodecBuffer;
    Flags.DemodFinished = 0;
    Flags.LoadmodFinished = 0;
    DemodState = DEMOD_SOC_STATE;
    DataRegister = 0;
    SampleRegister = 0;
    BitSampleCount = 0;
    SampleDataCount = 0;
    ModulationPauseCount = 0;
    ByteCount = 0;
    ShiftRegister = 0;

    //Disable sample timer
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_OFF_gc;
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;

    //Disable load modulation
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_OFF_gc;
    CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCBINTLVL_OFF_gc;
    CODEC_TIMER_LOADMOD.INTFLAGS = TC0_CCBIF_bm;

    CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OFF, 0);
    CodecSetDemodPower(false);
    CodecSetLoadmodState(false);
}

void ISO15693CodecTask(void) 
{
    if (Flags.DemodFinished) {
        Flags.DemodFinished = 0;

        uint16_t DemodByteCount = ByteCount;
        uint16_t AppReceivedByteCount = 0;
        bool bDualSubcarrier = false;

        if (DemodByteCount > 0)
        {
            LogEntry(LOG_INFO_CODEC_RX_DATA, CodecBuffer, DemodByteCount);
            LEDHook(LED_CODEC_RX, LED_PULSE);

            if (CodecBuffer[0] & ISO15693_REQ_SUBCARRIER_DUAL)
            {
                bDualSubcarrier = true;
            }
            AppReceivedByteCount = ApplicationProcess(CodecBuffer, DemodByteCount);
        }

        /* This is only reached when we've received a valid frame */
        if (AppReceivedByteCount != ISO15693_APP_NO_RESPONSE) {
            LogEntry(LOG_INFO_CODEC_TX_DATA, CodecBuffer, AppReceivedByteCount);
            LEDHook(LED_CODEC_TX, LED_PULSE);

            ByteCount = AppReceivedByteCount;
            CodecBufferPtr = CodecBuffer;

            /* Start loadmodulating */
            if (bDualSubcarrier) {
                CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OOK, SUBCARRIER_2);
                StateRegister = LOADMOD_START_DUAL;
            } else {
                CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OOK, SUBCARRIER_1);
                StateRegister = LOADMOD_START_SINGLE;
            }

        } else {
            /* No data to be processed. Disable T1 waiting and start listening again */
            CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
            CODEC_TIMER_LOADMOD.INTCTRLB = 0;

            StartISO15693Demod();
        }
    }

    if (Flags.LoadmodFinished) {
        Flags.LoadmodFinished = 0;
        /* Load modulation has been finished. Stop it and start to listen
         * for incoming data again. */
        StartISO15693Demod();
    }
}
