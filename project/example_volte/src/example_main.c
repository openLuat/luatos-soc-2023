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

#include "common_api.h"
#include "luat_gpio.h"
#include "luat_mobile.h"
#include "luat_i2c.h"
#include "luat_i2s.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_mem.h"

//demo 配置带ES8311硬件的云喇叭开发板，编译时需要把build.bat里的CHIP_TARGET改成ec718pv
#define CODEC_PWR_PIN HAL_GPIO_16
#define CODEC_PWR_PIN_ALT_FUN	4
#define PA_PWR_PIN HAL_GPIO_25
#define PA_PWR_PIN_ALT_FUN	0
#define I2C_ID	1
#define I2S_ID	0
#define NO_ANSWER_AUTO_HANGUP_TIME	30000	//无接听自动挂断时间，单位ms

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


static HalCodecSts_e es8311_init(void)
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
    return CODEC_EOK;
}

static HalCodecSts_e es8311_config_fmt(void)
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

static HalCodecSts_e es8311_set_bits(void)
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

static HalCodecSts_e es8311_config(void)
{
    int ret = CODEC_EOK;
    ret |= es8311_set_bits();
    ret |= es8311_config_fmt();
    return ret;
}



static HalCodecSts_e es8311_start(void)
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

static HalCodecSts_e es8311_stop(void)
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

