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

#ifndef PRIVATE_INCLUDE_LUAT_LCD_SERVICE_H_
#define PRIVATE_INCLUDE_LUAT_LCD_SERVICE_H_

#include "luat_base.h"
#include "soc_lcd.h"

/**
 * @brief 当前lcd刷新占用ram空间
 * @return ram空间大小
 */
uint32_t luat_lcd_draw_cache(void);

/**
 * @brief lcd刷新请求，这里会拷贝刷新区域的数据，从而额外占用ram
 * @param spi_id spi总线id
 * @param spi_mode spi时序模式，0~3
 * @param cs_pin cs pin
 * @param dc_pin dc pin
 * @param spi_speed spi速度
 * @param data 刷新的数据
 * @param w_start 横向起始位置
 * @param w_end 横向结束位置，最大只能是总宽度-1
 * @param h_start 纵向起始位置
 * @param h_end 纵向结束位置，最大只能是总高度-1
 * @param w_offset 横向偏移，与实际屏幕有关
 * @param h_offset 纵向偏移，与实际屏幕有关
 * @param color_mode 颜色模式，见COLOR_MODE_XXX，目前只支持COLOR_MODE_RGB_565和COLOR_MODE_GRAY
 * @return 成功返回0，其他失败，失败一般都是ram不足
 */
int luat_lcd_draw_require(uint8_t spi_id, uint8_t spi_mode, uint8_t cs_pin, uint8_t dc_pin,  uint32_t spi_speed, void *data, uint16_t w_start, uint16_t w_end, uint16_t h_start, uint16_t h_end, uint16_t w_offset, uint16_t h_offset, uint8_t color_mode);

/**
 * @brief lcd刷新camera数据请求，这里不会拷贝刷新区域的数据，如果camera速度比lcd刷新快，就需要注意数据同步问题，camera要等lcd刷新
 * @param spi_id spi总线id
 * @param spi_mode spi时序模式，0~3
 * @param cs_pin cs pin
 * @param dc_pin dc pin
 * @param spi_speed spi速度
 * @param data 刷新的数据
 * @param w_start 横向起始位置
 * @param w_end 横向结束位置，最大只能是总宽度-1
 * @param h_start 纵向起始位置
 * @param h_end 纵向结束位置，最大只能是总高度-1
 * @param w_offset 横向偏移，与实际屏幕有关
 * @param h_offset 纵向偏移，与实际屏幕有关
 * @param color_mode 颜色模式，见COLOR_MODE_XXX，目前只支持COLOR_MODE_RGB_565和COLOR_MODE_GRAY
 * @return 成功返回0，其他失败，失败一般都是ram不足
 */
int luat_lcd_draw_camera_require(uint8_t spi_id, uint8_t spi_mode, uint8_t cs_pin, uint8_t dc_pin,  uint32_t spi_speed, void *data, uint16_t w_start, uint16_t w_end, uint16_t h_start, uint16_t h_end, uint16_t w_offset, uint16_t h_offset, uint8_t color_mode);

/**
 * @brief lcd刷新服务初始化
 * @param priority 刷新任务优先级0~100
 */
void luat_lcd_service_init(uint8_t priority);

/**
 * @brief lcd任务跑用户API，可用于camera和lcd刷新的同步。
 * @param CB 用户api
 * @param data api的data参数
 * @param len api的len参数
 * @param timeout 请求超时时间，预留，无效
 */
void luat_lcd_run_user_api(CBDataFun_t CB, uint32_t data, uint32_t len, uint32_t timeout);

#endif /* PRIVATE_INCLUDE_LUAT_LCD_SERVICE_H_ */
