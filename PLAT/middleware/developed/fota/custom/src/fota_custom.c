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

/*----------------------------------------------------------------------------*
 *                    INCLUDES                                                *
 *----------------------------------------------------------------------------*/
#ifdef FEATURE_FOTA_ENABLE

#include "fota_sal.h"
#include "fota_utils.h"
#include "fota_mem.h"
#include "fota_nvm.h"
#include "fota_patch.h"
#include "fota_custom.h"

/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/

#if (WDT_FEATURE_ENABLE==1)
#if defined CHIP_EC616 || defined CHIP_EC616_Z0 || defined CHIP_EC616S || defined CHIP_EC626
extern void WDT_Kick(void);
extern void WDT_Start(void);
extern void WDT_Stop(void);

#define FOTA_WDT_KICK()   WDT_Kick()
#define FOTA_WDT_START()  WDT_Start()
#define FOTA_WDT_STOP()   WDT_Stop()
#else
extern void WDT_kick(void);
extern void WDT_start(void);
extern void WDT_stop(void);

#define FOTA_WDT_KICK()   WDT_kick()
#define FOTA_WDT_START()  WDT_start()
#define FOTA_WDT_STOP()   WDT_stop()
#endif
#endif
#ifdef __USER_CODE__
extern void user_code_run(void);
#endif

/*----------------------------------------------------------------------------*
 *                    DATA TYPE DEFINITION                                    *
 *----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*
 *                      PRIVATE FUNCTION DECLEARATION                         *
 *----------------------------------------------------------------------------*/


extern void delay_us(uint32_t us);
extern void SelNormalOrURCPrint(uint32_t Sel);

extern int32_t FOTA_getDfuProgress(FotaDefDfuProgress_t *prog);

/*----------------------------------------------------------------------------*
 *                      GLOBAL VARIABLES                                      *
 *----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*
 *                      PRIVATE FUNCTIONS                                     *
 *----------------------------------------------------------------------------*/
static void fotaGetRamSize(FotaDefGetRamSize_t *ram)
{
    ram->avlbSize = 512 * FOTA_BUF_SIZE_1K;;
}

static void fotaCheckBattery(FotaDefChkBattery_t *batt)
{
    batt->isBattLow = 0;
}

static void fotaCheckDynMem(FotaDefChkDynMem_t *dmem)
{
    dmem->isEnable = fotaIsSuppDynMem();
}

static void fotaCheckTotalProgress(FotaDefDfuProgress_t *prog)
{
    prog->isTotal = 1;
#if defined CHIP_EC718 || defined CHIP_EC716
    prog->rptFreq = FOTA_DPR_FREQ_HIGH;
#else
    prog->rptFreq = FOTA_DPR_FREQ_MEDIUM;
#endif
}

static int32_t fotaReportDfuProgress(FotaDefDfuProgress_t *prog)
{
    /* LOG PORT */
    FOTA_TRACE(LOG_DEBUG,"FOTA: PKG[%d/%d:%d] upgrading..., %d\n", \
                                    prog->pkgId + 1, prog->pkgNum, \
                                    prog->isTotal, prog->percent);

    /* AT PORT */
    SelNormalOrURCPrint(1);
    if(prog->percent == 0)
    {
        FOTA_TRACE(LOG_DEBUG,"+QIND: \"FOTA\",\"START\"\r\n");
    }
    FOTA_TRACE(LOG_DEBUG,"+QIND: \"FOTA\",\"UPDATING\",%d\r\n",prog->percent);
    //FOTA_TRACE(LOG_DEBUG,"+QIND: \"FOTA\",\"UPDATING\",%d%%,1,1\r\n",prog->percent);
    SelNormalOrURCPrint(0);
#ifdef __USER_CODE__
    user_code_run();
#endif
    return FOTA_EOK;
}

static int32_t fotaReportDfuResult(void)
{
    uint16_t        errCode = 0xffff;
    int32_t      deltaState = FOTA_DCS_DELTA_UNDEF;
    int32_t       otaResult = fotaNvmGetOtaResult(FOTA_NVM_ZONE_ANY, &deltaState);

    switch(otaResult)
    {
        case FOTA_DRC_DFU_SUCC:
            errCode = 0;
            break;
        case FOTA_DRC_DFU_FAIL:
            errCode = 100 + deltaState;
            break;

        default:
            break;
    }

    if(otaResult == FOTA_DRC_DFU_SUCC || otaResult == FOTA_DRC_DFU_FAIL)
    {
        /* AT PORT */
        SelNormalOrURCPrint(1);
        FOTA_TRACE(LOG_DEBUG,"+QIND: \"FOTA\",\"END\",%d\r\n", errCode);
        SelNormalOrURCPrint(0);
    }

    return FOTA_EOK;
}

