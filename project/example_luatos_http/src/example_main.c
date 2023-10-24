#include "common_api.h"
#include "luat_rtos.h"
#include "luat_gpio.h"
#include "luat_debug.h"
#include "luat_mobile.h"
#include "luat_network_adapter.h"
#include "networkmgr.h"
#include "luat_http.h"
#include "luat_mem.h"

static luat_rtos_task_handle g_s_task_handle;

enum
{
	TEST_HTTP_GET_HEAD_DONE = 1,
	TEST_HTTP_GET_DATA,
	TEST_HTTP_GET_DATA_DONE,
	TEST_HTTP_FAILED,
};
static void luatos_mobile_event_callback(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
	if (LUAT_MOBILE_EVENT_NETIF == event)
	{
		if (LUAT_MOBILE_NETIF_LINK_ON == status)
		{

		}
	}
}

static void luatos_http_cb(int status, void *data, uint32_t len, void *param)
{
	uint8_t *ota_data;
    if(status < 0) 
    {
		luat_rtos_event_send(param, TEST_HTTP_FAILED, 0, 0, 0, 0);
		return;
    }
	switch(status)
	{
	case HTTP_STATE_GET_BODY:
		if (data)
		{
			ota_data = malloc(len);
			memcpy(ota_data, data, len);
			luat_rtos_event_send(param, TEST_HTTP_GET_DATA, (uint32_t)ota_data, len, 0, 0);
		}
		else
		{
			luat_rtos_event_send(param, TEST_HTTP_GET_DATA_DONE, 0, 0, 0, 0);
		}
		break;
	case HTTP_STATE_GET_HEAD:
		if (data)
		{
			LUAT_DEBUG_PRINT("%s", data);
		}
		else
		{
			luat_rtos_event_send(param, TEST_HTTP_GET_HEAD_DONE, 0, 0, 0, 0);
		}
		break;
	case HTTP_STATE_IDLE:
		break;
	case HTTP_STATE_SEND_BODY_START:
		//如果是POST，在这里发送POST的body数据，如果一次发送不完，可以在HTTP_STATE_SEND_BODY回调里继续发送
		break;
	case HTTP_STATE_SEND_BODY:
		//如果是POST，可以在这里发送POST剩余的body数据
		break;
	default:
		break;
	}
}

static void luat_test_task(void *param)
{
	luat_event_t event = {0};
	uint8_t is_end = 0;
	uint32_t done_len = 0;
	luat_http_ctrl_t *http = luat_http_client_create(luatos_http_cb, luat_rtos_get_current_handle(), -1);
	const char remote_domain[200];
	snprintf((char *)remote_domain, 200, "%s", "http://www.baidu.com");
    LUAT_DEBUG_PRINT("print url %s", remote_domain);
	luat_http_client_start(http, remote_domain, 0, 0, 1);
	while (!is_end)
	{
		luat_rtos_event_recv(g_s_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
		switch(event.id)
		{
		case TEST_HTTP_GET_HEAD_DONE:
			// 在这里处理http响应头
			done_len = 0;
			LUAT_DEBUG_PRINT("status %d total %u", luat_http_client_get_status_code(http), http->total_len);
			break;
		case TEST_HTTP_GET_DATA:
			// 在这里处理用户数据
			done_len += event.param2;
			LUAT_DEBUG_PRINT("this is step count %d, total count %d", event.param2, done_len);
			free((char *)event.param1);
			break;
		case TEST_HTTP_GET_DATA_DONE:
			is_end = 1;
			break;
		case TEST_HTTP_FAILED:
			is_end = 1;
			break;
		default:
			break;
		}
	}

	luat_http_client_close(http);
	luat_http_client_destroy(&http);
	while(1)
	{
		luat_rtos_task_sleep(60000);
	}
}

static void luat_test_init(void)
{
	luat_mobile_event_register_handler(luatos_mobile_event_callback);
	luat_mobile_set_period_work(0, 5000, 0);
	luat_rtos_task_create(&g_s_task_handle, 4 * 1024, 50, "test", luat_test_task, NULL, 16);

}

INIT_TASK_EXPORT(luat_test_init, "1");
