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
#include "stdio.h"
#include "stdlib.h"
#include "ec7xx.h"
#include "string.h"
#include "Driver_Common.h"
#include "bsp.h"
#ifdef FEATURE_OS_ENABLE
#include "osasys.h"
#include "cmsis_os2.h"
#endif
#include "slpman.h"
#include "exception_process.h"
#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/
#define HAL_CODEC_VOL_DEFAULT 70

// Codec I2C address
#define ES8388_IICADDR                      0x11
#define ES8311_IICADDR                      0x18

#ifdef FEATURE_OS_ENABLE
#define HAL_CODEC_CHECK_NULL(a, format, b, ...) \
    if ((a) == 0) { \
        return b;\
    }

#define DEBUG_PRINT(moduleId, subId, debugLevel, format, ...)  ECPLAT_PRINTF(moduleId, subId, debugLevel, format, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(moduleId, subId, debugLevel, format, ...)
#define HAL_CODEC_CHECK_NULL(a, format, b, ...)

#endif


/** \brief CODEC_PA location */
#define CODEC_PA_GPIO_INSTANCE        (0)
#define CODEC_PA_GPIO_PIN             (2)
#define CODEC_PA_PAD_INDEX            (17)
#define CODEC_PA_PAD_ALT_FUNC         (PAD_MUX_ALT0)

/*----------------------------------------------------------------------------*
*                   DATA TYPE DEFINITION                                     *
*----------------------------------------------------------------------------*/
// codec status
typedef enum
{
    CODEC_EOK               =  0,    // operation completed successfull 
    CODEC_ERR               = -1,    // unspecified error: no other errno fits 
    CODEC_TIMEOUT           = -2,    // invalid argument(s) 
    CODEC_INIT_ERR          = -3,    // invalid argument(s) 
    CODEC_CFG_ERR           = -4,    // invalid argument(s) 
    CODEC_SET_VOLUME_ERR    = -5,    // invalid argument(s) 
    CODEC_START_ERR         = -6,    // invalid argument(s) 

    CODEC_STATUS_RSVD = 0x7FFFFFFF
}HalCodecSts_e;

// module gain
typedef enum 
{
    MODULE_MIN      = -1,
    MODULE_ADC      = 0x01,
    MODULE_DAC      = 0x02,
    MODULE_ADC_DAC  = 0x03,
    MODULE_LINE     = 0x04,
    MODULE_MAX
}HalCodecModule_e;

// Select media hal codec mode
typedef enum 
{
    CODEC_MODE_ENCODE = 1,  // select adc 
    CODEC_MODE_DECODE,      // select dac 
    CODEC_MODE_BOTH,        // select both adc and dac 
    CODEC_MODE_LINE_IN,     // set adc channel 
}HalCodecMode_e;


// Select adc channel for input mic signal
typedef enum 
{
    CODEC_ADC_INPUT_LINE1 = 0x00,  // mic input to adc channel 1
    CODEC_ADC_INPUT_LINE2,         // mic input to adc channel 2 
    CODEC_ADC_INPUT_ALL,           // mic input to both channels of adc 
    CODEC_ADC_INPUT_DIFFERENCE,    // mic input to adc difference channel 
}HalAdcInput_e;

// Select channel for dac output
typedef enum 
{
    CODEC_DAC_OUTPUT_LINE1 = 0x00,  // dac output signal to channel 1 
    CODEC_DAC_OUTPUT_LINE2,         // dac output signal to channel 2 
    CODEC_DAC_OUTPUT_ALL,           // dac output signal to both channels 
}HalDacOutput_e;


// Select operating mode i.e. start or stop for audio codec chip
typedef enum 
{
    CODEC_CTRL_STOP         = 0x00,  // set stop mode 
    CODEC_CTRL_START        = 0x01,  // set start mode 
    CODEC_CTRL_RESUME       = 0x02,  // set resume mode 
    CODEC_CTRL_POWERDONW    = 0x03,  // set powerdown mode 
}HalCodecCtrlState_e;

// Select I2S interface operating mode i.e. master or slave for audio codec chip
typedef enum 
{
    CODEC_MODE_MASTER  = 0x00,   // set master mode
    CODEC_MODE_SLAVE   = 0x01,   // set slave mode
}HalCodecIfaceMode_e;

