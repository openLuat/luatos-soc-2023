//demo用于AIR600EAC云喇叭开发
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
//AIR780P+TM8211开发板配置
#define CODEC_PWR_PIN HAL_GPIO_16
#define CODEC_PWR_PIN_ALT_FUN	4
#define PA_PWR_PIN HAL_GPIO_25
#define PA_PWR_PIN_ALT_FUN	0



extern void download_file();
static HANDLE g_s_delay_timer;

void app_pa_on(uint32_t arg)
{
	luat_gpio_set(PA_PWR_PIN, 1);
}

void audio_event_cb(uint32_t event, void *param)
{
//	PadConfig_t pad_config;
//	GpioPinConfig_t gpio_config;

	LUAT_DEBUG_PRINT("%d", event);
	switch(event)
	{
	case LUAT_MULTIMEDIA_CB_AUDIO_DECODE_START:
		luat_gpio_set(CODEC_PWR_PIN, 1);
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
		luat_gpio_set(CODEC_PWR_PIN, 0);
		//如果用软件DAC，打开下面的2句注释，消除POP音和允许进低功耗
//		luat_rtos_task_sleep(10);
//		SoftDAC_Stop();
		break;
	}
}

void audio_data_cb(uint8_t *data, uint32_t len, uint8_t bits, uint8_t channels)
{
	//这里可以对音频数据进行软件音量缩放，或者直接清空来静音
	//软件音量缩放参考HAL_I2sSrcAdjustVolumn
	//LUAT_DEBUG_PRINT("%x,%d,%d,%d", data, len, bits, channels);
}



static void demo_task(void *arg)
{
	size_t total = 0, used = 0, max_used = 0;
//	luat_rtos_task_sleep(3000);
	ivCStrA sdk_id = AISOUND_SDK_USERID_16K;
	//8K用下面的
//	ivCStrA sdk_id = AISOUND_SDK_USERID_8K;
	//8K英文用下面的
//	ivCStrA sdk_id = AISOUND_SDK_USERID_8K_ENG;
	//16K英文用下面的
//	ivCStrA sdk_id = AISOUND_SDK_USERID_16K_ENG;
	//中文测试用下面的
	char tts_string[] = "支付宝到账123.45元,微信收款9876.12元ABC,支付宝到账123.45元,微信收款9876.12元ABC,支付宝到账123.45元,微信收款9876.12元ABC,支付宝到账123.45元,微信收款9876.12元ABC";
	//英文测试用下面的
//	char tts_string[] = "hello world, now test once";
	luat_audio_play_info_t info[5] = {0};

	download_file();
	luat_rtos_timer_create(&g_s_delay_timer);
    luat_audio_play_global_init(audio_event_cb, audio_data_cb, luat_audio_play_file_default_fun, luat_audio_play_tts_default_fun, NULL);
    //如果用模拟DAC通道，打开下面
//    luat_audio_play_set_bus_type(LUAT_AUDIO_BUS_SOFT_DAC);
    // 无压缩版本
    luat_audio_play_tts_set_resource(ivtts_16k, sdk_id, NULL);
    //8K用下面的
//	luat_audio_play_tts_set_resource(ivtts_8k, sdk_id, NULL);
    //8K英文用下面的
//	luat_audio_play_tts_set_resource(ivtts_8k_eng, sdk_id, NULL);
    //16K英文用下面的
//	luat_audio_play_tts_set_resource(ivtts_16k_eng, sdk_id, NULL);
    //使用ES7149/ES7148，用如下配置，如果不是，请根据实际情况配置，bus_id直接写0
 //   luat_i2s_base_setup(0, I2S_MODE_I2S, I2S_FRAME_SIZE_16_16);
//	如下配置可使用TM8211
    luat_i2s_base_setup(0, I2S_MODE_MSB, I2S_FRAME_SIZE_16_16);
	memset(info, 0, sizeof(info));
	info[0].path = "test1.mp3";
	info[1].path = "test2.mp3";
	info[2].path = "test3.mp3";
	info[3].path = "test4.mp3";
	luat_audio_play_multi_files(0, info, 4);
	luat_rtos_task_sleep(9000);
//	require_lowpower_state(0);
    while(1)
    {


    	info[0].path = NULL;
    	info[0].address = (uint32_t)amr_alipay_data;
    	info[0].rom_data_len = sizeof(amr_alipay_data);
    	info[1].path = NULL;
    	info[1].address = (uint32_t)amr_2_data;
    	info[1].rom_data_len = sizeof(amr_2_data);
    	info[2].path = NULL;
    	info[2].address = (uint32_t)amr_10_data;
    	info[2].rom_data_len = sizeof(amr_10_data);
    	info[3].path = NULL;
    	info[3].address = (uint32_t)amr_2_data;
    	info[3].rom_data_len = sizeof(amr_2_data);
    	info[4].path = NULL;
    	info[4].address = (uint32_t)amr_yuan_data;
    	info[4].rom_data_len = sizeof(amr_yuan_data);
    	luat_audio_play_multi_files(0, info, 5);
    	luat_rtos_task_sleep(9000);
    	luat_meminfo_sys(&total, &used, &max_used);
    	LUAT_DEBUG_PRINT("meminfo total %d, used %d, max_used%d",total, used, max_used);
    	luat_audio_play_tts_text(0, tts_string, sizeof(tts_string));
    	luat_rtos_task_sleep(35000);
    	luat_meminfo_sys(&total, &used, &max_used);
    	LUAT_DEBUG_PRINT("meminfo total %d, used %d, max_used%d",total, used, max_used);
    	memset(info, 0, sizeof(info));
    	info[0].path = "test1.mp3";
    	info[1].path = "test2.mp3";
    	info[2].path = "test3.mp3";
    	info[3].path = "test4.mp3";
    	luat_audio_play_multi_files(0, info, 4);
    	luat_rtos_task_sleep(9000);
    	luat_meminfo_sys(&total, &used, &max_used);
    	LUAT_DEBUG_PRINT("meminfo total %d, used %d, max_used%d",total, used, max_used);
//    	info[0].path = NULL;
//    	info[0].address = (uint32_t)Fqdqwer;
//    	info[0].rom_data_len = sizeof(Fqdqwer);
//    	audio_play_multi_files(0, info, 1);
//    	vTaskDelay(20000);
    }
}

static void test_audio_demo_init(void)
{
	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);
	luat_rtos_task_handle task_handle;

	luat_gpio_open(&gpio_cfg);
	gpio_cfg.pin = PA_PWR_PIN;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.pin = CODEC_PWR_PIN;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.alt_fun = CODEC_PWR_PIN_ALT_FUN;
	luat_gpio_open(&gpio_cfg);
	luat_rtos_task_create(&task_handle, 2048, 20, "test", demo_task, NULL, 0);
}

INIT_TASK_EXPORT(test_audio_demo_init, "1");
