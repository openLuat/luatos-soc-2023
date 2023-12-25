/******************************************************************************

*(C) Copyright 2018 AirM2M International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: ccio_opaq.h
*
*  Description:
*
*  History: 2023/7/27 created by hyang
*
*  Notes:
*
******************************************************************************/
#ifndef CCIO_AUDIO_H
#define CCIO_AUDIO_H

/*----------------------------------------------------------------------------*
 *                    INCLUDES                                                *
 *----------------------------------------------------------------------------*/
#include "ccio_misc.h"


#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/
#define CCIO_STOP_PLAY_FLAG      0x2
#define CCIO_STOP_RECORD_FLAG    0x3
#define CCIO_START_PLAY_FLAG     0x4
#define CCIO_START_RECORD_FLAG   0x5


/*----------------------------------------------------------------------------*
 *                   DATA TYPE DEFINITION                                     *
 *----------------------------------------------------------------------------*/

/******************************************************************************
 *****************************************************************************
 * ENUM
 *****************************************************************************
******************************************************************************/
/*
* AudioSampleRate
*/
typedef enum audioSampleRateTag
{
    VOLTE_SAMPLE_RATE_INVALID     = 0,//invalid
    VOLTE_SAMPLE_RATE_8K          = 1,//sample rate 8000Hz
    VOLTE_SAMPLE_RATE_16K         = 2 //sample rate 16000Hz
    //...add if required
}AudioSampleRate_e;

/*
* AudioPlayType
*/
typedef enum audioPlayTypeTag
{
    PLAY_DIAL_TONE                  = 1, // dial tone
    PLAY_RINGING_TONE               = 2, // ringing tone
    PLAY_CONGESTION_TONE            = 3, // congestion tone
    PLAY_BUSY_TONE                  = 4, // busy tone
    PLAY_CALL_WAITING_TONE          = 5, // call waiting tone
    PLAY_MULTI_CALL_PROMPT_TONE     = 6, // multi call prompt tone
    PLAY_CALL_ALERT_RINGING         = 7, // incoming call alert ringing
    PLAY_SPEECH_PCM_DATA            = 8  // early media or voice in speech buffer
    //...add if required
}AudioPlayType_e;


/*----------------------------------------------------------------------------*
 *                    PRIVATE FUNCTIONS DECLEARATION                           *
 *----------------------------------------------------------------------------*/
int32_t audioDataInput(UlPduBlock_t *ulpdu, void *extras);
int32_t audioDataOutput(uint8_t chanNo, DlPduBlock_t *dlpdu, void *extras);
int32_t audioDataOutputEx(uint8_t audioCid, DlPduBlock_t *dlpdu, void *extras);


/*----------------------------------------------------------------------------*
 *                    GLOBAL FUNCTIONS DECLEARATION                           *
 *----------------------------------------------------------------------------*/

/**
  \fn          audioDrvInit
  \brief       this api called by media task to init CCIO rx/tx I2S, buffer... when call setup
  \param[in]   null
  \returns     BOOL, TRUE -init buffer memory, etc. ok; FALSE - init fail, shall stop call precesure
  \NOTE:       thsi api MUST be sync interface without block
*/
BOOL audioDrvInit();

/**
  \fn          audioDrvDeInit
  \brief       this api called by media task to deinit CCIO rx/tx I2S, free buffer... when call hangup
  \param[in]   null
  \returns     null
  \NOTE:       no block
*/
int32_t audioDrvDeInit();

/**
  \fn          audioStartRecordVoice
  \brief       this api called by media task to start ccio rx record voice
  \param[in]   UINT8 codecType, AMR-NB (0) or AMR-WB (1), refer to ACVOICECODECTYPE
  \returns     null
  \NOTE:       Async interface
*/
void audioStartRecordVoice(uint8_t codecType);

/**
  \fn          audioStopRecordVoice
  \brief       this api called by media task to stop ccio rx record voice
  \param[in]   UINT8 codecType, AMR-NB (0) or AMR-WB (1), refer to ACVOICECODECTYPE
  \returns     null
  \NOTE:       Sync interface, shall stop record done before return
*/
void audioStopRecordVoice(uint8_t codecType);

/**
  \fn          audioStartPlaySound
  \brief       this api called by media task to play sound according to call precedures
  \param[in]   UINT8 type, play type, refer to AudioPlayType_e. If type is PLAY_SPEECH_PCM_DATA, below params
  \            shall be valid, or else play local tone/ring.
  \            UINT8 *pSpeechBuf, the pointer to speech buffer
  \            UINT16 speechBufSize, speech buffer size
  \            UINT8 sampleRate, sample rate 8KHz(1)/16KHz(2), refer to AudioSampleRate_e
  \            UINT8 pcmBitWidth, bit width, now only 16
  \returns     null
  \NOTE:       1 Play tone (refer to TS 22.001 F.2.5 Comfort tones) for MO call (inital call) procedures;
  \            2 play alert ringing for MT call (incoming call) procedure;
  \            3 play speech buffer data (PCM) for early media for voice by DL RTP
  \            4 async interface
*/
void audioStartPlaySound(uint8_t type, uint8_t *pSpeechBuf, uint16_t speechBufSize, uint8_t sampleRate, uint8_t pcmBitWidth);

/**
  \fn          audioStopPlaySound
  \brief       this api called by media task to stop play sound according to call precedures
  \param[in]   UINT8 type, play type, refer to AudioPlayType_e
  \returns     null
  \NOTE:       sync interface, shall stop done before return
*/
void audioStopPlaySound(uint8_t type);

void audioFreeRecordBuf(void *ulpdu);


#ifdef __cplusplus
}
#endif
#endif

