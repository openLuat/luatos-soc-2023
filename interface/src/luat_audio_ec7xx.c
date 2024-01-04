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

#ifdef __LUATOS__
#include "bsp_common.h"
#include "luat_audio.h"
#include "luat_i2s.h"
#include "common_api.h"
#include "audio_play.h"
#include "driver_gpio.h"
#include "luat_msgbus.h"
#include "driver_gpio.h"
#include "soc_spi.h"
#include "ivTTSSDKID_all.h"
#include "ivTTS.h"
#include "luat_spi.h"
#include "sfud.h"
#include "luat_rtos.h"
#include "luat_gpio.h"
#include "driver_pcm.h"
#include "luat_fs.h"

typedef struct
{
	uint32_t dac_delay_len;
	uint32_t pa_delay_time;
	uint32_t dac_off_delay_time;
	uint32_t record_sample_rate[I2S_MAX];
	HANDLE pa_delay_timer;
	int pa_pin;
	int dac_pin;
	uint16_t vol;
	uint8_t pa_on_level;
	uint8_t dac_on_level;
	uint8_t raw_mode;
	uint8_t debug_on_off;
	uint8_t soft_dac_mode;
	uint8_t record_channel;
}luat_audio_hardware_t;

static luat_audio_hardware_t g_s_audio_hardware;
extern const unsigned char ivtts_16k[];
extern const unsigned char ivtts_8k[];

extern int l_multimedia_raw_handler(lua_State *L, void* ptr);
//extern void audio_play_file_default_fun(void *param);
//extern void audio_play_TTS_default_fun(void *param);
//extern void audio_play_tts_set_resource_ex(void *address, void *sdk_id, void *read_resource_fun);
//extern void audio_play_global_init_ex(audio_play_event_cb_fun_t event_cb, audio_play_data_cb_fun_t data_cb, audio_play_default_fun_t play_file_fun, audio_play_default_fun_t play_tts_fun, void *user_param);
//extern int audio_play_write_blank_raw_ex(uint8_t multimedia_id, uint8_t cnt, uint8_t add_font);

static void app_pa_on(uint32_t arg)
{
	luat_gpio_set(g_s_audio_hardware.pa_pin, g_s_audio_hardware.pa_on_level);
}

static void audio_event_cb(uint32_t event, void *param)
{
	if (g_s_audio_hardware.debug_on_off)
	{
		DBG("%d", event);
	}
	rtos_msg_t msg = {0};
	switch(event)
	{
	case MULTIMEDIA_CB_AUDIO_DECODE_START:
		luat_gpio_set(g_s_audio_hardware.dac_pin, g_s_audio_hardware.dac_on_level);
		audio_play_write_blank_raw_ex(0, g_s_audio_hardware.dac_delay_len, 1);
		break;
	case MULTIMEDIA_CB_AUDIO_OUTPUT_START:
		luat_rtos_timer_start(g_s_audio_hardware.pa_delay_timer, g_s_audio_hardware.pa_delay_time, 0, app_pa_on, NULL);
		break;
	case MULTIMEDIA_CB_AUDIO_NEED_DATA:
		if (g_s_audio_hardware.raw_mode)
		{
			msg.handler = l_multimedia_raw_handler;
			msg.arg1 = MULTIMEDIA_CB_AUDIO_NEED_DATA;
			msg.arg2 = (int)param;
			luat_msgbus_put(&msg, 1);
		}
		break;
	case MULTIMEDIA_CB_TTS_INIT:
		break;
	case MULTIMEDIA_CB_TTS_DONE:
		if (!audio_play_get_last_error(0))
		{
			audio_play_write_blank_raw_ex(0, 1, 0);
		}
		break;
	case MULTIMEDIA_CB_AUDIO_DONE:
		luat_rtos_timer_stop(g_s_audio_hardware.pa_delay_timer);
//		luat_audio_play_get_last_error(0);
		luat_gpio_set(g_s_audio_hardware.pa_pin, !g_s_audio_hardware.pa_on_level);
		if (g_s_audio_hardware.dac_off_delay_time)
		{
			luat_rtos_task_sleep(g_s_audio_hardware.dac_off_delay_time);
		}
		luat_gpio_set(g_s_audio_hardware.dac_pin, !g_s_audio_hardware.dac_on_level);
		if (g_s_audio_hardware.soft_dac_mode)
		{
			// SoftDAC_Stop();
		}
		msg.handler = l_multimedia_raw_handler;
		msg.arg1 = MULTIMEDIA_CB_AUDIO_DONE;
		msg.arg2 = (int)param;
		luat_msgbus_put(&msg, 1);
		break;
	}
}

