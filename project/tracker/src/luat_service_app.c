#include "luat_debug.h"
#include "luat_rtos.h"
#include "luat_i2c.h"
#include "luat_gpio.h"
#include "platform_define.h"
#include "socket_service.h"
#include "param_ctrl.h"
#include "gpsmsg.h"
//#include "luat_sms.h"
#include "luat_pm.h"
#include "luat_fs.h"
#include "bsp_common.h"
//#include "luat_sms_app.h"
#define SLEEP_TAG "sleep_main"
luat_rtos_semaphore_t g_s_send_data_from_task_semaphore_handle;
luat_rtos_semaphore_t g_s_send_history_data_from_task_semaphore_handle;
luat_rtos_semaphore_t g_s_send_heart_data_from_task_semaphore_handle;
luat_rtos_semaphore_t g_s_send_alarm_data_from_task_semaphore_handle;
luat_rtos_task_handle mx3416_monitor_task_handle;
luat_rtos_task_handle acc_monitor_task_handle;
luat_rtos_task_handle send_data_task_handle;
luat_rtos_task_handle locrpt_task_handle;
luat_rtos_task_handle heartrpt_task_handle;
luat_rtos_task_handle sms_task_handle;
luat_rtos_task_handle acc_task_handle;
luat_rtos_task_handle i2c_task_handle;
static luat_rtos_timer_t sleep_timer_handle;

extern Jt808Msg jt808_message;
extern nmea_msg gpsx;
static uint8_t locinfo_upload_enable = 1;
static uint8_t device_is_stop = 0;
// result：0成功，1 socket未连接；其余错误值是lwip send接口返回的错误原因值
static void send_data_from_task_callback(int result, uint32_t callback_param)
{
	LUAT_DEBUG_PRINT("async result %d, callback_param %d", result, callback_param);
	luat_rtos_semaphore_release(g_s_send_data_from_task_semaphore_handle);
}
static void send_history_data_from_task_callback(int result, uint32_t callback_param)
{
	LUAT_DEBUG_PRINT("history data async result %d, callback_param %d", result, callback_param);
	luat_rtos_semaphore_release(g_s_send_history_data_from_task_semaphore_handle);
}
static void send_heart_data_from_task_callback(int result, uint32_t callback_param)
{
	LUAT_DEBUG_PRINT("heart data async result %d, callback_param %d", result, callback_param);
	luat_rtos_semaphore_release(g_s_send_heart_data_from_task_semaphore_handle);
}
static void send_alarm_data_from_task_callback(int result, uint32_t callback_param)
{
	LUAT_DEBUG_PRINT("heart data async result %d, callback_param %d", result, callback_param);
	luat_rtos_semaphore_release(g_s_send_alarm_data_from_task_semaphore_handle);
}

static void locrpt_task_proc(void *arg)
{
	int result;
	uint8_t data[100] = {0};
	uint16_t len;

	while (1)
	{
		if (network_service_is_connect() == 1)
		{
			if (gpsx.gpssta == 1 && locinfo_upload_enable == 1 && device_is_stop == 0)
			{
				protocol_jt_pack_gps_msg(&gpsx, data, &len, 100, 0, 0);
				result = socket_service_send_data(data, len, send_data_from_task_callback, 0);
				if (0 == result)
				{
					luat_rtos_semaphore_take(g_s_send_data_from_task_semaphore_handle, LUAT_WAIT_FOREVER);
				}
				else
				{
					LUAT_DEBUG_PRINT("sync result %d", result);
				}
			}
		}
		luat_rtos_task_sleep((jt808_message.uploadtime) * 1000);
	}
	luat_rtos_task_delete(locrpt_task_handle);
}

