/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of AirM2M Ltd.
 * File name:    es8374.c
 * Description:  EC7xx es8374 file
 * History:      Rev1.0   2021-9-18
 *
 ****************************************************************************/

#include "es8374.h"


/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/

#define IS_DMIC             0
#define MCLK_DIV_FRE        256

#define ES8374_DEFAULT_CONFIG()                         \
{                                                       \
        .adcInput  = CODEC_ADC_INPUT_LINE1,             \
        .dacOutput = CODEC_DAC_OUTPUT_ALL,              \
        .codecMode = CODEC_MODE_BOTH,                   \
        .codecIface = {                                 \
            .mode    = CODEC_MODE_SLAVE,                \
            .fmt     = CODEC_I2S_MODE,                  \
            .samples = CODEC_16K_SAMPLES,               \
            .bits    = CODEC_BIT_LENGTH_16BITS,         \
            .channel = CODEC_MONO,                      \
        },                                              \
};

#define MSMODE_MASTERSELON      0       //0: SlaveMode, 1: MasterMode
#define RATIO                   256
#define FORMAT                  0x00
#define FORMAT_LEN              0x03
#define SCLK_DIV                4
#define SCLK_INV                0
#define ADC_CHANNEL_SEL         2
#define DAC_CHANNEL_SEL         0
#define ADC_PGA_DF2SE_15DB      0
#define ADC_PGA_GAIN            0       //(0~7)
#define ADC_VOLUME              0       //(0~192),0:0DB,-0.5dB/Step
#define DAC_VOLUME              0       //(0~192),0:0DB,-0.5dB/Step
#define DMIC_SELON              0
#define DMIC_GAIN               0
#define MICBIASOFF              0

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

// 8374 func list
HalCodecFuncList_t es8374DefaultHandle = 
{
    .halCodecInitFunc           = es8374Init,
    .halCodecDeinitFunc         = es8374DeInit,
    .halCodecCtrlStateFunc      = es8374StartStop,
    .halCodecCfgIfaceFunc       = es8374Config,
    .halCodecSetMuteFunc        = es8374SetMute,
    .halCodecSetVolumeFunc      = es8374SetVolume,
    .halCodecGetVolumeFunc      = es8374GetVolume,
    .halCodecEnablePAFunc       = es8374EnablePA,
    .halCodecSetMicVolumeFunc   = es8374SetMicVolume,
    .halCodecLock               = NULL,
    .handle                     = NULL,
    .halCodecGetDefaultCfg      = es8374GetDefaultCfg,
};



/*----------------------------------------------------------------------------*
 *                      PRIVATE VARIABLES                                     *
 *----------------------------------------------------------------------------*/

static uint8_t dacVolBak, adcVolBak;

/*----------------------------------------------------------------------------*
 *                      PRIVATE FUNCS                                      *
 *----------------------------------------------------------------------------*/

static int32_t es8374WriteReg(uint8_t regAddr, uint16_t data)
{
    int32_t ret = 0;
    ret = codecI2cWrite(ES8374_IICADDR, regAddr, data);

#if 0
#ifdef FEATURE_OS_ENABLE        
    DEBUG_PRINT(UNILOG_PLA_DRIVER, es8374WriteReg_1, P_DEBUG, "reg write: reg=%x, data=%x", regAddr, data);
#else      
    printf("reg write: reg=%02x, data=%02x\n", regAddr, data);
#endif
#endif

    return ret;
}

static uint8_t es8374ReadReg(uint8_t regAddr)
{
    return codecI2cRead(ES8374_IICADDR, regAddr);
}

// set es8374 into suspend mode
static void es8374Standby(uint8_t* dacVolB, uint8_t* adcVolB)
{
    // DEBUG_PRINT(UNILOG_PLA_DRIVER, es8374suspend_0, P_DEBUG, "Enter into es8374 suspend");
    // *dacVolB = es8374ReadReg(ES8374_REG_38);
    // *adcVolB = es8374ReadReg(ES8374_REG_25);
    // es8374WriteReg(ES8374_REG_38, 0xC0);
    // es8374WriteReg(ES8374_REG_25, 0xC0);
    // es8374WriteReg(ES8374_REG_15, 0x00);
    // es8374WriteReg(ES8374_REG_28, 0x1C);
    // es8374WriteReg(ES8374_REG_36, 0x20);
    // es8374WriteReg(ES8374_REG_37, 0x20);
    // es8374WriteReg(ES8374_REG_6D, 0x00);
    // es8374WriteReg(ES8374_REG_09, 0x80);
    // es8374WriteReg(ES8374_REG_1A, 0x08);
    // es8374WriteReg(ES8374_REG_1C, 0x10);
    // es8374WriteReg(ES8374_REG_1D, 0x10);
    // es8374WriteReg(ES8374_REG_24, 0x20);
    // es8374WriteReg(ES8374_REG_22, 0x00);
    // es8374WriteReg(ES8374_REG_21, 0xC0);
    // es8374WriteReg(ES8374_REG_14, 0x16);
    // es8374WriteReg(ES8374_REG_01, 0x03);
}

