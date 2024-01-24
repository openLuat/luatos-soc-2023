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
#include "luat_pm.h"
//AIR780P音频开发板配置，如果用的ES8311而且要低功耗的，不建议用LDO_CTL，换成AGPIO
#define CODEC_PWR_PIN HAL_GPIO_16
#define CODEC_PWR_PIN_ALT_FUN	4
#ifndef CHIP_EC716
#define PA_PWR_PIN HAL_GPIO_25
#define PA_PWR_PIN_ALT_FUN	0
#else
#define PA_PWR_PIN HAL_GPIO_20
#define PA_PWR_PIN_ALT_FUN	0
#endif

#define TEST_I2C_ID I2C_ID1
#define TEST_I2S_ID I2S_ID0
#define TEST_USE_ES8311	0
#define TEST_USE_TM8211 1

#define RECORD_ONCE_LEN	20	   //单声道 8K录音单次20个编码块，总共400ms回调 320B 20ms，amr编码要求，20ms一个块
int record_cb(uint8_t id ,luat_i2s_event_t event, uint8_t *rx_data, uint32_t rx_len, void *param);

static const luat_audio_codec_conf_t luat_audio_codec_es8311 = {
    .i2c_id = TEST_I2C_ID,
    .i2s_id = TEST_I2S_ID,
    .pa_pin = PA_PWR_PIN,
    .pa_on_level = 1,
	.power_pin = CODEC_PWR_PIN,
	.power_on_level = 1,
	.power_on_delay_ms = 10,
	.pa_delay_time = 100,
	.after_sleep_ready_time = 600,
	.codec_delay_off_time = 1,
    .codec_opts = &codec_opts_es8311,
};

static const luat_audio_codec_conf_t luat_audio_codec_tm8211 = {
    .i2s_id = TEST_I2S_ID,
    .pa_pin = PA_PWR_PIN,
    .pa_on_level = 1,
	.power_pin = CODEC_PWR_PIN,
	.power_on_level = 1,
	.pa_delay_time = 100,
	.after_sleep_ready_time = 400,
    .codec_opts = &codec_opts_tm8211,
};

static const luat_i2s_conf_t luat_i2s_conf_es8311 =
{
	.id = TEST_I2S_ID,
	.mode = LUAT_I2S_MODE_MASTER,
	.channel_format = LUAT_I2S_CHANNEL_RIGHT,
	.standard = LUAT_I2S_MODE_LSB,
	.channel_bits = LUAT_I2S_BITS_16,
	.data_bits = LUAT_I2S_BITS_16,
	.is_full_duplex = 1,
	.cb_rx_len = 320 * RECORD_ONCE_LEN,
	.luat_i2s_event_callback = record_cb,
};

static const luat_i2s_conf_t luat_i2s_conf_tm8211 =
{
	.id = TEST_I2S_ID,
	.mode = LUAT_I2S_MODE_MASTER,
	.channel_format = LUAT_I2S_CHANNEL_STEREO,
	.standard = LUAT_I2S_MODE_MSB,
	.channel_bits = LUAT_I2S_BITS_16,
	.data_bits = LUAT_I2S_BITS_16,
	.is_full_duplex = 0,
	.luat_i2s_event_callback = record_cb,
};

extern void download_file();

enum
{
	VOLTE_EVENT_PLAY_TONE = 1,
	VOLTE_EVENT_RECORD_VOICE_START,
	VOLTE_EVENT_RECORD_VOICE_UPLOAD,
	VOLTE_EVENT_PLAY_VOICE,
	VOLTE_EVENT_HANGUP,

	AUDIO_EVENT_PLAY_DONE,
};
static const int g_s_amr_nb_sizes[] = { 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };

static luat_rtos_task_handle g_s_task_handle;

__USER_FUNC_IN_RAM__ int record_cb(uint8_t id ,luat_i2s_event_t event, uint8_t *rx_data, uint32_t rx_len, void *param)
{
	switch(event)
	{
	case LUAT_I2S_EVENT_RX_DONE:
		luat_rtos_event_send(g_s_task_handle, VOLTE_EVENT_RECORD_VOICE_UPLOAD, (uint32_t)rx_data, rx_len, 0, 0);
		break;
	case LUAT_I2S_EVENT_TRANSFER_DONE:
		break;
	default:
		break;
	}
	return 0;
}

