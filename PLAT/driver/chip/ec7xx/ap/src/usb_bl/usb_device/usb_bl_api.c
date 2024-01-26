#include "string.h"
#include "cmsis_os2.h"
#include "hal_misc.h"
#include <stdlib.h>
#include <stdio.h>
#include "usb_bl_api.h"
#include "sctdef.h"

int vcomx_isconnect(uint8_t vcom_num);
int vcomx_get_in_waiting(uint8_t vcom_num);
int vcomx_poll_rx(uint8_t vcom_num, uint32_t timeout);
 int vcomx_recv(uint8_t vcom_num, uint8_t* buf, uint16_t len);
 int vcomx_get_out_avail(uint8_t vcom_num);
int vcomx_poll_tx(uint8_t vcom_num, uint32_t timeout);
int vcomx_send(uint8_t vcom_num, uint8_t* buf, uint16_t len);



#ifdef VCOM_DBGEN
uint16_t rcv_to_cnt_arr[VCOM_DBG_LOG_INST_NUM] = {0};
uint16_t snd_to_cnt_arr[VCOM_DBG_LOG_INST_NUM] = {0};


void LOGPutChar(uint8_t ch);

void vcomdbg_hex(uint32_t hex)
{
    uint16_t val;
    for (int32_t idx = 28; idx >=0; idx-=4)
    {
        val = ((hex>>idx)&0xf) + 0x30;
        if(val>=0x3a)
        {
           val = val + 0x27;
        }
        LOGPutChar(val);
    }
}

void vcomdbg_str(uint8_t* str)
{
    while(*str)
    {
        LOGPutChar(*str);
        str++;
    }
}

PLAT_BL_CIRAM_FLASH_TEXT void vcomdbg_str_and_hex(uint8_t* str, uint32_t hex)
{
    vcomdbg_str(str);
    //vcomdbg_str((uint8_t*)"\r\n");
    vcomdbg_hex(hex);
    vcomdbg_str((uint8_t*)"\r\n");
}
#endif

PLAT_BL_CIRAM_FLASH_TEXT void vcom_app_delay(uint32_t ticks)
{
    volatile int us_cnt = 0;
    volatile int loop_in_us;
    //depend on code speed, add 6 for flash code at 50M, cpu 204m
    for (us_cnt = 0; us_cnt < ticks*1000; us_cnt+=6){
        loop_in_us = 0;
        while(loop_in_us++< 70);
    }
}

int vcom_app_recv_proc(uint8_t vcom_num, uint8_t* rcvbuf_ptr, uint32_t size, uint8_t timeout)
{
    int rxwait_cnt;
    int poll_stat;
    uint32_t recvd_size = 0;
    int byte_cnt = 0;
    uint32_t ticks_tmout = timeout; // 1tick for 1ms
    uint32_t ticks_elapsed = 0;
    uint32_t ticks_step  = 1;
#ifdef VCOM_DBGEN
    uint32_t rcv_to_print = 0;
#endif
    if (size ==0)
    {
        return 0;
    }

    while(1)
    {
        if (vcomx_isconnect(vcom_num) <=0)
        {
            VCDBG_STR_AND_HEX("vcom[%d] not connected%x \r\n", vcom_num);
            return ERR_DEV_NO_CONNECT;
        }

        //rxwait_cnt = p_serial_vcom->p_ops->get_in_waiting(p_serial_vcom);
        rxwait_cnt = vcomx_get_in_waiting(vcom_num);
        if (rxwait_cnt <0)
        {
            VCDBG_STR_AND_HEX("get_in_waiting err %d \r\n", rxwait_cnt);
            return rxwait_cnt;
        }
        if (rxwait_cnt ==0)
        {
            if (ticks_elapsed >=ticks_tmout)
            {
                
                #ifdef VCOM_DBGEN
                rcv_to_print = 0;
                rcv_to_cnt_arr[vcom_num]++;
                
                if (vcom_num<VCOM_DBG_LOG_INST_NUM)
                {
                    if ((rcv_to_cnt_arr[vcom_num]%2000)==0)
                    {
                        rcv_to_print = 1;
                    }
                }
                if ((rcv_to_print==1) && (recvd_size==0))
                {
                    VCDBG_STR_AND_HEX("loop1K , recv timeout, vcom_num %d \r\n", vcom_num);                
                }
                #endif
                return recvd_size;
            }
#ifdef VCOM_DBGEN
            //restart to cnt once receive data
            rcv_to_cnt_arr[vcom_num] = 0;
#endif
            //poll_stat = p_serial_vcom->p_ops->poll_rx(p_serial_vcom, 0);
            poll_stat =vcomx_poll_rx(vcom_num, 0);
            if ((poll_stat <0) && (poll_stat!=ERR_DEV_RX_TIMOUT))
            {
                VCDBG_STR_AND_HEX("recv poll state break %d \r\n", poll_stat);
                return poll_stat;
            }
            vcom_app_delay(ticks_step);
            ticks_elapsed += ticks_step;
            continue;
        }

        if ((recvd_size + rxwait_cnt)> size)
        {
            rxwait_cnt = size - recvd_size;
        }

        //byte_cnt = p_serial_vcom->p_ops->recv(p_serial_vcom, rcvbuf_ptr, rxwait_cnt);
        byte_cnt = vcomx_recv(vcom_num, rcvbuf_ptr+recvd_size, rxwait_cnt);
        if (byte_cnt < 0)
        {
            VCDBG_STR_AND_HEX("recv state break %d \r\n", byte_cnt);
            return byte_cnt;
        }

        recvd_size += byte_cnt;
        if (recvd_size>=size)
        {
            return recvd_size;
        }
    }
    return 0;
}

