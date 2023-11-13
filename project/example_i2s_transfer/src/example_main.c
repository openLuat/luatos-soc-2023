#include "common_api.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_i2s.h"
#include "luat_mem.h"
#define TEST_ID   I2S_ID0
#ifdef PSRAM_EXIST
#define BUFFER_LEN	256 * 1024
#else
#define BUFFER_LEN	64 * 1024
#endif
extern void *luat_psram_static_alloc(size_t size);
luat_rtos_task_handle i2s_task_handle;
static uint8_t *tx_buf;
static int32_t callback(void *pdata, void *param)
{
	if (pdata)
	{
		Buffer_Struct *buffer = (Buffer_Struct *)pdata;
		LUAT_DEBUG_PRINT("i2s rx %dbyte", buffer->Pos);
	}
	else
	{
		luat_i2s_transfer(TEST_ID, tx_buf, BUFFER_LEN);
	}
}


static void task_test_i2s(void *param)
{
#ifdef PSRAM_EXIST
	tx_buf = (uint8_t *)luat_psram_static_alloc(BUFFER_LEN);
#else
	tx_buf = malloc(BUFFER_LEN);
#endif
	luat_i2s_base_setup(TEST_ID, I2S_MODE_I2S, I2S_FRAME_SIZE_16_16);
	luat_i2s_transfer_start(TEST_ID, 16000, 1, 8000, callback, NULL);
	luat_i2s_transfer(TEST_ID, tx_buf, BUFFER_LEN);
    while (1)
    {
        luat_rtos_task_sleep(100000000);

    }
}

static void task_demo_i2s(void)
{
    luat_rtos_task_create(&i2s_task_handle, 4 * 1024, 20, "i2s", task_test_i2s, NULL, 0);
}

INIT_TASK_EXPORT(task_demo_i2s,"1");
