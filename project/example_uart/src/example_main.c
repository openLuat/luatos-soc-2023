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
#include "luat_debug.h"

#include "luat_uart.h"
#include "plat_config.h"
/*
 一.上报接收数据中断的逻辑：
    1.串口初始化时，新建一个缓冲区
    2.可以考虑多为用户申请几百字节的缓冲长度，用户处理时防止丢包
    3.每次串口收到数据时，先存入缓冲区，记录长度
    4.遇到以下情况时，再调用串口中断
        a)缓冲区满（帮用户多申请的的情况）/缓冲区只剩几百字节（按实际长度申请缓冲区的情况）
        b)收到fifo接收超时中断（此时串口数据应该是没有继续收了）
    5.触发收到数据中断时，返回的数据应是缓冲区的数据
    6.关闭串口时，释放缓冲区资源
*/

uint16_t crc16ccitt(uint16_t crc, uint8_t *bytes, int start, int len)
{
    for (; start < len; start++) {
        crc = ((crc >> 8) | (crc << 8)) & 0xffff;
        crc ^= (bytes[start] & 0xff);
        crc ^= ((crc & 0xff) >> 4);
        crc ^= (crc << 12) & 0xffff;
        crc ^= ((crc & 0xFF) << 5) & 0xffff;
    }
    crc &= 0xffff;
    return crc;
}

static luat_rtos_task_handle uart1_task_handle;
static luat_rtos_task_handle uart1_recv_task_handle;
static luat_rtos_semaphore_t uart_send_semaphore_handle;
void luat_uart1_recv_cb(int uart_id, uint32_t data_len){
    char* data_buff = malloc(data_len + 1);
    memset(data_buff,0,data_len + 1);
    luat_uart_read(uart_id, data_buff, data_len);
    LUAT_DEBUG_PRINT("uart_id:%d data_len:%d",uart_id,data_len);
    luat_rtos_event_send(uart1_recv_task_handle, 2, data_buff, data_len, 0, 0);
}


void luat_uart1_send_cb(int uart_id, void *param)
{
    luat_rtos_semaphore_release(uart_send_semaphore_handle);
}

static void task_test_uart1(void *param)
{
    luat_rtos_task_sleep(2000);
    luat_event_t event;
    luat_rtos_semaphore_create(&uart_send_semaphore_handle, 1);
    char send_buff[] = " !!!\n";
    luat_uart_t uart = {
        .id = UART_ID1,
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity    = 0
    };
    luat_uart_setup(&uart);
    luat_uart_ctrl(UART_ID1, LUAT_UART_SET_RECV_CALLBACK, luat_uart1_recv_cb);
    luat_uart_ctrl(UART_ID1, LUAT_UART_SET_SENT_CALLBACK, luat_uart1_send_cb);
    while(1)
    {
        for (int i = 1; i < 65; i ++)
        {
            char *data = malloc(i * 1024);
            memset(data, 0x00, i * 1024);
            for(int j = 0; j < i * 1024; j++)
	        {
		        data[j] = j & 0xff;
	        }
            uint16_t crc = 0xFFFF;
            crc = crc16ccitt(crc, data, 0, i * 1024);
            luat_rtos_event_send(uart1_recv_task_handle, 1, i * 1024, crc, 0, 0);
            LUAT_DEBUG_PRINT("send result %d", luat_uart_write(UART_ID1, data, i * 1024));
            luat_rtos_semaphore_take(uart_send_semaphore_handle, LUAT_WAIT_FOREVER);
            free(data);
            luat_rtos_task_sleep(1000);
        }
        luat_rtos_task_sleep(600000);
    }

    while (1)
    {
        luat_rtos_task_sleep(1000);
        LUAT_DEBUG_PRINT("send result %d", luat_uart_write(UART_ID1, send_buff, strlen(send_buff)));
    }
    luat_rtos_task_delete(uart1_task_handle);
}


static void task_test_recv(void *param)
{
    luat_event_t event;
    uint32_t total = 0;
    uint32_t now = 0;
    uint16_t crc1, crc2;
    char *data = NULL;
    char *waitCrcData = NULL;
    uint32_t index = 0;
    while(1)
    {
        luat_rtos_event_recv(uart1_recv_task_handle, 0, &event, NULL, LUAT_WAIT_FOREVER);
        switch(event.id)
		{
		case 1:
			total = event.param1;
            crc1 = event.param2;
            crc2 = 0xFFFF;
            waitCrcData = malloc(total);
            now = 0;
            index = 0;
			break;
		case 2:
            data = event.param1;
            now += event.param2;
            if (now == total)
            {
                memcpy(waitCrcData + index, data, event.param2);
                crc2 = crc16ccitt(crc2, waitCrcData, 0, total);
                free(waitCrcData);
                if(crc1 == crc2)
                {
                    LUAT_DEBUG_PRINT("success %d, %d, %d, %d", now, total, crc1, crc2);
                }
            }
            else
            {
                memcpy(waitCrcData + index, data, event.param2);
            }
            index += event.param2;
            free(data);
            data = NULL;
			break;
        default:
            break;
		}
    }
}



static void uart_demo_task_init(void)
{
    // luat_rtos_task_create(&uart0_task_handle, 2048, 20, "uart0", task_test_uart0, NULL, NULL);
    luat_rtos_task_create(&uart1_task_handle, 2048, 20, "uart1", task_test_uart1, NULL, NULL);
    luat_rtos_task_create(&uart1_recv_task_handle, 2048, 20, "uart1 recv", task_test_recv, NULL, 64);
    // luat_rtos_task_create(&uart2_task_handle, 2048, 20, "uart2", task_test_uart2, NULL, NULL);
    // luat_rtos_task_create(&uart3_task_handle, 2048, 20, "uart3", task_test_uart3, NULL, NULL);
}

INIT_TASK_EXPORT(uart_demo_task_init,"1");