// Select I2S interface samples per second
typedef enum 
{
    CODEC_08K_SAMPLES,   // set to  8k samples per second 
    CODEC_16K_SAMPLES,   // set to 16k samples in per second 
    CODEC_22K_SAMPLES,   // set to 22.050k samples per second 
    CODEC_32K_SAMPLES,   // set to 32k samples in per second 
    CODEC_44K_SAMPLES,   // set to 44.1k samples per second 
    CODEC_48K_SAMPLES,   // set to 48k samples per second 
}HalCodecIfaceSample_e;

// Select I2S interface number of bits per sample
typedef enum 
{
    CODEC_BIT_LENGTH_16BITS = 0,   // set 16 bits per sample
    CODEC_BIT_LENGTH_24BITS = 2,   // set 24 bits per sample
    CODEC_BIT_LENGTH_32BITS = 3,   // set 32 bits per sample
}HalCodecIfaceBits_e;

// Select I2S interface format for audio codec chip
typedef enum 
{
    CODEC_MSB_MODE,         // set all left format */
    CODEC_LSB_MODE,         // set all right format */
    CODEC_I2S_MODE,         // set normal I2S format */
    CODEC_PCM_MODE,         // set dsp/pcm format */
}HalCodecIfaceFormat_e;

// codec channel
typedef enum
{
    CODEC_MONO,
    CODEC_DUAL,
}HalCodecChannel_e;

// I2s interface configuration for audio codec chip
typedef struct 
{
    HalCodecIfaceMode_e     mode;      // audio codec chip mode
    HalCodecIfaceFormat_e   fmt;       // I2S interface format
    HalCodecIfaceSample_e   samples;   // I2S interface samples per second
    HalCodecIfaceBits_e     bits;      // i2s interface number of bits per sample
    HalCodecChannel_e       channel;   // mono or dual channel
}HalCodecIface_t;

// Configure media hal for initialization of audio codec chip
typedef struct 
{
    HalAdcInput_e   adcInput;       // set adc channel 
    HalDacOutput_e  dacOutput;      // set dac channel 
    HalCodecMode_e  codecMode;      // select codec mode: adc, dac or both 
    HalCodecIface_t codecIface;     // set I2S interface configuration 
    bool            enablePA;
}HalCodecCfg_t;

// HAL codec func list
typedef struct 
{
    HalCodecSts_e (*halCodecInitFunc)(HalCodecCfg_t *codec_cfg);                                 // initialize codec
    void          (*halCodecDeinitFunc)(void);                                                   // deinitialize codec
    HalCodecSts_e (*halCodecCtrlStateFunc)(HalCodecMode_e mode, HalCodecCtrlState_e ctrl_state); // control codec mode and state
    HalCodecSts_e (*halCodecCfgIfaceFunc)(HalCodecMode_e mode, HalCodecIface_t *iface);          // configure i2s interface
    HalCodecSts_e (*halCodecSetMuteFunc) (bool mute);                                            // set codec mute
    HalCodecSts_e (*halCodecSetVolumeFunc)(int volume);                                          // set codec volume
    HalCodecSts_e (*halCodecGetVolumeFunc)(int *volume);                                         // get codec volume
    HalCodecSts_e (*halCodecSetMicVolumeFunc)(int volume);                                       // set codec mic volume
    void          (*halCodecEnablePAFunc) (bool enable);                                         // enable pa
    void           *halCodecLock;                                                                // semaphore of codec
    void           *handle;                                                                      // handle of audio codec
    HalCodecCfg_t (*halCodecGetDefaultCfg)();
}HalCodecFuncList_t;


/*----------------------------------------------------------------------------*
*                    GLOBAL FUNCTIONS DECLEARATION                           *
*----------------------------------------------------------------------------*/
void codecI2cInit();
int32_t codecI2cWrite(uint8_t slaveAddr, uint8_t regAddr, uint16_t regVal);
uint8_t codecI2cRead(uint8_t slaveAddr, uint8_t regAddr);
void codecI2cDeInit();

