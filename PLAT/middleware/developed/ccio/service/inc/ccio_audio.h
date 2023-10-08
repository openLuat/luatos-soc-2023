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


/*----------------------------------------------------------------------------*
 *                   DATA TYPE DEFINITION                                     *
 *----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*
 *                    GLOBAL FUNCTIONS DECLEARATION                           *
 *----------------------------------------------------------------------------*/

/**
  \fn     int32_t opaqDataInput(UlPduBlock_t *ulpdu, void *extras)
  \brief  to further handle opaq data received from serial device
  \return
  \note   invoked by RxTask automatically once opaq data is received
*/
void audioStartPlaySoundFunc(uint8_t type, uint8_t *pSpeechBuf, uint16_t speechBufSize, uint8_t sampleRate, uint8_t pcmBitWidth);


#ifdef __cplusplus
}
#endif
#endif

