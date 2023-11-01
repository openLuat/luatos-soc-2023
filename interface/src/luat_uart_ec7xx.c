#include "common_api.h"
#include "platform_define.h"
#include "luat_uart.h"
#include "luat_mcu.h"
#include "driver_gpio.h"
#include "driver_uart.h"
#include "cms_def.h"

#define MAX_DEVICE_COUNT (UART_MAX+1)
static luat_uart_ctrl_param_t uart_cb[MAX_DEVICE_COUNT]={0};
static Buffer_Struct g_s_vuart_rx_buffer;
static uint32_t g_s_vuart_rx_base_len;
static uint8_t g_s_vuart_tx_lock;
typedef struct
{
	timer_t *rs485_timer;
	union
	{
		uint32_t rs485_param;
		struct
		{
			uint32_t wait_time:30;
			uint32_t rx_level:1;
			uint32_t is_485used:1;
		}rs485_param_bit;
	};
	uint32_t rx_buf_size;
	uint8_t rs485_pin;
	struct
	{
		uint8_t is_enable;
		uint8_t is_start;
	}rx_info;
}serials_info;

static serials_info g_s_serials[MAX_DEVICE_COUNT - 1] ={0};

#ifdef __LUATOS__

void luat_uart_recv_cb(int uart_id, uint32_t data_len){
        rtos_msg_t msg;
        msg.handler = l_uart_handler;
        msg.ptr = NULL;
        msg.arg1 = uart_id;
        msg.arg2 = data_len;
        int re = luat_msgbus_put(&msg, 0);
}

void luat_uart_sent_cb(int uart_id, void *param){
        rtos_msg_t msg;
        msg.handler = l_uart_handler;
        msg.ptr = NULL;
        msg.arg1 = uart_id;
        msg.arg2 = 0;
        int re = luat_msgbus_put(&msg, 0);
}
#endif

void luat_uart_sent_dummy_cb(int uart_id, void *param) {;}
void luat_uart_recv_dummy_cb(int uart_id, void *param) {;}

static LUAT_RT_RET_TYPE luat_uart_wait_timer_cb(LUAT_RT_CB_PARAM)
{
    uint32_t uartid = (uint32_t)param;
    if (g_s_serials[uartid].rs485_param_bit.is_485used) {
    	GPIO_Output(g_s_serials[uartid].rs485_pin, g_s_serials[uartid].rs485_param_bit.rx_level);
    }
    uart_cb[uartid].sent_callback_fun(uartid, NULL);
}

static int32_t luat_uart_cb(void *pData, void *pParam){
    uint32_t uartid = (uint32_t)pData;
    uint32_t State = (uint32_t)pParam;
    uint32_t len;
//    DBG("luat_uart_cb pData:%d pParam:%d ",uartid,State);
    switch (State){
        case UART_CB_TX_BUFFER_DONE:
        	if (g_s_serials[uartid].rs485_param_bit.is_485used)
        	{
        		if (g_s_serials[uartid].rs485_param_bit.wait_time)
        		{
        			luat_start_rtos_timer(g_s_serials[uartid].rs485_timer, g_s_serials[uartid].rs485_param_bit.wait_time, 0);
        		}
        		else
        		{
        			GPIO_Output(g_s_serials[uartid].rs485_pin, g_s_serials[uartid].rs485_param_bit.rx_level);
        			uart_cb[uartid].sent_callback_fun(uartid, NULL);
        		}
        	}
        	else
        	{
        		uart_cb[uartid].sent_callback_fun(uartid, NULL);
        	}
            break;
        case UART_CB_TX_ALL_DONE:
			if (g_s_serials[uartid].rs485_param_bit.is_485used) {
				GPIO_Output(g_s_serials[uartid].rs485_pin, g_s_serials[uartid].rs485_param_bit.rx_level);
			}
			uart_cb[uartid].sent_callback_fun(uartid, NULL);
            break;
        case UART_CB_RX_BUFFER_FULL:
        	//只有UART1可以唤醒
#ifdef __LUATOS__
        	if (UART_ID1 == uartid)
        	{
        		uart_cb[uartid].recv_callback_fun(uartid, 0xffffffff);
        	}
#else
        	if (UART_ID1 == uartid)
        	{
        		uart_cb[uartid].recv_callback_fun(uartid, 0);
        	}
#endif
        	break;
        case UART_CB_RX_TIMEOUT:
			if (g_s_serials[uartid].rx_info.is_enable)
			{
				g_s_serials[uartid].rx_info.is_start = 0;
			}
            len = Uart_RxBufferRead(uartid, NULL, 0);
            uart_cb[uartid].recv_callback_fun(uartid, len);
            break;
        case UART_CB_RX_NEW:
        	len = Uart_RxBufferRead(uartid, NULL, 0);
			if(g_s_serials[uartid].rx_info.is_enable)
			{
				if (!g_s_serials[uartid].rx_info.is_start)
				{
					g_s_serials[uartid].rx_info.is_start = 1;
					uart_cb[uartid].recv_callback_fun(uartid, 0xffffffff);
				}
			}
        	if (len > g_s_serials[uartid].rx_buf_size)
        	{
        		uart_cb[uartid].recv_callback_fun(uartid, len);
        	}
            break;
        case UART_CB_ERROR:
            break;
	}
}

