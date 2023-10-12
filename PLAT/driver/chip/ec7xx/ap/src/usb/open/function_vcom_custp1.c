#include "usbd_clscdc.h"
#include "usbd_clscdc_cust_tp.h"

//#include "func_dbg.h"
#include "string.h"
#include "usbd_macro_def.h"
#include "usbd_func_cconf.h"
#include "usbd_func_cc.h"
#include "usbd_multi_usrcfg_common.h"


const usbd_cdc_desc_custp1_st t_usbd_cdc_desc_custp1 = {
    .intf_ctrl_desc = {
        /*Interface Descriptor */
        0x09,   /* bLength: Interface Descriptor size */
        USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: Interface */
        /* Interface descriptor type */
        INVALID_INTF_NUM, //VCOM_INTF_NUM_0,   /* bInterfaceNumber: Number of Interface */
        0x00,   /* bAlternateSetting: Alternate setting */
        0x02,   /* bNumEndpoints: One endpoints used */
        0x02,   /* bInterfaceClass: Communication Interface Class */
        0x02,   /* bInterfaceSubClass: Abstract Control Model */
        0x01,   /* bInterfaceProtocol: Common AT commands */
        0x00,   /* iInterface: */    
    },

    .cdc_diep_desc = {
        /*Endpoint IN Descriptor*/
        0x07,   /* bLength: Endpoint Descriptor size */
        USB_ENDPOINT_DESCRIPTOR_TYPE,     /* bDescriptorType: Endpoint */
        INVALID_EP_NUM, //CDC_IN_EP,                        /* bEndpointAddress */
        0x02,                             /* bmAttributes: Bulk */
        LOBYTE(CDC_DATA_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
        HIBYTE(CDC_DATA_MAX_PACKET_SIZE),
        0x00                              /* bInterval */    
    },
    
    .cdc_doep_desc = {
        /*Endpoint OUT Descriptor*/
        0x07,   /* bLength: Endpoint Descriptor size */
        USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType: Endpoint */
        INVALID_EP_NUM, //CDC_OUT_EP,                        /* bEndpointAddress */
        0x02,                              /* bmAttributes: Bulk */
        LOBYTE(CDC_DATA_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
        HIBYTE(CDC_DATA_MAX_PACKET_SIZE),
        0x00,                              /* bInterval: ignore for Bulk transfer */
    }
    
};

uint8_t vcom_func_custp1_desc_ass(usb_func_ccinst_st *p_func_ccinst)
{
    ccinst_cdc_setting_st* p_ccinst_cdc_setting;
    p_ccinst_cdc_setting = p_func_ccinst->p_cc_setting;    
    p_ccinst_cdc_setting->desc_assigned = 0;
    
    #ifdef VCOM_CHK_CCINST_TYPE    
    if ((p_ccinst_cdc_setting->bs_set.setting_maintp!=ccinst_setting_cdc_vcom_maintp) ||
         (p_ccinst_cdc_setting->bs_set.setting_subtp != ccinst_setting_vcom_subtp1_2ep))
    {
        return usbd_ret_fail;
    }
    #endif    
    p_ccinst_cdc_setting->p_desc_src = (const uint8_t  *)(&t_usbd_cdc_desc_custp1);
    p_ccinst_cdc_setting->desc_src_size = sizeof(t_usbd_cdc_desc_custp1);
    
    p_ccinst_cdc_setting->desc_assigned = 1;
    return usbd_ret_no_err;    
}

uint8_t vcom_func_custp1_desc_parse(usb_func_ccinst_st *p_func_ccinst)
{
    const uint8_t * p_desc_buf;
    uint16_t pos = 0;
    uint8_t ret = usbd_ret_no_err;
    ccinst_cdc_setting_st* p_ccinst_cdc_setting;
    usb_endpoint_descriptor_st * p_endp_d;
    p_ccinst_cdc_setting = p_func_ccinst->p_cc_setting;
    #ifdef VCOM_CHK_CCINST_TYPE    
    if ((p_ccinst_cdc_setting->bs_set.setting_maintp!=ccinst_setting_cdc_vcom_maintp) ||
         (p_ccinst_cdc_setting->bs_set.setting_subtp != ccinst_setting_vcom_subtp1_2ep))
    {
        return usbd_ret_fail;
    }
    #endif
    if(p_ccinst_cdc_setting->desc_assigned!=1)
    {
        return usbd_ret_fail;
    }    
    p_desc_buf = p_ccinst_cdc_setting->p_desc_src;

    p_ccinst_cdc_setting->desc_parsed = 0;

    p_ccinst_cdc_setting->epctrl_valid = 0;
    p_ccinst_cdc_setting->epin_valid = 0;
    p_ccinst_cdc_setting->epout_valid = 0;

    p_ccinst_cdc_setting->interface_cnt = 0;
    
    while(pos < p_ccinst_cdc_setting->desc_src_size)
    {
        if (p_desc_buf[pos] == 0)
        {
            ret = usbd_ret_fail;
            break;
        }
        if ((pos+ p_desc_buf[pos]) > p_ccinst_cdc_setting->desc_src_size)
        {
            ret = usbd_ret_fail;
            break;        
        }
        
        switch  (p_desc_buf[pos+1])
        {
            case USB_ENDPOINT_DESCRIPTOR_TYPE:
                if (p_desc_buf[pos] != STANDARD_ENDPOINT_DESC_SIZE)
                {
                    ret = usbd_ret_fail;
                    break;
                }

            p_endp_d =  (usb_endpoint_descriptor_st *)(&(p_desc_buf[pos]));
            if (p_endp_d->bmAttributes == USBC_CTRL_EP_BULK) 
            {
                //if (p_endp_d->bEndpointAddress &0x80)
                
                if (p_endp_d == (&t_usbd_cdc_desc_custp1.cdc_diep_desc))
                {
                    p_ccinst_cdc_setting->epin_valid = 1;
                    p_ccinst_cdc_setting->p_epin_d = p_endp_d;
                }
                else if (p_endp_d == (&t_usbd_cdc_desc_custp1.cdc_doep_desc))
                {
                    p_ccinst_cdc_setting->epout_valid = 1;     
                    p_ccinst_cdc_setting->p_epout_d = p_endp_d;                    
                }
                else
                {
                    ret = usbd_ret_fail;
                    break;
                }
            }
            else if (p_endp_d->bmAttributes == USBC_CTRL_EP_INTR)
            {
                if (p_endp_d->bEndpointAddress &0x80)
                {
                    p_ccinst_cdc_setting->epctrl_valid = 1;
                    p_ccinst_cdc_setting->p_epctrl_d = p_endp_d;                    
                }    
                else
                {
                    ret = usbd_ret_fail;
                    break;
                }
            }
            else
            {
                ret = usbd_ret_fail;
                break;            
            }
                break;
            case USB_INTERFACE_DESCRIPTOR_TYPE:
                if (p_desc_buf[pos] !=STANDARD_INTERFACE_DESC_SIZE)
                {
                    ret = usbd_ret_fail;
                    break;
                }
                p_ccinst_cdc_setting->interface_cnt++;
                break;
            default:
                break;
        }
        if (ret!=usbd_ret_no_err)
        {
            break;
        }
        pos+= p_desc_buf[pos];
    }
    
    if ((p_ccinst_cdc_setting->epctrl_valid ==1) ||
        (p_ccinst_cdc_setting->epin_valid ==0) ||
        (p_ccinst_cdc_setting->epout_valid==0)||
        (p_ccinst_cdc_setting->interface_cnt!=1) )
    {
        ret = usbd_ret_fail;
    }
    if (ret == 0)
    {
        p_ccinst_cdc_setting->desc_parsed = 1;
    }
    return ret;
}

uint8_t vcom_func_custp1_bind(usb_func_ccinst_st *p_func_ccinst,
                ccinst_bind_call_data_st *p_bind_call_data)
{
    uint8_t ret = usbd_ret_no_err;
    ccinst_cdc_setting_st* p_ccinst_cdc_setting;

    p_ccinst_cdc_setting = p_func_ccinst->p_cc_setting;
    #ifdef VCOM_CHK_CCINST_TYPE    
    if ((p_ccinst_cdc_setting->bs_set.setting_maintp!=ccinst_setting_cdc_vcom_maintp) ||
         (p_ccinst_cdc_setting->bs_set.setting_subtp != ccinst_setting_vcom_subtp1_2ep))
    {
        return usbd_ret_fail;
    }
    #endif

    if ((p_ccinst_cdc_setting->desc_assigned==0)
    || (p_ccinst_cdc_setting->desc_parsed==0))
    {
        return usbd_ret_fail;    
    }

    p_ccinst_cdc_setting->ep_datain_num = p_bind_call_data->ep_datain_num;
    p_ccinst_cdc_setting->ep_dataout_num = p_bind_call_data->ep_dataout_num;
    
    p_ccinst_cdc_setting->interface_cnt = p_bind_call_data->interface_cnt;

    p_ccinst_cdc_setting->intf_base_idx = p_bind_call_data->intf_base_idx;
    p_ccinst_cdc_setting->intf_1st_idx = p_bind_call_data->intf_1st_idx;

    p_ccinst_cdc_setting->intf_str_id = p_bind_call_data->intf_str_id;    
    p_ccinst_cdc_setting->binded = 1;
    return ret;
}

uint8_t vcom_func_custp1_get_desc(usb_func_ccinst_st *p_func_ccinst, ccinst_desc_call_data_st*p_desc_call_data)
{    
    ccinst_cdc_setting_st* p_ccinst_cdc_setting;
    usbd_cdc_desc_custp1_st *p_usbd_desc_dst;
    
    p_ccinst_cdc_setting = p_func_ccinst->p_cc_setting;
    
#ifdef VCOM_CHK_CCINST_TYPE    
    if ((p_ccinst_cdc_setting->bs_set.setting_maintp!=ccinst_setting_cdc_vcom_maintp) ||
         (p_ccinst_cdc_setting->bs_set.setting_subtp != ccinst_setting_vcom_subtp1_2ep))
    {
        return usbd_ret_fail;
    }
#endif

    if (p_desc_call_data->desc_buff_size < p_ccinst_cdc_setting->desc_src_size)
    {
        return usbd_ret_fail;    
    }

    if (p_ccinst_cdc_setting->binded == 0)
    {
        return usbd_ret_fail;    
    }
    
    p_usbd_desc_dst = (usbd_cdc_desc_custp1_st *)(p_desc_call_data->p_desc_buff_ptr);

    memcpy(p_usbd_desc_dst, p_ccinst_cdc_setting->p_desc_src, p_ccinst_cdc_setting->desc_src_size);

    p_usbd_desc_dst->cdc_diep_desc.bEndpointAddress = EPIN_MARK_DIR(p_ccinst_cdc_setting->ep_datain_num);
    p_usbd_desc_dst->cdc_doep_desc.bEndpointAddress = p_ccinst_cdc_setting->ep_dataout_num;
    p_usbd_desc_dst->intf_ctrl_desc.iInterface = p_ccinst_cdc_setting->intf_str_id;


    p_usbd_desc_dst->intf_ctrl_desc.bInterfaceNumber = p_ccinst_cdc_setting->intf_1st_idx;

    if (p_desc_call_data->b_intf_ctrl_desc_upd)
    {
        p_usbd_desc_dst->intf_ctrl_desc.bInterfaceClass = p_desc_call_data->bInterfaceClass;
        p_usbd_desc_dst->intf_ctrl_desc.bInterfaceSubClass = p_desc_call_data->bInterfaceSubClass;        
        p_usbd_desc_dst->intf_ctrl_desc.bInterfaceProtocol = p_desc_call_data->bInterfaceProtocol;                
    }

    return usbd_ret_no_err;    
}

uint8_t vcom_func_custp1_get_othspd_desc(usb_func_ccinst_st *p_func_ccinst, ccinst_desc_call_data_st*p_desc_call_data)
{    

    ccinst_cdc_setting_st* p_ccinst_cdc_setting;
    usbd_cdc_desc_custp1_st *p_usbd_desc_dst;
    
    p_ccinst_cdc_setting = p_func_ccinst->p_cc_setting;
    
#ifdef VCOM_CHK_CCINST_TYPE    
    if ((p_ccinst_cdc_setting->bs_set.setting_maintp!=ccinst_setting_cdc_vcom_maintp) ||
         (p_ccinst_cdc_setting->bs_set.setting_subtp != ccinst_setting_vcom_subtp1_2ep))
    {
        return usbd_ret_fail;
    }
#endif

    if (p_desc_call_data->desc_buff_size < p_ccinst_cdc_setting->desc_src_size)
    {
        return usbd_ret_fail;    
    }

    if (p_ccinst_cdc_setting->binded == 0)
    {
        return usbd_ret_fail;    
    }
    
    p_usbd_desc_dst = (usbd_cdc_desc_custp1_st*)(p_desc_call_data->p_desc_buff_ptr);
    memcpy(p_usbd_desc_dst, p_ccinst_cdc_setting->p_desc_src, p_ccinst_cdc_setting->desc_src_size);


    p_usbd_desc_dst->cdc_diep_desc.bEndpointAddress = EPIN_MARK_DIR(p_ccinst_cdc_setting->ep_datain_num);
    p_usbd_desc_dst->cdc_doep_desc.bEndpointAddress = p_ccinst_cdc_setting->ep_dataout_num;
    p_usbd_desc_dst->intf_ctrl_desc.iInterface = p_ccinst_cdc_setting->intf_str_id;
    p_usbd_desc_dst->cdc_diep_desc.wMaxPacketSize_High = 0;
    p_usbd_desc_dst->cdc_diep_desc.wMaxPacketSize_Low = 0x40;
    p_usbd_desc_dst->cdc_doep_desc.wMaxPacketSize_High = 0;
    p_usbd_desc_dst->cdc_doep_desc.wMaxPacketSize_Low = 0x40;

    p_usbd_desc_dst->intf_ctrl_desc.bInterfaceNumber = p_ccinst_cdc_setting->intf_1st_idx;

    if (p_desc_call_data->b_intf_ctrl_desc_upd)
    {
        p_usbd_desc_dst->intf_ctrl_desc.bInterfaceClass = p_desc_call_data->bInterfaceClass;
        p_usbd_desc_dst->intf_ctrl_desc.bInterfaceSubClass = p_desc_call_data->bInterfaceSubClass;        
        p_usbd_desc_dst->intf_ctrl_desc.bInterfaceProtocol = p_desc_call_data->bInterfaceProtocol;                
    }

    return usbd_ret_no_err;    
}


uint8_t vcom_func_custp1_init(usb_func_ccinst_st *p_func_ccinst, uint8_t cfgidx)
{
    return usbd_ret_no_err;    
}

uint8_t vcom_func_custp1_deinit(usb_func_ccinst_st *p_func_ccinst, uint8_t cfgidx)
{
    return usbd_ret_no_err;    
}

uint8_t vcom_func_custp1_unbind(usb_func_ccinst_st *p_func_ccinst)
{
    ccinst_cdc_setting_st* p_ccinst_cdc_setting;

    p_ccinst_cdc_setting = p_func_ccinst->p_cc_setting;

#ifdef VCOM_CHK_CCINST_TYPE    
    if ((p_ccinst_cdc_setting->bs_set.setting_maintp!=ccinst_setting_cdc_vcom_maintp) ||
         (p_ccinst_cdc_setting->bs_set.setting_subtp != ccinst_setting_vcom_subtp1_2ep))
    {
        return usbd_ret_fail;
    }
#endif

    if (p_ccinst_cdc_setting->binded==0)
    {
        return usbd_ret_fail;    
    }
    p_ccinst_cdc_setting->binded = 0;

    return usbd_ret_no_err;    
}

#if (VCOM_CCINST_SUBTP1_2EP_CNT > 0)
const  ccinst_setting_base_st t_vcom_custp1_base_setting = 
{
    ccinst_setting_cdc_vcom_maintp,
    ccinst_setting_vcom_subtp1_2ep        
};

ccinst_cdc_setting_st t_vcom_custp1_setting_arr[VCOM_CCINST_SUBTP1_2EP_CNT] ={
//[0]
    {
    .bs_set = t_vcom_custp1_base_setting,   
    },

#if (VCOM_CCINST_SUBTP1_2EP_CNT >=2)
//[1]

    {
    .bs_set = t_vcom_custp1_base_setting,           
    },
#endif

#if (VCOM_CCINST_SUBTP1_2EP_CNT >= 3)
//[2]
    {
    .bs_set = t_vcom_custp1_base_setting,           
    },
#endif
};

usb_func_ccinst_st t_vcom_func_custp1_arr[VCOM_CCINST_SUBTP1_2EP_CNT] ={
//[0]
    {
    .p_cc_setting = &t_vcom_custp1_setting_arr[0],    

    .func_desc_ass = vcom_func_custp1_desc_ass,
    .func_desc_parse = vcom_func_custp1_desc_parse,
    .func_bind = vcom_func_custp1_bind,
    .func_get_desc = vcom_func_custp1_get_desc,
    .func_get_othspd_desc = vcom_func_custp1_get_othspd_desc,
    
    .func_init = vcom_func_custp1_init,
    .func_deinit = vcom_func_custp1_deinit,
    .func_unbind = vcom_func_custp1_unbind,        
    },

#if (VCOM_CCINST_SUBTP1_2EP_CNT >=2)
//[1]

    {
    .p_cc_setting = &t_vcom_custp1_setting_arr[1],        
    .func_desc_ass = vcom_func_custp1_desc_ass,
    .func_desc_parse = vcom_func_custp1_desc_parse,
    .func_bind = vcom_func_custp1_bind,
    .func_get_desc = vcom_func_custp1_get_desc,
    .func_get_othspd_desc = vcom_func_custp1_get_othspd_desc,
    
    .func_init = vcom_func_custp1_init,
    .func_deinit = vcom_func_custp1_deinit,
    .func_unbind = vcom_func_custp1_unbind,        
    },
#endif

#if (VCOM_CCINST_SUBTP1_2EP_CNT >= 3)
//[2]
    {
    .p_cc_setting = &t_vcom_custp1_setting_arr[2],        
    
    .func_desc_ass = vcom_func_custp1_desc_ass,
    .func_desc_parse = vcom_func_custp1_desc_parse,
    .func_bind = vcom_func_custp1_bind,
    .func_get_desc = vcom_func_custp1_get_desc,
    .func_get_othspd_desc = vcom_func_custp1_get_othspd_desc,
    
    .func_init = vcom_func_custp1_init,
    .func_deinit = vcom_func_custp1_deinit,
    .func_unbind = vcom_func_custp1_unbind,        
    },
#endif

};

#endif


  
