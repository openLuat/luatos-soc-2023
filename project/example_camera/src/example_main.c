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

/*
 * demo需要云喇叭开发板+air780EP + BF30A2/GC032A，或者CORE开发板+ BF30A2/GC032A，使用I2S1总线，I2C总线自适应，如果需要低功耗，在休眠前需要完全关闭摄像头和扫码功能。
 * lcd预览默认使用2.4寸TFT LCD 分辨率:240X320 屏幕ic:GC9306，如果需要低功耗，RST,背光控制和CS脚要用AONIO控制
 * Air780EP，使用内部LDO输出，就是VDD_EXT
 * 由于需要的ram比较多，如果有PSRAM，可以正常编译，如果没有，不建议使用摄像头功能
 * 根据实际需要开启相关功能
 * 扫码由于PSRAM的硬件限制，8W可以双buffer，30W只能单buffer
 * demo只有2种模式，扫码或者拍照，均默认启用预览，拍照时按下BOOT键启动一次照片并通过USB/UART上传
 * 扫码demo下，按下BOOT键启动/停止扫码
 */

#include "common_api.h"
#include "luat_camera.h"
#include "luat_rtos.h"
#include "luat_gpio.h"
#include "luat_debug.h"
#include "luat_uart.h"
#include "luat_i2c.h"
#include "luat_mem.h"
#include "luat_lcd.h"
#include "mem_map.h"

#define CAMERA_TEST_QRCODE			//扫码
#define LCD_ENABLE						//默认均开启LCD预览
#define USB_UART_ENABLE


#define CAMERA_I2C_ID	(g_s_camera_app.camera_id)
#define CAMERA_SPI_ID   CSPI_ID1
#define CAMERA_PD_PIN	HAL_GPIO_5
//#define CAMERA_POWER_PIN HAL_GPIO_25	//外部LDO控制
//#define CAMERA_POWER_PIN_ALT 0
//内部LDO就是VDD_EXT, 不需要单独控制

#define CAMERA_USE_BFXXXX
//#define CAMERA_USE_GC032A

#define BF30A2_I2C_ADDRESS	(0x6e)
#define GC032A_I2C_ADDR		(0x21)

#ifdef CAMERA_USE_GC032A

#ifdef CAMERA_TEST_QRCODE
#define CAMERA_SPEED	25500000
#else
#define CAMERA_SPEED	24000000
#endif

#else

#define CAMERA_SPEED	25500000

#endif

enum
{
	CAMERA_FRAME_START = USER_EVENT_ID_START,
	CAMERA_FRAME_END,
	CAMERA_FRAME_QR_DECODE,
	CAMERA_FRAME_JPEG_ENCODE,
	CAMERA_FRAME_NEW,
	CAMERA_FRAME_ERROR,
	PIN_PRESS,
};

