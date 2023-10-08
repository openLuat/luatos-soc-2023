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

    uint16_t  xferCnt;
    uint16_t  readIdx;
    uint16_t  writeIdx;
    uint16_t  lastRxCnt;
    uint16_t  cfgRecvSize;
    uint16_t  lastWriteIdx;
    uint16_t  totalSize;
    uint8_t  *rxBuffer;
}I2sDevRxWrap_t;



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
int32_t audioDrvInitFunc(uint8_t i2sIdx, I2sDevConf_t *i2sConf);

void audioStartRecordVoiceFunc(uint8_t type);



#ifdef __cplusplus
}
#endif
#endif