int luat_uart_exist(int uartid) {
    if (uartid >= LUAT_VUART_ID_0) uartid = MAX_DEVICE_COUNT - 1;
    return (uartid >= MAX_DEVICE_COUNT)?0:1;
}



int luat_uart_setup(luat_uart_t* uart) {
    if (!luat_uart_exist(uart->id)) {
        return -1;
    }
	size_t buffsize = uart->bufsz;
	if (buffsize < 2048)
		buffsize = 2048;
	else if (buffsize > 16*1024)
		buffsize = 16*1024;
    if (uart->id >= MAX_DEVICE_COUNT){
		OS_ReInitBuffer(&g_s_vuart_rx_buffer, buffsize);
		g_s_vuart_rx_base_len = g_s_vuart_rx_buffer.MaxLen;
        return 0;
    }
    char model[40] = {0};
#ifdef __LUATOS__
    //Uart_SetDebug(uart->id, 1);
	Uart_SetErrorDropData(uart->id, 1);
#endif
    if(luat_mcu_iomux_is_default(LUAT_MCU_PERIPHERAL_UART, uart->id))
    {
        switch (uart->id)
        {
        case UART_ID0:
            GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(HAL_GPIO_16, 0), 1, 0, 0);
	        GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(HAL_GPIO_17, 0), 1, 0, 0);
	        GPIO_PullConfig(GPIO_ToPadEC7XX(HAL_GPIO_16, 0), 1, 1);
        case UART_ID1:
            GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(HAL_GPIO_18, 0), 1, 0, 0);
	        GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(HAL_GPIO_19, 0), 1, 0, 0);
	        GPIO_PullConfig(GPIO_ToPadEC7XX(HAL_GPIO_18, 0), 1, 1);
            break;
        case UART_ID2:
            GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(HAL_GPIO_12, 0), 3, 0, 0);
		    GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(HAL_GPIO_13, 0), 3, 0, 0);
		    GPIO_PullConfig(GPIO_ToPadEC7XX(HAL_GPIO_12, 0), 1, 1);
            break;
        case UART_ID3:
            GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(HAL_GPIO_34, 0), 3, 0, 0);
		    GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(HAL_GPIO_35, 0), 3, 0, 0);
		    GPIO_PullConfig(GPIO_ToPadEC7XX(HAL_GPIO_34, 0), 1, 1);
        default:
            break;
        }
    }
    int parity = 0;
     if (uart->parity == 1)parity = UART_PARITY_ODD;
     else if (uart->parity == 2)parity = UART_PARITY_EVEN;
     int stop_bits = (uart->stop_bits)==1?UART_STOP_BIT1:UART_STOP_BIT2;
     {
    	 if (!uart_cb[uart->id].recv_callback_fun)
    		 uart_cb[uart->id].recv_callback_fun = luat_uart_recv_dummy_cb;
    	 if (!uart_cb[uart->id].sent_callback_fun)
    		 uart_cb[uart->id].sent_callback_fun = luat_uart_sent_dummy_cb;
    	 g_s_serials[uart->id].rx_buf_size = buffsize;
         Uart_BaseInitEx(uart->id, uart->baud_rate, 1024, buffsize, (uart->data_bits), parity, stop_bits, luat_uart_cb);
 #ifdef __LUATOS__
          g_s_serials[uart->id].rs485_param_bit.is_485used = (uart->pin485 < HAL_GPIO_NONE)?1:0;
 #else
 		 g_s_serials[uart->id].rs485_param_bit.is_485used = (uart->delay > 0)?1:0;
 #endif
 		 g_s_serials[uart->id].rs485_pin = uart->pin485;
          g_s_serials[uart->id].rs485_param_bit.rx_level = uart->rx_level;

          g_s_serials[uart->id].rs485_param_bit.wait_time = uart->delay/1000;
          if (!g_s_serials[uart->id].rs485_param_bit.wait_time)
          {
         	 g_s_serials[uart->id].rs485_param_bit.wait_time = 1;
          }
 		 if (g_s_serials[uart->id].rs485_param_bit.is_485used)
 		 {
          	if (!g_s_serials[uart->id].rs485_timer) {
          		g_s_serials[uart->id].rs485_timer = luat_create_rtos_timer(luat_uart_wait_timer_cb, uart->id, NULL);
          	}
          	GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(g_s_serials[uart->id].rs485_pin, 0), 0, 0, 0);
          	GPIO_Config(g_s_serials[uart->id].rs485_pin, 0, g_s_serials[uart->id].rs485_param_bit.rx_level);
 		 }
    }
    return 0;
}