static void audio_data_cb(uint8_t *data, uint32_t len, uint8_t bits, uint8_t channels)
{
	//这里可以对音频数据进行软件音量缩放，或者直接清空来静音
	if (g_s_audio_hardware.vol != 100)
	{
		int16_t val = g_s_audio_hardware.vol;
		int16_t *i16 = (int16_t *)data;
		uint32_t i = 0;
		uint32_t pos = 0;
		int32_t temp;
		while(pos < len)
		{
			temp = i16[i];
			temp = temp * val / 100;
			if (temp > 32767)
			{
				temp = 32767;
			}
			else if (temp < -32768)
			{
				temp = -32768;
			}
			i16[i] = temp;
			i++;
			pos += 2;
		}
	}
}
#ifdef LUAT_USE_TTS
#ifdef LUAT_USE_TTS_ONCHIP
static ivBool tts_read_data(
		  ivPointer		pParameter,			/* [in] user callback parameter */
		  ivPointer		pBuffer,			/* [out] read resource buffer */
		  ivResAddress	iPos,				/* [in] read start position */
ivResSize		nSize )			/* [in] read size */
{
	memcpy(pBuffer, pParameter + iPos, nSize);
	return ivTrue;
}
#else
#define FLASH_TTS_ADDR (64 * 1024)
extern sfud_flash sfud_flash_tables[];
static PV_Union g_s_spi_config;
static ivBool tts_read_data(
		  ivPointer		pParameter,			/* [in] user callback parameter */
		  ivPointer		pBuffer,			/* [out] read resource buffer */
		  ivResAddress	iPos,				/* [in] read start position */
ivResSize		nSize )			/* [in] read size */
{
	iPos += FLASH_TTS_ADDR;
	GPIO_FastOutput(g_s_spi_config.u8[1], 0);
	char cmd[4] = {0x03, iPos >> 16, (iPos >> 8) & 0xFF, iPos & 0xFF};
	SPI_FastTransfer(g_s_spi_config.u8[0], cmd, cmd, 4);
	if (nSize >= 4096) {
		SPI_BlockTransfer(g_s_spi_config.u8[0], pBuffer, pBuffer, nSize);
	}
	else {
		SPI_FastTransfer(g_s_spi_config.u8[0], pBuffer, pBuffer, nSize);
	}
	
	GPIO_FastOutput(g_s_spi_config.u8[1], 1);

	return ivTrue;
}
#endif
#endif
HANDLE soc_audio_fopen(const char *fname, const char *mode)
{
	return luat_fs_fopen(fname, mode);
}

int soc_audio_fread(void *buffer, uint32_t size, uint32_t num, void *fp)
{
	return luat_fs_fread(buffer, size, num, fp);
}

int soc_audio_fseek(void *fp, long offset, int origin)
{
	return luat_fs_fseek(fp, offset, origin);
}

int soc_audio_fclose(void *fp)
{
	return luat_fs_fclose(fp);
}

