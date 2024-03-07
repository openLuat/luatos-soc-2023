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

#ifdef __LUATOS__
#include "luat_msgbus.h"
#include "luat_mem.h"
enum
{
	LUAT_CAMERA_EVENT_FRAME_QRDECODE = USER_EVENT_ID_START + 100,
	LUAT_CAMERA_EVENT_FRAME_JPEG_ENCODE,
	LUAT_CAMERA_EVENT_FRAME_NEW,
	LUAT_CAMERA_EVENT_FRAME_END,
	LUAT_CAMERA_EVENT_FRAME_START,
	LUAT_CAMERA_EVENT_FRAME_ERROR,

};

typedef struct
{
	luat_spi_camera_t config;
	luat_rtos_task_handle task_handle;
	Buffer_Struct result_buffer;
	void *p_cache[2];
	uint32_t jpeg_data_point;
	uint16_t image_w;
	uint16_t image_h;
	uint8_t cur_cache;
	uint8_t is_process_image;
	uint8_t double_buffer_mode;
	uint8_t rx_to_user;				//单缓冲区时，当前是否在接收数据到用户区
	uint8_t scan_mode;					//扫码模式
	uint8_t raw_mode;					//原始图像模式
	uint8_t capture_stage;				//拍照流程
	uint8_t scan_pause;				//扫码暂停
	uint8_t camera_id;
}luat_camera_app_t;

static luat_camera_app_t luat_camera_app;

static int luat_image_decode_callback(void *pdata, void *param)
{
	uint32_t buffer_sn = (uint32_t)param;
	uint32_t len = (uint32_t)pdata;
//	DBG("buffer %d decode done!", buffer_sn);
	if (len)
	{
		if (len > luat_camera_app.result_buffer.MaxLen)
		{
			OS_ReInitBuffer(&luat_camera_app.result_buffer, len);
		}
		else if ((len < 1024) && (luat_camera_app.result_buffer.MaxLen >= 1024))
		{
			OS_ReInitBuffer(&luat_camera_app.result_buffer, len);
		}
		luat_camera_image_decode_get_result(luat_camera_app.result_buffer.Data);
	    rtos_msg_t msg = {0};
		msg.handler = l_camera_handler;
		msg.ptr = luat_camera_app.result_buffer.Data;
		msg.arg1 = 0;
		msg.arg2 = len;
		luat_msgbus_put(&msg, 1);
	}
	luat_camera_app.is_process_image = 0;
	return 0;
}

