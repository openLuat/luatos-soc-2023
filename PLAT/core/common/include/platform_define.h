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

#ifndef __PLATFORM_DEFINE_H__
#define __PLATFORM_DEFINE_H__

#include "sctdef.h"

enum
{
	UART_DATA_BIT5 = 5,
	UART_DATA_BIT6 = 6,
	UART_DATA_BIT7 = 7,
	UART_DATA_BIT8 = 8,
	UART_DATA_BIT9 = 9,
	UART_PARITY_NONE = 0,
	UART_PARITY_ODD,
	UART_PARITY_EVEN,
	UART_STOP_BIT1 = 0,
	UART_STOP_BIT1_5,
	UART_STOP_BIT2,

	I2C_OP_READ_REG = 0,	//i2c通用读寄存器，一写一读，自动带start信号
	I2C_OP_READ,		//i2c通用读，只读
	I2C_OP_WRITE,		//i2c通用写，只写

	OP_QUEUE_CMD_END = 0,
	OP_QUEUE_CMD_ONE_TIME_DELAY,	//只有一次delay
	OP_QUEUE_CMD_CONTINUE_DELAY,	//连续delay，配合OP_QUEUE_CMD_REPEAT_DELAY使用
	OP_QUEUE_CMD_REPEAT_DELAY,	//重复OP_QUEUE_CMD_CONTINUE_DELAY
	OP_QUEUE_CMD_SET_GPIO_DIR_OUT,
	OP_QUEUE_CMD_SET_GPIO_DIR_IN,
	OP_QUEUE_CMD_GPIO_OUT,
	OP_QUEUE_CMD_GPIO_IN,
	OP_QUEUE_CMD_GPIO_IN_CB,
	OP_QUEUE_CMD_CB,
	OP_QUEUE_CMD_CAPTURE_SET,
	OP_QUEUE_CMD_CAPTURE,
	OP_QUEUE_CMD_CAPTURE_CB,
	OP_QUEUE_CMD_CAPTURE_END,
	OP_QUEUE_CMD_IO_PULL_NONE = 0,
	OP_QUEUE_CMD_IO_PULL_UP,
	OP_QUEUE_CMD_IO_PULL_DOWN,

	OP_QUEUE_CMD_IO_EXTI_UP = 0,		//上升沿中断
	OP_QUEUE_CMD_IO_EXTI_DOWN,		//下降沿中断
	OP_QUEUE_CMD_IO_EXTI_BOTH,	//双边沿中断

	COLOR_MODE_RGB_565 = 0,
	COLOR_MODE_GRAY,
	COLOR_MODE_RGB_888,
	COLOR_MODE_YCBCR_422_UYVY,
	COLOR_MODE_YCBCR_422_YUYV,
	COLOR_MODE_YCBCR_422_CBYCRY,

	CORE_OTA_MODE_FULL = 0,	//param1的byte0
	CORE_OTA_MODE_DIFF,
	CORE_OTA_MODE_MIX,
	CORE_OTA_IN_FLASH = 0,	//param1的byte1
	CORE_OTA_OUT_SPI_FLASH,
	CORE_OTA_IN_FILE,

	I2S_MODE_I2S = 0,
	I2S_MODE_LSB,
	I2S_MODE_MSB,
	I2S_FRAME_SIZE_16_16,
	I2S_FRAME_SIZE_16_32,
	I2S_FRAME_SIZE_24_32,
	I2S_FRAME_SIZE_32_32,

};


enum
{

	I2C_ID0 = 0,
	I2C_ID1,
	I2C_MAX,
	UART_ID0 = 0,
	UART_ID1,
	UART_ID2,
#if defined TYPE_EC716S
#else
	UART_ID3,
#endif
//	UART_ID4,
//	UART_ID5,
	UART_MAX,
	VIRTUAL_UART0 = 0,
	VIRTUAL_UART_MAX,
	SPI_ID0 = 0,
#ifndef CHIP_EC716
	SPI_ID1,
#endif
	SPI_MAX,

	USP_ID0 = 0,
	USP_ID1,
	USP_ID2,
	USP_MAX,

	I2S_ID0 = USP_ID0,
#if defined TYPE_EC716S
#else
	I2S_ID1,
#endif
	I2S_MAX,

	CSPI_ID0 = USP_ID0,
	CSPI_ID1,
	CSPI_ID2,
	CSPI_MAX,