void audio_event_cb(uint32_t event, void *param)
{
	LUAT_DEBUG_PRINT("%d", event);
	switch(event)
	{
	case LUAT_MULTIMEDIA_CB_AUDIO_DECODE_START:
		luat_audio_check_ready(0);
		break;
	case LUAT_MULTIMEDIA_CB_AUDIO_OUTPUT_START:
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
		LUAT_DEBUG_PRINT("audio play done, result=%d!", luat_audio_play_get_last_error(0));
		luat_audio_standby(0);
		//如果追求极致的功耗，用luat_audio_sleep代替luat_audio_standby
		//luat_audio_sleep(0, 1);
		//通知一下用户task播放完成了
		luat_rtos_event_send(g_s_task_handle, AUDIO_EVENT_PLAY_DONE, luat_audio_play_get_last_error(0), 0, 0, 0);
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


static void demo_task(void *arg)
{
	size_t total = 0, alloc = 0, peak = 0;
	luat_event_t event;
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG);

#if defined FEATURE_IMS_ENABLE	//VOLTE固件不支持TTS
	luat_audio_play_global_init_with_task_priority(audio_event_cb, audio_data_cb, luat_audio_play_file_default_fun, NULL, NULL, 50);
#else
	luat_audio_play_global_init_with_task_priority(audio_event_cb, audio_data_cb, luat_audio_play_file_default_fun, luat_audio_play_tts_default_fun, NULL, 50);
	tts_config();
#endif
	if (TEST_USE_ES8311)
	{
		luat_i2c_setup(luat_audio_codec_es8311.i2c_id, 1);
		luat_i2s_setup(&luat_i2s_conf_es8311);
		luat_audio_setup_codec(0, &luat_audio_codec_es8311);
		luat_audio_init_codec(0, 70, 65);
	}
	if (TEST_USE_TM8211)
	{
		luat_i2s_setup(&luat_i2s_conf_tm8211);
		luat_audio_setup_codec(0, &luat_audio_codec_tm8211);
		luat_audio_init_codec(0, 100, 0);
	}

#if defined FEATURE_IMS_ENABLE	//VOLTE固件不支持TTS
#else
	// 中文测试用下面的
	char tts_string[] = "支付宝到账123.45元,微信收款9876.12元ABC";
	// 英文测试用下面的
	// char tts_string[] = "hello world, now test once";
#endif
	luat_audio_play_info_t mp3_info[4] = {0};
	luat_audio_play_info_t amr_info[5] = {0};
	download_file();

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
		luat_rtos_event_recv(g_s_task_handle, AUDIO_EVENT_PLAY_DONE, &event, NULL, LUAT_WAIT_FOREVER);
		luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
		LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
		luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
		LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
		luat_audio_play_multi_files(0, amr_info, 5);
		luat_rtos_event_recv(g_s_task_handle, AUDIO_EVENT_PLAY_DONE, &event, NULL, LUAT_WAIT_FOREVER);
		luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
		LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
		luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
		LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
#if defined FEATURE_IMS_ENABLE	//VOLTE固件不支持TTS
#else
		luat_audio_play_tts_text(0, tts_string, sizeof(tts_string));
		luat_rtos_event_recv(g_s_task_handle, AUDIO_EVENT_PLAY_DONE, &event, NULL, LUAT_WAIT_FOREVER);
		luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
		LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
		luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
		LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
#endif
		//演示一下休眠
		luat_audio_sleep(0, 1);
		luat_pm_power_ctrl(LUAT_PM_POWER_USB, 0);	//没接USB的，只需要在开始的时候关闭一次USB就行了
		luat_pm_request(LUAT_PM_SLEEP_MODE_LIGHT);
		luat_rtos_task_sleep(10000);
		luat_pm_request(LUAT_PM_SLEEP_MODE_IDLE);
		luat_pm_power_ctrl(LUAT_PM_POWER_USB, 1);
		if (TEST_USE_ES8311)
		{
			luat_audio_init_codec(0, 70, 65);	//如果没有AGPIO来控制，需要重新初始化ES8311，如果用AGPIO来控制的，就不需要重新初始化
			luat_audio_sleep(0, 0);
		}
		//带ES8311的演示录音后再放音
		if (TEST_USE_ES8311)
		{
			luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
			LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
			luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
			LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
		}
		//带ES8311的演示双向对讲
		if (TEST_USE_ES8311)
		{
			luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
			LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
			luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
			LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
		}
		//带ES8311+780EPV的演示VOLTE通话
		if (TEST_USE_ES8311)
		{
			luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
			LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
			luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
			LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
		}
    }
}
extern void audio_play_set_ram_type(LUAT_HEAP_TYPE_E Type);
static void test_audio_demo_init(void)
{
	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);


	// pa power ctrl init
	gpio_cfg.pin = PA_PWR_PIN;
	gpio_cfg.alt_fun = PA_PWR_PIN_ALT_FUN;
	luat_gpio_open(&gpio_cfg);

	// codec power ctrl init
	gpio_cfg.pin = CODEC_PWR_PIN;
	gpio_cfg.alt_fun = CODEC_PWR_PIN_ALT_FUN;
	luat_gpio_open(&gpio_cfg);

	// 当前仅EC718p/EC718pv支持这个demo
	#if defined TYPE_EC718P
	luat_rtos_task_create(&g_s_task_handle, 4096, 100, "test", demo_task, NULL, 0);
//	audio_play_set_ram_type(LUAT_HEAP_SRAM);		//打开后消耗RAM较多的地方将使用SRAM，否则使用AUTO模式
	#endif

}

INIT_TASK_EXPORT(test_audio_demo_init, "1");
