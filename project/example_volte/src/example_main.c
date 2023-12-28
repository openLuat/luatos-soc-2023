/*
 * Copyright (c) 2023 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "common_api.h"

//#if defined FEATURE_IMS_ENABLE
#if 1
#include "common_api.h"
#include "luat_gpio.h"
#include "luat_mobile.h"
#include "luat_i2c.h"
#include "luat_i2s.h"
#include "luat_rtos.h"

//demo 配置带ES8311硬件的云喇叭开发板
#define CODEC_PIN	HAL_GPIO_16
#define PA_PIN	HAL_GPIO_25
#define I2C_ID	0
#define I2S_ID	0

extern uint8_t callAlertRing16k[];
extern uint8_t tone450_8k[];
extern uint8_t tone950_8k[];

/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/
#include "es8311.h"
#define ES8311_DBG LUAT_DEBUG_PRINT
#define IS_DMIC             0
#define MCLK_DIV_FRE        256
#define ES8311_IICADDR		0x18


/*----------------------------------------------------------------------------*
 *                    DATA TYPE DEFINITION                                    *
 *----------------------------------------------------------------------------*/

// Clock coefficient structer
struct _coeffDiv
{
    uint32_t mclk;          // mclk frequency
    uint32_t rate;          // sample rate
    uint8_t preDiv;         // the pre divider with range from 1 to 8
    uint8_t preMulti;       // the pre multiplier with x1, x2, x4 and x8 selection
    uint8_t adcDiv;         // adcclk divider
    uint8_t dacDiv;         // dacclk divider
    uint8_t fsMode;         // double speed or single speed, =0, ss, =1, ds
    uint8_t lrckH;          // adclrck divider and daclrck divider
    uint8_t lrckL;
    uint8_t bclkDiv;        // sclk divider
    uint8_t adcOsr;         // adc osr
    uint8_t dacOsr;         // dac osr
};

/*----------------------------------------------------------------------------*
 *                      PRIVATE FUNCTION DECLEARATION                         *
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*
 *                      GLOBAL VARIABLES                                      *
 *----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*
 *                      PRIVATE VARIABLES                                     *
 *----------------------------------------------------------------------------*/

