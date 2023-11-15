#include "luat_base.h"
#include "luat_lcd.h"
#include "common_api.h"
#include "driver_usp.h"
#include "driver_gpio.h"

typedef struct
{
	uint64_t wait_bytes;
	uint64_t done_bytes;
	HANDLE task_handle;
}lcd_service_t;

static lcd_service_t g_s_lcd;

typedef struct
{
	luat_lcd_conf_t* conf;
	void *static_buf;
	int16_t x1;
	int16_t y1;
	int16_t x2;
	int16_t y2;
	uint32_t size;
	uint32_t data[0];
}lcd_service_draw_t;

enum
{
	SERVICE_LCD_DRAW = SERVICE_EVENT_ID_START + 1,
	SERVICE_RUN_USER_API,
};

LUAT_WEAK int luat_lcd_draw(luat_lcd_conf_t* conf, int16_t x1, int16_t y1, int16_t x2, int16_t y2, luat_color_t* color)
{
    return 0;
}

static void prvLCD_Task(void* params)
{
	OS_EVENT event;

	CBDataFun_t callback;
	lcd_service_draw_t *draw;
	uint8_t *data;
	uint8_t is_static_buf;
	while(1)
	{
		get_event_from_task(g_s_lcd.task_handle, CORE_EVENT_ID_ANY, &event, NULL, 0xffffffff);
		switch(event.ID)
		{
		case SERVICE_LCD_DRAW:

			draw = (lcd_service_draw_t *)event.Param1;

			if (draw->static_buf)
			{
				data = (uint8_t *)draw->static_buf;
				is_static_buf = 1;
			}
			else
			{
				data = (uint8_t *)draw->data;
				is_static_buf = 0;
			}

			if (draw->conf->port != LUAT_LCD_HW_INFERFACE_ID)
			{
				luat_lcd_draw(draw->conf, draw->x1, draw->y1, draw->x2, draw->y2, data);
			}
			else
			{
				luat_lcd_IF_draw(draw->conf, draw->x1, draw->y1, draw->x2, draw->y2, data);

			}
			free(draw);
			if (!is_static_buf)
			{
				g_s_lcd.done_bytes += draw->size;
			}
//			DBG("%llu,%llu",g_s_lcd.wait_bytes, g_s_lcd.done_bytes);
			break;

		case SERVICE_RUN_USER_API:
			callback = (CBDataFun_t)event.Param1;
			callback((uint8_t *)event.Param2, event.Param3);
			break;

		}
	}
}



void luat_lcd_service_init(uint32_t pro)
{
	if (!g_s_lcd.task_handle)
	{
		g_s_lcd.task_handle = create_event_task(prvLCD_Task, NULL, 2048, pro, 0, "lcdSer");
	}
}

void luat_lcd_service_run(void *CB, void *data, uint32_t param, uint32_t timeout)
{
	send_event_to_task(g_s_lcd.task_handle, NULL, SERVICE_RUN_USER_API, (uint32_t)CB, data, param, timeout);
}

int luat_lcd_service_draw(luat_lcd_conf_t* conf, int16_t x1, int16_t y1, int16_t x2, int16_t y2, luat_color_t *data, uint8_t is_static_buf)
{
	lcd_service_draw_t *draw;
	uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1) * sizeof(luat_color_t);
	if (is_static_buf)
	{
		draw = malloc(sizeof(lcd_service_draw_t));
		if (!draw) return -ERROR_NO_MEMORY;
		draw->static_buf = data;
	}
	else
	{

		draw = malloc(sizeof(lcd_service_draw_t) + size);
		if (!draw) return -ERROR_NO_MEMORY;
		memcpy(draw->data, data, size);
		draw->static_buf = 0;
		g_s_lcd.wait_bytes += size;
	}
	draw->conf = conf;
	draw->x1 = x1;
	draw->y1 = y1;
	draw->x2 = x2;
	draw->y2 = y2;
	draw->size = size;
	send_event_to_task(g_s_lcd.task_handle, NULL, SERVICE_LCD_DRAW, (uint32_t)draw, 0, 0, 0);

	return ERROR_NONE;
}

