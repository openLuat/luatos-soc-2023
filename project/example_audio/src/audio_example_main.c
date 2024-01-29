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
#include "luat_mobile.h"
#include "luat_mcu.h"

#include "interf_enc.h"
#include "interf_dec.h"

//AIR780P音频开发板配置，如果用的ES8311而且要低功耗的，不建议用LDO_CTL，换成AGPIO，不换AGPIO的话，需要看休眠演示，在唤醒后重新初始化codec
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
#define TEST_USE_ES8311	1
#define TEST_USE_TM8211 0
#if defined FEATURE_IMS_ENABLE	//VOLTE固件才支持通话测试
//#define CALL_TEST		//通话测试会关闭掉其他测试，防止冲突
#endif

#define RECORD_ONCE_LEN	10	   //单声道 8K录音单次10个编码块，总共200ms回调 320B 20ms，amr编码要求，20ms一个块
#define RECORD_TIME	(5)	//设置5秒录音，只要ram够，当然可以更长
#define TEST_MAX_TIME	100		//单次测试时间，如果是0就是无限长，单位是录音回调次数

int record_cb(uint8_t id ,luat_i2s_event_t event, uint8_t *rx_data, uint32_t rx_len, void *param);

static HANDLE g_s_amr_encoder_handler;
static uint32_t g_s_record_time;
static Buffer_Struct g_s_amr_rom_file;
static uint8_t g_s_test_only_record = 0;

static const luat_audio_codec_conf_t luat_audio_codec_es8311 = {
    .i2c_id = TEST_I2C_ID,
    .i2s_id = TEST_I2S_ID,
    .pa_pin = PA_PWR_PIN,
    .pa_on_level = 1,
	.power_pin = CODEC_PWR_PIN,
	.power_on_level = 1,
	.power_on_delay_ms = 10,
	.pa_delay_time = 200,
	.after_sleep_ready_time = 600,
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
	VOLTE_EVENT_CALL_READY,
	AUDIO_EVENT_PLAY_DONE,
};
static const int g_s_amr_nb_sizes[] = { 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };

static luat_rtos_task_handle g_s_task_handle;

static void record_encode_amr(uint8_t *data, uint32_t len)
{
	uint8_t outbuf[64];
	int16_t *pcm = (int16_t *)data;
	uint32_t total_len = len >> 1;
	uint32_t done_len = 0;
	int out_len;
	while ((total_len - done_len) >= 160)
	{
		out_len = Encoder_Interface_Encode(g_s_amr_encoder_handler, MR122, &pcm[done_len], outbuf, 0);

		if (out_len <= 0)
		{
			LUAT_DEBUG_PRINT("encode error in %d,result %d", done_len, out_len);
		}
		else
		{
			OS_BufferWrite(&g_s_amr_rom_file, outbuf, out_len);
		}
		done_len += 160;
	}
}

static void record_stop_encode_amr(uint8_t *data, uint32_t len)
{
	luat_audio_record_stop(0);
	luat_audio_standby(0);
	Encoder_Interface_exit(g_s_amr_encoder_handler);
	g_s_amr_encoder_handler = NULL;
	LUAT_DEBUG_PRINT("amr encode stop");
}

