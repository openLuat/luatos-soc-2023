#include "common_api.h"
#include "luat_rtos.h"
#include "luat_audio_play.h"
#include "luat_i2s.h"
#include "ivTTSSDKID_all.h"
#include "ivTTS.h"
#include "amr_alipay_data.h"
#include "amr_2_data.h"
#include "amr_10_data.h"
#include "amr_yuan_data.h"
#include "power_audio.h"
#include "luat_gpio.h"
#include "luat_debug.h"
#include "luat_fs.h"
#include "luat_mem.h"
#include "luat_i2c.h"
//AIR780P音频开发板配置
#define CODEC_PWR_PIN HAL_GPIO_16
#define CODEC_PWR_PIN_ALT_FUN	4
#ifndef CHIP_EC716
#define PA_PWR_PIN HAL_GPIO_25
#define PA_PWR_PIN_ALT_FUN	0
#else
#define PA_PWR_PIN HAL_GPIO_20
#define PA_PWR_PIN_ALT_FUN	0
#endif

#define ES8311  0
#define TM8211	1

typedef struct
 {
     uint8_t                 regAddr;    ///< Register addr
     uint8_t                regVal;     ///< Register value
 }i2c_reg_t;

static i2c_reg_t es8311_reg_table[] = 
{
	{0x45,0x00},
	{0x01,0x30},
	{0x02,0x10},
	//Ratio=MCLK/LRCK=256：12M288-48K；4M096-16K; 2M048-8K
	{0x02,0x00},//MCLK DIV=1
	{0x03,0x10},
	{0x16,0x24},
	{0x04,0x20},
	{0x05,0x00},
	{0x06,(0<<5) + 4 -1},//(0x06,(SCLK_INV<<5) + SCLK_DIV -1); SCLK=BCLK
	{0x07,0x00},
	{0x08,0xFF},//0x07 0x08 fs=256
	{0x09,0x0C},//I2S mode, 16bit
	{0x0A,0x0C},//I2S mode, 16bit
	{0x0B,0x00},
	{0x0C,0x00},
	// {0x10,(0x1C*0) + (0x60*0x01) + 0x03},	//(0x10,(0x1C*DACHPModeOn) + (0x60*VDDA_VOLTAGE) + 0x03);	//VDDA_VOLTAGE=1.8V  close es8311MasterInit 3.3PWR setting
	{0x10,(0x1C*0) + (0x60*0x00) + 0x03},	//(0x10,(0x1C*DACHPModeOn) + (0x60*VDDA_VOLTAGE) + 0x03);	//VDDA_VOLTAGE=3.3V open es8311MasterInit 3.3PWR setting
	{0x11,0x7F},
	{0x00,0x80 + (0<<6)},//Slave  Mode	(0x00,0x80 + (MSMode_MasterSelOn<<6));//Slave  Mode
	{0x0D,0x01},
	{0x01,0x3F + (0x00<<7)},//(0x01,0x3F + (MCLK<<7));
	{0x14,(0<<6) + (1<<4) + 10},//选择CH1输入+30DB GAIN	(0x14,(Dmic_Selon<<6) + (ADCChannelSel<<4) + ADC_PGA_GAIN);
	{0x12,0x28},
	{0x13,0x00 + (0<<4)},	//(0x13,0x00 + (DACHPModeOn<<4));
	{0x0E,0x02},
	{0x0F,0x44},
	{0x15,0x00},
	{0x1B,0x0A},
	{0x1C,0x6A},
	{0x37,0x48},
	{0x44,(0 << 7)},	//(0x44,(ADC2DAC_Sel <<7));
	{0x17,0xd2},//(0x17,ADC_Volume);
	{0x32,0xc8},//(0x32,DAC_Volume);
};

static i2c_reg_t es8311_standby_reg_table[] = 
{
	{0x32,0x00},
	{0x17,0x00},
	{0x0E,0xFF},
	{0x12,0x02},
	{0x14,0x00},
	{0x0D,0xFA},
	{0x15,0x00},
	{0x37,0x08},
	{0x02,0x10},
	{0x00,0x00},
	{0x00,0x1F},
	{0x01,0x30},
	{0x01,0x00},
	{0x45,0x00},
	{0x0D,0xFC},
	{0x02,0x00},
};

static i2c_reg_t es8311_resume_reg_table[] = 
{
	{0x0D,0x01},
	{0x45,0x00},
	{0x01,0x3F},
	{0x00,0x80},
	{0x02,0x00},
	{0x37,0x08},
	{0x15,0x40},
	{0x14,0x10},
	{0x12,0x00},
	{0x0E,0x00},
	{0x32,0xc8},
	{0x17,0xd2},
};

void es8311_standby()
{
	for (int i = 0; i < sizeof(es8311_standby_reg_table) / sizeof(i2c_reg_t); i++)
	{
		luat_i2c_send(1, 0x18, &es8311_standby_reg_table[i], 2, 1);
	}
}