uint32_t luat_lcd_service_cache_len(void)
{
	return (uint32_t)(g_s_lcd.wait_bytes - g_s_lcd.done_bytes);
}

void luat_lcd_IF_init(luat_lcd_conf_t* conf)
{
	GPIO_IomuxEC7XX(40, 1, 0, 0);
	if (conf->lcd_cs_pin != 0xff)
	{

	}
	else
	{
		GPIO_IomuxEC7XX(41, 1, 0, 0);
	}
	GPIO_IomuxEC7XX(43, 1, 0, 0);
	GPIO_IomuxEC7XX(44, 2, 0, 0);

	LSPI_Setup(USP_ID2, 0, conf->interface_mode, NULL, 0);
	conf->opts->write_cmd_data = luat_lcd_IF_write_cmd_data;
	conf->opts->read_cmd_data = luat_lcd_IF_read_cmd_data;
	conf->opts->lcd_draw = luat_lcd_IF_draw;
}
int luat_lcd_IF_write_cmd_data(luat_lcd_conf_t* conf,const uint8_t cmd, const uint8_t *data, uint8_t data_len)
{
	GPIO_Output(conf->lcd_cs_pin, 0);
	int res = LSPI_WriteCmd(USP_ID2, cmd, data, data_len);
	GPIO_Output(conf->lcd_cs_pin, 1);
	return res;
}

int luat_lcd_IF_read_cmd_data(luat_lcd_conf_t* conf,const uint8_t cmd, uint8_t *data, uint8_t data_len, uint8_t dummy_bit)
{
	GPIO_Output(conf->lcd_cs_pin, 0);
	int res = LSPI_ReadData(USP_ID2, cmd, data, data_len, dummy_bit);
	GPIO_Output(conf->lcd_cs_pin, 1);
	return res;
}

int luat_lcd_IF_draw(luat_lcd_conf_t* conf, int16_t x1, int16_t y1, int16_t x2, int16_t y2, luat_color_t* color)
{
	uint32_t size,w,h;
	uint8_t *dummy = NULL;
	uint32_t temp[16];
	int res;
    uint8_t data_x[] = {(x1+conf->xoffset)>>8,x1+conf->xoffset,(x2+conf->xoffset)>>8,x2+conf->xoffset};
    luat_lcd_IF_write_cmd_data(conf,0x2a, data_x, 4);
    uint8_t data_y[] = {(y1+conf->yoffset)>>8,y1+conf->yoffset,(y2+conf->yoffset)>>8,y2+conf->yoffset};
    luat_lcd_IF_write_cmd_data(conf,0x2b, data_y, 4);


	w = x2 - x1 + 1;
	h = y2 - y1 + 1;
	size = w * h * sizeof(luat_color_t);

	if ((uint32_t)color & 0x03)
	{
		if (size <= sizeof(temp))
		{
			memcpy(temp, color, size);
			GPIO_Output(conf->lcd_cs_pin, 0);
			LSPI_WriteImageData(USP_ID2, w, h, (uint32_t)temp, size, 1);
			GPIO_Output(conf->lcd_cs_pin, 1);

		}
		else
		{
			dummy = (uint8_t *)malloc(size + 4);
			memcpy(dummy, color, size);
			GPIO_Output(conf->lcd_cs_pin, 0);
			LSPI_WriteImageData(USP_ID2, w, h, (uint32_t)dummy, size, 1);
			GPIO_Output(conf->lcd_cs_pin, 1);
			free(dummy);
		}
	}
	else
	{
		GPIO_Output(conf->lcd_cs_pin, 0);
		LSPI_WriteImageData(USP_ID2, w, h, (uint32_t)color, size, 1);
		GPIO_Output(conf->lcd_cs_pin, 1);
	}

	return 0;
}