static int luat_camera_irq_callback(void *pdata, void *param)
{
	uint8_t cur_cache = luat_camera_app.cur_cache;
	switch ((uint32_t)pdata)
	{
	case LUAT_CAMERA_FRAME_RX_DONE:
		if (luat_camera_app.scan_mode)
		{
			if (luat_camera_app.scan_pause)
			{
				if (luat_camera_app.rx_to_user)
				{
					luat_camera_app.rx_to_user = 0;
					luat_camera_continue_with_buffer(luat_camera_app.camera_id, 0);	//摄像头数据发送到底层，不传递给用户
				}
				luat_rtos_event_send(luat_camera_app.task_handle, LUAT_CAMERA_EVENT_FRAME_NEW, cur_cache, 0, 0, 0);
				return 0;
			}
			else
			{
				if (!luat_camera_app.rx_to_user)
				{
					luat_camera_app.rx_to_user = 1;
					luat_camera_app.cur_cache = 0;
					luat_camera_continue_with_buffer(luat_camera_app.camera_id, luat_camera_app.p_cache[0]);	//摄像头数据发送给用户
					luat_rtos_event_send(luat_camera_app.task_handle, LUAT_CAMERA_EVENT_FRAME_NEW, cur_cache, 0, 0, 0);
					return 0;
				}

			}
			if (luat_camera_app.double_buffer_mode)
			{
				if (!luat_camera_app.rx_to_user)
				{
					luat_rtos_event_send(luat_camera_app.task_handle, LUAT_CAMERA_EVENT_FRAME_NEW, 0, 0, 0, 0);
					luat_camera_continue_with_buffer(luat_camera_app.camera_id, luat_camera_app.p_cache[0]);
					luat_camera_app.rx_to_user = 1;
					return 0;
				}
				//双缓冲模式下，扫码时允许1个解码，另一个接收数据
				if (!luat_camera_app.is_process_image)
				{
					luat_camera_app.is_process_image = 1;
					luat_rtos_event_send(luat_camera_app.task_handle, LUAT_CAMERA_EVENT_FRAME_QRDECODE, cur_cache, 0, 0, 0);
					luat_camera_app.cur_cache = !luat_camera_app.cur_cache;
					luat_camera_continue_with_buffer(luat_camera_app.camera_id, luat_camera_app.p_cache[luat_camera_app.cur_cache]);
				}
				else
				{
					luat_camera_continue_with_buffer(luat_camera_app.camera_id, luat_camera_app.p_cache[luat_camera_app.cur_cache]);
				}
			}
			else
			{
				if (!luat_camera_app.is_process_image)
				{
					if (luat_camera_app.rx_to_user)	//本次接收数据在用户区，则开始解码，并停止传递给用户区
					{
						luat_rtos_event_send(luat_camera_app.task_handle, LUAT_CAMERA_EVENT_FRAME_QRDECODE, 0, 0, 0, 0);
						luat_camera_continue_with_buffer(luat_camera_app.camera_id, 0);	//摄像头数据发送到底层，不传递给用户
						luat_camera_app.rx_to_user = 0;
						luat_camera_app.is_process_image = 1;
					}
					else	//本次接收数据不在用户区，则开始传递新的图像数据到用户区
					{
						luat_camera_continue_with_buffer(luat_camera_app.camera_id, luat_camera_app.p_cache[0]);
						luat_camera_app.rx_to_user = 1;
					}
				}
			}
		}
		else if (luat_camera_app.capture_stage && !luat_camera_app.is_process_image)
		{
			switch (luat_camera_app.capture_stage)
			{
			case 1:
				luat_camera_continue_with_buffer(luat_camera_app.camera_id, luat_camera_app.p_cache[0]);
				luat_camera_app.capture_stage = 2;
				break;
			case 2:
				luat_rtos_event_send(luat_camera_app.task_handle, LUAT_CAMERA_EVENT_FRAME_JPEG_ENCODE, 0, 0, 0, 0);
				luat_camera_continue_with_buffer(luat_camera_app.camera_id, 0);	//摄像头数据发送到底层，不传递给用户
				luat_camera_app.is_process_image = 1;
				break;
			default:
				break;
			}
		}
		else
		{
			luat_rtos_event_send(luat_camera_app.task_handle, LUAT_CAMERA_EVENT_FRAME_NEW, cur_cache, 0, 0, 0);
		}
		break;
	case LUAT_CAMERA_FRAME_ERROR:
		luat_rtos_event_send(luat_camera_app.task_handle, LUAT_CAMERA_EVENT_FRAME_ERROR, cur_cache, 0, 0, 0);
		break;
	}
	return 0;
}
#endif

static luat_camera_ctrl_t g_s_camera[CSPI_MAX];

static int luat_camera_dummy_callback(void *pdata, void *param)
{
	int id = (int)param;
	return g_s_camera[id].callback(pdata, g_s_camera[id].param);
}


int luat_camera_setup(int id, luat_spi_camera_t *conf, void * callback, void *param)
{
	if (id < 0 || id >= USP_ID2 || !conf || !callback) return -ERROR_PARAM_INVALID;
	if (g_s_camera[id].is_init) return -ERROR_OPERATION_FAILED;

#ifdef __LUATOS__
#else
	g_s_camera[id].callback = callback;
	g_s_camera[id].param = param;
#endif

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
	g_s_camera[id].image_w = conf->sensor_width;
	g_s_camera[id].image_h = conf->sensor_height;
#ifdef __LUATOS__
	luat_camera_app.camera_id = id;
	luat_camera_app.config = *conf;
	g_s_camera[id].callback = luat_camera_irq_callback;
	g_s_camera[id].param = NULL;
	luat_camera_app.double_buffer_mode = ((conf->sensor_width * conf->sensor_height) <= 80000 );
	luat_camera_app.p_cache[0] = luat_heap_opt_malloc(LUAT_HEAP_PSRAM, luat_camera_app.config.sensor_width * luat_camera_app.config.sensor_height * 2);
	luat_camera_app.p_cache[1] = luat_heap_opt_malloc(LUAT_HEAP_PSRAM, luat_camera_app.config.sensor_width * luat_camera_app.config.sensor_height * 2);
#endif
	return 0;
}

int luat_camera_set_image_w_h(int id, uint16_t w, uint16_t h)
{
	if (id < 0 || id >= USP_ID2 ) return -ERROR_PARAM_INVALID;
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
#ifdef __LUATOS__
	if (!luat_camera_app.task_handle || !g_s_camera[id].is_init || g_s_camera[id].is_running) return -1;

#else
	return -1;
#endif
}

int luat_camera_start_with_buffer(int id, void *buf)
{
	if (id < 0 || id >= USP_ID2) return -1;
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
#ifdef __LUATOS__

#else
	return -1;
#endif
}

int luat_camera_pause(int id, uint8_t is_pause)
{
	return -1;
}