/*----------------------------------------------------------------------------*
 *                      GLOBAL FUNCTIONS                                      *
 *----------------------------------------------------------------------------*/
/******************************************************************************
 * @brief : fotaDoExtension
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
int32_t fotaDoExtension(uint8_t flags, void *args)
{
    int32_t retCode = FOTA_EOK;

    switch(flags)
    {
        case FOTA_DEF_US_DELAY:
            delay_us(((FotaDefUsDelay_t*)args)->usec);
            break;
    #if (WDT_FEATURE_ENABLE==1)
        case FOTA_DEF_WDT_KICK:
            FOTA_WDT_KICK();
            break;
        case FOTA_DEF_WDT_START:
            FOTA_WDT_START();
            break;
        case FOTA_DEF_WDT_STOP:
            FOTA_WDT_STOP();
            break;
    #else
        case FOTA_DEF_WDT_KICK:
        case FOTA_DEF_WDT_START:
        case FOTA_DEF_WDT_STOP:
            FOTA_TRACE(LOG_DEBUG,"FotaDoExt: WDT is disabled!\n");
            break;
    #endif
        case FOTA_DEF_GET_RAM_SIZE:
            fotaGetRamSize((FotaDefGetRamSize_t*)args);
            break;
        case FOTA_DEF_CHK_BATTERY:
            fotaCheckBattery((FotaDefChkBattery_t*)args);
            break;
        case FOTA_DEF_CHK_DYN_MEM:
            fotaCheckDynMem((FotaDefChkDynMem_t*)args);
            break;
        case FOTA_DEF_CHK_TOTAL_PROGRESS:
            fotaCheckTotalProgress((FotaDefDfuProgress_t*)args);
            break;
        case FOTA_DEF_RPT_DFU_PROGRESS:
            retCode = fotaReportDfuProgress((FotaDefDfuProgress_t*)args);
            break;
        case FOTA_DEF_GET_DFU_PROGRESS:
            retCode = FOTA_getDfuProgress((FotaDefDfuProgress_t*)args);
            break;
        case FOTA_DEF_RPT_DFU_RESULT:
            retCode = fotaReportDfuResult();
            break;
        case FOTA_DEF_CHK_REMAP_ZONE:
        case FOTA_DEF_CHK_DELTA_STATE:
        case FOTA_DEF_CHK_BASE_IMAGE:
        case FOTA_DEF_IS_IMAGE_IDENTICAL:
        case FOTA_DEF_SET_DFU_RESULT:
        case FOTA_DEF_GET_DFU_RESULT:
        case FOTA_DEF_PREPARE_DFU:
        case FOTA_DEF_CLOSING_DFU:
            retCode = fotaNvmDoExtension(flags, args);
            break;

        default:
            FOTA_TRACE(LOG_DEBUG,"FotaDoExt: undef flags(%d)!\n", flags);
            break;
    }

    return retCode;
}

#ifdef FEATURE_FOTA_CORE2_ENABLE
int32_t FOTA_patchImage(FotaPsegCtrlUnit_t *pscu, const FotaPscuCallbacks_t *callbacks)
{
#ifdef FOTA_PATCH_STUB
    return FOTA_patchStub(pscu, callbacks);
#else
    extern int32_t FOTA_patchEntry(FotaPsegCtrlUnit_t *pscu, const FotaPscuCallbacks_t *callbacks);
    return FOTA_patchEntry(pscu, callbacks);
#endif
}

#else
int32_t FOTA_patchImage(FotaPsegCtrlUnit_t *pscu, const FotaPscuCallbacks_t *callbacks)
{
#ifdef FOTA_PATCH_STUB
    return FOTA_patchStub(pscu, callbacks);
#else
    extern int32_t patch_integrate_entry(FotaPsegCtrlUnit_t *pscu, const FotaPscuCallbacks_t *callbacks);
    return patch_integrate_entry(pscu, callbacks);
#endif
}

#if (!defined CHIP_EC618 && !defined CHIP_EC618_Z0)
uint8_t BSP_QSPI_Write(uint8_t* pData, uint32_t WriteAddr, uint32_t Size)
{
    return FLASH_write(pData, WriteAddr, Size);
}

uint8_t  BSP_QSPI_Erase_Sector(uint32_t SectorAddress)
{
    return FLASH_eraseSectorSafe(SectorAddress);
}
#endif

uint32_t GetDltFileZoneId(void)
{
    return FOTA_NVM_ZONE_DELTA;
}

uint32_t GetBackupZoneId(void)
{
    return FOTA_NVM_ZONE_BKUP;
}
#endif

#endif