static void codecInit()
{

    es8311_init();
    es8311_config();
    es8311_start();
    es8311SetVolume(HAL_CODEC_VOL_PLAY_DEFAULT);
    es8311SetMicVolume(HAL_CODEC_MIC_GAIN_DEFAULT, HAL_CODEC_VOL_MIC_DEFAULT);

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
static HANDLE g_s_delay_timer;
static luat_i2s_conf_t *g_s_i2s_conf;
enum
{
	VOLTE_EVENT_PLAY_TONE = 1,
	VOLTE_EVENT_RECORD_VOICE_START,
	VOLTE_EVENT_RECORD_VOICE_UPLOAD,
	VOLTE_EVENT_PLAY_VOICE,
	VOLTE_EVENT_HANGUP,
};

static luat_rtos_task_handle g_s_task_handle;


static LUAT_RT_RET_TYPE hangup_delay(LUAT_RT_CB_PARAM)
{
	luat_rtos_event_send(g_s_task_handle, VOLTE_EVENT_HANGUP, 0, 0, 0, 0);
}

static void play_tone(uint8_t param)
{
	bool needIrq = true;
	if (LUAT_MOBILE_CC_PLAY_STOP == param)
	{
		g_s_record_type = 0;
		g_s_play_type = 0;
		luat_i2s_close(I2S_ID);
		if (g_s_codec_is_on)
		{
			g_s_codec_is_on = 0;
			es8311_stop();
		}
		g_s_i2s_conf->is_full_duplex = 0;
		luat_rtos_timer_stop(g_s_delay_timer);
		return;
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
		g_s_i2s_conf->is_full_duplex = 1;
		g_s_i2s_conf->cb_rx_len = 0;	//这样不会有RX回调
		luat_i2s_modify(I2S_ID, LUAT_I2S_CHANNEL_RIGHT, LUAT_I2S_BITS_16, 8000);
		g_s_tone_on_cnt = 0;
		g_s_tone_off_cnt = 0;
		g_s_tone_play_type = 0;
		luat_i2s_transfer_loop(I2S_ID, g_s_tone_src, 160, 2, needIrq);
	}
	else
	{
		g_s_i2s_conf->is_full_duplex = 0;
		luat_i2s_modify(I2S_ID, LUAT_I2S_CHANNEL_RIGHT, LUAT_I2S_BITS_16, 16000);
		luat_i2s_transfer_loop(I2S_ID, callAlertRing16k, 4872, 12, 0);
	}
	g_s_codec_is_on = 1;
	es8311AllResume();
	luat_rtos_timer_start(g_s_delay_timer, NO_ANSWER_AUTO_HANGUP_TIME, 0, hangup_delay, NULL);
}

static void mobile_event_cb(uint8_t event, uint8_t index, uint8_t status)
{
	char number[64];
	switch (event)
	{
	case LUAT_MOBILE_EVENT_IMS_REGISTER_STATUS:
		LUAT_DEBUG_PRINT("ims reg state %d", status);
		break;
	case LUAT_MOBILE_EVENT_CC:
		switch(status)
		{
		case LUAT_MOBILE_CC_READY:
			LUAT_DEBUG_PRINT("volte ready!");
			break;
		case LUAT_MOBILE_CC_INCOMINGCALL:
			LUAT_DEBUG_PRINT("ring!");
			g_s_ring_cnt++;
			if (g_s_ring_cnt >= 2)
			{
				luat_mobile_answer_call(0);
				g_s_ring_cnt = 0;
			}
			break;
		case LUAT_MOBILE_CC_CALL_NUMBER:
			luat_mobile_get_last_call_num(number, sizeof(number));
			LUAT_DEBUG_PRINT("incoming call %s", number);
			break;
		case LUAT_MOBILE_CC_CONNECTED_NUMBER:
			luat_mobile_get_last_call_num(number, sizeof(number));
			LUAT_DEBUG_PRINT("connected call %s", number);
			break;
		case LUAT_MOBILE_CC_CONNECTED:
			LUAT_DEBUG_PRINT("call connected");
			break;
		case LUAT_MOBILE_CC_DISCONNECTED:
			LUAT_DEBUG_PRINT("call disconnected");
			luat_rtos_timer_stop(g_s_delay_timer);
			break;
		case LUAT_MOBILE_CC_SPEECH_START:
			LUAT_DEBUG_PRINT("now speech type %d", index);
			luat_rtos_event_send(g_s_task_handle, VOLTE_EVENT_RECORD_VOICE_START, index + 1, 0, 0, 0);
			break;
		case LUAT_MOBILE_CC_MAKE_CALL_OK:
			LUAT_DEBUG_PRINT("call OK");
			break;
		case LUAT_MOBILE_CC_MAKE_CALL_FAILED:
			LUAT_DEBUG_PRINT("call FAILED, result %d", index);
			break;
		case LUAT_MOBILE_CC_ANSWER_CALL_DONE:
			LUAT_DEBUG_PRINT("answer call result %d", index);
			break;
		case LUAT_MOBILE_CC_HANGUP_CALL_DONE:
			LUAT_DEBUG_PRINT("hangup call result %d", index);
			break;
		case LUAT_MOBILE_CC_LIST_CALL_RESULT:
			break;
		case LUAT_MOBILE_CC_PLAY:
			LUAT_DEBUG_PRINT("play %d", index);
			luat_rtos_event_send(g_s_task_handle, VOLTE_EVENT_PLAY_TONE, index, 0, 0, 0);
		}
		break;
	default:
		LUAT_DEBUG_PRINT("event %d, index %d status %d", event, index, status);
		break;
	}

}

void mobile_voice_data_input(uint8_t *input, uint32_t len, uint32_t sample_rate, uint8_t bits)
{

	luat_rtos_event_send(g_s_task_handle, VOLTE_EVENT_PLAY_VOICE, (uint32_t)input, len, sample_rate, 0);

}

__USER_FUNC_IN_RAM__ int record_cb(uint8_t id ,luat_i2s_event_t event, uint8_t *rx_data, uint32_t rx_len, void *param)
{
	switch(event)
	{
	case LUAT_I2S_EVENT_RX_DONE:
		luat_rtos_event_send(g_s_task_handle, VOLTE_EVENT_RECORD_VOICE_UPLOAD, (uint32_t)rx_data, rx_len, 0, 0);
		break;
	case LUAT_I2S_EVENT_TX_DONE:
	case LUAT_I2S_EVENT_TRANSFER_DONE:
		if (g_s_tone_play_type)
		{
			g_s_tone_off_cnt++;
			if (g_s_tone_off_cnt < g_s_tone_off_total)
			{
				luat_i2s_transfer_loop(I2S_ID, NULL, 1600, 2, 1);
			}
			else
			{
				g_s_tone_on_cnt = 0;
				g_s_tone_play_type = 0;
				luat_i2s_transfer_loop(I2S_ID, g_s_tone_src, 160, 2, 1);
			}
		}
		else
		{
			g_s_tone_on_cnt++;
			if (g_s_tone_on_cnt < g_s_tone_on_total)
			{
				luat_i2s_transfer_loop(I2S_ID, g_s_tone_src, 160, 2, 1);
			}
			else
			{
				g_s_tone_off_cnt = 0;
				g_s_tone_play_type = 1;
				luat_i2s_transfer_loop(I2S_ID, NULL, 1600, 2, 1);
			}
		}
		break;
	default:
		break;
	}
	return 0;
}



static void volte_task(void *param)
{
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG);
	luat_rtos_timer_create(&g_s_delay_timer);
	luat_i2c_setup(I2C_ID, 0);
	luat_i2c_set_polling_mode(I2C_ID, 1);


	size_t total, alloc, peak;
	luat_rtos_task_sleep(50);
	luat_gpio_set(CODEC_PWR_PIN, 1);
	luat_rtos_task_sleep(100);
	luat_gpio_set(PA_PWR_PIN, 1);
	luat_event_t event;
	uint8_t tx_buf[2];
	uint8_t rx_buf[2];
	tx_buf[0] = 0xfd;
	luat_i2c_transfer(I2C_ID, ES8311_IICADDR, tx_buf, 1, rx_buf, 1);
	tx_buf[0] = 0xfe;
	luat_i2c_transfer(I2C_ID, ES8311_IICADDR, tx_buf, 1, rx_buf + 1, 1);
//
	if (0x83 == rx_buf[0] && 0x11 == rx_buf[1])
	{
		LUAT_DEBUG_PRINT("find es8311");
		codecInit();
		es8311_stop();
	}
	else
	{
		LUAT_DEBUG_PRINT("no es8311");
		while (1)
		{
			luat_rtos_event_recv(g_s_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
		}

	}
	luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
	LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
	luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
	LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
//	luat_rtos_task_sleep(15000);			//如果需要自动拨打出去，这里延迟一下，等volte准备好
//	luat_mobile_make_call(0, "xxx", 11);	//这里填入手机号可以自动拨打
	while (1)
	{
		luat_rtos_event_recv(g_s_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
		switch(event.id)
		{
		case VOLTE_EVENT_PLAY_TONE:
			play_tone(event.param1);
			luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
			LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
			luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
			LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
			break;
		case VOLTE_EVENT_RECORD_VOICE_START:
			g_s_codec_is_on = 1;
			g_s_record_type = event.param1;
			luat_i2s_close(I2S_ID);
			luat_rtos_task_sleep(1);
			g_s_i2s_conf->is_full_duplex = 1;
			g_s_i2s_conf->cb_rx_len = 320 * g_s_record_type;
			luat_i2s_modify(I2S_ID, LUAT_I2S_CHANNEL_RIGHT, LUAT_I2S_BITS_16, g_s_record_type * 8000);
			luat_i2s_transfer_loop(I2S_ID, NULL, 3200, 2, 0);	//address传入空地址就是播放空白音
			es8311AllResume();
			luat_rtos_timer_stop(g_s_delay_timer);
			break;
		case VOLTE_EVENT_RECORD_VOICE_UPLOAD:
			if (g_s_record_type)
			{
				luat_mobile_speech_upload((uint8_t *)event.param1, event.param2);
			}
			break;
		case VOLTE_EVENT_PLAY_VOICE:
			g_s_play_type = event.param3; //1 = 8K 2 = 16K
			if (!g_s_record_type)
			{
				luat_i2s_close(I2S_ID);
				g_s_i2s_conf->is_full_duplex = 0;
				luat_i2s_modify(I2S_ID, LUAT_I2S_CHANNEL_RIGHT, LUAT_I2S_BITS_16, g_s_play_type * 8000);
				if (2 == g_s_play_type)
				{
					luat_i2s_transfer_loop(I2S_ID, (uint8_t *)event.param1, event.param2/3, 3, 0);
				}
				else
				{
					luat_i2s_transfer_loop(I2S_ID, (uint8_t *)event.param1, event.param2/6, 6, 0);
				}
				if (!g_s_codec_is_on)
				{
					g_s_codec_is_on = 1;
					es8311AllResume();
				}

			}
			else
			{
				LUAT_DEBUG_PRINT("%x,%d", event.param1, event.param2);
				if (2 == g_s_record_type)
				{
					luat_i2s_transfer_loop(I2S_ID, (uint8_t *)event.param1, event.param2/3, 3, 0);
				}
				else
				{
					luat_i2s_transfer_loop(I2S_ID, (uint8_t *)event.param1, event.param2/6, 6, 0);
				}
			}
			break;
		case VOLTE_EVENT_HANGUP:
			luat_mobile_hangup_call(0);
			break;
		}
	}
}

static void task_demo_init(void)
{
	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);
	luat_rtos_task_handle task_handle;
	gpio_cfg.output_level = 0;
	// pa power ctrl init
	gpio_cfg.pin = PA_PWR_PIN;
	gpio_cfg.alt_fun = PA_PWR_PIN_ALT_FUN;
	luat_gpio_open(&gpio_cfg);

	// codec power ctrl init
	gpio_cfg.pin = CODEC_PWR_PIN;
	gpio_cfg.alt_fun = CODEC_PWR_PIN_ALT_FUN;
	luat_gpio_open(&gpio_cfg);

	luat_mobile_event_register_handler(mobile_event_cb);
	luat_mobile_speech_init(mobile_voice_data_input);
	luat_rtos_task_create(&g_s_task_handle, 4096, 100, "volte", volte_task, NULL, 64);

	luat_i2s_conf_t conf = {0};
	conf.id = I2S_ID;
	conf.mode = LUAT_I2S_MODE_MASTER;
	conf.channel_format = LUAT_I2S_CHANNEL_RIGHT;
	conf.standard = LUAT_I2S_MODE_LSB;
	conf.channel_bits = LUAT_I2S_BITS_16;
	conf.data_bits = LUAT_I2S_BITS_16;
	conf.luat_i2s_event_callback = record_cb;
	luat_i2s_setup(&conf);
	g_s_i2s_conf = luat_i2s_get_config(I2S_ID);
}

INIT_TASK_EXPORT(task_demo_init, "1");