static void heartrpt_task_proc(void *arg)
{
	int result;
	uint8_t data[100] = {0};
	uint16_t len;

	while (1)
	{
		protocol_jt_pack_heartbeat_msg(data, &len, 100);
		result = socket_service_send_data(data, len, send_heart_data_from_task_callback, 0);
		if (0 == result)
		{
			luat_rtos_semaphore_take(g_s_send_heart_data_from_task_semaphore_handle, LUAT_WAIT_FOREVER);
		}
		else
		{
			LUAT_DEBUG_PRINT("sync result %d", result);
		}

		luat_rtos_task_sleep((jt808_message.heart_interval) * 1000);
	}
	luat_rtos_task_delete(heartrpt_task_handle);
}

static int g_s_acc_status = 0;
static int g_s_accelerated_speed_status = 0;

int config_acc_get(void)
{
	return g_s_acc_status;
}

int config_speed_get(void)
{
	return g_s_accelerated_speed_status;
}

int gpio_irq(int pin, void *args)
{
	if (pin == HAL_GPIO_20)
	{
		if (luat_gpio_get(pin) == 1)
			g_s_acc_status = 0;
		else
			g_s_acc_status = 1;
		luat_rtos_message_send(acc_monitor_task_handle, 0, NULL);
	}
	else if (pin == HAL_GPIO_21)
	{
		if (luat_gpio_get(pin) == 1)
			g_s_accelerated_speed_status = 0;
		else
			g_s_accelerated_speed_status = 1;
		// if (0 == g_s_acc_status)
		luat_rtos_message_send(mx3416_monitor_task_handle, 0, NULL);
	}
	// LUAT_DEBUG_PRINT("gpio_irq status:%d,%d", g_s_acc_status, g_s_accelerated_speed_status);
}

#define I2C_ID 0
#define MC3416_ADDRESS_ADR 0x4C

static void i2c_task_proc(void *arg)
{
	char recv_data[8] = {0};
	char statusAddr[] = {0x05};
	char modeAddr[] = {0x07, 0xC3};
	char intrCtrlAddr[] = {0x06, 0x44};
	char motionCtrlAddr[] = {0x09, 0x04};
	char sampleAAddr[] = {0x08, 0x02};
	char threshLSBAddr[] = {0x43, 0x50};
	char thershMSBAddr[] = {0x44, 0x00};
	char debounceAddr[] = {0x45, 0x00};
	char mode1Addr[] = {0x07, 0xC1};
	int ret;
	luat_gpio_cfg_t gpio_cfg;

	// 配置计算器传感器震动中断引脚
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pin = HAL_GPIO_21;
	gpio_cfg.mode = LUAT_GPIO_IRQ;

	gpio_cfg.irq_type = LUAT_GPIO_RISING_IRQ;

	gpio_cfg.pull = LUAT_GPIO_PULLDOWN;
	gpio_cfg.irq_cb = gpio_irq;
	luat_gpio_open(&gpio_cfg);

	while (1)
	{
		config_accelerated_speed_set(1);
		luat_i2c_setup(I2C_ID, 0);
		ret = luat_i2c_send(I2C_ID, MC3416_ADDRESS_ADR, statusAddr, 1, 1);
		luat_i2c_recv(I2C_ID, MC3416_ADDRESS_ADR, recv_data, 1);
		luat_i2c_send(I2C_ID, MC3416_ADDRESS_ADR, modeAddr, 2, 1);
		luat_i2c_send(I2C_ID, MC3416_ADDRESS_ADR, intrCtrlAddr, 2, 1);
		luat_i2c_send(I2C_ID, MC3416_ADDRESS_ADR, motionCtrlAddr, 2, 1);
		luat_i2c_send(I2C_ID, MC3416_ADDRESS_ADR, sampleAAddr, 2, 1);
		luat_i2c_send(I2C_ID, MC3416_ADDRESS_ADR, threshLSBAddr, 2, 1);
		luat_i2c_send(I2C_ID, MC3416_ADDRESS_ADR, thershMSBAddr, 2, 1);
		luat_i2c_send(I2C_ID, MC3416_ADDRESS_ADR, debounceAddr, 2, 1);
		luat_i2c_send(I2C_ID, MC3416_ADDRESS_ADR, mode1Addr, 2, 1);
		luat_rtos_task_sleep(60 * 1000);
		luat_i2c_close(I2C_ID);
		config_accelerated_speed_set(0);
	}
	luat_rtos_task_delete(i2c_task_handle);
}