void es8311_resume()
{
	for (int i = 0; i < sizeof(es8311_resume_reg_table) / sizeof(i2c_reg_t); i++)
	{
		luat_i2c_send(1, 0x18, &es8311_resume_reg_table[i], 2, 1);
	}
}

void codec_ctrl(uint8_t onoff)
{
	if (1 == onoff)
	{
#if ES8311 == 1
	es8311_resume();
#else
	luat_gpio_set(CODEC_PWR_PIN, 1);
#endif
	}
	else
	{
#if ES8311 == 1
	es8311_standby();
#else
	luat_gpio_set(CODEC_PWR_PIN, 0);
#endif
	}
}


extern void download_file();
static HANDLE g_s_delay_timer;

LUAT_RT_RET_TYPE app_pa_on(LUAT_RT_CB_PARAM)
{
	luat_gpio_set(PA_PWR_PIN, 1);
}

void audio_event_cb(uint32_t event, void *param)
{
	LUAT_DEBUG_PRINT("%d", event);
	switch(event)
	{
	case LUAT_MULTIMEDIA_CB_AUDIO_DECODE_START:
		codec_ctrl(1);
		luat_audio_play_write_blank_raw(0, 3, 1);
		break;
	case LUAT_MULTIMEDIA_CB_AUDIO_OUTPUT_START:
		luat_rtos_timer_start(g_s_delay_timer, 100, 0, app_pa_on, NULL);
		break;
	case LUAT_MULTIMEDIA_CB_TTS_INIT:
		break;
	case LUAT_MULTIMEDIA_CB_TTS_DONE:
		if (!luat_audio_play_get_last_error(0))
		{
			luat_audio_play_write_blank_raw(0, 1, 0);
		}
		break;
	case LUAT_MULTIMEDIA_CB_AUDIO_DONE:
		luat_rtos_timer_stop(g_s_delay_timer);
		LUAT_DEBUG_PRINT("audio play done, result=%d!", luat_audio_play_get_last_error(0));
		luat_gpio_set(PA_PWR_PIN, 0);
		codec_ctrl(0);
		//如果用软件DAC，打开下面的2句注释，消除POP音和允许进低功耗
//		luat_rtos_task_sleep(10);
//		SoftDAC_Stop();
		break;
	}
}

void HAL_I2sSrcAdjustVolumn(int16_t* srcBuf, uint32_t srcTotalNum, uint16_t volScale)
{
	int integer = volScale / 10;
	int decimal = volScale % 10;
	int scale = 0;
	int32_t tmp = 0;
	uint32_t totalNum = srcTotalNum;
	uint32_t step = 0;
	
	while (totalNum)
	{
		if (volScale < 10)
		{
			tmp = ((*(srcBuf + step)) * (256 * integer + 26 * decimal)) >> 8;
		}
		else
		{
			scale = (256 * integer + 26 * decimal) >> 8;
			tmp = (*(srcBuf + step)) * scale;
		}
		
		if (tmp > 32767)
		{
			tmp = 32767;
		}
		else if (tmp < -32768)
		{
			tmp = -32768;
		}
			
		*(srcBuf + step) = (int16_t)tmp;
		step += 1;
		totalNum -= 2;
	}
}

void audio_data_cb(uint8_t *data, uint32_t len, uint8_t bits, uint8_t channels)
{
	//这里可以对音频数据进行软件音量缩放，或者直接清空来静音
	//软件音量缩放参考HAL_I2sSrcAdjustVolumn
#if ES8311 == 1
	HAL_I2sSrcAdjustVolumn((int16_t *)data, len, 5);
#endif
	//LUAT_DEBUG_PRINT("%x,%d,%d,%d", data, len, bits, channels);
}

static void tts_config(void)
{
	// 8K英文
	// ivCStrA sdk_id = AISOUND_SDK_USERID_8K_ENG;
	// luat_audio_play_tts_set_resource(ivtts_8k_eng, sdk_id, NULL);

	// 16K英文
	// ivCStrA sdk_id = AISOUND_SDK_USERID_16K_ENG;
	// luat_audio_play_tts_set_resource(ivtts_16k_eng, sdk_id, NULL);

	// 8K中文
	// ivCStrA sdk_id = AISOUND_SDK_USERID_8K;
	// luat_audio_play_tts_set_resource(ivtts_8k, sdk_id, NULL);

	// 16K中文
	ivCStrA sdk_id = AISOUND_SDK_USERID_16K;
	luat_audio_play_tts_set_resource((void*)ivtts_16k, (void*)sdk_id, NULL);
}