int luat_uart_write(int uartid, void* data, size_t length) {
    if (luat_uart_exist(uartid)) {
         if (uartid >= MAX_DEVICE_COUNT){
         	g_s_vuart_tx_lock = 1;
		 	unsigned i = 0;
		 	while(i < length)
		 	{
		 		if ((length - i) <= 512)
		 		{
		 			g_s_vuart_tx_lock = 0;
		 			usb_serial_output(CMS_CHAN_4, data + i, length - i);
		 			i = length;
		 		}
		 		else
		 		{
		 			usb_serial_output(CMS_CHAN_4, data + i, 512);
		 			i += 512;
		 		}
		 	}

             return length;
         }else{
        	 if (g_s_serials[uartid].rs485_param_bit.is_485used) GPIO_Output(g_s_serials[uartid].rs485_pin, !g_s_serials[uartid].rs485_param_bit.rx_level);
        	int ret = Uart_TxTaskSafe(uartid, data, length);
            if (ret == 0)
                return length;
            return 0;
         }
    }
    else {
        return -1;
    }
    return 0;
}

void luat_uart_clear_rx_cache(int uartid)
{
	 if (luat_uart_exist(uartid)) {
	 	if (uartid >= MAX_DEVICE_COUNT){
	 		g_s_vuart_rx_buffer.Pos = 0;
	 	}
	 	else
	 	{
			Uart_RxBufferClear(uartid);
	 	}
	 }
}

int luat_uart_read(int uartid, void* buffer, size_t len) {
    int rcount = 0;
    if (luat_uart_exist(uartid)) {

         if (uartid >= MAX_DEVICE_COUNT){
         	if (!buffer)
         	{
         		return g_s_vuart_rx_buffer.Pos;
         	}
             rcount = (g_s_vuart_rx_buffer.Pos > len)?len:g_s_vuart_rx_buffer.Pos;
             memcpy(buffer, g_s_vuart_rx_buffer.Data, rcount);
             OS_BufferRemove(&g_s_vuart_rx_buffer, rcount);
             if (!g_s_vuart_rx_buffer.Pos && g_s_vuart_rx_buffer.MaxLen > g_s_vuart_rx_base_len)
             {
             	OS_ReInitBuffer(&g_s_vuart_rx_buffer, g_s_vuart_rx_base_len);
             }
         }
         else
         {
        	rcount = Uart_RxBufferRead(uartid, (uint8_t *)buffer, len);
         }
    }
    return rcount;
}

