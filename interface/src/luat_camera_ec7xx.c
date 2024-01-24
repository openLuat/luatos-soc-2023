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
#include "driver_usp.h"
#include "soc_image_decode.h"

typedef struct
{
	CBFuncEx_t callback;
	void *param;
	uint16_t image_w;
	uint16_t image_h;
	uint8_t is_init;
	uint8_t is_running;
	uint8_t is_scan_mode;
}luat_camera_ctrl_t;

static luat_camera_ctrl_t g_s_camera[CSPI_MAX];

static int luat_camera_dummy_callback(void *pdata, void *param)
{
	int id = (int)param;
	return g_s_camera[id].callback(pdata, g_s_camera[id].param);
}


int luat_camera_setup(int id, luat_spi_camera_t *conf, void * callback, void *param)
{
	if (id < 0 || id >= CSPI_ID2 || !conf || !callback) return -ERROR_PARAM_INVALID;
	if (g_s_camera[id].is_init) return -ERROR_OPERATION_FAILED;

//	DBG("one buf height %d", conf->one_buf_height);
	g_s_camera[id].callback = callback;
	g_s_camera[id].param = param;

	if (id)
	{
		GPIO_IomuxEC7XX(18, 1, 1, 0);
		GPIO_IomuxEC7XX(19, 1, 1, 0);
		GPIO_IomuxEC7XX(21, 1, 1, 0);
		GPIO_IomuxEC7XX(22, 1, 1, 0);
	}
	else
	{
		GPIO_IomuxEC7XX(35, 1, 1, 0);
		GPIO_IomuxEC7XX(37, 1, 1, 0);
		GPIO_IomuxEC7XX(38, 1, 1, 0);
		GPIO_IomuxEC7XX(39, 1, 1, 0);
	}

	CSPI_Setup(id, conf->camera_speed, conf->spi_mode, conf->is_msb, conf->is_two_line_rx, conf->only_y, conf->seq_type, conf->rowScaleRatio, conf->colScaleRatio, conf->scaleBytes, conf->plat_param[0], conf->plat_param[1], conf->plat_param[2]);

	g_s_camera[id].is_running = 0;
	g_s_camera[id].is_init = 1;
	g_s_camera[id].is_scan_mode = conf->image_scan;
	g_s_camera[id].image_w = conf->sensor_width;
	g_s_camera[id].image_h = conf->sensor_height;
	return 0;
}

int luat_camera_set_image_w_h(int id, uint16_t w, uint16_t h)
{
	if (id < 0 || id >= CSPI_ID2 ) return -ERROR_PARAM_INVALID;
	g_s_camera[id].image_w = w;
	g_s_camera[id].image_h = h;
	return 0;
}

int luat_camera_image_decode_init(uint8_t type, void *stack, uint32_t stack_length, uint32_t priority)
{
	return soc_image_decode_init(stack, stack_length, priority, type);
}

int luat_camera_image_decode_once(uint8_t *data, uint16_t image_w, uint16_t image_h, uint32_t timeout, void *callback, void *param)
{
	return soc_image_decode_once(data, image_w, image_h, timeout, callback, param);
}

void luat_camera_image_decode_deinit(void)
{
	soc_image_decode_deinit();
}

int luat_camera_image_decode_get_result(uint8_t *buf)
{
	return soc_image_decode_get_result(buf);
}

int luat_camera_start(int id)
{
	return -1;
}

int luat_camera_start_with_buffer(int id, void *buf)
{
	if (id < 0 || id >= CSPI_ID2) return -1;
	if (!g_s_camera[id].is_init || g_s_camera[id].is_running) return -ERROR_OPERATION_FAILED;
	CSPI_Rx(id, buf, g_s_camera[id].image_w, g_s_camera[id].image_h, luat_camera_dummy_callback, id);
	g_s_camera[id].is_running = 1;
	return 0;
}

void luat_camera_continue_with_buffer(int id, void *buf)
{
	CSPI_RxContinue(id, buf);
}

int luat_camera_stop(int id)
{
	if (id < 0 || id >= CSPI_ID2) return -1;
	if (!g_s_camera[id].is_init || !g_s_camera[id].is_running) return -ERROR_OPERATION_FAILED;
	CSPI_RxStop(id);
	g_s_camera[id].is_running = 0;
	return 0;
}

int luat_camera_pause(int id, uint8_t is_pause)
{
	if (id < 0 || id >= CSPI_ID2) return -1;
	if (is_pause)
	{
		XIC_DisableIRQ(id?PXIC0_USP1_IRQn:PXIC0_USP0_IRQn);
	}
	else
	{
		XIC_EnableIRQ(id?PXIC0_USP1_IRQn:PXIC0_USP0_IRQn);
		XIC_SuppressOvfIRQ(id?PXIC0_USP1_IRQn:PXIC0_USP0_IRQn);
	}
	return 0;
}

int luat_camera_close(int id)
{
	if (id < 0 || id >= CSPI_ID2) return -1;
	if (!g_s_camera[id].is_init) return -ERROR_OPERATION_FAILED;
	CSPI_Stop(id);
	g_s_camera[id].is_init = 0;
	g_s_camera[id].callback = luat_camera_dummy_callback;
	return 0;

}

#ifdef __LUATOS__
int l_camera_handler(lua_State *L, void* ptr) {return -1;}
int luat_camera_init(luat_camera_conf_t *conf) {return -1;}
int luat_camera_capture(int id, uint8_t quality, const char *path) {return -1;}
#endif