void luat_audio_global_init(void)
{
#if defined(LUAT_USE_TTS_ONLY)
#define PLAY_FILE NULL
#else
#define PLAY_FILE audio_play_file_default_fun
#endif

#ifdef LUAT_USE_TTS
	audio_play_global_init_with_task_priority(audio_event_cb, audio_data_cb, PLAY_FILE, audio_play_TTS_default_fun, NULL, 90);
#ifdef LUAT_USE_TTS_16K
#ifdef LUAT_USE_TTS_ONCHIP
	audio_play_tts_set_resource_ex(ivtts_16k, AISOUND_SDK_USERID_16K, tts_read_data);
#else
	audio_play_tts_set_resource_ex(NULL, AISOUND_SDK_USERID_16K, tts_read_data);
#endif
#else
#ifdef LUAT_USE_TTS_ONCHIP
	audio_play_tts_set_resource_ex(ivtts_8k, AISOUND_SDK_USERID_8K, tts_read_data);
#else
	audio_play_tts_set_resource_ex(NULL, AISOUND_SDK_USERID_8K, tts_read_data);
#endif
#endif
#else
	audio_play_global_init_with_task_priority(audio_event_cb, audio_data_cb, PLAY_FILE, NULL, NULL, 90);
#endif
	g_s_audio_hardware.dac_delay_len = 6;
	g_s_audio_hardware.dac_off_delay_time = 0;
	g_s_audio_hardware.pa_delay_time = 200;
	g_s_audio_hardware.pa_delay_timer = luat_create_rtos_timer(app_pa_on, NULL, NULL);
	g_s_audio_hardware.vol = 100;
	g_s_audio_hardware.dac_pin = -1;
	g_s_audio_hardware.pa_pin = -1;
}

int luat_audio_play_multi_files(uint8_t multimedia_id, uData_t *info, uint32_t files_num, uint8_t error_stop)
{
	g_s_audio_hardware.raw_mode = 0;
	uint32_t i;
	if (files_num > 256) files_num = 256;
	audio_play_info_t play_info[files_num];
	for(i = 0; i < files_num; i++)
	{
		play_info[i].path = info[i].value.asBuffer.buffer;
		play_info[i].address = 0;
		play_info[0].fail_continue = !error_stop;
	}
	return audio_play_multi_files(multimedia_id, play_info, files_num);
}

int luat_audio_play_file(uint8_t multimedia_id, const char *path)
{
	g_s_audio_hardware.raw_mode = 0;
	audio_play_info_t play_info[1];
	play_info[0].path = path;
	play_info[0].fail_continue = 0;
	play_info[0].address = NULL;
	return audio_play_multi_files(multimedia_id, play_info, 1);
}

uint8_t luat_audio_is_finish(uint8_t multimedia_id)
{
	return audio_play_is_finish(multimedia_id);
}

int luat_audio_play_stop(uint8_t multimedia_id)
{
	return audio_play_stop(multimedia_id);
}

int luat_audio_pause_raw(uint8_t multimedia_id, uint8_t is_pause)
{
	return audio_play_pause_raw(multimedia_id, is_pause);
}

int luat_audio_play_get_last_error(uint8_t multimedia_id)
{
	return audio_play_get_last_error(multimedia_id);
}

int luat_audio_start_raw(uint8_t multimedia_id, uint8_t audio_format, uint8_t num_channels, uint32_t sample_rate, uint8_t bits_per_sample, uint8_t is_signed)
{
	return audio_play_start_raw(multimedia_id, audio_format, num_channels, sample_rate, bits_per_sample, is_signed);
}

int luat_audio_write_raw(uint8_t multimedia_id, uint8_t *data, uint32_t len)
{
	int result = audio_play_write_raw(multimedia_id, data, len);
	if (!result)
	{
		g_s_audio_hardware.raw_mode = 1;
	}
	return result;
}

int luat_audio_stop_raw(uint8_t multimedia_id)
{
	return audio_play_stop_raw(multimedia_id);
}

