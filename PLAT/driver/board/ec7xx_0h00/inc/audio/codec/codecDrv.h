/******************************************************************************

*(C) Copyright 2018 AirM2M International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: codecDrv.h
*
*  Description:
*
*  History: Rev1.0   2020-02-24
*
*  Notes: codec interface
*
******************************************************************************/


#ifndef _CODEC_DRV_H
#define _CODEC_DRV_H

/*----------------------------------------------------------------------------*
 *                    INCLUDES                                                *
 *----------------------------------------------------------------------------*/
#include "i2s.h"
#include "es8311.h"
#include "es8388.h"
/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/




 /*----------------------------------------------------------------------------*
 *                   DATA TYPE DEFINITION                                     *
 *----------------------------------------------------------------------------*/
typedef enum
{
    ES8388,
    ES7148,
    ES7149,
    ES8311,
    TM8211
}CodecType_e;

 typedef struct
 {
     uint8_t                 regAddr;    ///< Register addr
     uint16_t                regVal;     ///< Register value
 }I2sI2cCfg_t;

 /*----------------------------------------------------------------------------*
 *                    GLOBAL FUNCTIONS DECLEARATION                           *
 *----------------------------------------------------------------------------*/
void codecInit(CodecType_e codecType, I2sRole_e codecRole);
void codecWriteVal(CodecType_e codecType, uint8_t regAddr, uint16_t regVal);
void codecSetMode(CodecType_e codecType, I2sMode_e mode);
void codecI2cInit(void);
void codecI2cWrite(char* codecName, uint8_t slaveAddr, I2sI2cCfg_t* i2sI2cCfg);
uint8_t codecI2cRead(uint8_t slaveAddr, uint8_t regAddr);

void codecWriteVal(CodecType_e codecType, uint8_t regAddr, uint16_t regVal);


#ifdef __cplusplus
}
#endif

#endif /* _CODEC_DRV_H */