static void luat_mx3416_monitor_task(void *args)
{
	uint8_t message_id;
	uint8_t *tmp = NULL;
	size_t count = 0;
	int result;
	uint8_t data[100] = {0};
	uint16_t len;
	while (1)
	{
		int result = luat_rtos_message_recv(mx3416_monitor_task_handle, &message_id, (void **)&tmp, 10000);
		LUAT_DEBUG_PRINT("monitor task recv message timeout %d", result);
		if (-ERROR_TIMEOUT == result)
		{
			//静止状态
			device_is_stop = 1;
			count = 0;
			LUAT_DEBUG_PRINT("wait acc interrupt timeout");
		}
		else if(ERROR_NONE == result)
		{
			//acc断了_但还处于运动状态的话，报警
			count++;
			if (0 == g_s_acc_status)
			{
				if (count > 20)
				{
					count = 0;
					device_is_stop = 1;
					LUAT_DEBUG_PRINT("震动报警！！！");
					protocol_jt_pack_gps_msg(&gpsx, data, &len, 100, 1, 0);
					result = socket_service_send_data(data, len, send_alarm_data_from_task_callback, 0);
					if (0 == result)
					{
						luat_rtos_semaphore_take(g_s_send_alarm_data_from_task_semaphore_handle, LUAT_WAIT_FOREVER);
					}
					else
					{
						LUAT_DEBUG_PRINT("sync result %d", result);
					}
				}
			}
			else
			{
				if (count > 20)
				{
					device_is_stop = 0;	
				}
			}
		}
	}
}

// todo 数据补传
#define HISTORY_DATA_PATH "/historydata.txt"
#define HISTORY_DATA_POS_PATH "/pos.txt"
luat_rtos_task_handle history_data_trans_task_handle;

static void create_data_file()
{
	FILE *fp = luat_fs_fopen(HISTORY_DATA_PATH, "w");
	luat_fs_fclose(fp);
	luat_fs_remove(HISTORY_DATA_POS_PATH);
}

