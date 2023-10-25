/******************************************************************************

*(C) Copyright 2018 AirM2M International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename:
*
*  Description:
*
*  History: initiated by xuwang
*
*  Notes:
*
******************************************************************************/

#ifdef FEATURE_FOTA_ENABLE

#include <stdint.h>
#include "plat_config.h"
#include "fota_pub.h"
#include "fota_custom.h"
#include "bl_bsp.h"
#include "fota_nvm.h"


extern void FOTA_setSchemOemName(uint8_t *name);
extern void CopyRamCodeAndDatForUsb(void);
extern uint8_t usbstack_clear_ctx_stat(void);
extern void usbstack_set_ctx_vbus_mode(uint8_t vbus_mode_en, uint8_t vbus_pad_idx);
extern uint8_t usbstack_deinit(void);
extern uint32_t BSP_UsbInit(void);
extern int32_t FOTA_checkDfuMission(uint8_t *hasDfu);
extern int vcomx_isconnect(uint8_t vcom_num);
extern void vcom_app_delay(uint32_t ticks);
extern void WDT_stop(void);



#if 0
extern void FOTA_mainProc(void);
#else
extern void FOTA_initProc(void);
extern void FOTA_execProc(void);
extern void FOTA_exitProc(void);
#endif

static uint8_t          gFotaIsUsbUrcEn = 0;


extern uint32_t         URCBaudValue;
extern uint8_t          URCSelSerlIdx;
extern URCSelPrintType  URCSelSerlType;
#ifdef __USER_CODE__
#else
static void FotaUsbUrcEn(uint8_t vcomNum)
{
#ifdef FEATURE_FOTA_USBURC_ENABLE
    uint32_t loopCnt = 10;


    if(0 == BSP_UsbInit())
    {
        FOTA_TRACE(LOG_DEBUG, "init FOTA USB URC succ! \n");
    }
    else
    {
        FOTA_TRACE(LOG_DEBUG, "init FOTA USB URC fail! \n");
    }

    while(1)/*loop for enum done*/
    {
        vcom_app_delay(1000);

        fotaDoExtension(FOTA_DEF_WDT_KICK, NULL);

        if(vcomx_isconnect(vcomNum) == 0)
        {
            loopCnt --;
            if(loopCnt == 0)
            {
               FOTA_TRACE(LOG_DEBUG, "loop FOTA USB ENUM fail! \n");
               break;
            }
        }
        else
        {
            FOTA_TRACE(LOG_DEBUG, "loop FOTA USB ENUM succ %d! \n", loopCnt);
            break;
        }
    }

    /*add here to wait for VCOM open on pc,
      for MCU or auto test tool, customer define it or remove it*/
#if 0
    loop_cnt = 100;
    while(loop_cnt --)
    {
        vcom_app_delay(200);//200ms
    }

    SelNormalOrURCPrint(1);
    FOTA_TRACE(LOG_DEBUG,"+QIND: \"FOTA\",\"USB URC INIT\"\r\n");//test urc, may be removed
    SelNormalOrURCPrint(0);
#endif

#endif
}
#endif

#if 0
void FotaProcedure(void)
{
    FOTA_setSchemOemName((uint8_t*)"EiGENCOMM");
    FOTA_mainProc();
}

#else
void FOTA_displayUrcInfo(void)
{
    gFotaIsUsbUrcEn = URCSelSerlType >= URCVCom0PrintType ? 1 : 0;

    FOTA_TRACE(LOG_DEBUG,"%s(%d) urc baud: %d\n", gFotaIsUsbUrcEn ? "usb" : "uart",
                                                  URCSelSerlIdx,
                                                  URCBaudValue);
}

#ifdef __USER_CODE__
int FotaProcedure(void)
#else
void FotaProcedure(void)
#endif
{
    uint8_t   hasNewDfu = 0;

    FOTA_setSchemOemName((uint8_t*)"EiGENCOMM");

    FOTA_initProc();
    //WDT_stop();// un-comment for test if necessary

    FOTA_checkDfuMission(&hasNewDfu);
#ifdef __USER_CODE__
#else
    if(hasNewDfu == 1 && gFotaIsUsbUrcEn == 1)//start, init URC port( UARTx or USB VCOMx default UART1)
    {
        FotaUsbUrcEn(URCSelSerlIdx);
    }
#endif
    FOTA_execProc();
    FOTA_exitProc();
#ifdef __USER_CODE__
    if(hasNewDfu == 1)//end
    {
        fotaDoExtension(FOTA_DEF_RPT_DFU_RESULT, NULL);
        fotaNvmClearDelta(0, fotaNvmGetDeltaSize(1));
        FotaDefDfuResult_t result;
        fotaDoExtension(FOTA_DEF_GET_DFU_RESULT, (void*)&result);
        if (FOTA_DRC_DFU_SUCC == result.dfuResult)
        {
        	return 1;
        }
        else
        {
        	return -1;
        }
    }
    return 0;
#else
    if(hasNewDfu == 1)//end
    {
        fotaDoExtension(FOTA_DEF_RPT_DFU_RESULT, NULL);

        if(gFotaIsUsbUrcEn == 1)
        {
        #ifdef FEATURE_FOTA_USBURC_ENABLE
            vcom_app_delay(50);
            if(0 != usbstack_deinit())
            {
                FOTA_TRACE(LOG_DEBUG, "deinit FOTA USB URC fail! \n");
            }
        #endif
        }

        fotaNvmClearDelta(0, fotaNvmGetDeltaSize(1));
    }
#endif
}
#endif


#endif


