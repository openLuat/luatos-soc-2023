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
#include "luat_audio.h"
#include "luat_multimedia.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_mem.h"
#include "luat_mcu.h"
#include "luat_audio.h"

//AIR780EP音频扩展板配置

//demo 配置带ES8311硬件的云喇叭开发板，编译时需要把build.bat里的CHIP_TARGET改成ec718pv
#define CODEC_PWR_PIN HAL_GPIO_16
#define CODEC_PWR_PIN_ALT_FUN	4
#define PA_PWR_PIN HAL_GPIO_25
#define PA_PWR_PIN_ALT_FUN	0

#define PA_ON_LEVEL 1
#define PWR_ON_LEVEL 1

//#define SPEECH_RECORD_ENABLE	//通话录音，由于上行语音都是用户自己采集的，这里演示如果采集下行的语音

#define I2C_ID	0
#define I2S_ID	0

#define VOICE_VOL   70
#define MIC_VOL     65

#define MULTIMEDIA_ID 0

static luat_audio_codec_conf_t luat_audio_codec = {
    .i2c_id = I2C_ID,
    .i2s_id = I2S_ID,
    .codec_opts = &codec_opts_es8311,
};

#define NO_ANSWER_AUTO_HANGUP_TIME	30000	//无接听自动挂断时间，单位ms

extern uint8_t callAlertRing16k[];
extern uint8_t tone450_8k[];
extern uint8_t tone950_8k[];

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
//通话录音相关控制
#ifdef SPEECH_RECORD_ENABLE
static HANDLE g_s_record_timer;
static volatile uint32_t g_s_next_dl_point;
static uint32_t g_s_download_data[480];		//最大消耗1920字节，开启模拟录音功能后，将由这个buffer代替原先播放的buffer，只要声音连续不断，音质正常，说明录音功能正常
static uint8_t *g_s_org_buffer;
static uint8_t g_s_dl_index;
static luat_rtos_queue_t g_s_record_queue;
#endif
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
		if (g_s_codec_is_on)
		{
			g_s_codec_is_on = 0;
            luat_audio_codec.codec_opts->stop(&luat_audio_codec);
            // luat_audio_codec.codec_opts->control(&luat_audio_codec,LUAT_CODEC_MODE_STANDBY,LUAT_CODEC_MODE_ALL);
		}
		g_s_i2s_conf->is_full_duplex = 0;
        luat_rtos_timer_stop(g_s_delay_timer);
#ifdef SPEECH_RECORD_ENABLE
        luat_rtos_timer_stop(g_s_record_timer);
#endif
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
	luat_audio_codec.codec_opts->control(&luat_audio_codec,LUAT_CODEC_MODE_NORMAL,LUAT_CODEC_MODE_ALL);
    // luat_audio_codec.codec_opts->start(&luat_audio_codec);
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
#ifdef SPEECH_RECORD_ENABLE
	memset(g_s_download_data, 0, sizeof(g_s_download_data));
	g_s_org_buffer = input;
	g_s_next_dl_point = 1;	//第一个20ms里已经有数据了
	if (1 == sample_rate)
	{
		g_s_dl_index = 6;
	}
	else
	{
		g_s_dl_index = 3;
	}
	memcpy(g_s_download_data + sample_rate * 320, g_s_org_buffer,  sample_rate * 320); //由于是模拟录音，为了播放流畅，需要往后挪一个20ms，如果是真实录音，不需要+ sample_rate * 320
	luat_start_rtos_timer(g_s_record_timer, 20, 1);
#endif
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
#ifdef SPEECH_RECORD_ENABLE
static __USER_FUNC_IN_RAM__ LUAT_RT_RET_TYPE download_data_callback(LUAT_RT_CB_PARAM)
{
	uint8_t *point = (uint8_t *)g_s_download_data;
	volatile uint32_t play_cnt = 0;
	if (g_s_record_type && g_s_play_type)
	{
		play_cnt = (g_s_next_dl_point + 1) % g_s_dl_index; //由于是模拟录音，为了播放流畅，需要往后挪一个20ms，如果是真实录音，不需要+ 1
		memcpy(point + play_cnt * g_s_record_type * 320, g_s_org_buffer + g_s_next_dl_point * g_s_record_type * 320, g_s_record_type * 320);//20ms录音完成
		g_s_next_dl_point = (g_s_next_dl_point + 1) % g_s_dl_index;
	}
}