static void history_data_trans_task()
{
	while (1)
	{
		if (network_service_is_connect() == 0)
		{
			if (gpsx.gpssta == 1 && locinfo_upload_enable == 1 && device_is_stop == 0)
			{
				if (luat_fs_fexist(HISTORY_DATA_PATH) == 0)
				{
					create_data_file();
				}
				if (luat_fs_fexist(HISTORY_DATA_POS_PATH) == 0)
				{
					size_t pos = 0;
					FILE *fp1 = luat_fs_fopen(HISTORY_DATA_PATH, "r+");
					luat_fs_fseek(fp1, 0, SEEK_SET);
					luat_fs_fwrite(&gpsx, sizeof(nmea_msg), 1, fp1);
					luat_fs_fclose(fp1);

					pos = pos + sizeof(nmea_msg);
					FILE *fp2 = luat_fs_fopen(HISTORY_DATA_POS_PATH, "w+");
					luat_fs_fwrite(&pos, sizeof(size_t), 1, fp2);
					luat_fs_fclose(fp2);
				}
				else
				{
					size_t pos = 0;
					FILE *fp2 = luat_fs_fopen(HISTORY_DATA_POS_PATH, "r");
					luat_fs_fread((void *)&pos, sizeof(size_t), 1, fp2);
					luat_fs_fclose(fp2);

					pos += sizeof(nmea_msg);
					if (pos > 25920)
						pos = 0;

					FILE *fp1 = luat_fs_fopen(HISTORY_DATA_PATH, "r+");
					luat_fs_fseek(fp1, pos, SEEK_SET);
					luat_fs_fwrite(&gpsx, sizeof(nmea_msg), 1, fp1);
					luat_fs_fclose(fp1);

					
					fp2 = luat_fs_fopen(HISTORY_DATA_POS_PATH, "w+");
					luat_fs_fwrite(&pos, sizeof(size_t), 1, fp2);
					luat_fs_fclose(fp2);
				}
			}
		}
		else
		{
			size_t size = luat_fs_fsize(HISTORY_DATA_PATH);
			if (size > 0)
			{
				int result;
				uint8_t pdata[100] = {0};
				uint16_t len;
				char *data = malloc(size);
				FILE *fp = luat_fs_fopen(HISTORY_DATA_PATH, "r");
				luat_fs_fread((void *)data, size, 1, fp);
				luat_fs_fclose(fp);
				create_data_file();
				nmea_msg gps_data = {0};
				for (size_t count = 0; count < size; count += sizeof(nmea_msg))
				{
					memcpy(&gps_data, data + count, sizeof(nmea_msg));
					protocol_jt_pack_gps_msg(&gps_data, pdata, &len, 100, 0, 0);
					result = socket_service_send_data(pdata, len, send_history_data_from_task_callback, 0);
					if (0 == result)
					{
						luat_rtos_semaphore_take(g_s_send_history_data_from_task_semaphore_handle, LUAT_WAIT_FOREVER);
					}
					else
					{
						LUAT_DEBUG_PRINT("sync result %d", result);
					}
					luat_rtos_task_sleep(1000);
					memset(&gps_data, 0x00, sizeof(nmea_msg));
				}
				memset(data, 0x00, size);
				free(data);
			}
		}
		LUAT_DEBUG_PRINT("history data file len %d", luat_fs_fsize(HISTORY_DATA_PATH));
		luat_rtos_task_sleep(10 * 1000);
	}
}

static luat_rtos_timer_callback_t sleep_timer_callback(void *param)
{
	LUAT_DEBUG_PRINT("entry sleep mode");
	// locinfo_upload_enable = 0;
	config_gps_set(0);
	luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_LIGHT, SLEEP_TAG);
}

static void luat_acc_monitor_task(void *args)
{
	uint8_t message_id;
	uint8_t *data = NULL;
	if (0 == g_s_acc_status)
	{
		luat_rtos_timer_start(sleep_timer_handle, 300000, 0, sleep_timer_callback, NULL);
	}
	else
	{
		locinfo_upload_enable = 1;
	}
	locinfo_upload_enable = 1;
	while (1)
	{
		if (0 == luat_rtos_message_recv(acc_monitor_task_handle, &message_id, (void **)&data, LUAT_WAIT_FOREVER))
		{
			if (g_s_acc_status == 1)
			{
				LUAT_DEBUG_PRINT("exit sleep mode");
				if (luat_rtos_timer_is_active(sleep_timer_handle))
				{
					luat_stop_rtos_timer(sleep_timer_handle);
				}
				locinfo_upload_enable = 1;
				config_gps_set(1);
				luat_pm_set_sleep_mode(LUAT_PM_SLEEP_MODE_IDLE, SLEEP_TAG);
			}
			else
			{
				luat_rtos_timer_start(sleep_timer_handle, 300000, 0, sleep_timer_callback, NULL);
			}
		}
	}
}

