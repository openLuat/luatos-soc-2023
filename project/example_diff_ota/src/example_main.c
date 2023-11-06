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

/**
 * 如果使用全量升级，必须是718P，必须在项目的xmake.lua里设置AP_FLASH_LOAD_SIZE，AP_PKGIMG_LIMIT_SIZE，FULL_OTA_SAVE_ADDR。
 * AP_FLASH_LOAD_SIZE必须减小到能放下代码，同时有剩余空间能放下OTA包，这里的剩余空间包括了原本的DFOTA区域
 * AP_PKGIMG_LIMIT_SIZE必须和AP_FLASH_LOAD_SIZE一致，FULL_OTA_SAVE_ADDR必须在实际AP占用空间之后，OTA保存空间长度=FLASH_FS_REGION_START-FULL_OTA_SAVE_ADDR
 * 如果不清楚的，直接参考本demo的xmake.lua配置
 * luatools需要2.2.15及以上版本，进入选项及工具->SOC差分/整包升级包制作工具，新版固件选择要升级的soc包，
 * 用户标识可以填版本号，也可以不填，看自己的代码对升级版本是否有控制，如果用合宙IOT升级服务，则必须填写版本号！！！
 * 选好输出路径，点生成，在输出目录下生成相应的xxx.sota文件为升级包，放在服务器上下载即可升级
 *
 * 如果使用差分升级，luatools需要2.2.15及以上版本，进入选项及工具->SOC差分/整包升级包制作工具，
 * 旧版固件选择当前模块烧录的soc包，新版固件选择要升级的soc包，用户标识可以填版本号，也可以不填，看自己的代码对升级版本是否有控制，如果用合宙IOT升级服务，则必须填写版本号！！！
 * 选好输出路径，点生成，在输出目录下生成相应的xxx.sota文件为升级包，放在服务器上下载即可升级
 *
 *
 *

*/

#include "common_api.h"

#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_flash.h"
#include "luat_crypto.h"
#include "luat_fota.h"
#include "mem_map.h"
#include "luat_ymodem.h"
#include "luat_uart.h"

//#define OTA_BY_HTTP		//OTA文件通过自定义HTTP服务器下载
//#define OTA_BY_IOT		//OTA文件通过合宙IOT服务器下载
#define OTA_BY_USB			//OTA文件通过USB虚拟串口下载，打开串口工具时需要勾选DTR或者RTS，否则模块发不出同步码
#define UART_ID LUAT_VUART_ID_0
luat_rtos_task_handle g_s_task_handle;
enum
{
	OTA_NEW_DATA = USER_EVENT_ID_START+1,
};

#ifdef OTA_BY_USB
static void luat_uart_recv_cb(int uart_id, uint32_t data_len)
{
	luat_rtos_event_send(g_s_task_handle, OTA_NEW_DATA, 0, 0, 0, 0);
}
static void luat_ymodem_cb(uint8_t *data, uint32_t data_len)
{
	if (data)
	{
		if (data_len)
		{
			LUAT_DEBUG_PRINT("get data %u", data_len);
			luat_fota_write(data, data_len);
		}
		else
		{
			LUAT_DEBUG_PRINT("file name %s, but no need", data);
		}
	}
	else
	{
		if (data_len)
		{
			LUAT_DEBUG_PRINT("file len %u", data_len);
		}
		else
		{
			LUAT_DEBUG_PRINT("end");
			if (!luat_fota_done())
			{
				LUAT_DEBUG_PRINT("OTA包写入完成，重启模块!");
				luat_os_reboot(0);
			}
		}
	}
}

#endif

static void luat_test_task(void *param)
{
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET);
#ifdef OTA_BY_USB
	uint8_t temp[128];
    luat_uart_t uart = {
        .id = UART_ID,
        .baud_rate = 3000000,
        .data_bits = 8,
        .stop_bits = 1,
        .parity    = 0
    };
    LUAT_DEBUG_PRINT("setup result %d", luat_uart_setup(&uart));
    LUAT_DEBUG_PRINT("ctrl result %d", luat_uart_ctrl(UART_ID, LUAT_UART_SET_RECV_CALLBACK, luat_uart_recv_cb));
    void *ymodem_handler = luat_ymodem_create_handler(luat_ymodem_cb);
    luat_event_t event;
    uint32_t read_len;
    uint8_t ack, flag, file_ok, all_done;
    int result;
    luat_fota_init(0,0,NULL,NULL,0);
    while(1)
    {
    	if (luat_rtos_event_recv(g_s_task_handle, OTA_NEW_DATA, &event, NULL, 250))
    	{
    		DBG("!");
    		luat_ymodem_reset(ymodem_handler);
    		luat_uart_write(UART_ID, "C", 1);
    	}
    	else
    	{
    		read_len = luat_uart_read(UART_ID, temp, 128);
    		while(read_len > 0)
    		{
    			result = luat_ymodem_receive(ymodem_handler, temp, read_len, &ack, &flag, &file_ok, &all_done);
    			if (result)
    			{
    				LUAT_DEBUG_PRINT("%d,%d,%d", result, ack, flag);
    			}
    			if (ack)
    			{
    				luat_uart_write(UART_ID, &ack, 1);
    			}
    			if (flag)
    			{
    				luat_uart_write(UART_ID, &flag, 1);
    			}
    			read_len = luat_uart_read(UART_ID, temp, 128);
    		}

    	}
    }

#endif

}

static void luat_test_init(void)
{
	luat_rtos_task_create(&g_s_task_handle, 4 * 1024, 50, "test", luat_test_task, NULL, 0);

}

INIT_TASK_EXPORT(luat_test_init, "1");