int luat_uart_close(int uartid) {
    if (luat_uart_exist(uartid)) {
         if (uartid >= MAX_DEVICE_COUNT){
         	OS_DeInitBuffer(&g_s_vuart_rx_buffer);
             return 0;
         }
        Uart_DeInit(uartid);
        return 0;
    }
    return -1;
}


#ifdef __LUATOS__
void soc_usb_serial_output_done(uint8_t channel)
{
	if (!g_s_vuart_tx_lock)
	{
		luat_uart_sent_cb(LUAT_VUART_ID_0, NULL);
	}

}
#endif

static void luat_usb_recv_cb(uint8_t channel, uint8_t *input, uint32_t len){
	if (input) {
		if (!g_s_vuart_rx_buffer.MaxLen)
		{
			DBG("usb serial not init,%d,%x,%d!", channel, input, len);
			return;
		}
        OS_BufferWrite(&g_s_vuart_rx_buffer, input, len);
#ifdef __LUATOS__
        rtos_msg_t msg;
        msg.handler = l_uart_handler;
        msg.ptr = NULL;
        msg.arg1 = LUAT_VUART_ID_0;
        msg.arg2 = len;
        int re = luat_msgbus_put(&msg, 0);
#endif
        if (uart_cb[UART_MAX].recv_callback_fun){
            uart_cb[UART_MAX].recv_callback_fun(LUAT_VUART_ID_0,len);
        }
	}else{
		switch(len){
            case 0:
                DBG("usb serial connected");
                break;
            default:
                DBG("usb serial disconnected");
                break;
		}
	}
}

int luat_setup_cb(int uartid, int received, int sent) {
    if (luat_uart_exist(uartid)) {
#ifdef __LUATOS__
        if (uartid >= UART_MAX){
            set_usb_serial_input_callback(luat_usb_recv_cb);
        }else{
            if (received){
                uart_cb[uartid].recv_callback_fun = luat_uart_recv_cb;
            }

            if (sent){
                uart_cb[uartid].sent_callback_fun = luat_uart_sent_cb;
            }
        }
#endif
    }
    return 0;
}

int luat_uart_ctrl(int uart_id, LUAT_UART_CTRL_CMD_E cmd, void* param){
    if (luat_uart_exist(uart_id)) {
        if (uart_id >= MAX_DEVICE_COUNT){
            uart_id = UART_MAX;
            set_usb_serial_input_callback(luat_usb_recv_cb);
        }
        if (cmd == LUAT_UART_SET_RECV_CALLBACK){
            uart_cb[uart_id].recv_callback_fun = param;
        }else if(cmd == LUAT_UART_SET_SENT_CALLBACK){
            uart_cb[uart_id].sent_callback_fun = param;
        }
        return 0;
    }
    return -1;
}

int luat_uart_wait_485_tx_done(int uartid)
{
	int cnt = 0;
    if (luat_uart_exist(uartid)){
		if (g_s_serials[uartid].rs485_param_bit.is_485used) {
			while(!Uart_IsTSREmpty(uartid)) {cnt++;}
			GPIO_Output(g_s_serials[uartid].rs485_pin, g_s_serials[uartid].rs485_param_bit.rx_level);
		}
    }
    return cnt;
}

int luat_uart_rx_start_notify_enable(int uart_id, uint8_t is_enable)
{
	if (luat_uart_exist(uart_id)){
		g_s_serials[uart_id].rx_info.is_enable = is_enable;
		return 0;
    }
    return -1;
}