void luat_audio_config_pa(uint8_t multimedia_id, uint32_t pin, int level, uint32_t dummy_time_len, uint32_t pa_delay_time)
{
	if (pin < HAL_GPIO_MAX)
	{
		g_s_audio_hardware.pa_pin = pin;
		g_s_audio_hardware.pa_on_level = level;
		GPIO_Config(pin, 0, !level);
		uint8_t alt_fun = (HAL_GPIO_16 == pin)?4:0;
		GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(pin, alt_fun), alt_fun, 0, 0);
	}
	else
	{
		g_s_audio_hardware.pa_pin = -1;
	}
	g_s_audio_hardware.dac_delay_len = dummy_time_len;
	g_s_audio_hardware.pa_delay_time = pa_delay_time;
}

void luat_audio_config_dac(uint8_t multimedia_id, int pin, int level, uint32_t dac_off_delay_time)
{
	if (pin < 0)
	{	g_s_audio_hardware.dac_pin = HAL_GPIO_16;
		g_s_audio_hardware.dac_on_level = 1;
		GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(g_s_audio_hardware.dac_pin, 4), 4, 0, 0);
		GPIO_Config(g_s_audio_hardware.dac_pin, 0, 0);
	}
	else
	{
		if (pin < HAL_GPIO_MAX)
		{
			g_s_audio_hardware.dac_pin = pin;
			g_s_audio_hardware.dac_on_level = level;
			GPIO_Config(pin, 0, !level);
			uint8_t alt_fun = (HAL_GPIO_16 == g_s_audio_hardware.dac_pin)?4:0;
			GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(g_s_audio_hardware.dac_pin, alt_fun), alt_fun, 0, 0);
		}
		else
		{
			g_s_audio_hardware.dac_pin = -1;
		}
	}
	g_s_audio_hardware.dac_off_delay_time = dac_off_delay_time;
}

uint16_t luat_audio_vol(uint8_t multimedia_id, uint16_t vol)
{
	g_s_audio_hardware.vol = vol;
	return g_s_audio_hardware.vol;
}

/**
 * @brief 设置音频硬件输出类型
 *
 * @param bus_type 见MULTIMEDIA_AUDIO_BUS，目前只有0=DAC 1=I2S 2=SOFT_DAC
 */
void luat_audio_set_bus_type(uint8_t multimedia_id,uint8_t bus_type)
{
	audio_play_set_bus_type(bus_type);
	g_s_audio_hardware.soft_dac_mode = (bus_type == 2);
}

void luat_audio_set_debug(uint8_t on_off)
{
	g_s_audio_hardware.debug_on_off = on_off;
	audio_play_debug_onoff(0, on_off);
}

int luat_i2s_setup(luat_i2s_conf_t *conf)
{
	uint8_t frame_size = I2S_FRAME_SIZE_16_16;
	if (conf->id >= I2S_MAX) return -1;
	if (conf->channel_bits != 16)
	{
		switch (conf->data_bits)
		{
		case 24:
			frame_size = I2S_FRAME_SIZE_24_32;
			break;
		case 32:
			frame_size = I2S_FRAME_SIZE_32_32;
			break;
		default:
			frame_size = I2S_FRAME_SIZE_16_32;
			break;
		}
	}
	luat_i2s_base_setup(conf->id, conf->standard, frame_size);
	g_s_audio_hardware.record_sample_rate[conf->id] = conf->sample_rate;
	g_s_audio_hardware.record_channel = (conf->channel_format < 2)?1:2;
	if (conf->channel_format < 2)
	{
		luat_i2s_set_lr_channel(conf->id, conf->channel_format);
	}
	return 0;
}
int luat_i2s_send(uint8_t id, uint8_t* buff, size_t len)
{
	return -1;
}

int luat_i2s_rx_cb(void *pdata, void *param){
	luat_audio_conf_t* audio_conf = luat_audio_get_config((uint8_t)param);
	if (audio_conf->codec_conf.i2s_conf->luat_i2s_event_callback){
		audio_conf->codec_conf.i2s_conf->luat_i2s_event_callback((uint8_t)param ,LUAT_I2S_EVENT_RX_DONE, pdata);
	}
}

