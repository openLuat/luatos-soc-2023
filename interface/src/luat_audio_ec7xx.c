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


#include "bsp_common.h"
#include "luat_audio.h"
#include "luat_i2s.h"
#include "common_api.h"
#include "audio_play.h"

#include "soc_spi.h"
#include "ivTTSSDKID_all.h"
#include "ivTTS.h"
#include "luat_spi.h"
#ifdef __LUATOS__
#include "luat_msgbus.h"
#include "sfud.h"
#endif
#include "luat_rtos.h"
#include "luat_mcu.h"
#include "luat_gpio.h"
#include "driver_usp.h"
#include "luat_fs.h"
#include "luat_audio_play.h"


extern void audio_play_file_default_fun(void *param);
extern void audio_play_tts_default_fun(void *param);
extern void audio_play_tts_set_resource_ex(void *address, void *sdk_id, void *read_resource_fun);
extern void audio_play_global_init_ex(audio_play_event_cb_fun_t event_cb, audio_play_data_cb_fun_t data_cb, audio_play_default_fun_t play_file_fun, audio_play_default_fun_t play_tts_fun, void *user_param);
extern void audio_play_global_init_with_task_priority(audio_play_event_cb_fun_t event_cb, audio_play_data_cb_fun_t data_cb, audio_play_default_fun_t play_file_fun, audio_play_default_fun_t play_tts_fun, void *user_param, uint8_t priority);
extern int audio_play_write_blank_raw_ex(uint32_t multimedia_id, uint8_t cnt, uint8_t add_font);
static luat_audio_conf_t prv_audio_config;
int luat_audio_play_blank(uint8_t multimedia_id);
static void app_pa_on(uint32_t arg)
{
	prv_audio_config.pa_on_enable = 1;
	luat_gpio_set(prv_audio_config.codec_conf.pa_pin, prv_audio_config.codec_conf.pa_on_level);
}
#ifdef __LUATOS__
extern const unsigned char ivtts_16k[];
extern const unsigned char ivtts_8k[];
extern int l_multimedia_raw_handler(lua_State *L, void* ptr);



static void audio_event_cb(uint32_t event, void *param){
	if (prv_audio_config.debug_on_off){
		DBG("%d", event);
	}
	rtos_msg_t msg = {0};
	switch(event)
	{
	case LUAT_MULTIMEDIA_CB_AUDIO_DECODE_START:
		luat_audio_check_ready(0);
		break;
	case LUAT_MULTIMEDIA_CB_AUDIO_OUTPUT_START:
		break;
	case LUAT_MULTIMEDIA_CB_AUDIO_NEED_DATA:
		if (prv_audio_config.raw_mode)
		{
			msg.handler = l_multimedia_raw_handler;
			msg.arg1 = LUAT_MULTIMEDIA_CB_AUDIO_NEED_DATA;
			msg.arg2 = (int)param;
			luat_msgbus_put(&msg, 1);
		}
		break;
	case LUAT_MULTIMEDIA_CB_TTS_INIT:
		break;
	case LUAT_MULTIMEDIA_CB_TTS_DONE:
		if (!audio_play_get_last_error(0))
		{
			audio_play_write_blank_raw_ex(0, 1, 0);
		}
		break;
	case LUAT_MULTIMEDIA_CB_AUDIO_DONE:
		luat_audio_standby(0);
		msg.handler = l_multimedia_raw_handler;
		msg.arg1 = LUAT_MULTIMEDIA_CB_AUDIO_DONE;
		msg.arg2 = (int)param;
		luat_msgbus_put(&msg, 1);
		break;
	}
}

