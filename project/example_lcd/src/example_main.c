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

#include "common_api.h"
#include "luat_rtos.h"
#include "luat_debug.h"

#include "luat_spi.h"
#include "luat_lcd.h"

#define LCD_SPI	1

luat_rtos_task_handle lcd_task_handle;
extern const luat_lcd_opts_t lcd_opts_st7735;
static void task_test_lcd(void *param)
{
    luat_spi_t spi_conf = {
        .id = LCD_SPI,
        .CPHA = 0,
        .CPOL = 0,
        .dataw = 8,
        .bit_dict = 0,
        .master = 1,
        .mode = 1,             // mode设置为1，全双工
        .bandrate = 25600000,
        .cs = 12
    };

    luat_spi_setup(&spi_conf);

	luat_lcd_conf_t lcd_conf = {0};
    lcd_conf.port = LCD_SPI;
    lcd_conf.auto_flush = 1;

    lcd_conf.opts = &lcd_opts_st7735;
    lcd_conf.pin_dc = 14;
    lcd_conf.pin_rst = 3;
    lcd_conf.pin_pwr = 255;
    lcd_conf.direction = 0;
    lcd_conf.w = 128;
    lcd_conf.h = 160;
    lcd_conf.xoffset = 0;
    lcd_conf.yoffset = 0;
	luat_rtos_task_sleep(2000);
    luat_lcd_init(&lcd_conf);
    luat_lcd_clear(&lcd_conf,WHITE);

    luat_lcd_draw_line(&lcd_conf,20,35,140,35,0x001F);
    luat_lcd_draw_rectangle(&lcd_conf,20,40,120,70,0xF800);
    luat_lcd_draw_circle(&lcd_conf,60,60,10,0x0CE0);

    while (1){
        luat_rtos_task_sleep(1000);
    }
}

static void task_demo_lcd(void)
{
    luat_rtos_task_create(&lcd_task_handle, 4096, 20, "lcd", task_test_lcd, NULL, 0);
}

INIT_TASK_EXPORT(task_demo_lcd,"1");



