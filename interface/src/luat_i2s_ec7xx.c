/*
 * Copyright (c) 2022 OpenLuat & AirM2M
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
#include "luat_i2s.h"
#include "driver_i2s.h"
#include "driver_gpio.h"
void luat_i2s_init(void)
{

}

void luat_i2s_base_setup(uint8_t bus_id, uint8_t mode,  uint8_t frame_size)
{
	int pad;
	I2S_BaseConfig(bus_id, mode, frame_size);
	switch(bus_id)
	{
	case I2S_ID0:
		for(pad = 35; pad <= 39; pad++)
		{
			GPIO_IomuxEC718(pad, 1, 1, 0);
		}
		break;
	case I2S_ID1:
		for(pad = 18; pad <= 22; pad++)
		{
			GPIO_IomuxEC718(pad, 1, 1, 0);
		}
		break;
	}
}

int luat_i2s_start(uint8_t bus_id, uint8_t is_play, uint32_t sample, uint8_t channel_num)
{
	return I2S_Start(bus_id, is_play, sample, channel_num);
}

void luat_i2s_no_block_tx(uint8_t bus_id, uint8_t* address, uint32_t byte_len, void* cb, void *param)
{
	I2S_Tx(bus_id, address, byte_len, cb, param);
}
void luat_i2s_no_block_rx(uint8_t bus_id, uint32_t byte_len, void* cb, void *param)
{
	I2S_Rx(bus_id,byte_len, cb, param);
}


void luat_i2s_tx_stop(uint8_t bus_id)
{
	I2S_TxStop(bus_id);
}

void luat_i2s_rx_stop(uint8_t bus_id)
{
	I2S_RxStop(bus_id);
}

void luat_i2s_deinit(uint8_t bus_id)
{
	I2S_TxStop(bus_id);
}
void luat_i2s_pause(uint8_t bus_id)
{
	I2S_TxPause(bus_id);
}

int luat_i2s_tx_stat(uint8_t id, size_t *buffsize, size_t* remain) {
    (void)id;
    (void)buffsize;
    (void)remain;
    return -1;
}
