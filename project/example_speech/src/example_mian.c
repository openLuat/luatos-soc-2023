#include "common_api.h"
#include "luat_gpio.h"
#include "luat_mobile.h"
#include "luat_i2c.h"
#include "luat_i2s.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_mem.h"
#include "luat_mcu.h"

#include "interf_enc.h"
#include "interf_dec.h"
//demo配合云喇叭开发实现一边录音一边放音功能，无限循环，来模拟实现对讲功能，无软件降噪算法，MIC和喇叭尽量远离
//AIR780EP音频开发板配置

#define CODEC_PWR_PIN HAL_GPIO_16
#define CODEC_PWR_PIN_ALT_FUN	4
#define PA_PWR_PIN HAL_GPIO_25
#define PA_PWR_PIN_ALT_FUN	0

#define TEST_I2C_ID I2C_ID1
#define TEST_I2S_ID I2S_ID0
#define ES8311_I2C_ADDR	0x18
#define RECORD_ONCE_LEN	20	   //单声道 8K录音单次20个编码块，总共400ms回调 320B 20ms，amr编码要求，20ms一个块
#define TEST_MAX_TIME	0		//单次测试时间，如果是0就是无限长，单位是录音回调次数
//#define ONLY_RECORD			//如果只想录音不想放音，可以打开这个宏

typedef struct
{
	uint8_t reg;
	uint8_t value;
}i2c_reg_t;

static const i2c_reg_t es8311_reg_table[] =
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
	{0x14,(0<<6) + (1<<4) + 7},//选择CH1输入+21DB GAIN	(0x14,(Dmic_Selon<<6) + (ADCChannelSel<<4) + ADC_PGA_GAIN);
	{0x12,0x28},
	{0x13,0x00 + (0<<4)},	//(0x13,0x00 + (DACHPModeOn<<4));
	{0x0E,0x02},
	{0x0F,0x44},
	{0x15,0x00},
	{0x1B,0x0A},
	{0x1C,0x6A},
	{0x37,0x48},
	{0x44,(0 << 7)},	//(0x44,(ADC2DAC_Sel <<7));
