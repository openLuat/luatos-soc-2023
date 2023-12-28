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
#include "luat_mcu.h"
#include "luat_i2c.h"
#include "luat_pwm.h"
#include "luat_pm.h"
#include "luat_gpio.h"
#include "luat_camera.h"
#include "driver_gpio.h"
#include "driver_usp.h"
luat_rtos_task_handle i2c_task_handle;

#define I2C_ID                  1
#define AHT10_ADDRESS_ADR_LOW   0x38

#define AHT10_INIT              0xE1 //初始化命令
#define AHT10_MEASURE           0xAC //触发测量命令
#define AHT10_SOFT_RESET        0xBA
#define AHT10_STATE             0x71 //状态字.

#define SHT30_ADDRESS	0x44

extern uint8_t CRC8Cal(void *Data, uint16_t Len, uint8_t CRC8Last, uint8_t CRCRoot, uint8_t IsReverse);

static void task_test_aht10(void *param)
{

    char soft_reset[] = {AHT10_SOFT_RESET};
    char init_cmd[] = {AHT10_INIT,0x08,0x00};
    char measure_cmd[] = {AHT10_MEASURE, 0x33, 0x00};
    char read_cmd[] = {AHT10_STATE};
    char recv_date[7] = {0};
    luat_i2c_setup(I2C_ID,1);
    luat_rtos_task_sleep(40);
    luat_i2c_send(I2C_ID, AHT10_ADDRESS_ADR_LOW, soft_reset, 1, 1);
    luat_rtos_task_sleep(20);

    luat_i2c_recv(I2C_ID, AHT10_ADDRESS_ADR_LOW, recv_date, 1);

    if (recv_date[0]&(1<<3) == 0){
        luat_i2c_send(I2C_ID, AHT10_ADDRESS_ADR_LOW, init_cmd, 3, 1);
    }
    while (1)
    {
        luat_rtos_task_sleep(1000);
        luat_i2c_send(I2C_ID, AHT10_ADDRESS_ADR_LOW, measure_cmd, 3, 1);
        luat_rtos_task_sleep(80);
        luat_i2c_send(I2C_ID, AHT10_ADDRESS_ADR_LOW, read_cmd, 1, 1);
        luat_i2c_recv(I2C_ID, AHT10_ADDRESS_ADR_LOW, recv_date, 6);

        float cur_temp = ((recv_date[3] & 0xf) << 16 | recv_date[4] << 8 | recv_date[5]) * 200.0 / (1 << 20) - 50;
        LUAT_DEBUG_PRINT("temp: %f",cur_temp);

    }

}

static void task_test_sht30(void *param)
{
	uint8_t measure_cmd[] = {0x2c, 0x0d};
	uint8_t recv_date[7] = {0};
	uint8_t crc1,crc2;
	uint32_t t,r;
	double te,hu;
	int result;
	luat_i2c_setup(I2C_ID,1);
	luat_rtos_task_sleep(40);
	while(1)
	{
		result = luat_i2c_send(I2C_ID, SHT30_ADDRESS, measure_cmd, 2, 1);
		luat_rtos_task_sleep(1000);
		if (!result)
		{
			result = luat_i2c_recv(I2C_ID, SHT30_ADDRESS, recv_date, 6);
			if (!result)
			{
				crc1 = CRC8Cal(recv_date, 2, 0xff, 0x31, 0);
				crc2 = CRC8Cal(recv_date + 3, 2, 0xff, 0x31, 0);
				if ((recv_date[2] == crc1) && (recv_date[5] == crc2))
				{
					t = BytesGetBe16(recv_date);
					r = BytesGetBe16(recv_date);
					te = t * 175.0 / 65535.0 - 45.0;
					hu = r * 100.0 /65535.0;
					LUAT_DEBUG_PRINT("温度 %f，湿度 %f", te, hu);
				}
				else
				{
					LUAT_DEBUG_PRINT("校验失败 %x,%x,%x,%x", recv_date[2], crc1, recv_date[5], crc2);
				}

			}
		}
	}

}

static void task_test_g(void *param)
{
    luat_gpio_cfg_t Net_led_struct;
    luat_gpio_set_default_cfg(&Net_led_struct);
    Net_led_struct.pin=HAL_GPIO_15;
    Net_led_struct.pull=Luat_GPIO_PULLUP;
    Net_led_struct.mode=Luat_GPIO_OUTPUT;
    Net_led_struct.output_level=1;
    luat_gpio_open(&Net_led_struct);
	int result;
	uint8_t data[1];
	luat_i2c_setup(I2C_ID,1);
	luat_rtos_task_sleep(40);
	while(1)
	{
		data[0] = 1;
		result = luat_i2c_send(I2C_ID, 0x62, data, 1, 1);
		if (!result)
		{
			result = luat_i2c_recv(I2C_ID, 0x62, data, 1);
			if (!result)
			{
				LUAT_DEBUG_PRINT("read reg 1 value %x", data[0]);
			}
		}
		luat_rtos_task_sleep(50);

	}

}