__USER_FUNC_IN_RAM__ int record_cb(uint8_t id ,luat_i2s_event_t event, uint8_t *rx_data, uint32_t rx_len, void *param)
{
	switch(event)
	{
	case LUAT_I2S_EVENT_RX_DONE:
		if (g_s_test_only_record)
		{
			soc_call_function_in_audio(record_encode_amr, (uint32_t)rx_data, rx_len, LUAT_WAIT_FOREVER);
			g_s_record_time++;
			if (g_s_record_time >= (RECORD_TIME * 5))	//15秒
			{
				soc_call_function_in_audio(record_stop_encode_amr, 0, 0, LUAT_WAIT_FOREVER);
			}
		}
		else
		{
			luat_rtos_event_send(g_s_task_handle, VOLTE_EVENT_RECORD_VOICE_UPLOAD, (uint32_t)rx_data, rx_len, 0, 0);
		}

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
	case LUAT_MULTIMEDIA_CB_DECODE_DONE:
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
	uint8_t *buff;
	uint8_t *amr_buff;
	luat_event_t event;
	size_t total, alloc, peak;
	uint64_t start_tick, end_tick;
	uint32_t i, out_len, done_len, run_cnt, speech_test;
	volatile uint32_t cur_play_buf, cur_decode_buf, next_decode_buf;
	uint8_t *org_data, *pcm_data;
	uint8_t rx_buf[2];

	HANDLE amr_encoder_handler;
	HANDLE amr_decoder_handler;

	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG);
//如果不需要播放文件/TTS，例如只需要录音（amr编码放到自己的task里），或者全双工对讲，可以不需要开启audio_play任务，初始化的调用如下代码，可以节省13KB ram
//	luat_audio_play_global_init_with_task_priority(NULL, NULL, NULL, NULL, NULL, 50)
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
		luat_audio_init_codec(0, 70, 75);
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

#ifdef CALL_TEST
	uint8_t is_call_uplink_on = 0;
#else
	luat_audio_play_info_t mp3_info[4] = {0};
	luat_audio_play_info_t amr_info[5] = {0};
	download_file();
#endif
#ifdef CALL_TEST
	//luat_audio_sleep(0, 1);
    luat_rtos_event_recv(g_s_task_handle, VOLTE_EVENT_CALL_READY, &event, NULL, LUAT_WAIT_FOREVER);
    // luat_mobile_make_call(0, "xxxxxxxxxxx", 11);
#endif

    while(1)
    {
#ifdef CALL_TEST
		//带ES8311+780EPV的演示VOLTE通话
    	luat_rtos_event_recv(g_s_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
		if (TEST_USE_ES8311)
		{
			switch(event.id)
			{
			case VOLTE_EVENT_PLAY_TONE:
				if (!event.param1)
				{
					is_call_uplink_on = 0;
					luat_audio_speech_stop(0);
					//如果后续还要播放其他音频，或者PA无法控制的，用luat_audio_standby();
					//luat_audio_sleep(0, 1);
				}
				luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
				LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
				luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
				LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
				break;
			case VOLTE_EVENT_RECORD_VOICE_START:
				is_call_uplink_on = 1;
				luat_audio_speech(0, 0, event.param1, NULL, 0, 1);
				break;
			case VOLTE_EVENT_RECORD_VOICE_UPLOAD:
				if (is_call_uplink_on)
				{
					luat_mobile_speech_upload((uint8_t *)event.param1, event.param2);
				}
				break;
			case VOLTE_EVENT_PLAY_VOICE:
				luat_audio_speech(0, 1, event.param3, (uint8_t *)event.param1, event.param2, 1);
				break;
			case VOLTE_EVENT_HANGUP:
				luat_mobile_hangup_call(0);
				break;
			}
		}
#else

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

		//带ES8311的演示录音后再放音，录音15秒
		if (TEST_USE_ES8311)
		{
			LUAT_DEBUG_PRINT("record test start");
			g_s_amr_encoder_handler = Encoder_Interface_init(0);
			g_s_record_time = 0;
			g_s_amr_rom_file.Pos = 0;
			OS_BufferWrite(&g_s_amr_rom_file, "#!AMR\n", 6);
			g_s_test_only_record = 1;
			luat_audio_record_and_play(0, 8000, NULL, 3200, 2); //放音buffer填NULL，就是喇叭静音
			luat_rtos_task_sleep((RECORD_TIME + 1) * 1000);
			g_s_test_only_record = 0;
			amr_info[0].address = (uint32_t)g_s_amr_rom_file.Data;
			amr_info[0].rom_data_len = g_s_amr_rom_file.Pos;
			luat_audio_play_multi_files(0, amr_info, 1);
			luat_rtos_event_recv(g_s_task_handle, AUDIO_EVENT_PLAY_DONE, &event, NULL, LUAT_WAIT_FOREVER);
			luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
			LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
			luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
			LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
			luat_rtos_task_sleep(1000);
		}
		//带ES8311的演示双向对讲，默认演示20秒
		if (TEST_USE_ES8311)
		{
			amr_encoder_handler = Encoder_Interface_init(0);
			amr_decoder_handler = Decoder_Interface_init();
			cur_play_buf = 0;
			cur_decode_buf = 0;
			next_decode_buf = 0;
			buff = luat_heap_opt_zalloc(LUAT_HEAP_AUTO, RECORD_ONCE_LEN * 320 * 4);
			amr_buff = luat_heap_opt_zalloc(LUAT_HEAP_AUTO, RECORD_ONCE_LEN * 320);
			g_s_test_only_record = 0;
			run_cnt = 0;
			speech_test = 1;
			luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
			LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
			luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
			LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
			luat_audio_record_and_play(0, 8000, buff, 320 * RECORD_ONCE_LEN, 4);
			while (speech_test)
			{
				luat_rtos_event_recv(g_s_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
				if (event.id == VOLTE_EVENT_RECORD_VOICE_UPLOAD)
				{
					org_data = (uint8_t *)event.param1;
					start_tick = luat_mcu_tick64_ms();
					done_len = 0;
					cur_play_buf = (cur_play_buf + 1) & 3;
					for(i = 0; i < RECORD_ONCE_LEN; i++)
					{
						out_len = Encoder_Interface_Encode(amr_encoder_handler, MR122, (uint16_t *)&org_data[i * 320], &amr_buff[done_len], 0);
						done_len += out_len;
					}

					cur_decode_buf = (cur_play_buf + 1) & 3;
					pcm_data = &buff[cur_decode_buf * 320 * RECORD_ONCE_LEN];
					done_len = 0;
					for(i = 0; i < RECORD_ONCE_LEN; i++)
					{
						out_len = g_s_amr_nb_sizes[(amr_buff[done_len] >> 3) & 0x0f];
						Decoder_Interface_Decode(amr_decoder_handler, &amr_buff[done_len], (uint16_t *)&pcm_data[320 * i], 0);
						done_len += 1+out_len;
					}
					next_decode_buf = (cur_decode_buf + 1) & 3;
					memset(&buff[next_decode_buf * 320 * RECORD_ONCE_LEN], 0, 320 * RECORD_ONCE_LEN);

					end_tick = luat_mcu_tick64_ms();
					LUAT_DEBUG_PRINT("take time %ums %d,%d,%d", (uint32_t)(end_tick - start_tick), cur_play_buf, cur_decode_buf, next_decode_buf);
					if (!TEST_MAX_TIME)
					{
						luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
						LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
						luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
						LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
					}
					if (TEST_MAX_TIME)
					{
						if (run_cnt++ > TEST_MAX_TIME)
						{
							speech_test = 0;
							LUAT_DEBUG_PRINT("test stop");
							luat_audio_record_stop(0);
							luat_audio_standby(0);
							//如果追求极致的功耗，用luat_audio_sleep代替luat_audio_standby
							//luat_audio_sleep(0, 1);
							luat_heap_opt_free(LUAT_HEAP_AUTO, buff);
							buff = NULL;
							luat_heap_opt_free(LUAT_HEAP_AUTO, amr_buff);
							amr_buff = NULL;
							Encoder_Interface_exit(amr_encoder_handler);
							amr_encoder_handler = NULL;
							Decoder_Interface_exit(amr_decoder_handler);
							amr_decoder_handler = NULL;
							luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
							LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
							luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
							LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
						}
					}
				}
			}

		}


		//演示一下休眠
		luat_audio_sleep(0, 1);
		luat_pm_power_ctrl(LUAT_PM_POWER_USB, 0);	//没接USB的，只需要在开始的时候关闭一次USB就行了
		luat_pm_request(LUAT_PM_SLEEP_MODE_LIGHT);
		luat_rtos_task_sleep(10000);
		luat_pm_request(LUAT_PM_SLEEP_MODE_IDLE);
		luat_pm_power_ctrl(LUAT_PM_POWER_USB, 1);
		if (TEST_USE_ES8311)
		{
			luat_audio_init_codec(0, 70, 75);	//如果没有AGPIO来控制，需要重新初始化ES8311，如果用AGPIO来控制的，就不需要重新初始化
			luat_audio_sleep(0, 0);
		}
#endif
    }
}
extern void audio_play_set_ram_type(LUAT_HEAP_TYPE_E Type);
//自动接听相关
static uint8_t g_s_ring_cnt;

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
			luat_rtos_event_send(g_s_task_handle, VOLTE_EVENT_CALL_READY, 0, 0, 0, 0);
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
			if (!index)	//不对本地铃声做控制，只对关闭通话做控制
			{
				luat_rtos_event_send(g_s_task_handle, VOLTE_EVENT_PLAY_TONE, index, 0, 0, 0);
			}

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

static void test_audio_demo_init(void)
{
	luat_mobile_event_register_handler(mobile_event_cb);
	luat_mobile_speech_init(0,mobile_voice_data_input);
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
	luat_rtos_task_create(&g_s_task_handle, 8192, 100, "test", demo_task, NULL, 0);
//	audio_play_set_ram_type(LUAT_HEAP_SRAM);		//打开后消耗RAM较多的地方将使用SRAM，否则使用AUTO模式
	#endif

}

INIT_TASK_EXPORT(test_audio_demo_init, "1");