// set es8374 into resume mode
static HalCodecSts_e es8374Resume(HalCodecModule_e mode)
{
    // DEBUG_PRINT(UNILOG_PLA_DRIVER, es8374Resume_0, P_DEBUG, "Enter into es8374 resume");
    // es8374WriteReg(ES8374_REG_01, 0x7F);
    // es8374WriteReg(ES8374_REG_14, 0x8A);
    // es8374WriteReg(ES8374_REG_15, 0x40);
    // es8374WriteReg(ES8374_REG_21, 0x10);
    // es8374WriteReg(ES8374_REG_22, 0xFF);
    // es8374WriteReg(ES8374_REG_24, 0x08);
    // es8374WriteReg(ES8374_REG_1E, 0x20);
    // delay_us(1000);
    // es8374WriteReg(ES8374_REG_1E, 0xA0);
    // es8374WriteReg(ES8374_REG_1D, 0x2B);
    // es8374WriteReg(ES8374_REG_1C, 0x90);
    // es8374WriteReg(ES8374_REG_1A, 0xA0);
    // es8374WriteReg(ES8374_REG_09, 0x41);
    // es8374WriteReg(ES8374_REG_6D, 0x60);
    // es8374WriteReg(ES8374_REG_37, 0x00);
    // es8374WriteReg(ES8374_REG_36, 0x00);
    // es8374WriteReg(ES8374_REG_28, 0x00);
    // es8374WriteReg(ES8374_REG_38, dacVolBak);
    // es8374WriteReg(ES8374_REG_25, adcVolBak);
    
    return CODEC_EOK;
}

// set es8374 into powerdown mode
static HalCodecSts_e es8374PwrDown(HalCodecModule_e mode)
{
    DEBUG_PRINT(UNILOG_PLA_DRIVER, es8374pwrDown_0, P_DEBUG, "Enter into es8374 powerdown");

    return CODEC_EOK;
}


/*----------------------------------------------------------------------------*
 *                      GLOBAL FUNCTIONS                                      *
 *----------------------------------------------------------------------------*/
 
// enable pa power
void es8374EnablePA(bool enable)
{
    es8374WriteReg(ES8374_REG_1E, 0x20);
    delay_us(1000);
    es8374WriteReg(ES8374_REG_1E, ((enable == true) ? 0xA0 : 0x40));
}

HalCodecSts_e es8374Init(HalCodecCfg_t *codecCfg)
{
    codecI2cInit();

    es8374WriteReg(ES8374_REG_00, 0x3F);
    es8374WriteReg(ES8374_REG_00, 0x03);
    es8374WriteReg(ES8374_REG_01, 0x7F);
    es8374WriteReg(ES8374_REG_02, 0x00);
    es8374WriteReg(ES8374_REG_03, 0x20);
    es8374WriteReg(ES8374_REG_04, 0x00);
    es8374WriteReg(ES8374_REG_05, 0x11);
    es8374WriteReg(ES8374_REG_06, (RATIO>>8));
    es8374WriteReg(ES8374_REG_07, (RATIO & 0xFF));
    es8374WriteReg(ES8374_REG_10, FORMAT + (FORMAT_LEN<<2));
    es8374WriteReg(ES8374_REG_11, FORMAT + (FORMAT_LEN<<2));
    es8374WriteReg(ES8374_REG_0F, (MSMODE_MASTERSELON<<7) + (SCLK_INV<<5) + SCLK_DIV);
    es8374WriteReg(ES8374_REG_24, 0x08 + (DMIC_GAIN<<7) + DMIC_SELON);
    es8374WriteReg(ES8374_REG_36, (DAC_CHANNEL_SEL<<6));
    es8374WriteReg(ES8374_REG_12, 0x30);
    es8374WriteReg(ES8374_REG_13, 0x20);
    es8374WriteReg(ES8374_REG_18, 0xFF);
    es8374WriteReg(ES8374_REG_21, 0x50);
    es8374WriteReg(ES8374_REG_22, ADC_PGA_GAIN + (ADC_PGA_GAIN<<4));
    es8374WriteReg(ES8374_REG_21, (ADC_CHANNEL_SEL<<4) + (ADC_PGA_DF2SE_15DB<<2));
    es8374WriteReg(ES8374_REG_00, 0x80);
    es8374WriteReg(ES8374_REG_14, 0x8A + (MICBIASOFF<<4));
    es8374WriteReg(ES8374_REG_15, 0x40);
    es8374WriteReg(ES8374_REG_1A, 0xA0);
    es8374WriteReg(ES8374_REG_1B, 0x19);
    es8374WriteReg(ES8374_REG_1C, 0x90);
    es8374WriteReg(ES8374_REG_1D, 0x2B);
    es8374WriteReg(ES8374_REG_1F, 0x00);
    es8374WriteReg(ES8374_REG_20, 0x00);
    es8374WriteReg(ES8374_REG_1E, 0x20);
    es8374WriteReg(ES8374_REG_1E, 0xA0);
    es8374WriteReg(ES8374_REG_28, 0x00);
    es8374WriteReg(ES8374_REG_25, ADC_VOLUME);
    es8374WriteReg(ES8374_REG_38, DAC_VOLUME);
    es8374WriteReg(ES8374_REG_37, 0x00);
    es8374WriteReg(ES8374_REG_6D, 0x60);

    return CODEC_EOK;
}