static void audio_data_cb(uint8_t *data, uint32_t len, uint8_t bits, uint8_t channels)
{
	//这里可以对音频数据进行软件音量缩放，或者直接清空来静音
	if (!prv_audio_config.codec_conf.codec_opts->no_control && prv_audio_config.soft_vol <= 100) return;
	if (prv_audio_config.soft_vol != 100)
	{
		int16_t val = prv_audio_config.soft_vol;
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
	prv_audio_config.pa_delay_timer = luat_create_rtos_timer(app_pa_on, NULL, NULL);
	prv_audio_config.soft_vol = 100;
}

#endif
static void luat_audio_prepare(void)
{
	if (prv_audio_config.is_sleep)
	{
		luat_audio_sleep(0, 0);
	}
}

int luat_audio_play_file(uint8_t multimedia_id, const char *path)
{
	luat_audio_prepare();
	prv_audio_config.raw_mode = 0;
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

int luat_audio_start_raw(uint8_t multimedia_id, uint8_t audio_format, uint8_t num_channels, uint32_t sample_rate, uint8_t bits_per_sample, uint8_t is_signed)
{
	luat_audio_prepare();
	prv_audio_config.raw_mode = 1;
	return audio_play_start_raw(prv_audio_config.codec_conf.i2s_id, audio_format, num_channels, sample_rate, bits_per_sample, is_signed);
}


int luat_audio_pause_raw(uint8_t multimedia_id, uint8_t is_pause)
{
	return audio_play_pause_raw(prv_audio_config.codec_conf.i2s_id, is_pause);
}

int luat_audio_write_raw(uint8_t multimedia_id, uint8_t *data, uint32_t len)
{
	int result = audio_play_write_raw(prv_audio_config.codec_conf.i2s_id, data, len);
	if (!result)
	{
		prv_audio_config.raw_mode = 1;
	}
	return result;
}

int luat_audio_stop_raw(uint8_t multimedia_id)
{
	return audio_play_stop_raw(prv_audio_config.codec_conf.i2s_id);
}

void luat_audio_config_pa(uint8_t multimedia_id, uint32_t pin, int level, uint32_t dummy_time_len, uint32_t pa_delay_time)
{
	luat_audio_conf_t* audio_conf = luat_audio_get_config(multimedia_id);
	if (pin < HAL_GPIO_MAX)
	{
		prv_audio_config.codec_conf.pa_pin = pin;
		prv_audio_config.codec_conf.pa_on_level = level;
		GPIO_Config(pin, 0, !level);
		uint8_t alt_fun = (HAL_GPIO_16 == pin)?4:0;
		GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(pin, alt_fun), alt_fun, 0, 0);
	}
	else
	{
		prv_audio_config.codec_conf.pa_pin = LUAT_CODEC_PA_NONE;
		prv_audio_config.codec_conf.pa_pin = -1;
	}

	prv_audio_config.codec_conf.pa_delay_time = pa_delay_time;
	prv_audio_config.codec_conf.after_sleep_ready_time = dummy_time_len;
}

void luat_audio_config_dac(uint8_t multimedia_id, int pin, int level, uint32_t dac_off_delay_time)
{
	if (pin < 0)
	{	prv_audio_config.codec_conf.power_pin = HAL_GPIO_16;
		prv_audio_config.codec_conf.power_on_level = 1;
		GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(prv_audio_config.codec_conf.power_pin, 4), 4, 0, 0);
		GPIO_Config(prv_audio_config.codec_conf.power_pin, 0, 0);
	}
	else
	{
		if (pin < HAL_GPIO_MAX)
		{
			prv_audio_config.codec_conf.power_pin = pin;
			prv_audio_config.codec_conf.power_on_level = level;
			GPIO_Config(pin, 0, !level);
			uint8_t alt_fun = (HAL_GPIO_16 == prv_audio_config.codec_conf.power_pin)?4:0;
			GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(prv_audio_config.codec_conf.power_pin, alt_fun), alt_fun, 0, 0);
		}
		else
		{
			prv_audio_config.codec_conf.power_pin = -1;
		}
	}
	prv_audio_config.codec_conf.codec_delay_off_time = dac_off_delay_time;
}

