#include "common_api.h"

#include "luat_rtos.h"
#include "luat_audio_play.h"
#include "luat_i2s.h"
#include "luat_gpio.h"
#include "luat_debug.h"
#include "luat_i2c.h"
#include "interf_enc.h"
#include "luat_uart.h"
#include "luat_mem.h"

#define TEST_I2S_ID I2S_ID0

__USER_FUNC_IN_RAM__ int record_cb(uint8_t id ,luat_i2s_event_t event, uint8_t *rx_data, uint32_t rx_len, void *param)
{
	switch(event)
	{
	case LUAT_I2S_EVENT_RX_DONE:
		DBG("%d", rx_len);
		break;
	case LUAT_I2S_EVENT_TRANSFER_DONE:
		DBG("!");
		break;
	default:
		break;
	}
	return 0;
}

static uint8_t data[8000];
static void es8311_demo_task(void *arg)
{
	luat_i2s_conf_t i2s_conf = {
		.id = TEST_I2S_ID,
		.mode = LUAT_I2S_MODE_MASTER,
		.channel_format = LUAT_I2S_CHANNEL_RIGHT,
		.standard = LUAT_I2S_MODE_LSB,
		.channel_bits = LUAT_I2S_BITS_32,
		.data_bits = LUAT_I2S_BITS_16,
		.is_full_duplex = 1,
		.sample_rate = 0,
		.cb_rx_len = 8000,
		.luat_i2s_event_callback = record_cb,
	};
	luat_i2s_setup(&i2s_conf);
	luat_i2s_modify(TEST_I2S_ID, LUAT_I2S_CHANNEL_RIGHT, 32, 8000);
	luat_i2s_transfer_loop(TEST_I2S_ID, NULL, 4000, 2, 1);
	while (1)
	{
		luat_rtos_task_sleep(10000);
	}
}

static void test_record_demo_init(void)
{
	luat_rtos_task_handle task_handle;
	luat_rtos_task_create(&task_handle, 4096, 20, "es8311", es8311_demo_task, NULL, 0);
}

INIT_TASK_EXPORT(test_record_demo_init, "1");