int vcom_app_send_proc(uint8_t vcom_num, uint8_t* sndbuf_ptr, uint32_t size, uint8_t timeout)
{
    int txavail_cnt;
    int poll_stat;
    int byte_cnt;
    uint32_t sended_size = 0;
    uint32_t ticks_tmout = timeout; // 1tick for 1ms
    uint32_t ticks_elapsed = 0;
    uint32_t ticks_step  = 1;
#ifdef VCOM_DBGEN
        uint32_t snd_to_print = 0;
#endif

    while(1)
    {
        if (vcomx_isconnect(vcom_num) <= 0)
        {
            VCDBG_STR_AND_HEX("vcom[%d] not connected \r\n", vcom_num);
            return ERR_DEV_NO_CONNECT;
        }

        //txavail_cnt = p_serial_vcom->p_ops->get_out_avail(p_serial_vcom);
        txavail_cnt = vcomx_get_out_avail(vcom_num);
        if (txavail_cnt == 0)
        {
            if (ticks_elapsed >=ticks_tmout)
            {
                #ifdef VCOM_DBGEN
                snd_to_print = 0;
                snd_to_cnt_arr[vcom_num]++;
                
                if (vcom_num<VCOM_DBG_LOG_INST_NUM)
                {
                    if ((snd_to_cnt_arr[vcom_num]%2000)==0)
                    {
                        snd_to_print = 1;
                    }
                }
                if ((snd_to_print==1) && (sended_size==0))
                {
                    VCDBG_STR_AND_HEX("loop1K , snd timeout, vcom_num %d \r\n", vcom_num);                
                }
                #endif                
                return sended_size;
            }
            //poll_stat = p_serial_vcom->p_ops->poll_tx(p_serial_vcom, 0);
            poll_stat = vcomx_poll_tx(vcom_num, 0);
            if ((poll_stat <0) && (poll_stat!=ERR_DEV_RX_TIMOUT))
            {
                VCDBG_STR_AND_HEX("send poll state break %d \r\n", poll_stat);
                return poll_stat;
            }

            vcom_app_delay(ticks_step);
            ticks_elapsed += ticks_step;
            continue;
        }

#ifdef VCOM_DBGEN
        //any data buf avail, clear snd to cnt
        snd_to_cnt_arr[vcom_num] = 0;
#endif

        if (size < (sended_size + txavail_cnt))
        {
            txavail_cnt = size - sended_size;
        }

        //byte_cnt = p_serial_vcom->p_ops->send(p_serial_vcom, sndbuf_ptr, txavail_cnt);
        byte_cnt = vcomx_send(vcom_num, sndbuf_ptr+sended_size, txavail_cnt);
        if (byte_cnt <0)
        {
            VCDBG_STR_AND_HEX("send state break %d \r\n", byte_cnt);
            return byte_cnt;
        }

        sended_size +=byte_cnt;
        if (sended_size>=size)
        {
            return sended_size;
        }
    }
}