//	{0x17,0xd2},//(0x17,ADC_Volume);
//	{0x32,0xc8},//(0x32,DAC_Volume);
	{0x17,0xc2},//(0x17,ADC_Volume);
	{0x32,0xc8},//(0x32,DAC_Volume);
};
#if 0
static const i2c_reg_t es8218_reg_table[] =
	{
		{0x00, 0x00},
		// Ratio=MCLK/LRCK=256：12M288-48K；4M096-16K; 2M048-8K
		{0x01, 0x2F + (0 << 7)}, //(0x01,0x2F + (MSMode_MasterSelOn<<7))
		{0x02, 0x01},
		{0x03, 0x20},
		{0x04, 0x01},		  // LRCKDIV
		{0x05, 0x00},		  // LRCKDIV=256
		{0x06, 4 + (0 << 5)}, //(0x06,SCLK_DIV + (SCLK_INV<<5))
		{0x10, 0x18 + 0},	  //(0x10,0x18 + Dmic_Selon)

		{0x07, 0 + (3 << 2)}, //(0X07,NORMAL_I2S + (Format_Len<<2));//IIS 16BIT
		{0x09, 0x00},
		{0x0A, 0x22 + (0xCC * 0)}, //(0x0A,0x22 + (0xCC*VDDA_VOLTAGE)) 0 = 3.3V 1 = 1.8V
		{0x0B, 0x02 - 0},		   //(0x0B,0x02 - VDDA_VOLTAGE)0 = 3.3V 1 = 1.8V
		{0x14, 0xA0},
		{0x0D, 0x30},
		{0x0E, 0x20},
		{0x23, 0x00},
		{0x24, 0x00},
		{0x18, 0x04},
		{0x19, 0x04},
		{0x0F, (0 << 5) + (1 << 4) + 7}, //(0x0F,(ADCChannelSel<<5) + (ADC_PGA_DF2SE_18DB<<4) + ADC_PGA_GAIN);
		{0x08, 0x00},
		{0x00, 0x80},
		{0x12, 0x1C}, // ALC OFF
		{0x11, 0},	  // ADC_Volume
};
static const i2c_reg_t es7243e_reg_table[] =
	{
		{0x01, 0x3A},
		{0x00, 0x80},
		{0xF9, 0x00},
		{0x04, 0x02},
		{0x04, 0x01},
		{0xF9, 0x01},
		{0x00, 0x1E},
		{0x01, 0x00},

		// radio 256
		{0x03, 0x20},
		{0x04, 0x01},
		{0x0D, 0},
		{0x05, 0x00},
		{0x06, 4 - 1},
		{0x07, 0x00},
		{0x08, 0xFF},
		

		{0x02, (0x00 << 7) + 0},
		{0x09, 0xCA},
		{0x0A, 0x85},
		{0x0B, 0xC0 + 0x00 + (0x03 << 2)},
		{0x0E, 191},
		{0x10, 0x38},
		{0x11, 0x16},
		{0x14, 0x0C},
		{0x15, 0x0C},
		{0x17, 0x02},
		{0x18, 0x26},
		{0x0F, 0x80},
		{0x19, 0x77},
		{0x1F, 0x08 + (0 << 5) - 0x00},
		{0x1A, 0xF4},
		{0x1B, 0x66},
		{0x1C, 0x44},
		{0x1E, 0x00},
		{0x20, 0x10 + 14},
		{0x21, 0x10 + 14},
		{0x00, 0x80 + (0 << 6)},
		{0x01, 0x3A},
		{0x16, 0x3F},
		{0x16, 0x00},
		{0x0B, 0x00 + (0x03 << 2)},
};
#endif

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

	luat_i2c_setup(TEST_I2C_ID, 0);

	buff[0] = 0xfd;
    luat_i2c_send(TEST_I2C_ID, 0x18, buff, 1, 1);
    luat_i2c_recv(TEST_I2C_ID, 0x18, rx_buf, 1);

    buff[0] = 0xfe;
    luat_i2c_send(TEST_I2C_ID, 0x18, buff, 1, 1);
    luat_i2c_recv(TEST_I2C_ID, 0x18, rx_buf + 1, 1);
    //1秒最高音质的AMRNB编码是1600
    while(1)
    {
		if (0x83 == rx_buf[0] && 0x11 == rx_buf[1])
		{
			LUAT_DEBUG_PRINT("find es8311");
			//如果有休眠操作，且控制codec的电源的IO不是AONGPIO，又没有外部上拉保持IO电平，则在唤醒时必须重新初始化codec
			for(i = 0; i < sizeof(es8311_reg_table)/sizeof(i2c_reg_t);i++)
			{
				luat_i2c_send(TEST_I2C_ID, ES8311_I2C_ADDR, (uint8_t *)&es8311_reg_table[i], 2, 1);
			}
			luat_i2s_modify(TEST_I2S_ID, LUAT_I2S_CHANNEL_RIGHT, LUAT_I2S_BITS_16, 8000);
			luat_i2s_transfer_loop(TEST_I2S_ID, buff, 320 * RECORD_ONCE_LEN, 4, 0);
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
		else
		{
			LUAT_DEBUG_PRINT("not find es8311");
			while(1)
			{
				luat_rtos_task_sleep(500000);
			}
		}
    }
}

static void speech_demo_init(void)
{
	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);


	gpio_cfg.pin = PA_PWR_PIN;
	luat_gpio_open(&gpio_cfg);

	gpio_cfg.pin = CODEC_PWR_PIN;
	gpio_cfg.alt_fun = CODEC_PWR_PIN_ALT_FUN;
	luat_gpio_open(&gpio_cfg);
	luat_gpio_set(CODEC_PWR_PIN, 1);
	luat_gpio_set(PA_PWR_PIN, 1);
	luat_i2s_conf_t conf = {0};
	conf.id = TEST_I2S_ID;
	conf.mode = LUAT_I2S_MODE_MASTER;
	conf.channel_format = LUAT_I2S_CHANNEL_RIGHT;
	conf.standard = LUAT_I2S_MODE_LSB;
	conf.channel_bits = LUAT_I2S_BITS_16;
	conf.data_bits = LUAT_I2S_BITS_16;
	conf.is_full_duplex = 1;
	conf.cb_rx_len = 320 * RECORD_ONCE_LEN;
	conf.luat_i2s_event_callback = record_cb;
	luat_i2s_setup(&conf);

	luat_rtos_task_create(&g_s_task_handle, 8 * 1024, 90, "speech", speech_demo_task, NULL, 0);
}

INIT_TASK_EXPORT(speech_demo_init, "1");