uint16_t luat_audio_vol(uint8_t multimedia_id, uint16_t vol)
{
	if (prv_audio_config.codec_conf.codec_opts->no_control)
	{
		prv_audio_config.soft_vol = vol;
		return prv_audio_config.soft_vol;
	}
	if (vol <= 100)
	{
		prv_audio_config.soft_vol = 100;
		prv_audio_config.hardware_vol = vol;
		prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_SET_VOICE_VOL,vol);
		return prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_GET_VOICE_VOL,0);
	}
	else
	{
		prv_audio_config.soft_vol = vol;
		prv_audio_config.hardware_vol = 100;
		prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_SET_VOICE_VOL,100);
		return prv_audio_config.soft_vol;
	}


}

uint8_t luat_audio_mic_vol(uint8_t multimedia_id, uint16_t vol){
	if(vol < 0 || vol > 100 || prv_audio_config.codec_conf.codec_opts->no_control){
		return -1;
	}

	if (prv_audio_config.bus_type == AUSTREAM_BUS_I2S && prv_audio_config.codec_conf.codec_opts){
		prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_SET_MIC_VOL,vol);
		return prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_GET_MIC_VOL,0);
	}

	return -1;
}

/**
 * @brief 设置音频硬件输出类型
 *
 * @param bus_type 见MULTIMEDIA_AUDIO_BUS，目前只有0=DAC 1=I2S 2=SOFT_DAC
 */
int luat_audio_set_bus_type(uint8_t multimedia_id,uint8_t bus_type)
{
	audio_play_set_bus_type(bus_type);
	prv_audio_config.bus_type = bus_type;
	if (bus_type == AUSTREAM_BUS_I2S){
		prv_audio_config.bus_type = AUSTREAM_BUS_I2S;
		prv_audio_config.codec_conf.multimedia_id = multimedia_id;
		if (prv_audio_config.codec_conf.codec_opts->init(&prv_audio_config.codec_conf,LUAT_CODEC_MODE_SLAVE)){
			DBG("no codec %s",prv_audio_config.codec_conf.codec_opts->name);
			return -1;
		}else{
			DBG("find codec %s",prv_audio_config.codec_conf.codec_opts->name);
		}
		prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_SET_FORMAT,LUAT_CODEC_FORMAT_I2S);
	}
	return 0;


}

void luat_audio_set_debug(uint8_t on_off)
{
	audio_play_debug_onoff(0, on_off);
}


void luat_audio_play_file_default_fun(void *param)
{
	audio_play_file_default_fun(param);
}
void luat_audio_play_tts_default_fun(void *param)
{
	audio_play_TTS_default_fun(param);
}

void luat_audio_play_global_init_with_task_priority(audio_play_event_cb_fun_t event_cb, audio_play_data_cb_fun_t data_cb, audio_play_default_fun_t play_file_fun, audio_play_default_fun_t play_tts_fun, void *user_param, uint8_t priority)
{
	audio_play_global_init_with_task_priority(event_cb, data_cb, play_file_fun, play_tts_fun, user_param, priority);
	prv_audio_config.soft_vol = 100;
	prv_audio_config.pa_delay_timer = luat_create_rtos_timer(app_pa_on, NULL, NULL);
}



uint8_t luat_audio_play_is_finish(uint8_t multimedia_id)
{
	return audio_play_is_finish(prv_audio_config.codec_conf.i2s_id);
}



int luat_audio_play_fast_stop(uint8_t multimedia_id)
{
	if (audio_play_fast_stop(prv_audio_config.codec_conf.i2s_id))
	{
		luat_audio_play_blank(multimedia_id);
		return -1;
	}
	return 0;
}

int luat_audio_play_clear_stop_flag(uint8_t multimedia_id)
{
	return audio_play_clear_stop_flag(prv_audio_config.codec_conf.i2s_id);
}

int luat_audio_play_pause_raw(uint8_t multimedia_id, uint8_t is_pause)
{
	return audio_play_pause_raw(prv_audio_config.codec_conf.i2s_id, is_pause);
}

int luat_audio_play_get_last_error(uint8_t multimedia_id)
{
	return audio_play_get_last_error(prv_audio_config.codec_conf.i2s_id);
}



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

