#include "luat_base.h"
#include "luat_lcd.h"
#include "common_api.h"
#include "driver_usp.h"
#include "driver_gpio.h"
#include "cmsis_gcc.h"

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

#define __SWAP_RB(value) ((value & 0x07e0) | (value >> 11) | (value << 11))

#ifdef LUAT_USE_LCD_CUSTOM_DRAW
int luat_lcd_draw2(luat_lcd_conf_t* conf, int16_t x1, int16_t y1, int16_t x2, int16_t y2, luat_color_t* color) {
    if (x1 >= conf->w || y1 >= conf->h || x2 < 0 || y2 < 0 || x2 < x1 || y2 < y1) {
        // LLOGE("out of lcd buff range %d %d %d %d", x1, y1, x2, y2);
        // LLOGE("out of lcd buff range %d %d %d %d %d", x1 >= conf->w, y1 >= conf->h, y2 < 0, x2 < x1, y2 < y1);
        return 0;
    }
    if (y2 >= conf->h) {
        y2 = conf->h - 1;
    }
    if (conf->opts->lcd_draw)
    	return conf->opts->lcd_draw(conf, x1, y1, x2, y2, color);
    // 直接刷屏模式
    if (conf->buff == NULL) {
        // 常规数据, 整体传输
        if (x1 >= 0 && y1 >= 0 && x2 <= conf->w && y2 <= conf->h) {
            uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1);
            // LLOGD("draw %dx%d %dx%d %d", x1, y1, x2, y2, size);
            luat_lcd_set_address(conf, x1, y1, x2, y2);
	        if (conf->port == LUAT_LCD_SPI_DEVICE){
		        luat_spi_device_send((luat_spi_device_t*)(conf->lcd_spi_device), (const char*)color, size* sizeof(luat_color_t));
	        }else{
		        luat_spi_send(conf->port, (const char*)color, size * sizeof(luat_color_t));
	        }
        }
        // 超出边界的数据, 按行传输
        else {
            int line_size = (x2 - x1 + 1);
            // LLOGD("want draw %dx%d %dx%d %d", x1, y1, x2, y2, line_size);
            luat_color_t* ptr = (luat_color_t*)color;
            for (int i = y1; i <= y2; i++)
            {
                if (i < 0) {
                    ptr += line_size;
                    continue;
                }
                luat_color_t* line = ptr;
                int lsize = line_size;
                int tmp_x1 = x1;
                int tmp_x2 = x2;
                if (x1 < 0) {
                    line += ( - x1);
                    lsize += (x1);
                    tmp_x1 = 0;
                }
                if (x2 > conf->w) {
                    lsize -= (x2 - conf->w);
                    tmp_x2 = conf->w;
                }
                // LLOGD("action draw %dx%d %dx%d %d", tmp_x1, i, tmp_x2, i, lsize);
                luat_lcd_set_address(conf, tmp_x1, i, tmp_x2, i);
	            if (conf->port == LUAT_LCD_SPI_DEVICE){
		            luat_spi_device_send((luat_spi_device_t*)(conf->lcd_spi_device), (const char*)line, lsize * sizeof(luat_color_t));
	            }else{
		            luat_spi_send(conf->port, (const char*)line, lsize * sizeof(luat_color_t));
	            }
                ptr += line_size;
            }
            
            // TODO
            // LLOGD("超出边界,特殊处理");
        }
        return 0;
    }
    // buff模式
    int16_t x_end = x2 >= conf->w?  (conf->w - 1):x2;
    luat_color_t* dst = (conf->buff);
    size_t lsize = (x2 - x1 + 1);
    for (int16_t x = x1; x <= x2; x++)
    {
        if (x < 0 || x >= conf->w)
            continue;
        for (int16_t y = y1; y <= y2; y++)
        {
            if (y < 0 || y >= conf->h)
                continue;
            memcpy((char*)(dst + (conf->w * y + x)), (char*)(color + (lsize * (y-y1) + (x-x1))), sizeof(luat_color_t));
        }
    }
    // 存储需要刷新的区域
    if (y1 < conf->flush_y_min) {
        if (y1 >= 0)
            conf->flush_y_min = y1;
        else
            conf->flush_y_min = 0;
    }
    if (y2 > conf->flush_y_max) {
        conf->flush_y_max = y2;
    }
    return 0;
}
#endif