	LSPI_ID0 = USP_ID0,
	LSPI_ID1,
	LSPI_ID2,
	LSPI_MAX,

	SPI_MODE_0 = 0,
	SPI_MODE_1,
	SPI_MODE_2,
	SPI_MODE_3,
//
//	HW_TIMER0 = 0,
//	HW_TIMER1,
//	HW_TIMER2,
//	HW_TIMER3,
//	HW_TIMER4,
//	HW_TIMER5,
//	HW_TIMER_MAX,
//
//	ADC_CHANNEL_0 = 0,
//	ADC_CHANNEL_1,
//	ADC_CHANNEL_2,
//	ADC_CHANNEL_3,
//	ADC_CHANNEL_4,
//	ADC_CHANNEL_5,
//	ADC_CHANNEL_6,
//	ADC_CHANNEL_MAX,


	HAL_GPIO_0 = 0,
	HAL_GPIO_1,
	HAL_GPIO_2,
	HAL_GPIO_3,
	HAL_GPIO_4,
	HAL_GPIO_5,
	HAL_GPIO_6,
	HAL_GPIO_7,
	HAL_GPIO_8,
	HAL_GPIO_9,
	HAL_GPIO_10,
	HAL_GPIO_11,
	HAL_GPIO_12,
	HAL_GPIO_13,
	HAL_GPIO_14,
	HAL_GPIO_15,
	HAL_GPIO_16,
	HAL_GPIO_17,
	HAL_GPIO_18,
	HAL_GPIO_19,
	HAL_GPIO_20,
#if defined TYPE_EC716S
#else
	HAL_GPIO_21,
	HAL_GPIO_22,
	HAL_GPIO_23,
	HAL_GPIO_24,
	HAL_GPIO_25,
	HAL_GPIO_26,
	HAL_GPIO_27,
	HAL_GPIO_28,
	HAL_GPIO_29,
	HAL_GPIO_30,
	HAL_GPIO_31,
	HAL_GPIO_32,
	HAL_GPIO_33,
	HAL_GPIO_34,
	HAL_GPIO_35,
	HAL_GPIO_36,
	HAL_GPIO_37,
	HAL_GPIO_38,
#endif
	HAL_GPIO_MAX,
	HAL_GPIO_NONE = HAL_GPIO_MAX, //大于等于HAL_GPIO_NONE，说明不存在
	HAL_WAKEUP_0 = HAL_GPIO_MAX,	//EC718特殊的几个输入IO，WAKEUPPAD0~5,Charge,PWRKEY
	HAL_WAKEUP_1,
	HAL_WAKEUP_2,			//EC716S没有
	HAL_WAKEUP_3,			//和GPIO20同一个PIN
	HAL_WAKEUP_4,			//和GPIO21同一个PIN
	HAL_WAKEUP_5,			//和GPIO22同一个PIN
	HAL_WAKEUP_CHARGE,
	HAL_WAKEUP_PWRKEY,
	HAL_GPIO_QTY,
};

#define SOC_TICK_1US	(26ul)
#define SOC_TICK_1MS	(26000ul)
#define SOC_TICK_1S		(26000000ul)
#define SOC_TICK_TIMER	(3)

#define CP_VERSION	0x01130001

#define __FLASH_BLOCK_SIZE__ (0x00010000)
#define __FLASH_SECTOR_SIZE__ (0x00001000)
#define __FLASH_PAGE_SIZE__ (0x00000100)
#define __APP_START_MAGIC__ (0xeac37218)

#define __AP_FLASH_SAVE_ADDR__             (0x00024000)
#define __BL_FLASH_SAVE_ADDR__             (0x00003000)
#define __SOC_OTA_INFO_DATA_LOAD_ADDRESS__	(__BL_FLASH_SAVE_ADDR__ + BOOTLOADER_FLASH_LOAD_SIZE + AP_FLASH_XIP_ADDR)
#define __SOC_OTA_INFO_DATA_SAVE_ADDRESS__	(__BL_FLASH_SAVE_ADDR__ + BOOTLOADER_FLASH_LOAD_SIZE)
#if defined FEATURE_IMS_ENABLE
#define __ISR_IN_RAM__ PLAT_FM_RAMCODE
#else
#define __ISR_IN_RAM__ PLAT_FA_RAMCODE
#endif
#define __CORE_FUNC_IN_RAM__ PLAT_FM_RAMCODE
#define __USER_FUNC_IN_RAM__ PLAT_FM_RAMCODE
#endif