#ifdef __LUATOS__

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
	luat_audio_prepare();
	prv_audio_config.raw_mode = 0;
	return audio_play_tts_text(multimedia_id, text, text_bytes);
}

int luat_audio_play_tts_set_param(uint32_t multimedia_id, uint32_t param_id, uint32_t param_value)
{
	return audio_play_tts_set_param(multimedia_id, param_id, param_value);
}
#endif

#else
int luat_audio_play_tts_text(uint8_t multimedia_id, void *text, uint32_t text_bytes)
{
	luat_audio_prepare();
	prv_audio_config.raw_mode = 0;
	return audio_play_tts_text(prv_audio_config.codec_conf.i2s_id, text, text_bytes);
}

int luat_audio_play_tts_set_param(uint8_t multimedia_id, uint32_t param_id, uint32_t param_value)
{
	return audio_play_tts_set_param(prv_audio_config.codec_conf.i2s_id, param_id, param_value);
}
#endif


int luat_audio_play_stop(uint8_t multimedia_id)
{
	if (audio_play_stop(prv_audio_config.codec_conf.i2s_id))
	{
		luat_audio_play_blank(multimedia_id);
		return -1;
	}
	return 0;
}

void luat_audio_play_debug_onoff(uint8_t multimedia_id, uint8_t onoff)
{
	audio_play_debug_onoff(0, onoff);
	prv_audio_config.debug_on_off = onoff;
}

void luat_audio_play_tts_set_resource(void *address, void *sdk_id, void *tts_resource_read_fun)
{
	audio_play_tts_set_resource_ex(address, sdk_id, tts_resource_read_fun);
}

void luat_audio_play_set_user_lock(uint8_t multimedia_id, uint8_t onoff)
{
	audio_play_set_user_lock(0, onoff);
}

void *luat_audio_play_get_stream(uint8_t multimedia_id)
{
	return audio_play_get_stream(0);
}

int luat_audio_play_write_blank_raw(uint8_t multimedia_id, uint8_t cnt, uint8_t add_font)
{
	return audio_play_write_blank_raw_ex(prv_audio_config.codec_conf.i2s_id, cnt, add_font);
}

int luat_audio_play_start_raw(uint8_t multimedia_id, uint8_t audio_format, uint8_t num_channels, uint32_t sample_rate, uint8_t bits_per_sample, uint8_t is_signed)
{
	luat_audio_prepare();
	prv_audio_config.raw_mode = 1;
	return audio_play_start_raw(prv_audio_config.codec_conf.i2s_id, audio_format, num_channels, sample_rate, bits_per_sample, is_signed);
}

int luat_audio_play_write_raw(uint8_t multimedia_id, uint8_t *data, uint32_t len)
{
	return audio_play_write_raw(prv_audio_config.codec_conf.i2s_id, data, len);
}

int luat_audio_play_stop_raw(uint8_t multimedia_id)
{
	return audio_play_stop_raw(prv_audio_config.codec_conf.i2s_id);
}

