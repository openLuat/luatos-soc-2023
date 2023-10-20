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
#include "luat_base.h"
#include "luat_rtos.h"
#include "common_api.h"
#include "luat_camera.h"
#if 0
extern int32_t CSPI_Setup(uint8_t I2SID, uint32_t BusSpeed, uint8_t SpiMode, uint8_t IsMSB, uint8_t Is2RxWire, uint8_t OnlyY, uint8_t SeqType, uint8_t rowScaleRatio, uint8_t colScaleRatio, uint8_t scaleBytes);
extern void CSPI_Rx(uint8_t I2SID, uint32_t ByteLen, CBFuncEx_t cb, void *param);
extern void CSPI_RxStop(uint8_t I2SID);
extern void CSPI_Stop(uint8_t I2SID);

typedef struct
{
	CBFuncEx_t callback;
	void *param;
	uint32_t total_byte;
	uint8_t is_init;
	uint8_t is_running;
}luat_camera_ctrl_t;

static luat_camera_ctrl_t g_s_camera[I2S_MAX];

static int luat_camera_dummy_callback(void *pdata, void *param)
{
	return 0;
}



int luat_camera_setup(int id, luat_spi_camera_t *conf, void * callback, void *param)
{
	if (id < 0 || id >= I2S_MAX || !conf || !callback) return -ERROR_PARAM_INVALID;
	if (g_s_camera[id].is_init) return -ERROR_OPERATION_FAILED;
	uint16_t color_byte = conf->only_y?1:2;
	conf->one_buf_height = 8000 / (conf->sensor_width * color_byte);
//	DBG("one buf height %d", conf->one_buf_height);
	g_s_camera[id].total_byte = conf->one_buf_height * conf->sensor_width * color_byte;
	g_s_camera[id].callback = callback;
	g_s_camera[id].param = param;

	if (id)
	{
		GPIO_IomuxEC618(18, 1, 1, 0);
		GPIO_IomuxEC618(19, 1, 1, 0);
		GPIO_IomuxEC618(21, 1, 1, 0);
		GPIO_IomuxEC618(22, 1, 1, 0);
	}
	else
	{
		GPIO_IomuxEC618(39, 1, 1, 0);
		GPIO_IomuxEC618(35, 1, 1, 0);
		GPIO_IomuxEC618(37, 1, 1, 0);
		GPIO_IomuxEC618(38, 1, 1, 0);
	}

	CSPI_Setup(id, conf->camera_speed, conf->spi_mode, conf->is_msb, conf->is_two_line_rx, conf->only_y, conf->seq_type, conf->rowScaleRatio, conf->colScaleRatio, conf->scaleBytes);

	g_s_camera[id].is_running = 0;
	g_s_camera[id].is_init = 1;

	return 0;
}


int luat_camera_start(int id)
{
	if (id < 0 || id >= I2S_MAX) return -1;
	if (!g_s_camera[id].is_init || g_s_camera[id].is_running) return -ERROR_OPERATION_FAILED;
	CSPI_Rx(id, g_s_camera[id].total_byte, g_s_camera[id].callback, g_s_camera[id].param);
	g_s_camera[id].is_running = 1;
	return 0;
}


int luat_camera_stop(int id)
{
	if (id < 0 || id >= I2S_MAX) return -1;
	if (!g_s_camera[id].is_init || !g_s_camera[id].is_running) return -ERROR_OPERATION_FAILED;
	CSPI_RxStop(id);
	g_s_camera[id].is_running = 0;
	return 0;
}


int luat_camera_close(int id)
{
	if (id < 0 || id >= I2S_MAX) return -1;
	if (!g_s_camera[id].is_init) return -ERROR_OPERATION_FAILED;
	CSPI_Stop(id);
	g_s_camera[id].callback = luat_camera_dummy_callback;
	return 0;

}

#ifdef __LUATOS__
int l_camera_handler(lua_State *L, void* ptr) {return -1;}
int luat_camera_init(luat_camera_conf_t *conf) {return -1;}
int luat_camera_capture(int id, uint8_t quality, const char *path) {return -1;}
#endif

#endif