int luat_i2s_recv(uint8_t id, uint8_t* buff, size_t len)
{
	if (id >= I2S_MAX) return -1;
	if (I2S_Start(id, 0, g_s_audio_hardware.record_sample_rate[id], g_s_audio_hardware.record_channel))
	{
		DBG("!");
		return -1;
	}
	return I2S_Rx(id, len, luat_i2s_rx_cb, id);
}
int luat_i2s_close(uint8_t id)
{
	luat_i2s_stop(id);
	return 0;
}
int l_i2s_play(lua_State *L) {
    return -1;
}

int l_i2s_pause(lua_State *L) {
    return 0;
}

int l_i2s_stop(lua_State *L) {
	uint8_t id = luaL_checkinteger(L, 1);
	I2S_RxStop(id);
}
#ifdef LUAT_USE_TTS
int luat_audio_play_tts_text(uint32_t multimedia_id, void *text, uint32_t text_bytes)
{
#ifdef LUAT_USE_TTS_ONCHIP
#else
	if (!g_s_spi_config.u32)
	{
		if (LUAT_TYPE_SPI == sfud_flash_tables[0].luat_sfud.luat_spi)
		{
			luat_spi_t *spi = (luat_spi_t *)sfud_flash_tables[0].luat_sfud.user_data;
			if (spi == NULL) {
				DBG("本固件支持TTS功能但未包含TTS资源,需要外挂SPI Flash才能正常工作");
				return -1;
			}
			g_s_spi_config.u8[0] = spi->id;
			g_s_spi_config.u8[1] = spi->cs;
		}
		else if (LUAT_TYPE_SPI_DEVICE == sfud_flash_tables[0].luat_sfud.luat_spi)
		{
			luat_spi_device_t* spi_device = (luat_spi_t *)sfud_flash_tables[0].luat_sfud.user_data;
			if (spi_device == NULL) {
				DBG("本固件支持TTS功能但未包含TTS资源,需要外挂SPI Flash才能正常工作");
				return -1;
			}
			g_s_spi_config.u8[0] = spi_device->bus_id;
			g_s_spi_config.u8[1] = spi_device->spi_config.cs;
		}
		else
		{
			return -1;
		}
	}
#endif
	return audio_play_tts_text(multimedia_id, text, text_bytes);
}

int luat_audio_play_tts_set_param(uint32_t multimedia_id, uint32_t param_id, uint32_t param_value)
{
	return audio_play_tts_set_param(multimedia_id, param_id, param_value);
}
#endif

#else
#include "luat_audio_play.h"
#include "luat_fs.h"
HANDLE soc_audio_fopen(const char *fname, const char *mode)
{
	return luat_fs_fopen(fname, mode);
}

int soc_audio_fread(void *buffer, uint32_t size, uint32_t num, void *fp)
{
	return luat_fs_fread(buffer, size, num, fp);
}

int soc_audio_fseek(void *fp, long offset, int origin)
{
	return luat_fs_fseek(fp, offset, origin);
}

int soc_audio_fclose(void *fp)
{
	return luat_fs_fclose(fp);
}

extern void audio_play_file_default_fun(void *param);
extern void audio_play_tts_default_fun(void *param);
extern void audio_play_tts_set_resource_ex(void *address, void *sdk_id, void *read_resource_fun);
extern void audio_play_global_init_ex(audio_play_event_cb_fun_t event_cb, audio_play_data_cb_fun_t data_cb, audio_play_default_fun_t play_file_fun, audio_play_default_fun_t play_tts_fun, void *user_param);
extern void audio_play_global_init_with_task_priority(audio_play_event_cb_fun_t event_cb, audio_play_data_cb_fun_t data_cb, audio_play_default_fun_t play_file_fun, audio_play_default_fun_t play_tts_fun, void *user_param, uint8_t priority);
extern int audio_play_write_blank_raw_ex(uint32_t multimedia_id, uint8_t cnt, uint8_t add_font);
void luat_audio_play_file_default_fun(void *param)
{
	audio_play_file_default_fun(param);
}
void luat_audio_play_tts_default_fun(void *param)
{
	audio_play_TTS_default_fun(param);
}

