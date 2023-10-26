/**
 * @file driver_i2s.h
 * @brief 使用本模块API时，不可以使用原厂的I2S API。所有API都是任务不安全的和中断不安全的！！！
 * @version 0.1
 * @date 2023-1-9
 *
 * @copyright
 *
 */

#ifndef __CORE_I2S_H__
#define __CORE_I2S_H__
#include "bsp_common.h"
#include "i2s.h"
void I2S_FullConfig(uint8_t I2SID, I2sDataFmt_t DataFmt, I2sSlotCtrl_t SlotCtrl, I2sBclkFsCtrl_t BclkFsCtrl, I2sDmaCtrl_t DmaCtrl);
void I2S_BaseConfig(uint8_t I2SID, uint8_t Mode, uint8_t FrameSize);
int32_t I2S_Start(uint8_t I2SID, uint8_t IsPlay, uint32_t SampleRate, uint8_t ChannelNum);
void I2S_Tx(uint8_t I2SID, uint8_t* Data, uint32_t ByteLen, CBFuncEx_t cb, void *param);
void I2S_Rx(uint8_t I2SID, uint32_t ByteLen, CBFuncEx_t cb, void *param);
void I2S_TxStop(uint8_t I2SID);
void I2S_RxStop(uint8_t I2SID);
void I2S_TxPause(uint8_t I2SID);
void I2S_TxDebug(uint8_t I2SID);
void I2S_RxDebug(uint8_t I2SID);

int32_t CSPI_Setup(uint8_t ID, uint32_t BusSpeed, uint8_t SpiMode, uint8_t IsMSB, uint8_t Is2RxWire, uint8_t OnlyY, uint8_t SeqType, uint8_t rowScaleRatio, uint8_t colScaleRatio, uint8_t scaleBytes, uint8_t ddrMode, uint8_t dummyAllowed);
void CSPI_Rx(uint8_t ID, void *buf, uint16_t W, uint16_t H, CBFuncEx_t cb, void *param);
void CSPI_RxContinue(uint8_t ID, void *buf);
void CSPI_RxStop(uint8_t ID);
void CSPI_Stop(uint8_t ID);
void CSPI_Pause(uint8_t ID, uint8_t OnOff);
#endif
