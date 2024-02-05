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

#include"luat_audio.h"

#include "interf_enc.h"
#include "interf_dec.h"
//demo配合音频扩展板开发实现一边录音一边放音功能，无限循环，来模拟实现对讲功能，无软件降噪算法，MIC和喇叭尽量远离
//AIR780EP音频扩展板配置

#define CODEC_PWR_PIN HAL_GPIO_16
#define CODEC_PWR_PIN_ALT_FUN	4
#define PA_PWR_PIN HAL_GPIO_25
#define PA_PWR_PIN_ALT_FUN	0

#define PA_ON_LEVEL 1
#define PWR_ON_LEVEL 1

#define TEST_I2C_ID I2C_ID0
#define TEST_I2S_ID I2S_ID0

#define VOICE_VOL   65
#define MIC_VOL     75

#define RECORD_ONCE_LEN	20	   //单声道 8K录音单次20个编码块，总共400ms回调 320B 20ms，amr编码要求，20ms一个块
#define TEST_MAX_TIME	0		//单次测试时间，如果是0就是无限长，单位是录音回调次数
//#define ONLY_RECORD			//如果只想录音不想放音，可以打开这个宏

#define MULTIMEDIA_ID 0

static luat_audio_codec_conf_t luat_audio_codec = {
    .i2c_id = TEST_I2C_ID,
    .i2s_id = TEST_I2S_ID,
    .codec_opts = &codec_opts_es8311,
};

enum
{
	VOLTE_EVENT_PLAY_TONE = 1,
	VOLTE_EVENT_RECORD_VOICE_START,
	VOLTE_EVENT_RECORD_VOICE_UPLOAD,
	VOLTE_EVENT_PLAY_VOICE,
	VOLTE_EVENT_HANGUP,
};
static const int g_s_amr_nb_sizes[] = { 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };

static luat_rtos_task_handle g_s_task_handle;

__USER_FUNC_IN_RAM__ int record_cb(uint8_t id ,luat_i2s_event_t event, uint8_t *rx_data, uint32_t rx_len, void *param)
{
	LUAT_DEBUG_PRINT("record_cb event %d",event);
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

static void speech_demo_task(void *arg)
{
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG);

	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);

	gpio_cfg.pin = PA_PWR_PIN;
	luat_gpio_open(&gpio_cfg);

	gpio_cfg.pin = CODEC_PWR_PIN;
	gpio_cfg.alt_fun = CODEC_PWR_PIN_ALT_FUN;
	luat_gpio_open(&gpio_cfg);

	luat_i2c_setup(TEST_I2C_ID, 0);
	luat_i2c_set_polling_mode(TEST_I2C_ID, 1);

    luat_i2s_conf_t i2s_conf = {
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
	luat_i2s_setup(&i2s_conf);

	uint8_t *buff = luat_heap_opt_zalloc(LUAT_HEAP_AUTO, RECORD_ONCE_LEN * 320 * 4);
	uint8_t *amr_buff = luat_heap_opt_zalloc(LUAT_HEAP_AUTO, RECORD_ONCE_LEN * 320);
	luat_event_t event;
	size_t total, alloc, peak;
	uint64_t start_tick, end_tick;
	uint32_t i, out_len, done_len, run_cnt;
	volatile uint32_t cur_play_buf, cur_decode_buf, next_decode_buf;
	uint8_t *org_data, *pcm_data;
	uint8_t rx_buf[2];
	HANDLE amr_encoder_handler = Encoder_Interface_init(0);
	HANDLE amr_decoder_handler = Decoder_Interface_init();

	luat_audio_set_bus_type(MULTIMEDIA_ID,LUAT_MULTIMEDIA_AUDIO_BUS_I2S);	//设置音频总线类型
	luat_audio_setup_codec(MULTIMEDIA_ID, &luat_audio_codec);			//设置音频codec
	luat_audio_config_pa(MULTIMEDIA_ID, PA_PWR_PIN, PA_ON_LEVEL, 0, 0);//配置音频pa
	luat_audio_config_dac(MULTIMEDIA_ID, CODEC_PWR_PIN, PWR_ON_LEVEL, 0);//配置音频dac_power

	int ret = luat_audio_init_codec(MULTIMEDIA_ID, VOICE_VOL, MIC_VOL);
    if (ret){
		while (1){
			luat_rtos_task_sleep(500000);
		}
    }else{
		//1秒最高音质的AMRNB编码是1600
		luat_i2s_modify(TEST_I2S_ID, LUAT_I2S_CHANNEL_RIGHT, LUAT_I2S_BITS_16, 8000);
		luat_i2s_transfer_loop(TEST_I2S_ID, buff, 320 * RECORD_ONCE_LEN, 4, 0);
		luat_audio_pm_request(MULTIMEDIA_ID,LUAT_AUDIO_PM_RESUME);			//工作模式
		
		cur_play_buf = 0;
		run_cnt = 0;
		while(1)
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
#ifdef ONLY_RECORD

#else
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
#endif
				end_tick = luat_mcu_tick64_ms();
				LUAT_DEBUG_PRINT("take time %ums %d,%d,%d", (uint32_t)(end_tick - start_tick), cur_play_buf, cur_decode_buf, next_decode_buf);
				luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &total, &alloc, &peak);
				LUAT_DEBUG_PRINT("psram total %u, used %u, max used %u", total, alloc, peak);
				luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &total, &alloc, &peak);
				LUAT_DEBUG_PRINT("sram total %u, used %u, max used %u", total, alloc, peak);
				if (TEST_MAX_TIME)
				{
					if (run_cnt++ > TEST_MAX_TIME)
					{
						LUAT_DEBUG_PRINT("test stop");
						luat_i2s_close(TEST_I2S_ID);
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
						while(1)
						{
							luat_rtos_task_sleep(500000);
						}
					}
				}
			}
		}
	}
}

static void speech_demo_init(void)
{
	luat_rtos_task_create(&g_s_task_handle, 8 * 1024, 90, "speech", speech_demo_task, NULL, 0);
}

INIT_TASK_EXPORT(speech_demo_init, "1");