/**
  \brief    Hal codec init
  \param[in] codecHalCfg    codec config
  \param[in] codecHalFunc   codecHal func list
  \param[in] needLock       need semaphore lock or not in codec HAL layer
  \return    HalCodecFuncList_t* codec funclist
  \note
 */
HalCodecFuncList_t* halCodecInit(HalCodecCfg_t* codecHalCfg, HalCodecFuncList_t* codecHalFunc, bool needLock);

/**
  \brief    HAL CODEC Deinit
  \param[in] codecHalFunc   codecHal func list
  \return    -CODEC_EOK   -CODEC_ERR
  \note
 */
HalCodecSts_e halCodecDeinit(HalCodecFuncList_t* codecHal);

/**
  \brief    Hal codec start/stop control
  \param[in] codecHalFunc       codecHal func list
  \param[in] mode               codec mode
  \param[in] codecStartStop     codec start or stop
  \param[in] needLock           need semaphore lock or not in codec HAL layer
  \return    -CODEC_EOK   -CODEC_ERR
  \note
 */
HalCodecSts_e halCodecCtrlState(HalCodecFuncList_t* codecHal, HalCodecMode_e mode, HalCodecCtrlState_e codecStartStop, bool needLock);

/**
  \brief    Hal codec interface config
  \param[in] codecHalFunc       codecHal func list
  \param[in] mode               codec mode
  \param[in] iface              codec interface
  \param[in] needLock           need semaphore lock or not in codec HAL layer
  \return    -CODEC_EOK   -CODEC_ERR
  \note
 */
HalCodecSts_e halCodecIfaceCfg(HalCodecFuncList_t* codecHal, HalCodecMode_e mode, HalCodecIface_t *iface, bool needLock);

/**
  \brief    Hal codec set mute
  \param[in] codecHalFunc       codecHal func list
  \param[in] mute               codec set mute
  \param[in] needLock           need semaphore lock or not in codec HAL layer
  \return    -CODEC_EOK   -CODEC_ERR
  \note
 */
HalCodecSts_e halCodecSetMute(HalCodecFuncList_t* codecHal, bool mute, bool needLock);

/**
  \brief    Hal codec enable pa
  \param[in] codecHalFunc       codecHal func list
  \param[in] enable             codec enable/disable pa
  \param[in] needLock           need semaphore lock or not in codec HAL layer
  \return    -CODEC_EOK   -CODEC_ERR
  \note
 */
HalCodecSts_e halCodecEnablePA(HalCodecFuncList_t* codecHal, bool enable, bool needLock);

/**
  \brief    Hal codec set volume
  \param[in] codecHalFunc       codecHal func list
  \param[in] volume             codec set volume
  \param[in] needLock           need semaphore lock or not in codec HAL layer
  \return    -CODEC_EOK   -CODEC_ERR
  \note
 */
HalCodecSts_e halCodecSetVolume(HalCodecFuncList_t* codecHal, int volume, bool needLock);

/**
  \brief    Hal codec get volume
  \param[in] codecHalFunc       codecHal func list
  \param[out] volume            codec get volume
  \param[in] needLock           need semaphore lock or not in codec HAL layer
  \return    -CODEC_EOK   -CODEC_ERR
  \note
 */
HalCodecSts_e halCodecGetVolume(HalCodecFuncList_t* codecHal, int *volume, bool needLock);

/**
  \brief    Hal codec set mic volume
  \param[in] codecHalFunc       codecHal func list
  \param[in] micVolume          codec set volume
  \param[in] needLock           need semaphore lock or not in codec HAL layer
  \return    -CODEC_EOK   -CODEC_ERR
  \note
 */
HalCodecSts_e halCodecSetMicVolume(HalCodecFuncList_t* codecHal, int micVolume, bool needLock);


/**
  \brief    Hal codec get default config
  \param[in] codecHalFunc       codecHal func list
  \return    HalCodecCfg_t codec defalt config
  \note
 */
HalCodecCfg_t halCodecGetDefaultCfg(HalCodecFuncList_t* codecHalFunc);




#ifdef __cplusplus
}
#endif

#endif /* _CODEC_DRV_H */