void es8374DeInit()
{
    //es8374PwrDown(0);
    codecI2cDeInit();
}

HalCodecSts_e es8374ConfigFmt(HalCodecIfaceFormat_e fmt)
{
    HalCodecSts_e ret = CODEC_EOK;

    return ret;
}

HalCodecSts_e es8374SetBitsPerSample(HalCodecIfaceBits_e bits)
{
    HalCodecSts_e ret = CODEC_EOK;

    return ret;
}

HalCodecSts_e es8374Config(HalCodecMode_e mode, HalCodecIface_t *iface)
{
    int ret = CODEC_EOK;

    return ret;
}

HalCodecSts_e es8374StartStop(HalCodecMode_e mode, HalCodecCtrlState_e ctrlState)
{
    HalCodecSts_e ret = CODEC_EOK;
    HalCodecModule_e es_mode = MODULE_MIN;

    switch (mode) 
    {
        case CODEC_MODE_ENCODE:
            es_mode  = MODULE_ADC;
            break;
        case CODEC_MODE_LINE_IN:
            es_mode  = MODULE_LINE;
            break;
        case CODEC_MODE_DECODE:
            es_mode  = MODULE_DAC;
            break;
        case CODEC_MODE_BOTH:
            es_mode  = MODULE_ADC_DAC;
            break;
        default:
            es_mode = MODULE_DAC;
            DEBUG_PRINT(UNILOG_PLA_DRIVER, es8374CtrlState_1, P_DEBUG, "Codec mode not support, default is decode mode");
            break;
    }

    switch(ctrlState)
    {
        case CODEC_CTRL_START:
            ret |= es8374Start(es_mode);
            break;
        case CODEC_CTRL_STOP:
            ret |= es8374Stop(es_mode);
            break;
        case CODEC_CTRL_RESUME:
            ret |= es8374Resume(es_mode);
            break;
        case CODEC_CTRL_POWERDONW:
            ret |= es8374PwrDown(es_mode);
            break;
    }
    

    return ret;
}

HalCodecSts_e es8374Start(HalCodecModule_e mode)
{
    HalCodecSts_e ret = CODEC_EOK;

    return ret;
}

HalCodecSts_e es8374Stop(HalCodecModule_e mode)
{
    HalCodecSts_e ret = CODEC_EOK;
    es8374Standby(&dacVolBak, &adcVolBak);
    return ret;
}

HalCodecSts_e es8374SetVolume(int volume)
{
    HalCodecSts_e res = CODEC_EOK;

    return res;
}

HalCodecSts_e es8374GetVolume(int *volume)
{
    HalCodecSts_e res = CODEC_EOK;

    return res;
}

HalCodecSts_e es8374SetMute(bool enable)
{ 
    return CODEC_EOK;
}

HalCodecSts_e es8374GetVoiceMute(int *mute)
{
    HalCodecSts_e res = CODEC_EOK;

    return res;
}

HalCodecSts_e es8374SetMicVolume(int micVolume)
{
    HalCodecSts_e res = CODEC_EOK;

    return res;
}

void es8374ReadAll()
{
#ifdef FEATURE_OS_ENABLE        
    DEBUG_PRINT(UNILOG_PLA_DRIVER, es8374ReadAll_1, P_DEBUG, "now read 8374 all");
#else      
    printf("now read 8374 all\n");
#endif

    for (int i = 0; i < 0x4A; i++) 
    {
        uint8_t reg = es8374ReadReg(i);
#ifdef FEATURE_OS_ENABLE        
        DEBUG_PRINT(UNILOG_PLA_DRIVER, es8374ReadAll_2, P_DEBUG, "REG:%x, %x", reg, i);
#else      
        printf("reg = 0x%02x, val = 0x%02x\n", i, reg);
#endif
    }

#ifdef FEATURE_OS_ENABLE        
    DEBUG_PRINT(UNILOG_PLA_DRIVER, es8374ReadAll_3, P_DEBUG, "now read 8374 all end");
#else      
    printf("now read 8374 all end\n");
#endif
}

HalCodecCfg_t es8374GetDefaultCfg()
{
    HalCodecCfg_t codecCfg = ES8374_DEFAULT_CONFIG();

    return codecCfg;
}