static void luat_send_data_task_proc(void *arg)
{
	// BSP_SetPlatConfigItemValue(0, 0); // 死机不重启而是打印信息
	// BSP_SetPlatConfigItemValue(PLAT_CONFIG_ITEM_LOG_PORT_SEL, 1);
	int result;
	uint8_t data[100] = {0};
	uint16_t len;

	luat_rtos_semaphore_create(&g_s_send_data_from_task_semaphore_handle, 1);
	luat_rtos_semaphore_create(&g_s_send_history_data_from_task_semaphore_handle, 1);
	luat_rtos_semaphore_create(&g_s_send_heart_data_from_task_semaphore_handle, 1);

	luat_gpio_cfg_t gpio_cfg;
	// 配置acc接入中断引脚
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pin = HAL_GPIO_20;
	gpio_cfg.mode = LUAT_GPIO_IRQ;
	gpio_cfg.irq_type = LUAT_GPIO_BOTH_IRQ;
	gpio_cfg.pull = LUAT_GPIO_PULLUP;
	gpio_cfg.irq_cb = gpio_irq;
	luat_gpio_open(&gpio_cfg);
	if (luat_gpio_get(HAL_GPIO_20) == 1)
		g_s_acc_status = 0;
	else
		g_s_acc_status = 1;
	luat_rtos_timer_create(&sleep_timer_handle);
	luat_rtos_task_create(&history_data_trans_task_handle, 2048, 20, "history data", history_data_trans_task, NULL, NULL);
	// luat_rtos_task_create(&acc_monitor_task_handle, 2048, 20, "acc_monitor", luat_acc_monitor_task, NULL, 10);
	luat_rtos_task_create(&mx3416_monitor_task_handle, 2048, 20, "mx3416 monitor", luat_mx3416_monitor_task, NULL, 20);
	luat_rtos_task_create(&i2c_task_handle, 2 * 1024, 20, "mx3416_i2c", i2c_task_proc, NULL, NULL);

	while (!network_service_is_connect())
	{
		// LUAT_DEBUG_PRINT("wait network_service_is_connect");
		luat_rtos_task_sleep(1000);
	}
	LUAT_DEBUG_PRINT("send request");

	if (jt808_message.isregistered == 0)
	{
		protocol_jt_pack_regist_msg(data, &len, 100);

		result = socket_service_send_data(data, len, send_data_from_task_callback, 0);
		if (0 == result)
		{
			luat_rtos_semaphore_take(g_s_send_data_from_task_semaphore_handle, LUAT_WAIT_FOREVER);
		}
		else
		{
			LUAT_DEBUG_PRINT("sync result %d", result);
		}

		luat_rtos_task_sleep(2000);
	}

	protocol_jt_pack_auth_msg(data, &len, 100);

	result = socket_service_send_data(data, len, send_data_from_task_callback, 0);
	if (0 == result)
	{
		luat_rtos_semaphore_take(g_s_send_data_from_task_semaphore_handle, LUAT_WAIT_FOREVER);
	}
	else
	{
		LUAT_DEBUG_PRINT("sync result %d", result);
	}

	luat_rtos_task_sleep(2000);

	protocol_jt_pack_iccid_msg(data, &len, 100);

	result = socket_service_send_data(data, len, send_data_from_task_callback, 0);
	if (0 == result)
	{
		luat_rtos_semaphore_take(g_s_send_data_from_task_semaphore_handle, LUAT_WAIT_FOREVER);
	}
	else
	{
		LUAT_DEBUG_PRINT("sync result %d", result);
	}

	// 基站定位
	luat_lbs_task_init();

	luat_rtos_task_sleep(2000);

	luat_rtos_task_create(&locrpt_task_handle, 2048, 30, "locrpt", locrpt_task_proc, NULL, 0);
	luat_rtos_task_create(&heartrpt_task_handle, 2048, 30, "heartrpt", heartrpt_task_proc, NULL, 0);
	// SMS
	luat_rtos_task_sleep(60000);
	if (gpsx.gpssta != 1)
	{
		protocol_jt_pack_gps_msg(&gpsx, data, &len, 100, 0, 1);
		result = socket_service_send_data(data, len, send_data_from_task_callback, 0);
		if (0 == result)
		{
			luat_rtos_semaphore_take(g_s_send_data_from_task_semaphore_handle, LUAT_WAIT_FOREVER);
		}
		else
		{
			LUAT_DEBUG_PRINT("sync result %d", result);
		}
	}
	luat_rtos_task_delete(send_data_task_handle);
}

void luat_service_init(void)
{
	luat_rtos_task_create(&send_data_task_handle, 14 * 1024, 30, "send_data_task", luat_send_data_task_proc, NULL, 0);
}