static int codec_config()
{
#if ES8311 == 1
	luat_gpio_set(CODEC_PWR_PIN, 1);
	luat_i2c_setup(1, 0);
	uint8_t tx_buf[2] = {0};
	uint8_t rx_buf[2] = {0};
	tx_buf[0] = 0xfd;
    luat_i2c_send(1, 0x18, tx_buf, 1, 1);
    luat_i2c_recv(1, 0x18, rx_buf, 1);

    tx_buf[0] = 0xfe;
    luat_i2c_send(1, 0x18, tx_buf, 1, 1);
    luat_i2c_recv(1, 0x18, rx_buf + 1, 1);

	if (rx_buf[0] != 0x83 && rx_buf[1] != 0x11)
	{
		LUAT_DEBUG_PRINT("not find es8311");
		return -1;
	}
	LUAT_DEBUG_PRINT("find es8311");
	for (int i = 0; i < sizeof(es8311_reg_table)/sizeof(i2c_reg_t); i++)
    {
		luat_i2c_send(1, 0x18, &es8311_reg_table[i], 2, 1);
    }
	luat_rtos_task_sleep(500);
#endif
	return 0;
}

static void play_channel_config(void)
{
	// 模拟DAC通道
	// luat_audio_play_set_bus_type(LUAT_AUDIO_BUS_SOFT_DAC);
	
	// ES7149/ES7148
	// luat_i2s_base_setup(0, I2S_MODE_I2S, I2S_FRAME_SIZE_16_16);

	// TM8211/ES8311
	luat_i2s_base_setup(0, I2S_MODE_MSB, I2S_FRAME_SIZE_16_16);
}


static void demo_task(void *arg)
{
	size_t total = 0, used = 0, max_used = 0;
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG);
	// 中文测试用下面的
	char tts_string[] = "支付宝到账123.45元,微信收款9876.12元ABC,支付宝到账123.45元,微信收款9876.12元ABC,支付宝到账123.45元,微信收款9876.12元ABC,支付宝到账123.45元,微信收款9876.12元ABC";
	// 英文测试用下面的
	// char tts_string[] = "hello world, now test once";
	luat_audio_play_info_t mp3_info[4] = {0};
	luat_audio_play_info_t amr_info[5] = {0};
	download_file();
	luat_rtos_timer_create(&g_s_delay_timer);
    luat_audio_play_global_init(audio_event_cb, audio_data_cb, luat_audio_play_file_default_fun, luat_audio_play_tts_default_fun, NULL);
	tts_config();
	int result = codec_config();
	if(result)
	{
		while (1)
		{
			LUAT_DEBUG_PRINT("codec config fail");
			luat_rtos_task_sleep(1000);
		}
	}
	play_channel_config();


	mp3_info[0].path = "test1.mp3";
	mp3_info[1].path = "test2.mp3";
	mp3_info[2].path = "test3.mp3";
	mp3_info[3].path = "test4.mp3";
	amr_info[0].path = NULL;
    amr_info[0].address = (uint32_t)amr_alipay_data;
    amr_info[0].rom_data_len = sizeof(amr_alipay_data);
    amr_info[1].path = NULL;
    amr_info[1].address = (uint32_t)amr_2_data;
    amr_info[1].rom_data_len = sizeof(amr_2_data);
    amr_info[2].path = NULL;
    amr_info[2].address = (uint32_t)amr_10_data;
    amr_info[2].rom_data_len = sizeof(amr_10_data);
    amr_info[3].path = NULL;
    amr_info[3].address = (uint32_t)amr_2_data;
    amr_info[3].rom_data_len = sizeof(amr_2_data);
    amr_info[4].path = NULL;
    amr_info[4].address = (uint32_t)amr_yuan_data;
    amr_info[4].rom_data_len = sizeof(amr_yuan_data);

    while(1)
    {
    	luat_audio_play_multi_files(0, mp3_info, 4);
		luat_rtos_task_sleep(9000);
		luat_meminfo_sys(&total, &used, &max_used);
    	LUAT_DEBUG_PRINT("meminfo total %d, used %d, max_used%d",total, used, max_used);

		luat_audio_play_multi_files(0, amr_info, 5);
		luat_rtos_task_sleep(9000);
    	luat_meminfo_sys(&total, &used, &max_used);
    	LUAT_DEBUG_PRINT("meminfo total %d, used %d, max_used%d",total, used, max_used);

		luat_audio_play_tts_text(0, tts_string, sizeof(tts_string));
		luat_rtos_task_sleep(35000);
		luat_meminfo_sys(&total, &used, &max_used);
    	LUAT_DEBUG_PRINT("meminfo total %d, used %d, max_used%d",total, used, max_used);
    }
}

static void test_audio_demo_init(void)
{
	luat_fs_init();
	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);
	luat_rtos_task_handle task_handle;

	// pa power ctrl init
	gpio_cfg.pin = PA_PWR_PIN;
	gpio_cfg.alt_fun = PA_PWR_PIN_ALT_FUN;
	luat_gpio_open(&gpio_cfg);

	// codec power ctrl init
	gpio_cfg.pin = CODEC_PWR_PIN;
	gpio_cfg.alt_fun = CODEC_PWR_PIN_ALT_FUN;
	luat_gpio_open(&gpio_cfg);

	// 当前仅EC718p支持这个demo
	#if defined TYPE_EC718P
	luat_rtos_task_create(&task_handle, 2048, 20, "test", demo_task, NULL, 0);
	#endif
}

INIT_TASK_EXPORT(test_audio_demo_init, "1");