static void prvLCD_Task(void* params)
{
	OS_EVENT event;
	uint32_t size;
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
			
#ifdef LUAT_USE_LCD_CUSTOM_DRAW
	luat_lcd_draw2(draw->conf, draw->x1, draw->y1, draw->x2, draw->y2, data);
#else
	luat_lcd_draw(draw->conf, draw->x1, draw->y1, draw->x2, draw->y2, data);
#endif
			size = draw->size;
			free(draw);
			if (!is_static_buf)
			{
				g_s_lcd.done_bytes += size;
				if (g_s_lcd.done_bytes > g_s_lcd.wait_bytes)
				{
					g_s_lcd.done_bytes = g_s_lcd.wait_bytes;
				}
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

uint32_t luat_lcd_service_debug(void)
{
	DBG("%llu, %llu", g_s_lcd.wait_bytes , g_s_lcd.done_bytes);
}

#include "luat_debug.h"
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
#ifdef LUAT_USE_LCD_CUSTOM_DRAW
	luat_lcd_service_init(60);
#endif
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
	uint32_t size,w,h,points,i;
	PV_Union dummy;
	uint32_t temp[16];
	int res;
    uint8_t data_x[] = {(x1+conf->xoffset)>>8,x1+conf->xoffset,(x2+conf->xoffset)>>8,x2+conf->xoffset};
    luat_lcd_IF_write_cmd_data(conf,0x2a, data_x, 4);
    uint8_t data_y[] = {(y1+conf->yoffset)>>8,y1+conf->yoffset,(y2+conf->yoffset)>>8,y2+conf->yoffset};
    luat_lcd_IF_write_cmd_data(conf,0x2b, data_y, 4);


	w = x2 - x1 + 1;
	h = y2 - y1 + 1;
	size = w * h * sizeof(luat_color_t);
	points = w * h;
	if ((uint32_t)color & 0x03)
	{
		if (size <= sizeof(temp))
		{
			dummy.pu32 = temp;
			for(i = 0; i < points; i++)
			{
				dummy.pu16[i] = __SWAP_RB(color[i]);
			}
			GPIO_Output(conf->lcd_cs_pin, 0);
			LSPI_WriteImageData(USP_ID2, w, h, (uint32_t)temp, size, 1);
			GPIO_Output(conf->lcd_cs_pin, 1);

		}
		else
		{
			dummy.pu8 = (uint8_t *)malloc(size + 4);
			for(i = 0; i < points; i++)
			{
				dummy.pu16[i] = __SWAP_RB(color[i]);
			}
			GPIO_Output(conf->lcd_cs_pin, 0);
			LSPI_WriteImageData(USP_ID2, w, h, dummy.u32, size, 1);
			GPIO_Output(conf->lcd_cs_pin, 1);
			free(dummy.p);
		}
	}
	else
	{
		for(i = 0; i < points; i++)
		{
			color[i] = __SWAP_RB(color[i]);
		}
		GPIO_Output(conf->lcd_cs_pin, 0);
		LSPI_WriteImageData(USP_ID2, w, h, (uint32_t)color, size, 1);
		GPIO_Output(conf->lcd_cs_pin, 1);
	}

	return 0;
}


#ifdef LUAT_USE_LCD_CUSTOM_DRAW
int luat_lcd_flush(luat_lcd_conf_t* conf) {
    if (conf->buff == NULL) {
        return 0;
    }
    //LLOGD("luat_lcd_flush range %d %d", conf->flush_y_min, conf->flush_y_max);
    if (conf->flush_y_max < conf->flush_y_min) {
        // 没有需要刷新的内容,直接跳过
        //LLOGD("luat_lcd_flush no need");
        return 0;
    }
    const char* tmp = (const char*)(conf->buff + conf->flush_y_min * conf->w);
	while (luat_lcd_service_draw(conf, 0, conf->flush_y_min, conf->w - 1, conf->flush_y_max, tmp, 0)){
		luat_rtos_task_sleep(5);
    }
    // 重置为不需要刷新的状态
    conf->flush_y_max = 0;
    conf->flush_y_min = conf->h;
    
    return 0;
}

int luat_lcd_draw(luat_lcd_conf_t* conf, int16_t x1, int16_t y1, int16_t x2, int16_t y2, luat_color_t* color) {
    if (x1 >= conf->w || y1 >= conf->h || x2 < 0 || y2 < 0 || x2 < x1 || y2 < y1) {
        // LLOGE("out of lcd buff range %d %d %d %d", x1, y1, x2, y2);
        // LLOGE("out of lcd buff range %d %d %d %d %d", x1 >= conf->w, y1 >= conf->h, y2 < 0, x2 < x1, y2 < y1);
        return 0;
    }
    if (y2 >= conf->h) {
        y2 = conf->h - 1;
    }

	uint32_t retry_cnt = 0;
	while ((luat_lcd_service_cache_len() >= (x2-x1+1)*(y2-y1+1))){
		retry_cnt++;
		luat_rtos_task_sleep(5);
		if (retry_cnt > 20)
		{
			LUAT_DEBUG_PRINT("too much wait");
			luat_lcd_service_debug();
		}
	}
	retry_cnt = 0;
    while (luat_lcd_service_draw(conf, x1, y1, x2, y2, color, 0)){
		retry_cnt++;
		luat_rtos_task_sleep(5);
		if (retry_cnt > 20)
		{
			LUAT_DEBUG_PRINT("too much wait no mem");
			luat_lcd_service_debug();
		}
    }
	return 0;
}
#endif