typedef struct
{
	luat_spi_camera_t camera;
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

static luat_camera_app_t g_s_camera_app;
luat_rtos_task_handle g_s_task_handle;

typedef struct
{
	uint8_t reg;
	uint8_t data;
}camera_reg_t;


static camera_reg_t g_s_bf302a_reg_table[] =
{
	{0xf2,0x01},//soft reset
	{0xcf,0xb0},//POWER UP
	{0x12,0x20},//MTK:20 ZX:10 RDA:40
	{0x15,0x80},
	{0x6b,0x71},
	{0x00,0x40},
	{0x04,0x00},
	{0x06,0x26},
	{0x08,0x07},
	{0x1c,0x12},
	{0x20,0x20},
	{0x21,0x20},
	{0x34,0x02},
	{0x35,0x02},
	{0x36,0x21},
	{0x37,0x13},
	//{0x03,0x23},
	{0xca,0x23},
	{0xcb,0x22},
	{0xcc,0x89},
	{0xcd,0x4c},
	{0xce,0x6b},
	{0xa0,0x8e},
	{0x01,0x1b},
	{0x02,0x1d},
	{0x13,0x08},
	{0x87,0x13},
	{0x8b,0x08},
	{0x70,0x17},
	{0x71,0x43},
	{0x72,0x0a},
	{0x73,0x62},
	{0x74,0xa2},
	{0x75,0xbf},
	{0x76,0x00},
	{0x77,0xcc},
	{0x40,0x32},
	{0x41,0x28},
	{0x42,0x26},
	{0x43,0x1d},
	{0x44,0x1a},
	{0x45,0x14},
	{0x46,0x11},
	{0x47,0x0f},
	{0x48,0x0e},
	{0x49,0x0d},
	{0x4B,0x0c},
	{0x4C,0x0b},
	{0x4E,0x0a},
	{0x4F,0x09},
	{0x50,0x09},
	{0x24,0x30},
	{0x25,0x36},
	{0x80,0x00},
	{0x81,0x20},
	{0x82,0x40},
	{0x83,0x30},
	{0x84,0x50},
	{0x85,0x30},
	{0x86,0xd8},
	{0x89,0x45},
	{0x8a,0x33},
	{0x8f,0x81},
	{0x91,0xff},
	{0x92,0x08},
	{0x94,0x82},
	{0x95,0xfd},
	{0x9a,0x20},
	{0x9e,0xbc},
	{0xf0,0x87},
	{0x51,0x06},
	{0x52,0x25},
	{0x53,0x2b},
	{0x54,0x0f},
	{0x57,0x2a},
	{0x58,0x22},
	{0x59,0x2c},
	{0x23,0x33},
	{0xa1,0x93},
	{0xa2,0x0f},
	{0xa3,0x2a},
	{0xa4,0x08},
	{0xa5,0x26},
	{0xa7,0x80},
	{0xa8,0x80},
	{0xa9,0x1e},
	{0xaa,0x19},
	{0xab,0x18},
	{0xae,0x50},
	{0xaf,0x04},
	{0xc8,0x10},
	{0xc9,0x15},
	{0xd3,0x0c},
	{0xd4,0x16},
	{0xee,0x06},
	{0xef,0x04},
	{0x55,0x34},
	{0x56,0x9c},
	{0xb1,0x98},
	{0xb2,0x98},
	{0xb3,0xc4},
	{0xb4,0x0c},
	{0xa0,0x8f},
	{0x13,0x07},
};

static camera_reg_t g_s_gc032a_reg_table[] =
{
    /*System*/
    {0xf3, 0x83}, //ff//1f//01 data output
    {0xf5, 0x08},
    {0xf7, 0x01},
    {0xf8, 0x01}, ////hai: from 0x4->0x1   pll-div----frank
    {0xf9, 0x4e},
    {0xfa, 0x00},
    {0xfc, 0x02},
    {0xfe, 0x02},
    {0x81, 0x03},

    {0xfe, 0x00},
    {0x77, 0x64},
    {0x78, 0x40},
    {0x79, 0x60},
    /*Analog&Cisctl*/
    {0xfe, 0x00},
    {0x03, 0x01},
    {0x04, 0xcb},
    {0x05, 0x01},
    {0x06, 0xb2},
    {0x07, 0x00},
    {0x08, 0x10},

    {0x0a, 0x00},
    {0x0c, 0x00},
    {0x0d, 0x01},
    {0x0e, 0xe8},
    {0x0f, 0x02},
    {0x10, 0x88},

    {0x17, 0x54},
    {0x19, 0x08},
    {0x1a, 0x0a},
    {0x1f, 0x40},
    {0x20, 0x30},
    {0x2e, 0x80},
    {0x2f, 0x2b},
    {0x30, 0x1a},
    {0xfe, 0x02},
    {0x03, 0x02},
    {0x05, 0xd7},
    {0x06, 0x60},
    {0x08, 0x80},
    {0x12, 0x89},

    /*SPI*/
    {0xfe, 0x03},
    {0x52, 0xba}, // hai: from 0xba -> 0x3A
    {0x53, 0x24},
    {0x54, 0x20},
    {0x55, 0x00},
    {0x59, 0x1f}, // {0x59,0x10}, 20190627 scaler output error
    {0x5a, 0x00}, //00 //yuv hai: from 0x40 to 0x00
    {0x5b, 0x80},
    {0x5c, 0x02},
    {0x5d, 0xe0},
    {0x5e, 0x01},
    {0x51, 0x03},
    {0x64, 0x04},
    {0xfe, 0x00},

    /*blk*/
    {0xfe, 0x00},
    {0x18, 0x02},
    {0xfe, 0x02},
    {0x40, 0x22},
    {0x45, 0x00},
    {0x46, 0x00},
    {0x49, 0x20},
    {0x4b, 0x3c},
    {0x50, 0x20},
    {0x42, 0x10},

    /*isp*/
    {0xfe, 0x01},
    {0x0a, 0xc5},
    {0x45, 0x00},
    {0xfe, 0x00},
    {0x40, 0xff},
    {0x41, 0x25},
    {0x42, 0xef},
    {0x43, 0x10},
    {0x44, 0x83}, // hai from 0x82 to 0x83
    {0x46, 0x22},
    {0x49, 0x03},
    {0x52, 0x02},
    {0x54, 0x00},
    {0xfe, 0x02},
    {0x22, 0xf6},

    /*Shading*/
    {0xfe, 0x01},
    {0xc1, 0x38},
    {0xc2, 0x4c},
    {0xc3, 0x00},
    {0xc4, 0x2c},
    {0xc5, 0x24},
    {0xc6, 0x18},
    {0xc7, 0x28},
    {0xc8, 0x11},
    {0xc9, 0x15},
    {0xca, 0x20},
    {0xdc, 0x7a},
    {0xdd, 0xa0},
    {0xde, 0x80},
    {0xdf, 0x88},

    /*AWB*/ /*20170110*/
    {0xfe, 0x01},
    {0x50, 0xc1},
    {0x56, 0x34},
    {0x58, 0x04},
    {0x65, 0x06},
    {0x66, 0x0f},
    {0x67, 0x04},
    {0x69, 0x20},
    {0x6a, 0x40},
    {0x6b, 0x81},
    {0x6d, 0x12},
    {0x6e, 0xc0},
    {0x7b, 0x2a},
    {0x7c, 0x0c},
    {0xfe, 0x01},
    {0x90, 0xe3},
    {0x91, 0xc2},
    {0x92, 0xff},
    {0x93, 0xe3},
    {0x95, 0x1c},
    {0x96, 0xff},
    {0x97, 0x44},
    {0x98, 0x1c},
    {0x9a, 0x44},
    {0x9b, 0x1c},
    {0x9c, 0x64},
    {0x9d, 0x44},
    {0x9f, 0x71},
    {0xa0, 0x64},
    {0xa1, 0x00},
    {0xa2, 0x00},
    {0x86, 0x00},
    {0x87, 0x00},
    {0x88, 0x00},
    {0x89, 0x00},
    {0xa4, 0xc2},
    {0xa5, 0x9b},
    {0xa6, 0xc8},
    {0xa7, 0x92},
    {0xa9, 0xc9},
    {0xaa, 0x96},
    {0xab, 0xa9},
    {0xac, 0x99},
    {0xae, 0xce},
    {0xaf, 0xa9},
    {0xb0, 0xcf},
    {0xb1, 0x9d},
    {0xb3, 0xcf},
    {0xb4, 0xac},
    {0xb5, 0x00},
    {0xb6, 0x00},
    {0x8b, 0x00},
    {0x8c, 0x00},
    {0x8d, 0x00},
    {0x8e, 0x00},
    {0x94, 0x55},
    {0x99, 0xa6},
    {0x9e, 0xaa},
    {0xa3, 0x0a},
    {0x8a, 0x00},
    {0xa8, 0x55},
    {0xad, 0x55},
    {0xb2, 0x55},
    {0xb7, 0x05},
    {0x8f, 0x00},
    {0xb8, 0xc7},
    {0xb9, 0xa0},

    {0xfe, 0x01},
    {0xd0, 0x40},
    {0xd1, 0x00},
    {0xd2, 0x00},
    {0xd3, 0xfa},
    {0xd4, 0x4a},
    {0xd5, 0x02},

    {0xd6, 0x44},
    {0xd7, 0xfa},
    {0xd8, 0x04},
    {0xd9, 0x08},
    {0xda, 0x5c},
    {0xdb, 0x02},
    {0xfe, 0x00},

    /*Gamma*/
    {0xfe, 0x00},
    {0xba, 0x00},
    {0xbb, 0x04},
    {0xbc, 0x0a},
    {0xbd, 0x0e},
    {0xbe, 0x22},
    {0xbf, 0x30},
    {0xc0, 0x3d},
    {0xc1, 0x4a},
    {0xc2, 0x5d},
    {0xc3, 0x6b},
    {0xc4, 0x7a},
    {0xc5, 0x85},
    {0xc6, 0x90},
    {0xc7, 0xa5},
    {0xc8, 0xb5},
    {0xc9, 0xc2},
    {0xca, 0xcc},
    {0xcb, 0xd5},
    {0xcc, 0xde},
    {0xcd, 0xea},
    {0xce, 0xf5},
    {0xcf, 0xff},

    /*Auto Gamma*/
    {0xfe, 0x00},
    {0x5a, 0x08},
    {0x5b, 0x0f},
    {0x5c, 0x15},
    {0x5d, 0x1c},
    {0x5e, 0x28},
    {0x5f, 0x36},
    {0x60, 0x45},
    {0x61, 0x51},
    {0x62, 0x6a},
    {0x63, 0x7d},
    {0x64, 0x8d},
    {0x65, 0x98},
    {0x66, 0xa2},
    {0x67, 0xb5},
    {0x68, 0xc3},
    {0x69, 0xcd},
    {0x6a, 0xd4},
    {0x6b, 0xdc},
    {0x6c, 0xe3},
    {0x6d, 0xf0},
    {0x6e, 0xf9},
    {0x6f, 0xff},

    /*Gain*/
    {0xfe, 0x00},
    {0x70, 0x50},

    /*AEC*/
    {0xfe, 0x00},
    {0x4f, 0x01},
    {0xfe, 0x01},
    {0x0c, 0x01},
    {0x0d, 0x00}, //08 add 20170110
    {0x12, 0xa0},
    {0x13, 0x38},//4a   3a  hai: from 0x28 to 0x38
    //{0x44, 0x04},
    {0x1f, 0x40},
    {0x20, 0x40},
    {0x23, 0x0a},
    {0x26, 0x9a},
    {0x3e, 0x20},
    {0x3f, 0x2d},
    {0x40, 0x40},
    {0x41, 0x5b},
    {0x42, 0x82},
    {0x43, 0xb7},
    {0x04, 0x0a},
    {0x02, 0x79},
    {0x03, 0xc0},

    /*measure window*/
    {0xfe, 0x01},
    {0xcc, 0x08},
    {0xcd, 0x08},
    {0xce, 0xa4},
    {0xcf, 0xec},

    /*DNDD*/
    {0xfe, 0x00},
    {0x81, 0xb8},
    {0x82, 0x04},
    {0x83, 0x10},
    {0x84, 0x01},
    {0x86, 0x50},
    {0x87, 0x18},
    {0x88, 0x10},
    {0x89, 0x70},
    {0x8a, 0x20},
    {0x8b, 0x10},
    {0x8c, 0x08},
    {0x8d, 0x0a},

    /*Intpee*/
    {0xfe, 0x00},
    {0x8f, 0xaa},
    {0x90, 0x1c},
    {0x91, 0x52},
    {0x92, 0x03},
    {0x93, 0x03},
    {0x94, 0x08},
    {0x95, 0x6a},
    {0x97, 0x00},
    {0x98, 0x00},

    /*ASDE*/
    {0xfe, 0x00},
    {0x9a, 0x30},
    {0x9b, 0x50},
    {0xa1, 0x30},
    {0xa2, 0x66},
    {0xa4, 0x28},
    {0xa5, 0x30},
    {0xaa, 0x28},
    {0xac, 0x32},

    /*YCP*/
    {0xfe, 0x00},
    {0xd1, 0x3f},
    {0xd2, 0x3f},
    {0xd3, 0x38},
    {0xd6, 0xf4},
    {0xd7, 0x1d},
    {0xdd, 0x72},
    {0xde, 0x84},

    {0xfe, 0x00},
    {0x05, 0x01},
    {0x06, 0xad},
    {0x07, 0x00},
    {0x08, 0x10},

    {0xfe, 0x01},
    {0x25, 0x00},
    {0x26, 0x4d},

    {0x27, 0x01},
    {0x28, 0xce}, //16.6fps
    {0x29, 0x01},
    {0x2a, 0xce}, //12.5fps
    {0x2b, 0x01},
    {0x2c, 0xce}, //10fps
    {0x2d, 0x01},
    {0x2e, 0xce}, //8.33fps
    {0x2f, 0x01},
    {0x30, 0xce}, //5.88fps
    {0x31, 0x01},
    {0x32, 0xce}, //4.34fps
    {0x33, 0x01},
    {0x34, 0xce}, //3.99fps
    {0x3c, 0x10}, //{0x3c,0x00}
    {0xfe, 0x00},
	{0x44, 0x03},	//透传到LCD能正常显示颜色需要YCrCb
};

#ifdef LCD_ENABLE
//CORE开发板上的配置
#define SPI_LCD_RST_PIN HAL_GPIO_36
#define SPI_LCD_BL_PIN HAL_GPIO_25	//不考虑低功耗的话，BL也可以省掉
#define SPI_LCD_W	240
#define SPI_LCD_H	320
#define SPI_LCD_X_OFFSET	0
#define SPI_LCD_Y_OFFSET	0

static luat_lcd_conf_t lcd_conf = {
    .port = LUAT_LCD_HW_ID_0,
    .opts = &lcd_opts_gc9306x,
	.pin_dc = 0xff,
    .pin_rst = SPI_LCD_RST_PIN,
    .pin_pwr = SPI_LCD_BL_PIN,
    .direction = 0,
    .w = SPI_LCD_W,
    .h = SPI_LCD_H,
    .xoffset = 0,
    .yoffset = 0,
	.interface_mode = LUAT_LCD_IM_4_WIRE_8_BIT_INTERFACE_I,
	.lcd_cs_pin = 0xff
};



#endif

static void lcd_camera_start_run(void)
{
	g_s_camera_app.cur_cache = 0;
	g_s_camera_app.rx_to_user = 0;
	luat_camera_start_with_buffer(CAMERA_SPI_ID, NULL);

}

#ifdef USB_UART_ENABLE
static void luat_usb_recv_cb(int uart_id, uint32_t data_len)
{
    char* data_buff = malloc(data_len);
    luat_uart_read(uart_id, data_buff, data_len);
    LUAT_DEBUG_PRINT("luat_uart_cb uart_id:%d data_len:%d",uart_id, data_len);
    free(data_buff);
}
#endif

static int luat_image_decode_callback(void *pdata, void *param)
{
	uint32_t buffer_sn = (uint32_t)param;
	uint8_t buf[2500];	//由于解码stack很大，才可以这么用
//	LUAT_DEBUG_PRINT("buffer %d decode done!", buffer_sn);
	if (pdata)
	{
		uint32_t len = (uint32_t)pdata;
		if (len < 2500)
		{
			buf[len] = 0;
			if (luat_camera_image_decode_get_result(buf) != 1)
			{
				LUAT_DEBUG_PRINT("解码结果获取失败");
			}
			else
			{
				LUAT_DEBUG_PRINT("解码结果 %s", buf);
			}

		}
		else
		{
			LUAT_DEBUG_PRINT("解码长度异常");
		}
	}
	else
	{
		LUAT_DEBUG_PRINT("解码失败");
	}

	g_s_camera_app.is_process_image = 0;
	return 0;
}

static int luat_camera_irq_callback(void *pdata, void *param)
{
	uint8_t cur_cache = g_s_camera_app.cur_cache;
	switch ((uint32_t)pdata)
	{
	case LUAT_CAMERA_FRAME_RX_DONE:
		if (g_s_camera_app.scan_mode)
		{
			if (g_s_camera_app.scan_pause)
			{
				if (g_s_camera_app.rx_to_user)
				{
					g_s_camera_app.rx_to_user = 0;
					luat_camera_continue_with_buffer(CAMERA_SPI_ID, 0);	//摄像头数据发送到底层，不传递给用户
				}
				luat_rtos_event_send(g_s_task_handle, CAMERA_FRAME_NEW, cur_cache, 0, 0, 0);
				return 0;
			}
			if (g_s_camera_app.double_buffer_mode)
			{
				if (!g_s_camera_app.rx_to_user)
				{
					luat_rtos_event_send(g_s_task_handle, CAMERA_FRAME_NEW, cur_cache, 0, 0, 0);
					luat_camera_continue_with_buffer(CAMERA_SPI_ID, g_s_camera_app.p_cache[0]);
					g_s_camera_app.rx_to_user = 1;
					return 0;
				}
				//双缓冲模式下，扫码时允许1个解码，另一个接收数据
				if (!g_s_camera_app.is_process_image)
				{
					g_s_camera_app.is_process_image = 1;
					luat_rtos_event_send(g_s_task_handle, CAMERA_FRAME_QR_DECODE, cur_cache, 0, 0, 0);
					g_s_camera_app.cur_cache = !g_s_camera_app.cur_cache;
					luat_camera_continue_with_buffer(CAMERA_SPI_ID, g_s_camera_app.p_cache[g_s_camera_app.cur_cache]);
				}
				else
				{
					luat_camera_continue_with_buffer(CAMERA_SPI_ID, g_s_camera_app.p_cache[g_s_camera_app.cur_cache]);
				}
			}
			else
			{
				if (!g_s_camera_app.is_process_image)
				{
					if (g_s_camera_app.rx_to_user)	//本次接收数据在用户区，则开始解码，并停止传递给用户区
					{
						luat_rtos_event_send(g_s_task_handle, CAMERA_FRAME_QR_DECODE, 0, 0, 0, 0);
						luat_camera_continue_with_buffer(CAMERA_SPI_ID, 0);	//摄像头数据发送到底层，不传递给用户
						g_s_camera_app.rx_to_user = 0;
						g_s_camera_app.is_process_image = 1;
					}
					else	//本次接收数据不在用户区，则开始传递新的图像数据到用户区
					{
						luat_camera_continue_with_buffer(CAMERA_SPI_ID, g_s_camera_app.p_cache[0]);
						g_s_camera_app.rx_to_user = 1;
					}
				}
			}
		}
		else if (g_s_camera_app.capture_stage && !g_s_camera_app.is_process_image)
		{
			switch (g_s_camera_app.capture_stage)
			{
			case 1:
				luat_camera_continue_with_buffer(CAMERA_SPI_ID, g_s_camera_app.p_cache[0]);
				g_s_camera_app.capture_stage = 2;
				break;
			case 2:
				luat_rtos_event_send(g_s_task_handle, CAMERA_FRAME_JPEG_ENCODE, 0, 0, 0, 0);
				luat_camera_continue_with_buffer(CAMERA_SPI_ID, 0);	//摄像头数据发送到底层，不传递给用户
				g_s_camera_app.is_process_image = 1;
				break;
			default:
				break;
			}
		}
		else
		{
			luat_rtos_event_send(g_s_task_handle, CAMERA_FRAME_NEW, cur_cache, 0, 0, 0);
		}
		break;
	case LUAT_CAMERA_FRAME_ERROR:
		luat_rtos_event_send(g_s_task_handle, CAMERA_FRAME_ERROR, 0, 0, 0, 0);
		break;
	}
	return 0;
}

static int luat_bfxxxx_init(void)
{
	uint8_t id[2];
	luat_spi_camera_t spi_camera =
	{
			.camera_speed = CAMERA_SPEED,
#ifdef CAMERA_TEST_QRCODE
			.only_y = 1,
#else
			.only_y = 0,
#endif
			.sensor_width = 240,
			.sensor_height = 320,
			.rowScaleRatio = 0,
			.colScaleRatio  = 0,
			.scaleBytes = 0,
			.spi_mode = SPI_MODE_0,
			.is_msb = 0,
			.is_two_line_rx = 0,
			.seq_type = 0,
			.plat_param = {0,0,0,0}
	};
	luat_gpio_set(CAMERA_PD_PIN, LUAT_GPIO_LOW);
	luat_camera_setup(CAMERA_SPI_ID, &spi_camera, luat_camera_irq_callback, NULL);	//输出MCLK供给camera时钟
	luat_rtos_task_sleep(1);
    luat_i2c_setup(0, 1);
    luat_i2c_setup(1, 1);

#ifdef CAMERA_POWER_PIN
	luat_gpio_set(CAMERA_POWER_PIN, LUAT_GPIO_HIGH);
#endif
	luat_rtos_task_sleep(1);
	id[0] = 0xfc;
	if (luat_i2c_transfer(0, BF30A2_I2C_ADDRESS, id, 1, id, 2))
	{
		if (luat_i2c_transfer(1, BF30A2_I2C_ADDRESS, id, 1, id, 2))
		{
			LUAT_DEBUG_PRINT("BF30A2 not i2c response");
			goto CAM_OPEN_FAIL;
		}
		else
		{
			DBG("find i2c device in i2c1");
			g_s_camera_app.camera_id = 1;
		}

	}
	else
	{
		DBG("find i2c device in i2c0");
		g_s_camera_app.camera_id = 0;
	}
	if (id[0] == 0x3b || id[1] == 0x02)
	{
		LUAT_DEBUG_PRINT("find BF30A2");
		g_s_camera_app.image_w = 240;
		g_s_camera_app.image_h = 320;
		g_s_camera_app.double_buffer_mode = 1;	//8W扫码才可以双buffer
		for(int i = 0; i < sizeof(g_s_bf302a_reg_table)/sizeof(camera_reg_t); i++)
		{
			if (luat_i2c_send(CAMERA_I2C_ID, BF30A2_I2C_ADDRESS, &g_s_bf302a_reg_table[i].reg, 2, 1))
			{
				LUAT_DEBUG_PRINT("write %d %x,%x failed", i, g_s_bf302a_reg_table[i].reg, g_s_bf302a_reg_table[i].data);
				goto CAM_OPEN_FAIL;
			}
		}
	}
//	else if (id[0] == 0x20 || id[1] == 0xa6)
//	{
//		LUAT_DEBUG_PRINT("find BF20A6");
//		spi_camera.sensor_width = 640;
//		spi_camera.sensor_height = 480;
//		g_s_camera_app.image_w = 640;
//		g_s_camera_app.image_h = 480;
//		spi_camera.is_two_line_rx = 1;
//		spi_camera.spi_mode = 0;
//		for(int i = 0; i < sizeof(g_s_bf206a_reg_table)/sizeof(camera_reg_t); i++)
//		{
//			if (luat_i2c_send(CAMERA_I2C_ID, BF30A2_I2C_ADDRESS, &g_s_bf206a_reg_table[i].reg, 2, 1))
//			{
//				LUAT_DEBUG_PRINT("write %d %x,%x failed", i, g_s_bf206a_reg_table[i].reg, g_s_bf206a_reg_table[i].data);
//				goto CAM_OPEN_FAIL;
//			}
//		}
//	}
	else
	{
		LUAT_DEBUG_PRINT("unknow id %x,%x", id[0], id[1]);
		goto CAM_OPEN_FAIL;
	}
	luat_camera_set_image_w_h(CAMERA_SPI_ID, g_s_camera_app.image_w, g_s_camera_app.image_h);
#ifdef PSRAM_EXIST
    g_s_camera_app.p_cache[0] = luat_heap_opt_malloc(LUAT_HEAP_PSRAM, g_s_camera_app.image_w * g_s_camera_app.image_h * 2);
    g_s_camera_app.p_cache[1] = luat_heap_opt_malloc(LUAT_HEAP_PSRAM, g_s_camera_app.image_w * g_s_camera_app.image_h * 2);
    LUAT_DEBUG_PRINT("psram use %x,%x",g_s_camera_app.p_cache[0], g_s_camera_app.p_cache[1]);
#else
    while(1)
    {
    	LUAT_DEBUG_PRINT("ram not enough");
    	luat_rtos_task_sleep(5000);
    }
#endif
    lcd_camera_start_run();
#if (defined LCD_ENABLE)
	g_s_camera_app.camera = spi_camera;
	g_s_camera_app.camera.lcd_conf = &lcd_conf;
	//camera_cut_info_t cut = {60, 60, 20, 20, 0, 0};
	//luat_lcd_show_camera_in_service(&g_s_camera_app.camera, &cut, 19, 59);
	luat_lcd_show_camera_in_service(&g_s_camera_app.camera, NULL, 0, 0);
#endif
	return 0;
CAM_OPEN_FAIL:
	luat_camera_close(CAMERA_SPI_ID);
	luat_i2c_close(CAMERA_I2C_ID);
#ifdef CAMERA_POWER_PIN
	luat_gpio_set(CAMERA_POWER_PIN, LUAT_GPIO_LOW);
#endif
	return -1;
}

static int luat_gc032a_init(void)
{
	uint8_t id[2];

	luat_spi_camera_t spi_camera =
	{
			.camera_speed = CAMERA_SPEED,
#ifdef CAMERA_TEST_QRCODE
			.only_y = 1,
#else
			.only_y = 0,
#endif

			.sensor_width = 640,
			.sensor_height = 480,
			.rowScaleRatio = 0,
			.colScaleRatio  = 0,
			.scaleBytes = 0,
			.spi_mode = SPI_MODE_1,
			.is_msb = 1,
			.is_two_line_rx = 1,
			.seq_type = 1,
			.plat_param = {1,1,1,0}
	};
	g_s_camera_app.double_buffer_mode = 0;
	luat_camera_setup(CAMERA_SPI_ID, &spi_camera, luat_camera_irq_callback, NULL);	//输出MCLK供给camera时钟
	luat_rtos_task_sleep(1);
    luat_i2c_setup(0, 1);
    luat_i2c_setup(1, 1);

#ifdef CAMERA_POWER_PIN
	luat_gpio_set(CAMERA_POWER_PIN, LUAT_GPIO_HIGH);
#endif
	luat_rtos_task_sleep(1);
	id[0] = 0xfc;
	if (luat_i2c_transfer(0, GC032A_I2C_ADDR, id, 1, id, 2))
	{
		if (luat_i2c_transfer(1, GC032A_I2C_ADDR, id, 1, id, 2))
		{
			LUAT_DEBUG_PRINT("gc032a not i2c response");
			goto CAM_OPEN_FAIL;
		}
		else
		{
			DBG("find i2c device in i2c1");
			g_s_camera_app.camera_id = 1;
		}

	}
	else
	{
		DBG("find i2c device in i2c0");
		g_s_camera_app.camera_id = 0;
	}
	DBG("%x,%x", id[0], id[1]);
	for(int i = 0; i < sizeof(g_s_gc032a_reg_table)/sizeof(camera_reg_t); i++)
	{
		if (luat_i2c_send(CAMERA_I2C_ID, GC032A_I2C_ADDR, &g_s_gc032a_reg_table[i].reg, 2, 1))
		{
			LUAT_DEBUG_PRINT("write %d %x,%x failed", i, g_s_gc032a_reg_table[i].reg, g_s_gc032a_reg_table[i].data);
			goto CAM_OPEN_FAIL;
		}
	}
	g_s_camera_app.image_w = 640;
	g_s_camera_app.image_h = 480;
	luat_camera_set_image_w_h(CAMERA_SPI_ID, g_s_camera_app.image_w, g_s_camera_app.image_h);
#ifdef PSRAM_EXIST
    g_s_camera_app.p_cache[0] = luat_heap_opt_malloc(LUAT_HEAP_PSRAM, g_s_camera_app.image_w * g_s_camera_app.image_h * 2);
    g_s_camera_app.p_cache[1] = luat_heap_opt_malloc(LUAT_HEAP_PSRAM, g_s_camera_app.image_w * g_s_camera_app.image_h * 2);
    LUAT_DEBUG_PRINT("psram use %x,%x",g_s_camera_app.p_cache[0], g_s_camera_app.p_cache[1]);
#else
    while(1)
    {
    	LUAT_DEBUG_PRINT("ram not enough");
    	luat_rtos_task_sleep(5000);
    }
#endif

    lcd_camera_start_run();
#if (defined LCD_ENABLE)
	g_s_camera_app.camera = spi_camera;
	g_s_camera_app.camera.lcd_conf = &lcd_conf;
	camera_cut_info_t cut = {80, 80, 200, 200, 0, 0};	// 640 * 480 在240 * 320居中显示
	luat_lcd_show_camera_in_service(&g_s_camera_app.camera, &cut, 0, 0);
#endif
	return 0;
CAM_OPEN_FAIL:
	luat_camera_close(CAMERA_SPI_ID);
	luat_i2c_close(CAMERA_I2C_ID);
#ifdef CAMERA_POWER_PIN
	luat_gpio_set(CAMERA_POWER_PIN, LUAT_GPIO_LOW);
#endif
	return -1;
}

static void luat_camera_save_JPEG_data(void *cxt, void *data, int size)
{
	uint8_t *p = g_s_camera_app.p_cache[1];
	memcpy(p + g_s_camera_app.jpeg_data_point, data, size);
	g_s_camera_app.jpeg_data_point += size;
}
int gpio_level_irq(void *data, void* args)
{

	if (!g_s_camera_app.scan_mode && !g_s_camera_app.capture_stage && !g_s_camera_app.is_process_image)
	{
		g_s_camera_app.capture_stage = 1;
	}
	else if (g_s_camera_app.scan_mode)
	{
		luat_rtos_event_send(g_s_task_handle, PIN_PRESS, 0, 0, 0, 0);
	}
}

static void luat_camera_task(void *param)
{
	luat_event_t event;
	void *stack = NULL;
	uint32_t all,now_used_block,max_used_block;
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG);
#ifdef LCD_ENABLE
	luat_gpio_cfg_t gpio_cfg;
	luat_lcd_service_init(60);
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.output_level = LUAT_GPIO_HIGH;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.pin = SPI_LCD_RST_PIN;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.pin = SPI_LCD_BL_PIN;
	luat_gpio_open(&gpio_cfg);

	gpio_cfg.pin = HAL_GPIO_0;
	gpio_cfg.mode = LUAT_GPIO_IRQ;
	gpio_cfg.irq_type = LUAT_GPIO_RISING_IRQ;
	gpio_cfg.pull = LUAT_GPIO_PULLDOWN;
	gpio_cfg.irq_cb = gpio_level_irq;
	luat_gpio_open(&gpio_cfg);

    luat_lcd_IF_init(&lcd_conf);
    luat_lcd_init_in_service(&lcd_conf);
#endif
#ifdef USB_UART_ENABLE
    luat_uart_t uart = {
        .id = LUAT_VUART_ID_0,
    };
    luat_uart_setup(&uart);
    luat_uart_ctrl(LUAT_VUART_ID_0, LUAT_UART_SET_RECV_CALLBACK, luat_usb_recv_cb);
#endif
#ifdef CAMERA_TEST_QRCODE
    g_s_camera_app.scan_mode = 1;
    g_s_camera_app.scan_pause = 1;
    LUAT_DEBUG_PRINT("按下boot开始扫码");
#else
    LUAT_DEBUG_PRINT("按下boot拍照");
#endif
#ifdef CAMERA_USE_GC032A
    // GC032A初始化流程，如果有低功耗休眠要求，在唤醒后需要重新走一遍
    if (luat_gc032a_init())
    {
    	while(1)
    	{
    		LUAT_DEBUG_PRINT("no find camera gc032a, test stop");
    		luat_rtos_task_sleep(5000);
    	}
    }
#endif

#ifdef CAMERA_USE_BFXXXX
    luat_pm_iovolt_ctrl(0, 3200);
    // BF20a6 or BF30a2初始化流程，如果有低功耗休眠要求，在唤醒后需要重新走一遍
    if (luat_bfxxxx_init())
    {
    	while(1)
    	{
    		LUAT_DEBUG_PRINT("no find camera bf20a6 or bf30a2, test stop");
    		luat_rtos_task_sleep(5000);
    	}
    }
#endif



    HANDLE JPEGEncodeHandle = NULL;
    uint8_t *ycb_cache = malloc(g_s_camera_app.image_w * 8 * 3);
    uint8_t *file_data;
    uint8_t *p_cache;
    uint32_t i,j,k;
    int error;
	luat_meminfo_sys(&all, &now_used_block, &max_used_block);
	LUAT_DEBUG_PRINT("meminfo %d,%d,%d",all,now_used_block,max_used_block);
	while(1)
	{
		error = luat_rtos_event_recv(g_s_task_handle, 0, &event, NULL, 1000);
		if (error)
		{
			LUAT_DEBUG_PRINT("没有接收到摄像头数据！！！");
			continue;
		}
		switch(event.id)
		{
		case CAMERA_FRAME_NEW:
			break;
		case CAMERA_FRAME_QR_DECODE:
			LUAT_DEBUG_PRINT("解码开始 buf%d", event.param1);
			luat_camera_image_decode_once(g_s_camera_app.p_cache[event.param1], g_s_camera_app.image_w, g_s_camera_app.image_h, 60, luat_image_decode_callback, event.param1);
			break;
		case CAMERA_FRAME_JPEG_ENCODE:
			p_cache = g_s_camera_app.p_cache[0];
			g_s_camera_app.jpeg_data_point = 0;
			LUAT_DEBUG_PRINT("转JPEG开始 ");
			JPEGEncodeHandle = jpeg_encode_init(luat_camera_save_JPEG_data, 0, 1, g_s_camera_app.image_w, g_s_camera_app.image_h, 3);
			for(i = 0; i < g_s_camera_app.image_h; i+= 8)
			{
#ifdef PIC_ONLY_Y
				for(j = i * g_s_camera_app.image_w , k = 0; j < (i + 8) * g_s_camera_app.image_w; j+=2, k+=6)
				{
					ycb_cache[k] = p_cache[j];
					ycb_cache[k + 1] = 128;
					ycb_cache[k + 2] = 128;
					ycb_cache[k + 3] = p_cache[j + 1];
					ycb_cache[k + 4] = 128;
					ycb_cache[k + 5] = 128;
				}
#else
				for(j = i * g_s_camera_app.image_w * 2, k = 0; j < (i + 8) * g_s_camera_app.image_w * 2; j+=4, k+=6)
				{
					ycb_cache[k] = p_cache[j];
					ycb_cache[k + 1] = p_cache[j + 1];
					ycb_cache[k + 2] = p_cache[j + 3];
					ycb_cache[k + 3] = p_cache[j + 2];
					ycb_cache[k + 4] = p_cache[j + 1];
					ycb_cache[k + 5] = p_cache[j + 3];
				}
#endif
				jpeg_encode_run(JPEGEncodeHandle, ycb_cache);
			}

			jpeg_encode_end(JPEGEncodeHandle);
			free(JPEGEncodeHandle);
			LUAT_DEBUG_PRINT("转JPEG完成，大小%ubyte", g_s_camera_app.jpeg_data_point);
			luat_uart_write(LUAT_VUART_ID_0, g_s_camera_app.p_cache[1], g_s_camera_app.jpeg_data_point);
			luat_rtos_task_sleep(1000);
			g_s_camera_app.is_process_image = 0;
			g_s_camera_app.capture_stage = 0;
			luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &all, &now_used_block, &max_used_block);
			LUAT_DEBUG_PRINT("sram %d,%d,%d",all,now_used_block,max_used_block);
			luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &all, &now_used_block, &max_used_block);
			LUAT_DEBUG_PRINT("prsam %d,%d,%d",all,now_used_block,max_used_block);
			LUAT_DEBUG_PRINT("按下boot拍照");
			break;
		case PIN_PRESS:
			if (g_s_camera_app.scan_pause)
			{
				stack = luat_heap_opt_malloc(LUAT_HEAP_AUTO, 250 * 1024);
				luat_camera_image_decode_init(0, stack, 250 * 1024, 10);
				LUAT_DEBUG_PRINT("扫码启动完成，按下boot关闭扫码");
				g_s_camera_app.scan_pause = 0;
				luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &all, &now_used_block, &max_used_block);
				LUAT_DEBUG_PRINT("sram %d,%d,%d",all,now_used_block,max_used_block);
				luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &all, &now_used_block, &max_used_block);
				LUAT_DEBUG_PRINT("prsam %d,%d,%d",all,now_used_block,max_used_block);
			}
			else
			{
				LUAT_DEBUG_PRINT("开始关闭扫码");
				luat_camera_image_decode_deinit();
				luat_heap_free(stack);
				g_s_camera_app.scan_pause = 1;
				LUAT_DEBUG_PRINT("关闭扫码完成，按下boot启动扫码");
				luat_meminfo_opt_sys(LUAT_HEAP_SRAM, &all, &now_used_block, &max_used_block);
				LUAT_DEBUG_PRINT("sram %d,%d,%d",all,now_used_block,max_used_block);
				luat_meminfo_opt_sys(LUAT_HEAP_PSRAM, &all, &now_used_block, &max_used_block);
				LUAT_DEBUG_PRINT("prsam %d,%d,%d",all,now_used_block,max_used_block);
			}
			break;
		case CAMERA_FRAME_ERROR:
			LUAT_DEBUG_PRINT("camera spi error!");
			break;
		}
	}
}

static void camera_demo_init(void)
{
	luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pin = CAMERA_PD_PIN;
	gpio_cfg.output_level = LUAT_GPIO_LOW;
	luat_gpio_open(&gpio_cfg);
#ifdef CAMERA_POWER_PIN
	gpio_cfg.pin = CAMERA_POWER_PIN;
	gpio_cfg.output_level = LUAT_GPIO_LOW;
	gpio_cfg.alt_fun = CAMERA_POWER_PIN_ALT;
#endif
	luat_gpio_open(&gpio_cfg);

#ifdef TYPE_EC718P
	luat_rtos_task_create(&g_s_task_handle, 6 * 1024, 50, "camera", luat_camera_task, NULL, 64);
#endif
}

INIT_TASK_EXPORT(camera_demo_init, "1");

