/******************************************************************************

*(C) Copyright 2018 AirM2M International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: radio_device.h
*
*  Description:
*
*  History: 2023/7/17 created by hyang
*
*  Notes:
*
******************************************************************************/
#ifndef RADIO_DEVICE_H
#define RADIO_DEVICE_H

/*----------------------------------------------------------------------------*
 *                    INCLUDES                                                *
 *----------------------------------------------------------------------------*/
#include "ccio_base.h"
#include "ccio_misc.h"
#include "ccio_opaq.h"
#include "ccio_pub.h"
#include "i2s.h"

#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/

/* only one device for ctrl plane */
#define I2S_DEV_REAL_MAXNUM    1

#define I2S_STOP_RECORD_CMD         0x1234bad9
#define I2S_STOP_PLAY_CMD           0xace59876

#define I2S_SEM_INTERNAL_TIMEOUT    3000

/*----------------------------------------------------------------------------*
 *                   DATA TYPE DEFINITION                                     *
 *----------------------------------------------------------------------------*/
/**
 * configuration about a i2s device.
 */
typedef struct
{
    I2sDrvInterface_t  *drvHandler;      /**< i2s driver handler */
    uint8_t             mainUsage;       /**< refer to 'CsioDevType_e' */
    uint8_t             bmCreateFlag;    /**< refer to 'CcioTaskOperFlag_e', bitmap type */
    uint8_t             isDftAtPort :1;  /**< default uartIdx for AT or not */
    uint8_t             rbufFlags   :4;  /**< which rbuf will be used? refer to 'CcioRbufUsage_e' */
    uint8_t             custFlags   :3;  /**< flags for customers' private purpose */
}I2sDevConf_t;

typedef struct
{
    I2sDrvInterface_t  *drvHandler;

    uint16_t  isTriggEn    :1;    
    uint16_t  isThresEn    :1;    
    uint16_t  isInputEn    :1;    
    uint16_t  xbatchFsm    :2;

    uint16_t  xferCnt;
    uint16_t  readIdx;
    uint16_t  writeIdx;
    uint16_t  cfgRecvSize;
    uint16_t  totalSize;
    uint8_t   *rxBuffer;
    uint8_t   codecType;
    bool      needPlayTone;
}I2sDevRxWrap_t;

typedef struct
{
    uint8_t   voiceType;
    uint8_t   sampleRate;
    uint8_t   pcmBitWidth;
    uint16_t  speechBufSize;
    uint32_t  speechBufAddr;
}I2sPlayParams_t;



/*----------------------------------------------------------------------------*
 *                    GLOBAL FUNCTIONS DECLEARATION                           *
 *----------------------------------------------------------------------------*/

/**
 * @brief i2sDevCreate(uint8_t cid)
 * @details create a radio device of dtype with 'cid'
 *
 * @param pdpCid    pdp context id
 * @param radioConf The configuration about a radio device
 * @return 0 succ; < 0 failure with errno.
 */
int32_t i2sCreate(I2sDevConf_t *i2sConf);

int32_t i2sDevRecv(uint8_t type);
int32_t i2sDevDeInit();
int32_t i2sDevPlay(CcioDevice_t *chdev);
int32_t i2sStopHandle(uint8_t stopType);


#ifdef __cplusplus
}
#endif
#endif