#ifdef __LUATOS__
int luat_audio_play_multi_files(uint8_t multimedia_id, uData_t *info, uint32_t files_num, uint8_t error_stop)
{
	luat_audio_prepare();
	prv_audio_config.raw_mode = 0;
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
#else
int luat_audio_play_multi_files(uint8_t multimedia_id, luat_audio_play_info_t info[], uint32_t files_num)
{
	luat_audio_prepare();
	prv_audio_config.raw_mode = 0;
	return audio_play_multi_files(prv_audio_config.codec_conf.i2s_id, (audio_play_info_t*)info, files_num);
}
#endif


luat_audio_conf_t *luat_audio_get_config(uint8_t multimedia_id){
    if (multimedia_id == 0) return &prv_audio_config;
    else return NULL;
}

int luat_audio_setup_codec(uint8_t multimedia_id, const luat_audio_codec_conf_t *codec_conf)
{
	prv_audio_config.codec_conf = *codec_conf;
	return 0;
}

int luat_audio_init_codec(uint8_t multimedia_id, uint16_t init_vol, uint16_t init_mic_vol)
{
	luat_audio_play_blank(0);
	luat_gpio_set(prv_audio_config.codec_conf.power_pin, prv_audio_config.codec_conf.power_on_level);
	prv_audio_config.last_wakeup_time_ms = luat_mcu_tick64_ms();
	prv_audio_config.hardware_vol = init_vol;
	if (prv_audio_config.codec_conf.power_on_delay_ms)
	{
		luat_rtos_task_sleep(prv_audio_config.codec_conf.power_on_delay_ms);
	}
	if (prv_audio_config.codec_conf.codec_opts->no_control)
	{

	}
	else
	{
		int result = prv_audio_config.codec_conf.codec_opts->init(&prv_audio_config.codec_conf, LUAT_CODEC_MODE_SLAVE);
		if (result)
		{
			DBG("no codec");
			return result;
		}
		result = prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_SET_RATE, 16000);
		if (result)
		{
			return result;
		}
		result = prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_SET_BITS, 16);
		if (result)
		{
			return result;
		}
		result = prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_SET_FORMAT,LUAT_CODEC_FORMAT_I2S);
		if (result)
		{
			return result;
		}
		result = prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_SET_VOICE_VOL, init_vol);
		if (result)
		{
			return result;
		}
		result = prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_SET_MIC_VOL, init_mic_vol);
		if (result)
		{
			return result;
		}
		result = prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_MODE_STANDBY,LUAT_CODEC_MODE_ALL);
		if (result)
		{
			return result;
		}
		result = prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_MODE_NORMAL,LUAT_CODEC_MODE_ALL);
		if (result)
		{
			return result;
		}
	}
	return 0;
}

int luat_audio_sleep(uint8_t multimedia_id, uint8_t on_off)
{
	int result;
	if (on_off != prv_audio_config.is_sleep)
	{
		if (on_off)
		{
			DBG("sleep");
			luat_gpio_set(prv_audio_config.codec_conf.pa_pin, !prv_audio_config.codec_conf.pa_on_level);
			if (prv_audio_config.codec_conf.codec_delay_off_time)
			{
				luat_rtos_task_sleep(prv_audio_config.codec_conf.codec_delay_off_time);
			}
			if (prv_audio_config.codec_conf.codec_opts->no_control)
			{
				luat_gpio_set(prv_audio_config.codec_conf.power_pin, !prv_audio_config.codec_conf.power_on_level);
			}
			else
			{
				result = prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_MODE_PWRDOWN,LUAT_CODEC_MODE_ALL);
				if (result)
				{
					return result;
				}
			}
			if (I2S_IsWorking(prv_audio_config.codec_conf.i2s_id))
			{
				I2S_Stop(prv_audio_config.codec_conf.i2s_id);
			}
			prv_audio_config.wakeup_ready = 0;
			prv_audio_config.pa_on_enable = 0;


		}
		else
		{
			DBG("wakeup");
			luat_audio_play_blank(0);
			if (prv_audio_config.codec_conf.codec_opts->no_control)
			{
				luat_gpio_set(prv_audio_config.codec_conf.power_pin, prv_audio_config.codec_conf.power_on_level);
			}
			else
			{
				result = prv_audio_config.codec_conf.codec_opts->control(&prv_audio_config.codec_conf,LUAT_CODEC_MODE_NORMAL,LUAT_CODEC_MODE_ALL);
				if (result)
				{
					return result;
				}
			}
			prv_audio_config.wakeup_ready = 0;
			prv_audio_config.pa_on_enable = 0;
			prv_audio_config.last_wakeup_time_ms = luat_mcu_tick64_ms();

		}
		prv_audio_config.is_sleep = on_off;
	}

	return 0;
}