#endif
static void volte_task(void *param)
{
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG);
	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);

	gpio_cfg.output_level = PA_ON_LEVEL;
	// pa power ctrl init
	gpio_cfg.pin = PA_PWR_PIN;
	gpio_cfg.alt_fun = PA_PWR_PIN_ALT_FUN;
	luat_gpio_open(&gpio_cfg);

	// codec power ctrl init
	gpio_cfg.pin = CODEC_PWR_PIN;
	gpio_cfg.alt_fun = CODEC_PWR_PIN_ALT_FUN;
	luat_gpio_open(&gpio_cfg);

	luat_i2c_setup(I2C_ID, 0);
	luat_i2c_set_polling_mode(I2C_ID, 1);

    luat_i2s_conf_t i2s_conf = {
        .id = I2S_ID,
        .mode = LUAT_I2S_MODE_MASTER,
        .channel_format = LUAT_I2S_CHANNEL_RIGHT,
        .standard = LUAT_I2S_MODE_LSB,
        .channel_bits = LUAT_I2S_BITS_16,
        .data_bits = LUAT_I2S_BITS_16,
        .luat_i2s_event_callback = record_cb,
    };

	luat_i2s_setup(&i2s_conf);
    g_s_i2s_conf = luat_i2s_get_config(I2S_ID);

	luat_rtos_timer_create(&g_s_delay_timer);

	size_t total, alloc, peak;
	luat_event_t event;

	luat_audio_set_bus_type(MULTIMEDIA_ID,MULTIMEDIA_AUDIO_BUS_I2S);	//设置音频总线类型
	luat_audio_setup_codec(MULTIMEDIA_ID, &luat_audio_codec);			//设置音频codec
	luat_audio_config_pa(MULTIMEDIA_ID, PA_PWR_PIN, PA_ON_LEVEL, 0, 0);//配置音频pa
	luat_audio_config_dac(MULTIMEDIA_ID, CODEC_PWR_PIN, PWR_ON_LEVEL, 0);//配置音频dac_power


    int ret = luat_audio_init_codec(MULTIMEDIA_ID, VOICE_VOL, MIC_VOL);
    if (ret){
		while (1){
			luat_rtos_event_recv(g_s_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
		}
    }else{
        // luat_audio_codec.codec_opts->control(&luat_audio_codec,LUAT_CODEC_MODE_STANDBY,LUAT_CODEC_MODE_ALL);
        luat_audio_codec.codec_opts->stop(&luat_audio_codec);
    }
    
    LUAT_DEBUG_PRINT("es8311 init ret :%d",ret);

	luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
	LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
	luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
	LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
#ifdef SPEECH_RECORD_ENABLE
	g_s_record_timer = luat_create_rtos_timer(download_data_callback, NULL, NULL);
#endif
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
			luat_rtos_task_sleep(1);
			g_s_i2s_conf->is_full_duplex = 1;
			g_s_i2s_conf->cb_rx_len = 320 * g_s_record_type;
			luat_i2s_modify(I2S_ID, LUAT_I2S_CHANNEL_RIGHT, LUAT_I2S_BITS_16, g_s_record_type * 8000);
			luat_i2s_transfer_loop(I2S_ID, NULL, 3200, 2, 0);	//address传入空地址就是播放空白音
            // luat_audio_codec.codec_opts->start(&luat_audio_codec);
			luat_audio_codec.codec_opts->control(&luat_audio_codec,LUAT_CODEC_MODE_NORMAL,LUAT_CODEC_MODE_ALL);
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
                    // luat_audio_codec.codec_opts->start(&luat_audio_codec);
					luat_audio_codec.codec_opts->control(&luat_audio_codec,LUAT_CODEC_MODE_NORMAL,LUAT_CODEC_MODE_ALL);
				}

			}
			else
			{
				LUAT_DEBUG_PRINT("%d,%d,%d", g_s_play_type, g_s_record_type, event.param2);
#ifdef SPEECH_RECORD_ENABLE
				//模拟录音时，下行数据播放buffer改成g_s_download_data，只要播放音质正常，就说明真实录音时，音质也正常
				if (2 == g_s_play_type)
				{
					luat_i2s_transfer_loop(I2S_ID, (uint8_t *)g_s_download_data, event.param2/3, 3, 0);
				}
				else
				{
					luat_i2s_transfer_loop(I2S_ID, (uint8_t *)g_s_download_data, event.param2/6, 6, 0);
				}
#else
				if (2 == g_s_record_type)
				{
					luat_i2s_transfer_loop(I2S_ID, (uint8_t *)event.param1, event.param2/3, 3, 0);
				}
				else
				{
					luat_i2s_transfer_loop(I2S_ID, (uint8_t *)event.param1, event.param2/6, 6, 0);
				}
#endif
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
    luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG);
	luat_mobile_event_register_handler(mobile_event_cb);
	luat_mobile_speech_init(0,mobile_voice_data_input);
	luat_rtos_task_create(&g_s_task_handle, 4*1024, 100, "volte", volte_task, NULL, 64);
}

INIT_TASK_EXPORT(task_demo_init, "1");

