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

luat_rtos_task_handle i2c_task_handle;

#define I2C_ID                  1
#define AHT10_ADDRESS_ADR_LOW   0x38

#define AHT10_INIT              0xE1 //初始化命令
#define AHT10_MEASURE           0xAC //触发测量命令
#define AHT10_SOFT_RESET        0xBA
#define AHT10_STATE             0x71 //状态字.

#define SHT30_ADDRESS	0x44

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
		vTaskDelay(1000);
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

static void task_demo_i2c(void)
{
//    luat_rtos_task_create(&i2c_task_handle, 2048, 20, "i2c", task_test_aht10, NULL, 0);
    luat_rtos_task_create(&i2c_task_handle, 2048, 20, "i2c", task_test_sht30, NULL, 0);
}

INIT_TASK_EXPORT(task_demo_i2c,"1");