// codec hifi mclk clock divider coefficients
static const struct _coeffDiv coeffDiv[] =
{
    //mclk     rate   preDiv  mult  adcDiv dacDiv fsMode lrch  lrcl  bckdiv adcOsr dacOsr
    // 8k
    {12288000, 8000 , 0x06,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04,  0x10,   0x20},
    {18432000, 8000 , 0x03,    0x02, 0x03,   0x03,   0x00,   0x05, 0xff, 0x18,  0x10,   0x20},
    {16384000, 8000 , 0x08,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04,  0x10,   0x20},
    {8192000 , 8000 , 0x04,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04,  0x10,   0x20},
    {6144000 , 8000 , 0x03,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04,  0x10,   0x20},
    {4096000 , 8000 , 0x02,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04,  0x10,   0x20},
    {3072000 , 8000 , 0x01,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04,  0x10,   0x20},
    {2048000 , 8000 , 0x01,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04,  0x10,   0x20},
    {1536000 , 8000 , 0x03,    0x04, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04,  0x10,   0x20},
    {1024000 , 8000 , 0x01,    0x02, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04,  0x10,   0x20},

    // 16k
    {12288000, 16000, 0x03,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x20},
    {18432000, 16000, 0x03,    0x02, 0x03,   0x03,   0x00,   0x02, 0xff, 0x0c, 0x10,    0x20},
    {16384000, 16000, 0x04,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x20},
    {8192000 , 16000, 0x02,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x20},
    {6144000 , 16000, 0x03,    0x02, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x20},
    {4096000 , 16000, 0x01,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x20},
    {3072000 , 16000, 0x03,    0x04, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x20},
    {2048000 , 16000, 0x01,    0x02, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x20},
    {1536000 , 16000, 0x03,    0x08, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x20},
    {1024000 , 16000, 0x01,    0x04, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x20},

    // 22.05k
    {11289600, 22050, 0x02,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {5644800 , 22050, 0x01,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {2822400 , 22050, 0x01,    0x02, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {1411200 , 22050, 0x01,    0x04, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},

    // 32k
    {12288000, 32000, 0x03,    0x02, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {18432000, 32000, 0x03,    0x04, 0x03,   0x03,   0x00,   0x02, 0xff, 0x0c, 0x10,    0x10},
    {16384000, 32000, 0x02,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {8192000 , 32000, 0x01,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {6144000 , 32000, 0x03,    0x04, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {4096000 , 32000, 0x01,    0x02, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {3072000 , 32000, 0x03,    0x08, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {2048000 , 32000, 0x01,    0x04, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {1536000 , 32000, 0x03,    0x08, 0x01,   0x01,   0x01,   0x00, 0x7f, 0x02, 0x10,    0x10},
    {1024000 , 32000, 0x01,    0x08, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},

    // 44.1k
    {11289600, 44100, 0x01,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {5644800 , 44100, 0x01,    0x02, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {2822400 , 44100, 0x01,    0x04, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {1411200 , 44100, 0x01,    0x08, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},

    // 48k
    {12288000, 48000, 0x01,    0x01, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {18432000, 48000, 0x03,    0x02, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {6144000 , 48000, 0x01,    0x02, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {3072000 , 48000, 0x01,    0x04, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {1536000 , 48000, 0x01,    0x08, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},

    // 96k
    {12288000, 96000, 0x01,    0x02, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {18432000, 96000, 0x03,    0x04, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {6144000 , 96000, 0x01,    0x04, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {3072000 , 96000, 0x01,    0x08, 0x01,   0x01,   0x00,   0x00, 0xff, 0x04, 0x10,    0x10},
    {1536000 , 96000, 0x01,    0x08, 0x01,   0x01,   0x01,   0x00, 0x7f, 0x02, 0x10,    0x10},
};

static uint8_t dacVolBak, adcVolBak;

/*----------------------------------------------------------------------------*
 *                      PRIVATE FUNCS                                      *
 *----------------------------------------------------------------------------*/

static int32_t es8311WriteReg(uint8_t regAddr, uint16_t data)
{
    int32_t ret = 0;
    uint8_t  cmd[2] = {0, 0};
    cmd[0]  = regAddr;
    cmd[1]  = data & 0xff;
    ret = luat_i2c_send(I2C_ID, ES8311_IICADDR, cmd, 2, 1);
    if (ret)
    {
    	ES8311_DBG("write %x,%x failed", regAddr, data);
    }
    return ret;
}

static uint8_t es8311ReadReg(uint8_t regAddr)
{
	uint8_t  data[1] = {0};
	int32_t ret = luat_i2c_transfer(I2C_ID, ES8311_IICADDR, &regAddr, 1, data, 1);
    if (ret)
    {
    	ES8311_DBG("read %x failed", regAddr);
    }
    return data[0];
}

static int getCoeff(uint32_t mclk, uint32_t rate)
{
    for (int i = 0; i < (sizeof(coeffDiv) / sizeof(coeffDiv[0])); i++) {
        if (coeffDiv[i].rate == rate && coeffDiv[i].mclk == mclk)
            return i;
    }
    return -1;
}

// set es8311 dac mute or not. mute = 0, dac un-mute; mute = 1, dac mute
static void es8311Mute(int mute)
{
    ES8311_DBG("Enter into es8311Mute(), mute = %d\n", mute);
    es8311SetVolume(0);
}

// set es8311 into suspend mode
static void es8311Standby(uint8_t* dacVolB, uint8_t* adcVolB)
{
	ES8311_DBG("Enter into es8311 suspend");
    *dacVolB = es8311ReadReg(ES8311_DAC_REG32);
    *adcVolB = es8311ReadReg(ES8311_ADC_REG17);
    es8311WriteReg(ES8311_DAC_REG32,    0x00);
    es8311WriteReg(ES8311_ADC_REG17,    0x00);
    es8311WriteReg(ES8311_SYSTEM_REG0E, 0xFF);
    es8311WriteReg(ES8311_SYSTEM_REG12, 0x02);
    es8311WriteReg(ES8311_SYSTEM_REG14, 0x00);
    es8311WriteReg(ES8311_SYSTEM_REG0D, 0xFA);
    es8311WriteReg(ES8311_ADC_REG15,    0x00);
    es8311WriteReg(ES8311_DAC_REG37,    0x08);
    es8311WriteReg(ES8311_GP_REG45,     0x01);
}

// set es8311 ADC into suspend mode
static void es8311AdcStandby(uint8_t* adcVolB)
{
	ES8311_DBG("Enter into es8311 adc suspend");
    *adcVolB = es8311ReadReg(ES8311_ADC_REG17);
    es8311WriteReg(ES8311_ADC_REG17,            0x00);
    es8311WriteReg(ES8311_SDPOUT_REG0A,         0x40);
    es8311WriteReg(ES8311_SYSTEM_REG0E,         0x7f);
    es8311WriteReg(ES8311_SYSTEM_REG14,         0x00);
    es8311WriteReg(ES8311_SYSTEM_REG0D,         0x31);
    es8311WriteReg(ES8311_ADC_REG15,            0x00);
    es8311WriteReg(ES8311_DAC_REG37,            0x08);
    es8311WriteReg(ES8311_RESET_REG00,          0x82);
    es8311WriteReg(ES8311_CLK_MANAGER_REG01,    0x35);
}

// set es8311 DAC into suspend mode
static void es8311DacStandby(uint8_t* dacVolB)
{
	ES8311_DBG("Enter into es8311 dac suspend");
    *dacVolB = es8311ReadReg(ES8311_DAC_REG32);
    es8311WriteReg(ES8311_DAC_REG32,            0x00);
    es8311WriteReg(ES8311_SYSTEM_REG0E,         0x0F);
    es8311WriteReg(ES8311_SYSTEM_REG12,         0x02);
    es8311WriteReg(ES8311_SYSTEM_REG0D,         0x09);
    es8311WriteReg(ES8311_ADC_REG15,            0x00);
    es8311WriteReg(ES8311_DAC_REG37,            0x08);
    es8311WriteReg(ES8311_RESET_REG00,          0x81);
    es8311WriteReg(ES8311_CLK_MANAGER_REG01,    0x3a);
}

// set es8311 into resume mode
static HalCodecSts_e es8311AllResume()
{
	ES8311_DBG("Enter into es8311 resume");
    es8311WriteReg(ES8311_SYSTEM_REG0D,         0x01);
    es8311WriteReg(ES8311_GP_REG45,             0x00);
    es8311WriteReg(ES8311_CLK_MANAGER_REG01,    0x3F);
    es8311WriteReg(ES8311_RESET_REG00,          0x80);
    luat_rtos_task_sleep(1);
    es8311WriteReg(ES8311_SYSTEM_REG0D,         0x01);
    es8311WriteReg(ES8311_CLK_MANAGER_REG02,    0x00);
    es8311WriteReg(ES8311_DAC_REG37,            0x08);
    es8311WriteReg(ES8311_ADC_REG15,            0x40);
    es8311WriteReg(ES8311_SYSTEM_REG14,         0x18);
    es8311WriteReg(ES8311_SYSTEM_REG12,         0x00);
    es8311WriteReg(ES8311_SYSTEM_REG0E,         0x00);
    es8311WriteReg(ES8311_DAC_REG32,            dacVolBak);
    es8311WriteReg(ES8311_ADC_REG17,            adcVolBak);

    return CODEC_EOK;
}

// set es8311 adc into resume mode
static HalCodecSts_e es8311AdcResume()
{
	ES8311_DBG("Enter into es8311 adc resume");
    es8311WriteReg(ES8311_SYSTEM_REG0D,         0x01);
    es8311WriteReg(ES8311_CLK_MANAGER_REG01,    0x3F);
    es8311WriteReg(ES8311_RESET_REG00,          0x80);
    delay_us(1000);
    es8311WriteReg(ES8311_SYSTEM_REG0D,         0x01);
    es8311WriteReg(ES8311_DAC_REG37,            0x08);
    es8311WriteReg(ES8311_ADC_REG15,            0x00);
    es8311WriteReg(ES8311_SYSTEM_REG14,         0x18);
    es8311WriteReg(ES8311_SYSTEM_REG0E,         0x02);
    es8311WriteReg(ES8311_ADC_REG17,            adcVolBak);
    delay_us(50*1000);
    es8311WriteReg(ES8311_SDPOUT_REG0A,         0x00);
    return CODEC_EOK;
}

// set es8311 dac into resume mode
static HalCodecSts_e es8311DacResume()
{
	ES8311_DBG("Enter into es8311 dac resume");
    es8311WriteReg(ES8311_SYSTEM_REG0D,         0x01);
    es8311WriteReg(ES8311_CLK_MANAGER_REG01,    0x3F);
    es8311WriteReg(ES8311_RESET_REG00,          0x80);
    delay_us(1000);
    es8311WriteReg(ES8311_SYSTEM_REG0D,         0x01);
    es8311WriteReg(ES8311_DAC_REG37,            0x08);
    es8311WriteReg(ES8311_ADC_REG15,            0x00);
    es8311WriteReg(ES8311_SYSTEM_REG12,         0x00);
    es8311WriteReg(ES8311_SYSTEM_REG0E,         0x02);
    es8311WriteReg(ES8311_DAC_REG32,            dacVolBak);

    return CODEC_EOK;
}


// set es8311 into powerdown mode
static HalCodecSts_e es8311PwrDown()
{
	ES8311_DBG("Enter into es8311 powerdown");
    es8311WriteReg(ES8311_DAC_REG32,            0x00);
    es8311WriteReg(ES8311_ADC_REG17,            0x00);
    es8311WriteReg(ES8311_SYSTEM_REG0E,         0xff);
    es8311WriteReg(ES8311_SYSTEM_REG12,         0x02);
    es8311WriteReg(ES8311_SYSTEM_REG14,         0x00);
    es8311WriteReg(ES8311_SYSTEM_REG0D,         0xf9);
    es8311WriteReg(ES8311_ADC_REG15,            0x00);
    es8311WriteReg(ES8311_DAC_REG37,            0x08);
    es8311WriteReg(ES8311_CLK_MANAGER_REG02,    0x10);
    es8311WriteReg(ES8311_RESET_REG00,          0x00);
    delay_us(1000);
    es8311WriteReg(ES8311_RESET_REG00,          0x1f);
    es8311WriteReg(ES8311_CLK_MANAGER_REG01,    0x30);
    delay_us(1000);
    es8311WriteReg(ES8311_CLK_MANAGER_REG01,    0x00);
    es8311WriteReg(ES8311_GP_REG45,             0x00);
    es8311WriteReg(ES8311_SYSTEM_REG0D,         0xfc);
    es8311WriteReg(ES8311_CLK_MANAGER_REG02,    0x00);
    return CODEC_EOK;
}


/*----------------------------------------------------------------------------*
 *                      GLOBAL FUNCTIONS                                      *
 *----------------------------------------------------------------------------*/

// enable pa power
void es8311EnablePA(bool enable)
{
	GPIO_Output(PA_PIN, enable);

}

HalCodecSts_e es8311Init(void)
{
    uint8_t datmp, regv;
    int coeff;
    HalCodecSts_e ret = CODEC_EOK;
    getCoeff(0,0);

    ret |= es8311WriteReg(ES8311_CLK_MANAGER_REG01, 0x3F);

    // Select clock source for internal mclk
    regv = es8311ReadReg(ES8311_CLK_MANAGER_REG01);

    ///////////////////////////////////////////////

    regv &= 0x7F;
    ret |= es8311WriteReg(ES8311_CLK_MANAGER_REG01, regv);

    // Set clock parammeters
    int sampleFre = 16000;
    int mclkFre = 0;
    mclkFre = sampleFre * MCLK_DIV_FRE;
    coeff = getCoeff(mclkFre, sampleFre);
    if (coeff < 0)
    {
    	ES8311_DBG("Unable to configure sample rate %dHz with %dHz MCLK", sampleFre, mclkFre);
        return CODEC_INIT_ERR;
    }

    if (coeff >= 0)
    {
        regv = es8311ReadReg(ES8311_CLK_MANAGER_REG02) & 0x07;
        regv |= (coeffDiv[coeff].preDiv - 1) << 5;
        datmp = 0;
        switch (coeffDiv[coeff].preMulti)
        {
            case 1:
                datmp = 0;
                break;
            case 2:
                datmp = 1;
                break;
            case 4:
                datmp = 2;
                break;
            case 8:
                datmp = 3;
                break;
            default:
                break;
        }

        regv |= (datmp) << 3;
        ret |= es8311WriteReg(ES8311_CLK_MANAGER_REG02, regv);

        regv = es8311ReadReg(ES8311_CLK_MANAGER_REG05) & 0x00;
        regv |= (coeffDiv[coeff].adcDiv - 1) << 4;
        regv |= (coeffDiv[coeff].dacDiv - 1) << 0;
        ret |= es8311WriteReg(ES8311_CLK_MANAGER_REG05, regv);

        regv = es8311ReadReg(ES8311_CLK_MANAGER_REG03) & 0x80;
        regv |= coeffDiv[coeff].fsMode << 6;
        regv |= coeffDiv[coeff].adcOsr << 0;
        ret |= es8311WriteReg(ES8311_CLK_MANAGER_REG03, regv);

        regv = es8311ReadReg(ES8311_CLK_MANAGER_REG04) & 0x80;
        regv |= coeffDiv[coeff].dacOsr << 0;
        ret |= es8311WriteReg(ES8311_CLK_MANAGER_REG04, regv);

        regv = es8311ReadReg(ES8311_CLK_MANAGER_REG07) & 0xC0;
        regv |= coeffDiv[coeff].lrckH << 0;
        ret |= es8311WriteReg(ES8311_CLK_MANAGER_REG07, regv);

        regv = es8311ReadReg(ES8311_CLK_MANAGER_REG08) & 0x00;
        regv |= coeffDiv[coeff].lrckL << 0;
        ret |= es8311WriteReg(ES8311_CLK_MANAGER_REG08, regv);

        regv = es8311ReadReg(ES8311_CLK_MANAGER_REG06) & 0xE0;
        if (coeffDiv[coeff].bclkDiv < 19)
        {
            regv |= (coeffDiv[coeff].bclkDiv - 1) << 0;
        }
        else
        {
            regv |= (coeffDiv[coeff].bclkDiv) << 0;
        }
        ret |= es8311WriteReg(ES8311_CLK_MANAGER_REG06, regv);
    }

    ret |= es8311WriteReg(ES8311_RESET_REG00,       0x80);

    // Set Codec into Master or Slave mode
    regv = es8311ReadReg(ES8311_RESET_REG00);

    // Set master/slave audio interface
	ES8311_DBG("es8311 in slave mode");
    regv &= 0xBF;
    ret |= es8311WriteReg(ES8311_RESET_REG00, regv);

    ret |= es8311WriteReg(ES8311_SYSTEM_REG13, 0x00/*0x10*/);
    ret |= es8311WriteReg(ES8311_ADC_REG1B, 0x0A);
    ret |= es8311WriteReg(ES8311_ADC_REG1C, 0x6A);
    if (ret != CODEC_EOK)
    {
    	ES8311_DBG("es8311 initialize failed");
    }
    #endif
    return CODEC_EOK;
}

HalCodecSts_e es8311ConfigFmt(void)
{
    HalCodecSts_e ret = CODEC_EOK;
    uint8_t adcIface = 0, dacIface = 0;
    dacIface = es8311ReadReg(ES8311_SDPIN_REG09);
    adcIface = es8311ReadReg(ES8311_SDPOUT_REG0A);
	ES8311_DBG("ES8311 in I2S Format");
    dacIface &= 0xFC;
    adcIface &= 0xFC;
    ret |= es8311WriteReg(ES8311_SDPIN_REG09, dacIface);
    ret |= es8311WriteReg(ES8311_SDPOUT_REG0A, adcIface);

    return ret;
}

HalCodecSts_e es8311SetBitsPerSample(void)
{
    HalCodecSts_e ret = CODEC_EOK;
    uint8_t adc_iface = 0, dac_iface = 0;
    dac_iface = es8311ReadReg(ES8311_SDPIN_REG09);
    adc_iface = es8311ReadReg(ES8311_SDPOUT_REG0A);
    dac_iface |= 0x0c;
    adc_iface |= 0x0c;
    ret |= es8311WriteReg(ES8311_SDPIN_REG09, dac_iface);
    ret |= es8311WriteReg(ES8311_SDPOUT_REG0A, adc_iface);

    return ret;
}

HalCodecSts_e es8311Config(void)
{
    int ret = CODEC_EOK;
    ret |= es8311SetBitsPerSample();
    ret |= es8311ConfigFmt();
    return ret;
}



HalCodecSts_e es8311Start(void)
{
    HalCodecSts_e ret = CODEC_EOK;
    uint8_t adcIface = 0, dacIface = 0;

    dacIface = es8311ReadReg(ES8311_SDPIN_REG09) & 0xBF;
    adcIface = es8311ReadReg(ES8311_SDPOUT_REG0A) & 0xBF;
    adcIface &= ~(BIT(6));
    dacIface &= ~(BIT(6));

    ret |= es8311WriteReg(ES8311_SDPIN_REG09,   dacIface);
    ret |= es8311WriteReg(ES8311_SDPOUT_REG0A,  adcIface);

    ret |= es8311WriteReg(ES8311_ADC_REG17,     0x88/*0xBF*/);
    ret |= es8311WriteReg(ES8311_SYSTEM_REG0E,  0x02);
    ret |= es8311WriteReg(ES8311_SYSTEM_REG12,  0x28/*0x00*/);
    ret |= es8311WriteReg(ES8311_SYSTEM_REG14,  0x18/*0x1A*/);

    // pdm dmic enable or disable
    int regv = 0;
    if (IS_DMIC)
    {
        regv = es8311ReadReg(ES8311_SYSTEM_REG14);
        regv |= 0x40;
        ret |= es8311WriteReg(ES8311_SYSTEM_REG14, regv);
    }
    else
    {
        regv = es8311ReadReg(ES8311_SYSTEM_REG14);
        regv &= ~(0x40);
        ret |= es8311WriteReg(ES8311_SYSTEM_REG14, regv);
    }
    es8311WriteReg(ES8311_SYSTEM_REG14, 0x18);  //add for debug

    ret |= es8311WriteReg(ES8311_SYSTEM_REG0D, 0x01);
    ret |= es8311WriteReg(ES8311_ADC_REG15, 0x00/*0x40*/);
    ret |= es8311WriteReg(ES8311_DAC_REG37, 0x48);
    ret |= es8311WriteReg(ES8311_GP_REG45, 0x00);

    // set internal reference signal (ADCL + DACR)
    ret |= es8311WriteReg(ES8311_GPIO_REG44, 0x00/*0x50*/);

    return ret;
}

HalCodecSts_e es8311Stop(void)
{
    HalCodecSts_e ret = CODEC_EOK;
    es8311Standby(&dacVolBak, &adcVolBak);
    return ret;
}

HalCodecSts_e es8311SetVolume(int volume)
{
    HalCodecSts_e res = CODEC_EOK;

    if (volume <= 0)
    {
        //return es8311SetMute(1);
        volume = 0;
    }

    if (volume >= 100)
    {
        volume = 100;
    }

    int vol = volume * 2550 / 1000;
    ES8311_DBG("Es8311 Set volume:%d", volume);
    res = es8311WriteReg(ES8311_DAC_REG32, vol);
    return res;
}

HalCodecSts_e es8311GetVolume(int *volume)
{
    HalCodecSts_e res = CODEC_EOK;
    int val = 0;
    val = es8311ReadReg(ES8311_DAC_REG32);
    *volume = val * 1000 / 2550;
    ES8311_DBG("Get volume:%d reg_value:0x%x", *volume, val);
    return res;
}

HalCodecSts_e es8311SetMute(bool enable)
{
#if 1
	ES8311_DBG("Es8311SetVoiceMute:%d", enable);
    es8311Mute(enable);
#endif
    return CODEC_EOK;
}

HalCodecSts_e es8311GetVoiceMute(int *mute)
{
    HalCodecSts_e res = CODEC_EOK;
    uint8_t reg = 0;
    res = es8311ReadReg(ES8311_DAC_REG31);
    if (res == CODEC_EOK)
    {
        reg = (res & 0x20) >> 5;
    }
    *mute = reg;
    return res;
}

HalCodecSts_e es8311SetMicVolume(uint8_t micGain, int micVolume)
{
    HalCodecSts_e res = CODEC_EOK;

    if (micVolume <= 0)
    {
        //return es8311SetMute(1);
        micVolume = 0;
    }

    if (micVolume >= 100)
    {
        micVolume = 100;
    }

    int vol = micVolume * 2550 / 1000;
    ES8311_DBG( "Es8311 Set mic volume:%d", micVolume);

    res = es8311WriteReg(ES8311_SYSTEM_REG14, (1<<4 | micGain));
    res = es8311WriteReg(ES8311_ADC_REG17, vol);
    return res;
}

void es8311ReadAll()
{
    ES8311_DBG("now read 8311 all\n");
    for (int i = 0; i < 0x4A; i++)
    {
        uint8_t reg = es8311ReadReg(i);
        ES8311_DBG("reg = 0x%02x, val = 0x%02x\n", i, reg);

    }
    ES8311_DBG("now read 8311 all end\n");

}

HalCodecCfg_t es8311GetDefaultCfg()
{
    HalCodecCfg_t codecCfg = ES8311_DEFAULT_CONFIG();

    return codecCfg;
}



static void codecInit()
{

    es8311Init();
    es8311Config();
    es8311Start();
    es8311SetVolume(HAL_CODEC_VOL_PLAY_DEFAULT);
    es8311SetMicVolume(HAL_CODEC_MIC_GAIN_DEFAULT, HAL_CODEC_VOL_MIC_DEFAULT);
    es8311EnablePA(true);

}

//振铃相关的控制
static uint32_t g_s_tone_on_cnt;
static uint32_t g_s_tone_off_cnt;
static uint32_t g_s_tone_on_total;
static uint32_t g_s_tone_off_total;
static uint8_t *g_s_tone_src;
static uint8_t g_s_tone_play_type;
//自动接听相关
static uint8_t g_s_ring_cnt;
//播放控制
static uint8_t g_s_codec_is_on;
static uint8_t g_s_record_type;
static uint8_t g_s_play_type;
enum
{
	VOLTE_EVENT_PLAY_TONE = 1,
	VOLTE_EVENT_RECORD_VOICE,
	VOLTE_EVENT_PLAY_VOICE,
};

static luat_rtos_task_handle g_s_task_handle;

static int32_t play_tone_cb(void *pdata, void *param)
{
	if (g_s_tone_play_type)
	{
		g_s_tone_off_cnt++;
		if (g_s_tone_off_cnt < g_s_tone_off_total)
		{
			I2S_TransferLoop(I2S_ID, NULL, 1600, 2, 1);
		}
		else
		{
			g_s_tone_on_cnt = 0;
			g_s_tone_play_type = 0;
			I2S_TransferLoop(I2S_ID, g_s_tone_src, 160, 2, 1);
		}
	}
	else
	{
		g_s_tone_on_cnt++;
		if (g_s_tone_on_cnt < g_s_tone_on_total)
		{
			I2S_TransferLoop(I2S_ID, g_s_tone_src, 160, 2, 1);
		}
		else
		{
			g_s_tone_off_cnt = 0;
			g_s_tone_play_type = 1;
			I2S_TransferLoop(I2S_ID, NULL, 1600, 2, 1);
		}
	}
	return 0;
}
static void play_tone(uint8_t param)
{
	bool needIrq = true;
	if (LUAT_MOBILE_CC_PLAY_STOP == param)
	{
		g_s_record_type = 0;
		g_s_play_type = 0;
		luat_i2s_stop(I2S_ID);
		if (g_s_codec_is_on)
		{
			g_s_codec_is_on = 0;
			es8311Stop();
		}

		return;
	}
	else
	{
		g_s_codec_is_on = 1;
		es8311AllResume();
	}
	if (param != LUAT_MOBILE_CC_PLAY_CALL_INCOMINGCALL_RINGING)
	{
		switch(param)
		{
		case LUAT_MOBILE_CC_PLAY_DIAL_TONE:
			g_s_tone_src  = tone450_8k;
			needIrq = false;
			break;
		case LUAT_MOBILE_CC_PLAY_RINGING_TONE:
			g_s_tone_src = tone450_8k;
			g_s_tone_on_total = 1000/20;
			g_s_tone_off_total = 400/20;
			break;
		case LUAT_MOBILE_CC_PLAY_CONGESTION_TONE:
			g_s_tone_src = tone450_8k;
			g_s_tone_on_total = 700/20;
			g_s_tone_off_total = 70/20;
			break;
		case LUAT_MOBILE_CC_PLAY_BUSY_TONE:
			g_s_tone_src = tone450_8k;
			g_s_tone_on_total = 350/20;
			g_s_tone_off_total = 30/20;
			break;
		case LUAT_MOBILE_CC_PLAY_CALL_WAITING_TONE:
			g_s_tone_src = tone450_8k;
			g_s_tone_on_total = 400/20;
			g_s_tone_off_total = 400/20;
			break;
		case LUAT_MOBILE_CC_PLAY_MULTI_CALL_PROMPT_TONE:
			g_s_tone_src = tone950_8k;
			g_s_tone_on_total = 400/20;
			g_s_tone_off_total = 1000/20;

			break;
		}
		luat_i2s_stop(I2S_ID);
		luat_i2s_transfer_start(I2S_ID, 8000, 1, 0, play_tone_cb, NULL);
		g_s_tone_on_cnt = 0;
		g_s_tone_off_cnt = 0;
		g_s_tone_play_type = 0;
		luat_i2s_transfer_loop(I2S_ID, g_s_tone_src, 160, 2, needIrq);
	}
	else
	{
		luat_i2s_start(I2S_ID, 1, 16000, 1);
		luat_i2s_transfer_loop(I2S_ID, callAlertRing16k, 4872, 12, 0);
	}

}

static void start_speech(uint8_t *data, uint32_t param)
{
	g_s_codec_is_on = 1;
	es8311AllResume();

}


static void mobile_event_cb(uint8_t event, uint8_t index, uint8_t status)
{
	char number[64];
	switch (event)
	{
	case LUAT_MOBILE_EVENT_IMS_REGISTER_STATUS:
		DBG("ims reg state %d", status);
		break;
	case LUAT_MOBILE_EVENT_CC:
		switch(status)
		{
		case LUAT_MOBILE_CC_READY:
			DBG("volte ready!");
			break;
		case LUAT_MOBILE_CC_INCOMINGCALL:
			DBG("ring!");
			g_s_ring_cnt++;
			if (g_s_ring_cnt >= 2)
			{
				luat_mobile_answer_call(0);
				g_s_ring_cnt = 0;
			}
			break;
		case LUAT_MOBILE_CC_CALL_NUMBER:
			luat_mobile_get_last_call_num(number, sizeof(number));
			DBG("incoming call %s", number);
			break;
		case LUAT_MOBILE_CC_CONNECTED_NUMBER:
			luat_mobile_get_last_call_num(number, sizeof(number));
			DBG("connected call %s", number);
			break;
		case LUAT_MOBILE_CC_CONNECTED:
			DBG("call connected");
			break;
		case LUAT_MOBILE_CC_DISCONNECTED:
			DBG("call disconnected");
			break;
		case LUAT_MOBILE_CC_SPEECH_START:
			DBG("now speech");
			g_s_record_type = index; //1 = 8K 2 = 16K
			luat_rtos_event_send(g_s_task_handle, VOLTE_EVENT_RECORD_VOICE, 0, 0, 0, 0);
			break;
		case LUAT_MOBILE_CC_MAKE_CALL_OK:
			DBG("call OK");
			break;
		case LUAT_MOBILE_CC_MAKE_CALL_FAILED:
			DBG("call FAILED, result %d", index);
			break;
		case LUAT_MOBILE_CC_ANSWER_CALL_DONE:
			DBG("answer call result %d", index);
			break;
		case LUAT_MOBILE_CC_HANGUP_CALL_DONE:
			DBG("hangup call result %d", index);
			break;
		case LUAT_MOBILE_CC_LIST_CALL_RESULT:
			break;
		case LUAT_MOBILE_CC_PLAY:
			DBG("play %d", index);
			luat_rtos_event_send(g_s_task_handle, VOLTE_EVENT_PLAY_TONE, index, 0, 0, 0);
		}
		break;
	default:
		DBG("event %d, index %d status %d", event, index, status);
		break;
	}

}

void mobile_voice_data_input(uint8_t *input, uint32_t len, uint32_t sample_rate, uint8_t bits)
{
	g_s_play_type = sample_rate; //1 = 8K 2 = 16K
	luat_rtos_event_send(g_s_task_handle, VOLTE_EVENT_PLAY_VOICE, input, len, bits, 0);

}

__CORE_FUNC_IN_RAM__ int32_t record_cb(void *pdata, void *param)
{
	return 0;
}


static void volte_task(void *param)
{
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG);

////	DBG("%u", GetcallAlertRing16KLen());//58464
	uint32_t total, alloc, peak;
	GPIO_Config(PA_PIN, 0, 1);
	GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(CODEC_PIN, 4), 4, 0, 0);
	GPIO_Config(CODEC_PIN, 0, 1);
	//i2c0
	GPIO_IomuxEC7XX(13, 2, 1, 0);
	GPIO_IomuxEC7XX(14, 2, 1, 0);
	I2C_MasterSetup(I2C_ID, 400000);
	I2C_UsePollingMode(I2C_ID, 1);
	vTaskDelay(500);

	uint8_t tx_buf[2];
	uint8_t rx_buf[2];
	tx_buf[0] = 0xfd;
	I2C_BlockRead(I2C_ID, ES8311_IICADDR, tx_buf, 1, rx_buf, 1, 100, NULL, NULL);
	tx_buf[0] = 0xfe;
	I2C_BlockRead(I2C_ID, ES8311_IICADDR, tx_buf, 1, rx_buf + 1, 1, 100, NULL, NULL);
//
	if (0x83 == rx_buf[0] && 0x11 == rx_buf[1])
	{
		DBG("find es8311");
		codecInit();
		es8311EnablePA(1);
		es8311Stop(CODEC_MODE_BOTH);
//		for(int i = 0; i < sizeof(es8311_reg_table)/sizeof(i2c_reg_t); i++)
//		{
//			I2C_BlockWrite(I2C_ID, ES8311_IICADDR, (uint8_t *)&es8311_reg_table[i], 2, 50, NULL, NULL);
//		}
		//i2s0
		GPIO_IomuxEC7XX(35, 1, 0, 0);
		GPIO_IomuxEC7XX(36, 1, 0, 0);
		GPIO_IomuxEC7XX(37, 1, 0, 0);
		GPIO_IomuxEC7XX(38, 1, 0, 0);
		GPIO_IomuxEC7XX(39, 1, 0, 0);
		I2S_BaseConfig(I2S_ID, I2S_MODE_LSB, I2S_FRAME_SIZE_16_16);
//		I2sDataFmt_t DataFmt;
//		I2sSlotCtrl_t  SlotCtrl;
//		I2sBclkFsCtrl_t BclkFsCtrl;
//		I2sDmaCtrl_t DmaCtrl;
//		I2S_GetConfig(I2S_ID, &DataFmt, &SlotCtrl, &BclkFsCtrl, &DmaCtrl);
//		BclkFsCtrl.bclkPolarity = 1;
//		I2S_FullConfig(I2S_ID, DataFmt, SlotCtrl,  BclkFsCtrl,  DmaCtrl);
		//I2S_Start(I2S_ID, 1, 16000, 1);
//		I2S_StartTransfer(I2S_ID, 16000, 1, 0, record_cb, NULL);
//		I2S_TransferLoop(I2S_ID, callAlertRing16k, 4872, 12, 0);
//
	}
	else
	{
		DBG("no es8311");
	}
	GetSRAMHeapInfo(&total, &alloc, &peak);
	DBG("%u,%u,%u", total, alloc, peak);
	GetPSRAMHeapInfo(&total, &alloc, &peak);
	DBG("%u,%u,%u", total, alloc, peak);
//	vTaskDelay(10000);
//	soc_mobile_make_call(0, "15068398077", 11);

	while(1)
	{
		vTaskDelay(5000);
		GetSRAMHeapInfo(&total, &alloc, &peak);
		DBG("%u,%u,%u", total, alloc, peak);
		GetPSRAMHeapInfo(&total, &alloc, &peak);
		DBG("%u,%u,%u", total, alloc, peak);

	}
}

static void task_demo_init(void)
{
	luat_mobile_event_register_handler(mobile_event_cb);
	luat_mobile_speech_init(mobile_voice_data_input);
	luat_rtos_task_create(&g_s_task_handle, 4096, 90, "volte", volte_task, NULL, 64);
}

INIT_TASK_EXPORT(task_demo_init, "1");


#endif