static void task_demo_i2c(void)
{
//	luat_pm_iovolt_ctrl(0, 3300);
//    luat_rtos_task_create(&i2c_task_handle, 2048, 20, "i2c", task_test_aht10, NULL, 0);
//    luat_rtos_task_create(&i2c_task_handle, 2048, 20, "i2c", task_test_sht30, NULL, 0);
//    luat_rtos_task_create(&i2c_task_handle, 2048, 20, "i2c", task_test_g, NULL, 0);
}

//INIT_TASK_EXPORT(task_demo_i2c,"1");

static void task_test_pwm(void *param)
{

	uint8_t channel = 4;
	LUAT_DEBUG_PRINT("测试26KHz, 连续输出，占空比每5秒增加1，从0循环到100");
	luat_pwm_open(channel, 32768, 500, 0);

    while(1)
	{
        luat_rtos_task_sleep(5000);
        LUAT_DEBUG_PRINT("关闭PWM4");
        luat_pwm_close(channel);
        luat_rtos_task_sleep(5000);
        LUAT_DEBUG_PRINT("开PWM4");
        luat_pwm_open(channel, 26000, 500, 0);
	}

}

static void task_demo_pwm(void)
{
//	luat_rtos_task_handle  pwm_task_handle;
//    luat_rtos_task_create(&pwm_task_handle, 2048, 20, "pwm", task_test_pwm, NULL, 0);
}

//INIT_TASK_EXPORT(task_demo_pwm,"1");

int gpio_level_irq(int pin, void* args)
{
	LUAT_DEBUG_PRINT("pin:%d, level:%d,", pin, luat_gpio_get(pin));
	return 0;
}

static void gpio_interrupt_task(void *param)
{
    luat_gpio_cfg_t Net_led_struct;
    luat_gpio_set_default_cfg(&Net_led_struct);
    Net_led_struct.pin=HAL_GPIO_1;
    Net_led_struct.pull=Luat_GPIO_PULLUP;
    Net_led_struct.mode=Luat_GPIO_OUTPUT;
    Net_led_struct.output_level=1;
    luat_gpio_open(&Net_led_struct);

    Net_led_struct.pin=HAL_GPIO_3;
    Net_led_struct.pull=Luat_GPIO_DEFAULT;
    Net_led_struct.mode=Luat_GPIO_INPUT;
    luat_gpio_open(&Net_led_struct);

    Net_led_struct.pin=HAL_WAKEUP_0;
    Net_led_struct.pull=Luat_GPIO_DEFAULT;
    Net_led_struct.mode=Luat_GPIO_IRQ;
    Net_led_struct.irq_type=Luat_GPIO_BOTH;
    Net_led_struct.irq_cb=gpio_level_irq;
    luat_gpio_open(&Net_led_struct);

    Net_led_struct.pin=HAL_WAKEUP_1;
    luat_gpio_open(&Net_led_struct);

    while (1)
    {
        luat_rtos_task_sleep(500);
        luat_gpio_set(HAL_GPIO_1, 0);
        LUAT_DEBUG_PRINT("%d", luat_gpio_get(HAL_GPIO_3));
        luat_rtos_task_sleep(500);
        luat_gpio_set(HAL_GPIO_1, 1);
        LUAT_DEBUG_PRINT("%d", luat_gpio_get(HAL_GPIO_3));
    }

}

void gpio_interrupt_demo(void)
{
    luat_rtos_task_handle gpio_interrupt_task_handler;
    luat_rtos_task_create(&gpio_interrupt_task_handler,2*1024,50,"gpio_interrupt_task",gpio_interrupt_task,NULL,0);
}



//INIT_TASK_EXPORT(gpio_interrupt_demo,"3");
extern void CSPI_SetupSpeedParam(uint8_t ID, uint32_t Rate1, uint32_t Rate2);
static void task_test_mclk(void *param)
{
	GPIO_IomuxEC7XX(16, 4, 1, 0);
	CSPI_SetupSpeedParam(0, 0x19, 0x000020);
	CSPI_Setup(1, 24000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    while(1)
	{
        luat_rtos_task_sleep(5000000);
	}

}

static void task_demo_mclk(void)
{
	luat_rtos_task_handle  task_handle;
    luat_rtos_task_create(&task_handle, 2048, 20, "mclk", task_test_mclk, NULL, 0);
}

INIT_TASK_EXPORT(task_demo_mclk,"1");