int luat_camera_close(int id)
{
	if (id < 0 || id >= USP_ID2) return -1;
	if (!g_s_camera[id].is_init) return -ERROR_OPERATION_FAILED;
	CSPI_Stop(id);
	g_s_camera[id].is_init = 0;
	g_s_camera[id].is_running = 0;
	g_s_camera[id].callback = luat_camera_dummy_callback;
#ifdef __LUATOS__
	luat_heap_free(luat_camera_app.p_cache[0]);
	luat_heap_free(luat_camera_app.p_cache[1]);
	luat_camera_app.p_cache[0] = NULL;
	luat_camera_app.p_cache[1] = NULL;
	OS_DeInitBuffer(&luat_camera_app.result_buffer);
#endif
	return 0;

}

#ifdef __LUATOS__

static void luat_camera_save_JPEG_data(void *cxt, void *data, int size)
{
	uint8_t *p = luat_camera_app.p_cache[1];
	memcpy(p + luat_camera_app.jpeg_data_point, data, size);
	luat_camera_app.jpeg_data_point += size;
}

static void luat_camera_task(void *param)
{
	luat_event_t event;
	rtos_msg_t msg = {0};
    HANDLE JPEGEncodeHandle = NULL;
    //uint8_t *ycb_cache = malloc(luat_camera_app.image_w * 8 * 3);
    uint8_t *ycb_cache;
    uint8_t *file_data;
    uint8_t *p_cache;
    uint32_t i,j,k;
	while(1)
	{
		luat_rtos_event_recv(luat_camera_app.task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
		switch(event.id)
		{
		case LUAT_CAMERA_EVENT_FRAME_QRDECODE:

			DBG("解码开始 buf%d", event.param1);
			luat_camera_image_decode_once(luat_camera_app.p_cache[event.param1], luat_camera_app.config.sensor_width, luat_camera_app.config.sensor_height, 120, luat_image_decode_callback, event.param1);
			break;
		case LUAT_CAMERA_EVENT_FRAME_JPEG_ENCODE:
			p_cache = luat_camera_app.p_cache[0];
			luat_camera_app.jpeg_data_point = 0;
			DBG("转JPEG开始 ");

			JPEGEncodeHandle = jpeg_encode_init(luat_camera_save_JPEG_data, 0, 1, luat_camera_app.config.sensor_width, luat_camera_app.config.sensor_height, 3);
			ycb_cache = malloc(luat_camera_app.config.sensor_width * 8 * 3);
			for(i = 0; i < luat_camera_app.config.sensor_height; i+= 8)
			{
				for(j = i * luat_camera_app.config.sensor_width * 2, k = 0; j < (i + 8) * luat_camera_app.config.sensor_width * 2; j+=4, k+=6)
				{
					ycb_cache[k] = p_cache[j];
					ycb_cache[k + 1] = p_cache[j + 1];
					ycb_cache[k + 2] = p_cache[j + 3];
					ycb_cache[k + 3] = p_cache[j + 2];
					ycb_cache[k + 4] = p_cache[j + 1];
					ycb_cache[k + 5] = p_cache[j + 3];
				}
				jpeg_encode_run(JPEGEncodeHandle, ycb_cache);
			}

			jpeg_encode_end(JPEGEncodeHandle);
			free(JPEGEncodeHandle);
			free(ycb_cache);
			luat_camera_app.is_process_image = 0;

			break;
		case LUAT_CAMERA_EVENT_FRAME_NEW:
			break;
		case LUAT_CAMERA_EVENT_FRAME_ERROR:
	        msg.handler = l_camera_handler;
	        msg.ptr = NULL;
	        msg.arg1 = 0;
	        msg.arg2 = 0;
	        luat_msgbus_put(&msg, 1);
	        break;
		}
	}
}

int luat_camera_init(luat_camera_conf_t *conf)
{
	if (!luat_camera_app.task_handle) luat_rtos_task_create(&luat_camera_app.task_handle, 6 * 1024, 10, "camera", luat_camera_task, NULL, 64);
	return 0;
}
int luat_camera_capture(int id, uint8_t quality, const char *path)
{

}

int luat_camera_work_mode(id, mode)
{
	switch(mode)
	{
	case LUAT_CAMERA_MODE_AUTO:
		luat_camera_app.scan_mode = 0;
		break;
	case LUAT_CAMERA_MODE_SCAN:
		if (luat_camera_app.config.only_y)
		{
			luat_camera_app.scan_mode = 1;
		}
		else
		{
			luat_camera_app.scan_mode = 0;
			return -1;
		}
		break;
	}
	return 0;
}
#else
int luat_camera_init(luat_camera_conf_t *conf) {return -1;}
int luat_camera_capture(int id, uint8_t quality, const char *path) {return -1;}
int luat_camera_work_mode(id, mode){return -1;}
#endif


