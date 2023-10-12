/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of AirM2M Ltd.
 * File name:    hal_i2s.c
 * Description:  EC618 i2s hal driver source file
 * History:      Rev1.0   2021-9-18
 *
 ****************************************************************************/

#include "codecDrv.h"

extern I2sDrvInterface_t    i2sDrvInterface0;
extern I2sDrvInterface_t    i2sDrvInterface1;

extern ARM_DRIVER_I2C Driver_I2C0;
static ARM_DRIVER_I2C   *i2cDrvInstance = &CREATE_SYMBOL(Driver_I2C, 0);

void codecI2cInit()
{
    i2cDrvInstance->Initialize(NULL);
    i2cDrvInstance->PowerControl(ARM_POWER_FULL);
    i2cDrvInstance->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
    i2cDrvInstance->Control(ARM_I2C_BUS_CLEAR, 0);
}

void codecI2cWrite(char* codecName, uint8_t slaveAddr, I2sI2cCfg_t* i2cCfg)
{
    uint8_t  cmd[2] = {0, 0};
    uint8_t  regAddr  = i2cCfg->regAddr;
    uint16_t regData  = i2cCfg->regVal;

    if (strcmp(codecName, "es8388") == 0)
    {
        cmd[0] = regAddr;
        cmd[1] = regData & 0xff;

    }
    else if (strcmp(codecName, "NAU88C22") == 0)
    {
        cmd[0] = (regAddr << 1) | ((regData & 0x100) >> 8);
        cmd[1] = regData & 0xff;
    }
    else if(strcmp(codecName, "es8311") == 0)
    {
        cmd[0] = regAddr;
        cmd[1] = regData & 0xff;
    }

    i2cDrvInstance->MasterTransmit(slaveAddr, cmd, 2, false);
	//delay_us(30 * 1000); // need to add delay in app layer which call it
}

uint8_t codecI2cRead(uint8_t slaveAddr, uint8_t regAddr)
{
    uint8_t a;
    a = regAddr;
    uint8_t readData;
    i2cDrvInstance->MasterTransmit(slaveAddr, (uint8_t *)&a, 1, true);   
    i2cDrvInstance->MasterReceive(slaveAddr, &readData, 1, false);
    return readData;
}

uint16_t es8388GetFs(void);
uint16_t es8311GetFs(void);

uint16_t codecGetFs(CodecType_e codecType)
{
    switch (codecType)
    {
        case ES8388:
            return es8388GetFs();
        
        case ES7148:
        case ES7149:
        case TM8211:
            return 1;

        case ES8311:
            return es8311GetFs();

    }

    return 0;
}

void codecWriteVal(CodecType_e codecType, uint8_t regAddr, uint16_t regVal)
{
    I2sI2cCfg_t i2cCfg;

    i2cCfg.regAddr = regAddr;
    i2cCfg.regVal  = regVal;
    
    switch (codecType)
    {
        case ES8388:
        {
            codecI2cWrite("es8388", ES8388_IICADDR, (I2sI2cCfg_t*)&i2cCfg); 
            return;
        }
        case ES7148:
        case TM8211:
        case ES7149:
        {
            return;
        }
         case ES8311:
        {
            codecI2cWrite("es8311", ES8311_IICADDR, (I2sI2cCfg_t*)&i2cCfg); 
            return;
        }
    }
}

uint8_t codecReadVal(CodecType_e codecType, uint8_t regAddr)
{
	uint8_t dataRead = 0x55;
    switch (codecType)
    {
        case ES8388:
        {
            dataRead = codecI2cRead(ES8388_IICADDR, regAddr);
            break;
        }
        case ES8311:
        {
            dataRead = codecI2cRead(ES8311_IICADDR, regAddr);
            break;
        }
        case ES7148:
        case ES7149:
        case TM8211:
        break;
    }
    
    return dataRead;
}

void codecInit(CodecType_e codecType, I2sRole_e codecRole)
{
    switch (codecType)
    {
        case ES8388:
        {
            if (codecRole == CODEC_SLAVE_MODE)
            {
                es8388SlaveInit();
            }
            else
            {
                es8388MasterInit();
            }
        }
        break;

        case ES8311:
        {
            if (codecRole == CODEC_SLAVE_MODE)
            {
                es8311SlaveInit();
            }
            else
            {
                es8311MasterInit();
            }
        }
        break;

        case ES7148:
        case ES7149:
        case TM8211:
        break;

        default:
        break;
        
    }
}

void codecSetMode(CodecType_e codecType, I2sMode_e mode)
{
    switch (codecType)
    {
        case ES8388:
        {
            es8388SetMode(mode);
        }
        break;

        case ES8311:
        {
            es8311SetMode(mode);
        }
        break;

        case ES7148:
        case ES7149:
        case TM8211:
        break;

        default:
        break;
        
    }

}
