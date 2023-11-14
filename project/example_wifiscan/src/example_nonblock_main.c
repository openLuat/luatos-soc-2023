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
#include "luat_rtos.h"
#include "luat_mem.h"
#include "luat_debug.h"
#include "luat_pm.h"
#include "luat_wlan.h"
#include "cmidev.h"

enum
{
	LUAT_GET_HEAD_DONE = 1,
};

luat_rtos_task_handle wifi_scan_nonblock_task_handle;
luat_wifisacn_get_info_t *wifiscan_get;
CHAR Wifiscan_req[96] = {0};
//soc_service_misc_callback为弱函数，参考下面直接调用
void soc_service_misc_callback(uint8_t *data, uint32_t len)
{
	uint8_t *http_down_data;
	uint8_t sg_id = (((len) >> 12) & 0x000F);
	uint16_t prim_id = ((len)&0x0FFF);
	int ret=-1;
	switch (sg_id)
	{
	case CAM_DEV:
		switch (prim_id)
		{
		case CMI_DEV_SET_WIFISCAN_CNF://下面为处理函数   
		 LUAT_DEBUG_PRINT("data len%d\r\n",len);
		 http_down_data = malloc(len);
		 memcpy(http_down_data, data, len);
		 luat_rtos_event_send(wifi_scan_nonblock_task_handle, LUAT_GET_HEAD_DONE, http_down_data, len, 0, 0);
		break;
		}
		break;
	}
}
static void wifi_scan_nonblock_task(void *param)
{
	luat_event_t event;
	int ret = -1;
	size_t total, used, max_used;
	luat_wifiscan_set_info_t wifiscan_info = {0};
	wifiscan_info.maxTimeOut = 10000;
	wifiscan_info.round = 1;
	wifiscan_info.maxBssidNum = 10;
	wifiscan_info.scanTimeOut = 3;
	wifiscan_info.wifiPriority = LUAT_WIFISCAN_DATA_PERFERRD;
	wifiscan_info.channelCount = 1;
	wifiscan_info.channelRecLen = 280;
	wifiscan_info.channelId[0] = 0;
	while (1)
	{
		luat_wlan_scan_nonblock(&wifiscan_info);//调用异步wifi_scan函数
		luat_rtos_event_recv(wifi_scan_nonblock_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
		switch (event.id)
		{
		case LUAT_GET_HEAD_DONE:
			wifiscan_get = (luat_wifisacn_get_info_t *)malloc(sizeof(luat_wifisacn_get_info_t));
			memset(wifiscan_get, 0, sizeof(luat_wifisacn_get_info_t));
			memcpy(wifiscan_get,event.param1, sizeof(luat_wifisacn_get_info_t));
			LUAT_DEBUG_PRINT("this hw demo%d", wifiscan_get->bssidNum);
			for (size_t i = 0; i < wifiscan_get->bssidNum; i++)
			{
				memset(Wifiscan_req, 0, sizeof(Wifiscan_req));
				snprintf_(Wifiscan_req, 64, "%.*s, \"%02x:%02x:%02x:%02x:%02x:%02x\",%d,%d", wifiscan_get->ssidHexLen[i], wifiscan_get->ssidHex[i], wifiscan_get->bssid[i][0], wifiscan_get->bssid[i][1], wifiscan_get->bssid[i][2],
						  wifiscan_get->bssid[i][3], wifiscan_get->bssid[i][4], wifiscan_get->bssid[i][5], wifiscan_get->rssi[i], wifiscan_get->channel[i]);
				LUAT_DEBUG_PRINT("wifiscan result %s", Wifiscan_req);
				
			}
			free(wifiscan_get);
			free(event.param1);
			break;
		
		default:
			break;
		}
		luat_rtos_task_sleep(10000);
		luat_meminfo_sys(&total, &used, &max_used);
    	LUAT_DEBUG_PRINT("meminfo total %d, used %d, max_used%d",total, used, max_used);
	}
}



static void wifi_scan_nonblock_task_init(void)
{	
	luat_rtos_task_create(&wifi_scan_nonblock_task_handle, 2 * 1024, 50, "wifi_scan_nonblock_task", wifi_scan_nonblock_task, NULL, 16);
}
INIT_TASK_EXPORT(wifi_scan_nonblock_task_init, "1");