void luat_audio_play_global_init(
		audio_play_event_cb_fun_t event_cb,
		audio_play_data_cb_fun_t data_cb,
		audio_play_default_fun_t play_file_fun,
		audio_play_default_fun_t play_tts_fun,
		void *user_param)
{
	audio_play_global_init_ex(event_cb, data_cb, play_file_fun, play_tts_fun, user_param);
}

void luat_audio_play_global_init_with_task_priority(audio_play_event_cb_fun_t event_cb, audio_play_data_cb_fun_t data_cb, audio_play_default_fun_t play_file_fun, audio_play_default_fun_t play_tts_fun, void *user_param, uint8_t priority)
{
	audio_play_global_init_with_task_priority(event_cb, data_cb, play_file_fun, play_tts_fun, user_param, priority);
}

int luat_audio_play_multi_files(uint32_t multimedia_id, luat_audio_play_info_t info[], uint32_t files_num)
{
	return audio_play_multi_files(multimedia_id, (audio_play_info_t*)info, files_num);
}

uint8_t luat_audio_play_is_finish(uint32_t multimedia_id)
{
	return audio_play_is_finish(multimedia_id);
}

int luat_audio_play_stop(uint32_t multimedia_id)
{
	return audio_play_stop(multimedia_id);
}

int luat_audio_play_fast_stop(uint32_t multimedia_id)
{
	return audio_play_fast_stop(multimedia_id);
}

int luat_audio_play_clear_stop_flag(uint32_t multimedia_id)
{
	return audio_play_clear_stop_flag(multimedia_id);
}

int luat_audio_play_pause_raw(uint32_t multimedia_id, uint8_t is_pause)
{
	return audio_play_pause_raw(multimedia_id, is_pause);
}

int luat_audio_play_get_last_error(uint32_t multimedia_id)
{
	return audio_play_get_last_error(multimedia_id);
}

int luat_audio_play_write_blank_raw(uint32_t multimedia_id, uint8_t cnt, uint8_t add_font)
{
	return audio_play_write_blank_raw_ex(multimedia_id, cnt, add_font);
}

int luat_audio_play_start_raw(uint32_t multimedia_id, uint8_t audio_format, uint8_t num_channels, uint32_t sample_rate, uint8_t bits_per_sample, uint8_t is_signed)
{
	return audio_play_start_raw(multimedia_id, audio_format, num_channels, sample_rate, bits_per_sample, is_signed);
}

int luat_audio_play_write_raw(uint32_t multimedia_id, uint8_t *data, uint32_t len)
{
	return audio_play_write_raw(multimedia_id, data, len);
}

int luat_audio_play_stop_raw(uint32_t multimedia_id)
{
	return audio_play_stop_raw(multimedia_id);
}

int luat_audio_play_tts_text(uint32_t multimedia_id, void *text, uint32_t text_bytes)
{
	return audio_play_tts_text(multimedia_id, text, text_bytes);
}

int luat_audio_play_tts_set_param(uint32_t multimedia_id, uint32_t param_id, uint32_t param_value)
{
	return audio_play_tts_set_param(multimedia_id, param_id, param_value);
}


void luat_audio_play_tts_set_resource(void *address, void *sdk_id, void *tts_resource_read_fun)
{
	audio_play_tts_set_resource_ex(address, sdk_id, tts_resource_read_fun);
}

void luat_audio_play_set_bus_type(uint8_t bus_type)
{
	audio_play_set_bus_type(bus_type);
}

void *luat_audio_play_get_stream(uint32_t multimedia_id)
{
	return audio_play_get_stream(multimedia_id);
}

void luat_audio_play_set_user_lock(uint32_t multimedia_id, uint8_t onoff)
{
	audio_play_set_user_lock(multimedia_id, onoff);
}

void luat_audio_play_debug_onoff(uint32_t multimedia_id, uint8_t onoff)
{
	audio_play_debug_onoff(multimedia_id, onoff);
}

#endif