int luat_audio_check_ready(uint8_t multimedia_id)
{
	uint32_t rest;
	if (prv_audio_config.wakeup_ready)
	{
		luat_audio_play_write_blank_raw(prv_audio_config.codec_conf.i2s_id, 1, 1);
		goto ENABLE_PA;
	}
	if ((luat_mcu_tick64_ms() - prv_audio_config.last_wakeup_time_ms) > prv_audio_config.codec_conf.after_sleep_ready_time)
	{
		prv_audio_config.wakeup_ready = 1;
		luat_audio_play_write_blank_raw(prv_audio_config.codec_conf.i2s_id, 1, 1);
		goto ENABLE_PA;
	}
	rest = prv_audio_config.codec_conf.after_sleep_ready_time - ((uint32_t)(luat_mcu_tick64_ms() - prv_audio_config.last_wakeup_time_ms));
	DBG("codec wakeup need %dms", rest);
	if (rest > 100)
	{
		luat_audio_play_write_blank_raw(prv_audio_config.codec_conf.i2s_id, rest/100, 1);
	}
	else
	{
		luat_audio_play_write_blank_raw(prv_audio_config.codec_conf.i2s_id, 1, 1);
	}
ENABLE_PA:
	if (prv_audio_config.pa_on_enable)
	{
		luat_gpio_set(prv_audio_config.codec_conf.pa_pin, prv_audio_config.codec_conf.pa_on_level);
	}
	else
	{
		if ((luat_mcu_tick64_ms() - prv_audio_config.last_wakeup_time_ms) > (prv_audio_config.codec_conf.pa_delay_time - 10))
		{
			prv_audio_config.pa_on_enable = 1;
			luat_gpio_set(prv_audio_config.codec_conf.pa_pin, prv_audio_config.codec_conf.pa_on_level);
		}
		else
		{
			rest = prv_audio_config.codec_conf.pa_delay_time - ((uint32_t)(luat_mcu_tick64_ms() - prv_audio_config.last_wakeup_time_ms));
			DBG("pa enable need %dms", rest);
			luat_start_rtos_timer(prv_audio_config.pa_delay_timer, rest, 0);
		}
	}
	return 0;
}

int luat_audio_play_blank(uint8_t multimedia_id)
{
	if (I2S_IsWorking(prv_audio_config.codec_conf.i2s_id))
	{
		I2S_Stop(prv_audio_config.codec_conf.i2s_id);
	}
	I2S_Start(prv_audio_config.codec_conf.i2s_id, 1, 16000, 2);
	I2S_TransferLoop(prv_audio_config.codec_conf.i2s_id, NULL, 3200, 2, 0);
}

int luat_audio_standby(uint8_t multimedia_id)
{
	luat_gpio_set(prv_audio_config.codec_conf.pa_pin, !prv_audio_config.codec_conf.pa_on_level);
	luat_audio_play_blank(multimedia_id);

}

int luat_audio_record_and_play(uint8_t multimedia_id, uint32_t sample_rate, const uint8_t *play_buffer, uint32_t one_trunk_len, uint32_t total_trunk_cnt)
{
	if (I2S_IsWorking(prv_audio_config.codec_conf.i2s_id))
	{
		I2S_Stop(prv_audio_config.codec_conf.i2s_id);
	}
	luat_i2s_conf_t *i2s = luat_i2s_get_config(prv_audio_config.codec_conf.i2s_id);
	luat_audio_sleep(0, 0);
	luat_i2s_modify(prv_audio_config.codec_conf.i2s_id, i2s->channel_format, i2s->data_bits, sample_rate);
	I2S_TransferLoop(prv_audio_config.codec_conf.i2s_id, play_buffer, one_trunk_len, total_trunk_cnt, 0);
}

int luat_audio_record_stop(uint8_t multimedia_id)
{
	if (I2S_IsWorking(prv_audio_config.codec_conf.i2s_id))
	{
		I2S_Stop(prv_audio_config.codec_conf.i2s_id);
	}
	return 0;
}


int luat_audio_speech(uint8_t multimedia_id, uint8_t only_play, uint32_t sample_rate)
{

}
