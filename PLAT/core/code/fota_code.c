/*----------------------------------------------------------------------------*
 *                    INCLUDES                                                *
 *----------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "mem_map.h"
#include "cmsis_os2.h"
#include "sctdef.h"
#include "nvram.h"
#include "fota_sal.h"
#include "fota_utils.h"
#include "fota_chksum.h"
#include "fota_custom.h"
#include "fota_nvm.h"

/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/
#ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
#define FOTA_IS_CPFLASH_DISABLED()     1
#else
#define FOTA_IS_CPFLASH_DISABLED()     apmuIsCpSleeped()
#endif

#define FOTA_NVM_SECTOR_SIZE           FOTA_BUF_SIZE_4K
#define FOTA_NVM_BLOCK_SIZE            FOTA_BUF_SIZE_32K

#ifdef FOTA_NVM_FS_ENABLE
#define FOTA_DELTA_PAR_NAME            "FotaDelta.par"
#define FOTA_DELTA_PAR_MAXSIZE         FOTA_BUF_SIZE_512K

/* caution:
 * the remapping zone should be erased every time
 * after the delta.par is downloaded completely OR
 * before the new upgrade is executed!
 */
#define FOTA_NVM_REMAP_ADDR            (xxxx)
#define FOTA_NVM_REMAP_SIZE            (FOTA_NVM_SECTOR_SIZE)

#define FOTA_NVM_DELTA_BACKUP_ADDR     (xxxx)
#if defined CHIP_EC718 || defined CHIP_EC716
#define FOTA_NVM_DELTA_BACKUP_SIZE     (FOTA_BUF_SIZE_1K * 44)
#else
#define FOTA_NVM_DELTA_BACKUP_SIZE     (FOTA_BUF_SIZE_32K)
#endif

#else  /* raw flash */
#define FOTA_NVM_DELTA_ADDR            (FLASH_FOTA_REGION_START)
#define FOTA_NVM_DELTA_SIZE            (FLASH_FOTA_REGION_LEN)

#define FOTA_NVM_DELTA_DOWNLOAD_ADDR   (FOTA_NVM_DELTA_ADDR)
#define FOTA_NVM_DELTA_DOWNLOAD_SIZE   (FOTA_NVM_DELTA_SIZE - FOTA_NVM_DELTA_BACKUP_SIZE)

#define FOTA_NVM_DELTA_BACKUP_ADDR     (FOTA_NVM_DELTA_ADDR + FOTA_NVM_DELTA_DOWNLOAD_SIZE)
#if defined CHIP_EC718 || defined CHIP_EC716
#define FOTA_NVM_DELTA_BACKUP_SIZE     (FOTA_BUF_SIZE_1K * 44)
#else
#define FOTA_NVM_DELTA_BACKUP_SIZE     (FOTA_BUF_SIZE_32K)
#endif

#endif

#define FOTA_NVM_REAL_BACKUP_ADDR      (FOTA_NVM_DELTA_BACKUP_ADDR)
#define FOTA_NVM_REAL_BACKUP_SIZE      (FOTA_NVM_DELTA_BACKUP_SIZE + FOTA_NVM_BACKUP_MUX_SIZE)
#if defined CHIP_EC718 || defined CHIP_EC716
#define FOTA_NVM_BACKUP_MUX_SIZE       (NVRAM_PHYSICAL_SIZE)
#else
#define FOTA_NVM_BACKUP_MUX_SIZE       0
#endif


#if defined CHIP_EC618 || defined CHIP_EC618_Z0 || defined CHIP_EC718 || defined CHIP_EC716
#define FOTA_NVM_A2AP_XIP_ADDR         (AP_FLASH_XIP_ADDR)
#if defined CHIP_EC618 || defined CHIP_EC618_Z0
#define FOTA_NVM_A2CP_XIP_ADDR         (AP_VIEW_CPFLASH_XIP_ADDR)
#elif defined CHIP_EC718 || defined CHIP_EC716
#ifdef TYPE_EC718H
#define FOTA_NVM_A2CP_XIP_ADDR         (AP_VIEW_CPFLASH_XIP_ADDR)
#else
#define FOTA_NVM_A2CP_XIP_ADDR         (AP_FLASH_XIP_ADDR)
#endif
#endif

#define FOTA_NVM_AP_LOAD_ADDR          (AP_FLASH_LOAD_ADDR & (~AP_FLASH_XIP_ADDR))
#define FOTA_NVM_AP_LOAD_SIZE          (AP_FLASH_LOAD_SIZE)

#define FOTA_NVM_CP_LOAD_ADDR          (CP_FLASH_LOAD_ADDR & (~CP_FLASH_XIP_ADDR))
#define FOTA_NVM_CP_LOAD_SIZE          (CP_FLASH_LOAD_SIZE)

#define FOTA_NVM_SYSH_LOAD_ADDR        (SYS_SEC_HAED_ADDR)
#define FOTA_NVM_SYSH_LOAD_SIZE        (SYS_FLASH_LOAD_SIZE)

#define FLASH_XIP_ADDR                 (AP_FLASH_XIP_ADDR)

#else /* @ec616/ec616s/ec626 */
#define FOTA_NVM_A2AP_XIP_ADDR         (FLASH_XIP_ADDR)
#define FOTA_NVM_A2CP_XIP_ADDR         (FLASH_XIP_ADDR)

/* APP type by default */
#define FOTA_CUST_APP_ENABLE
#endif

#ifdef FOTA_CUST_APP_ENABLE
#define FOTA_NVM_APP_LOAD_ADDR         (APP_FLASH_LOAD_ADDR & (~FOTA_NVM_A2AP_XIP_ADDR))
#define FOTA_NVM_APP_LOAD_SIZE         (APP_FLASH_LOAD_SIZE)
#endif

/*----------------------------------------------------------------------------*
 *                    DATA TYPE DEFINITION                                    *
 *----------------------------------------------------------------------------*/
typedef struct
{
    uint32_t  handle;    /* means addr @NFS, otherwise fd @FS */
    uint32_t  size;
    uint32_t  overhead;  /* reserved zone size for a special purpose */
    uint32_t  extras;    /* some supplementary info */
}FotaNvmZone_t;

typedef struct
{
    FotaNvmZoneId_bm  bmZoneId;
    FotaNvmZone_t     zone[FOTA_NVM_ZONE_MAXNUM];
}FotaNvmZoneMan_t;


/*----------------------------------------------------------------------------*
 *                      PRIVATE FUNCTION DECLEARATION                         *
 *----------------------------------------------------------------------------*/


extern osStatus_t osDelay (uint32_t ticks);
extern bool apmuIsCpSleeped(void);

/*----------------------------------------------------------------------------*
 *                      GLOBAL VARIABLES                                      *
 *----------------------------------------------------------------------------*/

static FotaNvmZoneMan_t   gFotaNvmZoneMan;

/* sha256 hash */
FOTA_PLAT_SCT_ZI uint8_t  gFotaHash[FOTA_SHA256_HASH_LEN];

/*----------------------------------------------------------------------------*
 *                      PRIVATE FUNCTIONS                                     *
 *----------------------------------------------------------------------------*/
static void fotaNvmSetZone(FotaNvmZoneId_e zid, uint32_t handle, uint32_t size, uint32_t ovhd, uint32_t extras)
{
    if(zid >= FOTA_NVM_ZONE_MAXNUM)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_SET, P_ERROR, "set: invalid zid(%d)! max(%d)\n", zid, FOTA_NVM_ZONE_MAXNUM);
        return;
    }

    gFotaNvmZoneMan.bmZoneId |= 1 << zid ;
    gFotaNvmZoneMan.zone[zid].handle   = handle;
    gFotaNvmZoneMan.zone[zid].size     = size;
    gFotaNvmZoneMan.zone[zid].overhead = ovhd;
    gFotaNvmZoneMan.zone[zid].extras   = extras;
}

#ifdef FOTA_NVM_FS_ENABLE

int32_t fotaNvmFsOpen(const char *fname, const char* mode)
{
    return -1;
}

int32_t fotaNvmFsClose(int32_t fd)
{
    return FOTA_EOK;
}

int32_t fotaNvmFsSize(int32_t fd)
{
    return 0;
}

int32_t fotaNvmFsTell(int32_t fd)
{
    return 0;
}

uint32_t fotaNvmFsRead(void *buf, uint32_t size, uint32_t count, int32_t fd)
{
    return 0;
}

uint32_t fotaNvmFsWrite(void *buf, uint32_t size, uint32_t count, int32_t fd)
{
    return 0;
}

int32_t fotaNvmFsSeek(int32_t fd, int32_t offset, uint8_t seekType)
{
    return FOTA_EOK;
}

int32_t fotaNvmFsRemove(const char *fname)
{
    return FOTA_EOK;
}
#endif

/* non fs: raw flash */
static int32_t fotaNvmNfsClear(uint32_t zid, uint32_t offset, uint32_t len)
{
    uint8_t    retValue = 0;
    uint32_t   eraseLen = 0;
    uint32_t   currAddr = 0;
    uint32_t    adjAddr = 0;
    uint32_t    currLen = len;

    if(zid >= FOTA_NVM_ZONE_MAXNUM)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_CLEAR_0, P_WARNING, "clr: invalid zid(%d)! max(%d)\n",
                                                                 zid, FOTA_NVM_ZONE_MAXNUM);
        return FOTA_EARGS;
    }

    if(!(gFotaNvmZoneMan.bmZoneId & (1 << zid)))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_CLEAR_1, P_WARNING, "clr: no fota zone(%d)! bmZid(0x%x)\n",
                                                                 zid, gFotaNvmZoneMan.bmZoneId);
        return FOTA_EUNFOUND;
    }

    if(FLASH_FOTA_ADDR_UNDEF == gFotaNvmZoneMan.zone[zid].handle)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_CLEAR_2, P_WARNING, "clr zone(%d): no fota zone?\n", zid);
        return FOTA_EUNDEF;
    }

    if(offset % FOTA_NVM_SECTOR_SIZE)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_CLEAR_3, P_WARNING, "clr zone(%d): offset(%d) unaligned by 4K!\n", zid, offset);
        offset = (offset / FOTA_NVM_SECTOR_SIZE) * FOTA_NVM_SECTOR_SIZE;
    }

    if(currLen > gFotaNvmZoneMan.zone[zid].size)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_CLEAR_4, P_WARNING, "clr zone(%d): len(%d) ovfl! set it with max(%d)!\n",
                                                                 zid, currLen, gFotaNvmZoneMan.zone[zid].size);
        currLen = gFotaNvmZoneMan.zone[zid].size;
    }

    if(offset >= gFotaNvmZoneMan.zone[zid].size)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_CLEAR_5, P_ERROR, "clr zone(%d): invalid offset(%d)! max(%d)\n",
                                                               zid, offset, gFotaNvmZoneMan.zone[zid].size);
        return FOTA_EARGS;
    }

    currAddr = ((gFotaNvmZoneMan.zone[zid].handle + offset) & (~ gFotaNvmZoneMan.zone[zid].extras));
    if(currAddr % FOTA_NVM_SECTOR_SIZE)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_CLEAR_6, P_ERROR, "clr zone(%d): addr(%d) unaligned by 4K err!\n", zid, currAddr);
        return FOTA_EPERM;
    }

    while(currLen > 0)
    {
        if(currLen >= FOTA_NVM_BLOCK_SIZE)
        {
            eraseLen = FOTA_NVM_BLOCK_SIZE;
            if(currAddr % FOTA_NVM_BLOCK_SIZE)
            {
                adjAddr  = FOTA_ALIGN_UP(currAddr, FOTA_NVM_BLOCK_SIZE);
                eraseLen = adjAddr - currAddr;
            }
        }
        else
        {
            eraseLen = currLen;
        }

        if(eraseLen > currLen || eraseLen % FOTA_NVM_SECTOR_SIZE)
        {
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_CLEAR_7, P_ERROR, "clr zone(%d): curr/erase Len(0x%x/0x%x) err!\n",
                                                                   zid, currLen, eraseLen);
            return FOTA_EFLERASE;
        }

        /* if @bl, split it to 4KB one by one! */
        if(FOTA_NVM_SECTOR_ERASE_MODE && eraseLen < FOTA_NVM_BLOCK_SIZE)
        {
            eraseLen = FOTA_NVM_SECTOR_SIZE;
        }

        if(zid == FOTA_NVM_ZONE_CP)
        {
            if(eraseLen == FOTA_NVM_BLOCK_SIZE)
            {
                retValue = BSP_QSPI_ERASE_CP_FLASH_32K(currAddr, eraseLen);
            }
            else
            {
                retValue = BSP_QSPI_ERASE_CP_FLASH(currAddr, eraseLen);
            }
        }
        else
        {
            if(eraseLen == FOTA_NVM_BLOCK_SIZE)
            {
                retValue = BSP_QSPI_ERASE_AP_FLASH_32K(currAddr, eraseLen);
            }
            else
            {
                retValue = BSP_QSPI_ERASE_AP_FLASH(currAddr, eraseLen);
            }
        }

        if (retValue != QSPI_OK)
        {
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_CLEAR_8, P_ERROR, "clr zone(%d): err! addr(0x%x), errno(%d)\n",
                                                                   zid, currAddr, retValue);
            return FOTA_EFLERASE;
        }

        currAddr += eraseLen;
        currLen  -= eraseLen;

        osDelay(100 / FOTA_TICK_RATE_MS);
    }

    return (retValue == QSPI_OK ? FOTA_EOK : FOTA_EFLERASE);
}

static int32_t fotaNvmNfsWrite(uint32_t zid, uint32_t offset, uint8_t *buf, uint32_t bufLen)
{
    uint8_t  retValue = 0;
    uint32_t currAddr = 0;
    uint32_t  currLen = bufLen;

    if(zid >= FOTA_NVM_ZONE_MAXNUM)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_WRITE_1, P_WARNING, "wr: invalid zid(%d)! max(%d)\n",
                                                                 zid, FOTA_NVM_ZONE_MAXNUM);
        return FOTA_EARGS;
    }

    if(!(gFotaNvmZoneMan.bmZoneId & (1 << zid)))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_WRITE_2, P_WARNING, "wr: no fota zone(%d)! bmZid(0x%x)\n",
                                                                 zid, gFotaNvmZoneMan.bmZoneId);
        return FOTA_EUNFOUND;
    }

    if(FLASH_FOTA_ADDR_UNDEF == gFotaNvmZoneMan.zone[zid].handle)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_WRITE_3, P_WARNING, "wr zone(%d): no fota zone?\n", zid);
        return FOTA_EUNDEF;
    }

    if(!currLen)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_WRITE_4, P_WARNING, "wr zone(%d): zero buf len?\n", zid);
        return FOTA_EOK;
    }
    else if(currLen > gFotaNvmZoneMan.zone[zid].size)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_WRITE_5, P_WARNING, "wr zone(%d): len(%d) ovfl! set it max(%d)!\n",
                                                                 zid, currLen, gFotaNvmZoneMan.zone[zid].size);
        currLen = gFotaNvmZoneMan.zone[zid].size;
    }

    if(offset >= gFotaNvmZoneMan.zone[zid].size)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_WRITE_6, P_ERROR, "wr zone(%d): invalid offset(%d)! max(%d)\n",
                                                               zid, offset, gFotaNvmZoneMan.zone[zid].size);
        return FOTA_EARGS;
    }

    currAddr = ((gFotaNvmZoneMan.zone[zid].handle + offset) & (~ gFotaNvmZoneMan.zone[zid].extras));

    if(zid == FOTA_NVM_ZONE_CP)
    {
        retValue = BSP_QSPI_WRITE_CP_FLASH(buf, currAddr, currLen);
    }
    else
    {
        retValue = BSP_QSPI_WRITE_AP_FLASH(buf, currAddr, currLen);
    }

    return (retValue == QSPI_OK ? FOTA_EOK : FOTA_EFLWRITE);
}

PLAT_BL_CIRAM_FLASH_TEXT static int32_t fotaNvmNfsRead(uint32_t zid, uint32_t offset, uint8_t *buf, uint32_t bufLen)
{
    uint8_t  retValue = 0;
    uint32_t currAddr = 0;
    uint32_t  currLen = bufLen;

    if(zid >= FOTA_NVM_ZONE_MAXNUM)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_READ_1, P_WARNING, "rd: invalid zid(%d)! max(%d)\n",
                                                                zid, FOTA_NVM_ZONE_MAXNUM);
        return FOTA_EARGS;
    }

    if(!(gFotaNvmZoneMan.bmZoneId & (1 << zid)))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_READ_2, P_WARNING, "rd: no fota zone(%d)! bmZid(0x%x)\n",
                                                                zid, gFotaNvmZoneMan.bmZoneId);
        return FOTA_EUNFOUND;
    }

    if(FLASH_FOTA_ADDR_UNDEF == gFotaNvmZoneMan.zone[zid].handle)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_READ_3, P_WARNING, "rd zone(%d): no fota zone?\n", zid);
        return FOTA_EUNDEF;
    }

    if(!currLen)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_READ_4, P_WARNING, "rd zone(%d): zero buf len?\n", zid);
        return FOTA_EOK;
    }
    else if(currLen > gFotaNvmZoneMan.zone[zid].size)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_READ_5, P_WARNING, "rd zone(%d): len(%d) ovfl! set it max(%d)!\n",
                                                                zid, currLen, gFotaNvmZoneMan.zone[zid].size);
        currLen = gFotaNvmZoneMan.zone[zid].size;
    }

    if(offset >= gFotaNvmZoneMan.zone[zid].size)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_READ_6, P_ERROR, "rd zone(%d): invalid offset(%d)! max(%d)\n",
                                                              zid, offset, gFotaNvmZoneMan.zone[zid].size);
        return FOTA_EARGS;
    }

    currAddr = ((gFotaNvmZoneMan.zone[zid].handle + offset)  & (~ gFotaNvmZoneMan.zone[zid].extras));

    if(zid == FOTA_NVM_ZONE_CP)
    {
        retValue = BSP_QSPI_READ_CP_FLASH(buf, currAddr, currLen);
    }
    else
    {
        retValue = BSP_QSPI_READ_AP_FLASH(buf, currAddr, currLen);
    }

    return (retValue == QSPI_OK ? FOTA_EOK : FOTA_EFLREAD);
}

static int32_t fotaNvmCheckRemapZone(FotaDefChkRemapZone_t *remapZone)
{
#ifdef FOTA_NVM_FS_ENABLE
    remapZone->isEnable = 1;
#else
    remapZone->isEnable = 0;
#endif

    return FOTA_EOK;
}

static int32_t fotaNvmSetDfuResult(FotaDefDfuResult_t *result)
{
    uint8_t  buf[4] = {0};

    if(!result)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_SET_DFU_0, P_WARNING, "set DFU: null ptr!\n");
        return FOTA_EARGS;
    }

    if(result->dfuResult == FOTA_DRC_DFU_SUCC)
    {
        snprintf((char*)buf, 4, "%s", "OK");
    }
    else if(result->dfuResult == FOTA_DRC_DFU_FAIL)
    {
        snprintf((char*)buf, 4, "%s", "NO");
    }
    /* errno */
    buf[3] = result->deltaState;

    if(FOTA_EOK != fotaNvmClear(FOTA_NVM_ZONE_BKUP, 0, fotaNvmGetSize(FOTA_NVM_ZONE_BKUP, 1)))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_SET_DFU_1, P_ERROR, "set DFU: clr bkup zone fail!\n");
        return FOTA_EFLERASE;
    }

    if(FOTA_EOK != fotaNvmWrite(FOTA_NVM_ZONE_BKUP, 0, buf, 4))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_SET_DFU_2, P_WARNING, "set DFU: '%s', errno(%d)!\n", buf, result->deltaState);
        return FOTA_EFLWRITE;
    }

    return FOTA_EOK;
}

PLAT_BL_CIRAM_FLASH_TEXT static int32_t fotaNvmGetDfuResult(FotaDefDfuResult_t *result)
{
    uint8_t   buf[4] = {0};

    if(!result)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_DFU_0, P_WARNING, "get DFU: null ptr!\n");
        return FOTA_EARGS;
    }

    if(!gFotaNvmZoneMan.bmZoneId)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_DFU_1, P_WARNING, "get DFU: init zones!\n");
        fotaNvmInit();
    }

    if(FOTA_EOK != fotaNvmRead(FOTA_NVM_ZONE_BKUP, 0, buf, 4))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_DFU_2, P_ERROR, "get DFU: rd bkup zone fail!\n");
        return FOTA_EFLREAD;
    }

    if (strncmp((char*)buf, "OK", 2) == 0)
    {
        result->dfuResult = FOTA_DRC_DFU_SUCC;
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_DFU_3, P_INFO, "get DFU: 'succ'\n");
    }
    else if (strncmp((char*)buf, "NO", 2) == 0)
    {
        result->dfuResult = FOTA_DRC_DFU_FAIL;
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_DFU_4, P_INFO, "get DFU: 'fail'\n");
    }
    else
    {
        result->dfuResult = FOTA_DRC_DFU_UNDEF;
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_DFU_5, P_INFO, "get DFU: 'no result'\n");
    }
    /* errno */
    result->deltaState = buf[3];

    return FOTA_EOK;
}

static int32_t fotaNvmPrepareDfu(FotaDefPrepareDfu_t *preDfu)
{
    return FOTA_EOK;
}

static int32_t fotaNvmClosingDfu(FotaDefClosingDfu_t *clsDfu)
{
#if (FOTA_NVM_BACKUP_MUX_SIZE != 0)
#ifdef REL_COMPRESS_EN
    if(gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_BKUP].overhead)
    {
        /*if(FOTA_EOK != fotaNvmClear(FOTA_NVM_ZONE_BKUP, FOTA_NVM_DELTA_BACKUP_SIZE, FOTA_NVM_BACKUP_MUX_SIZE))
        {
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_CLR_RCVRY_1, P_ERROR, "post DFU: clear bkup ovhd zone failure!\n");
            return FOTA_EFLERASE;
        }*/

        extern void nvramAfterInit(void);
        nvramAfterInit();
    }
#endif
#endif

    return FOTA_EOK;
}

static int32_t fotaNvmAdjustZoneSize(FotaDefAdjZoneSize_t *adjZone)
{
    if(adjZone->zid >= FOTA_NVM_ZONE_MAXNUM)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_ADJ_ZONESZ_1, P_ERROR, "adj zone: invalid zid(%d)! max(%d)\n", adjZone->zid, FOTA_NVM_ZONE_MAXNUM);
        return FOTA_EARGS;
    }

    if(!(gFotaNvmZoneMan.bmZoneId & (1 << adjZone->zid)))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_ADJ_ZONESZ_2, P_ERROR, "adj zone: no fota zone(%d)! bmZid(0x%x)\n", adjZone->zid, gFotaNvmZoneMan.bmZoneId);
        return FOTA_EUNFOUND;
    }

    uint32_t orginSize = gFotaNvmZoneMan.zone[adjZone->zid].size;
    gFotaNvmZoneMan.zone[adjZone->zid].size = FOTA_MAX(orginSize, adjZone->size);
    if(orginSize != gFotaNvmZoneMan.zone[adjZone->zid].size)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_ADJ_ZONESZ_3, P_INFO, "adj zone(%d): size %d --> %d\n", adjZone->zid, gFotaNvmZoneMan.bmZoneId);
    }
    else
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_ADJ_ZONESZ_4, P_INFO, "adj zone(%d): no need change!\n", adjZone->zid);
    }

    return FOTA_EOK;
}

static int32_t fotaResetParhHashField(CustFotaParHdr_t *parh, int32_t buflen)
{
    if(buflen < sizeof(CustFotaParHdr_t)) return FOTA_EARGS;

    memset(parh->parHash, 0, FOTA_SHA256_HASH_LEN);

    return FOTA_EOK;
}

static int32_t fotaNvmCheckDeltaState(FotaDefChkDeltaState_t *chkDelta)
{
    uint32_t          readLen = 0;
    uint32_t           offset = 0;
    CustFotaParHdr_t     parh;

    memset(gFotaHash, 0, FOTA_SHA256_HASH_LEN);

    if(chkDelta == NULL) return FOTA_EARGS;

    chkDelta->isValid = 0;
    chkDelta->state   = FOTA_DCS_DELTA_UNFOUND;

    fotaNvmRead(FOTA_NVM_ZONE_DELTA, offset, (uint8_t*)&parh, sizeof(CustFotaParHdr_t));
    if(!FOTA_CHECK_PAR_MAGIC(parh.pmagic))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHK_DELTA_1, P_SIG, "delta: not a par! pmagic(%x%x)\n", parh.pmagic[0], parh.pmagic[1]);
        return FOTA_EPAR;
    }

    chkDelta->state = FOTA_DCS_DELTA_INVALID;

    fotaInitChksum(FOTA_CA_SHA256SUM, NULL);

    readLen = 2 * FOTA_SHA256_HWALIGN_SIZE;
    if(FOTA_EOK != FOTA_chksumFlashData(FOTA_NVM_ZONE_DELTA, offset, readLen, &gFotaHash[0], 0, (buf_handle_callback)fotaResetParhHashField))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHK_DELTA_2, P_ERROR, "delta: parh chksum calc fail!\n");

        fotaDeinitChksum(FOTA_CA_SHA256SUM, NULL);
        return FOTA_ECHKSUM;
    }

    offset += readLen;
    readLen = parh.parLen - offset;
    if(FOTA_EOK != FOTA_chksumFlashData(FOTA_NVM_ZONE_DELTA, offset, readLen, &gFotaHash[0], 1, NULL))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHK_DELTA_3, P_ERROR, "delta: par-pl chksum calc fail!\n");

        fotaDeinitChksum(FOTA_CA_SHA256SUM, NULL);
        return FOTA_ECHKSUM;
    }

    if(0 != memcmp(&parh.parHash[0], &gFotaHash[0], FOTA_SHA256_HASH_LEN))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHK_DELTA_4, P_WARNING, "delta: unmatched chksum! curr/wanted as follows:\n");
        FOTA_dumpOctets(gFotaHash, FOTA_SHA256_HASH_LEN);
        FOTA_dumpOctets(parh.parHash, FOTA_SHA256_HASH_LEN);
        return FOTA_EPAR;
    }

    /* valid *.par */
    chkDelta->isValid = 1;
    chkDelta->state   = FOTA_DCS_DELTA_CHECKOK;

    return FOTA_EOK;
}

PLAT_BL_CIRAM_FLASH_TEXT static int32_t fotaNvmIsImageIdentical(FotaDefIsImageIdentical_t *isIdent)
{
    memset(gFotaHash, 0, FOTA_SHA256_HASH_LEN);

    fotaInitChksum(FOTA_CA_SHA256SUM, NULL);

    if(FOTA_EOK != FOTA_chksumFlashData(isIdent->zid, 0, isIdent->size, &gFotaHash[0], 1, NULL))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_IS_IDENTICAL_1, P_ERROR, "image(%d): fw chksum calc fail!\n", isIdent->zid);

        fotaDeinitChksum(FOTA_CA_SHA256SUM, NULL);
        return FOTA_ECHKSUM;
    }

    if(0 != memcmp((void*)isIdent->hash, (void*)&gFotaHash[0], FOTA_SHA256_HASH_LEN))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_IS_IDENTICAL_2, P_WARNING, "image(%d): non-identical! curr/wanted as follows:\n", isIdent->zid);
        FOTA_dumpOctets(gFotaHash, FOTA_SHA256_HASH_LEN);
        FOTA_dumpOctets(isIdent->hash, FOTA_SHA256_HASH_LEN);
        return FOTA_EFWNIDENT;
    }

    ECPLAT_PRINTF(UNILOG_FOTA, FOTA_IS_IDENTICAL_3, P_INFO, "image(%d): identical!\n", isIdent->zid);

    return FOTA_EOK;
}

static int32_t fotaNvmCheckBaseImage(FotaDefChkBaseImage_t *chkImage)
{
    uint32_t                  offset = 0;
    uint16_t                bmFwAttr = 0;
    CustFotaParHdr_t            parh;
    CustFotaPkgHdr_t            pkgh;
    FotaDefIsImageIdentical_t  ident;
    FotaDefChkBootState_t       boot;

    if(chkImage == NULL) return FOTA_EARGS;

    chkImage->isMatched = 0;

    fotaNvmRead(FOTA_NVM_ZONE_DELTA, offset, (uint8_t*)&parh, sizeof(CustFotaParHdr_t));
    if(!FOTA_CHECK_PAR_MAGIC(parh.pmagic))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHK_IMAGE_1, P_SIG, "image: not a *.par! pmagic(%x%x)\n",
                                                             parh.pmagic[0], parh.pmagic[1]);
        return FOTA_EPAR;
    }

    if(!parh.parLen || parh.parLen > fotaNvmGetDeltaSize(1))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHK_IMAGE_2, P_SIG, "image: *.par len(%d) err, maxsize(%d)\n",
                                                             parh.parLen, fotaNvmGetDeltaSize(1));
        return FOTA_EPARSZ;
    }

    ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHK_IMAGE_3, P_INFO, "image: *.par pcap(%d)\n", parh.pcap);

    offset = sizeof(CustFotaParHdr_t) + (parh.pcap ? FOTA_PAR_RETEN_SIZE_16M : FOTA_PAR_RETEN_SIZE_4M);
    for(; offset + sizeof(CustFotaPkgHdr_t) < parh.parLen; offset += pkgh.pkgLen)
    {
        fotaNvmRead(FOTA_NVM_ZONE_DELTA, offset, (uint8_t*)&pkgh, sizeof(CustFotaPkgHdr_t));
        if(!pkgh.pkgLen) return FOTA_EPERM;

        bmFwAttr |= (1 << pkgh.fwAttr);

        ident.zid  = FOTA_convToZoneId(pkgh.fwAttr);
        ident.size = pkgh.baseFwSize;
        ident.hash = pkgh.baseFwHash;
        if(FOTA_EOK != fotaNvmIsImageIdentical(&ident)) return FOTA_EFWNIDENT;
    }

    /* check system head bin exists or not if security boot is enabled! */
    fotaDoExtension(FOTA_DEF_CHK_BOOT_STATE, &boot);
    if(boot.isSigned && (!parh.psigned || !(bmFwAttr & (1 << FOTA_FA_SYSH))))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHK_IMAGE_4, P_WARNING, "image: no signed bin(0x%x) err, psigned(%d)!\n", bmFwAttr, parh.psigned);
        return FOTA_EPERM;
    }

    chkImage->isMatched = 1;

    return FOTA_EOK;
}

/*----------------------------------------------------------------------------*
 *                      GLOBAL FUNCTIONS                                      *
 *----------------------------------------------------------------------------*/
/******************************************************************************
 * @brief : fotaNvmInit
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
PLAT_BL_CIRAM_FLASH_TEXT int32_t fotaNvmInit(void)
{
#if (FOTA_NVM_A2CP_XIP_ADDR != FOTA_NVM_A2AP_XIP_ADDR)
    if(FOTA_IS_CPFLASH_DISABLED())
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_INIT, P_SIG, "cp flash wil be enabled...\n");

        uint32_t mask = SaveAndSetIRQMask();
        BSP_QSPI_ENABLE_CP_FLASH();
        RestoreIRQMask(mask);
    }
#endif

    gFotaNvmZoneMan.bmZoneId = FOTA_NVM_BM_ZONE_NONE;

#ifdef FOTA_NVM_FS_ENABLE
    fotaNvmSetZone(FOTA_NVM_ZONE_REMAP, FOTA_NVM_REMAP_ADDR, FOTA_NVM_REMAP_SIZE, 0, FOTA_NVM_A2AP_XIP_ADDR);
    fotaNvmSetZone(FOTA_NVM_ZONE_DELTA, fotaNvmFsOpen(FOTA_DELTA_PAR_NAME, "rd"), FOTA_DELTA_PAR_MAXSIZE, 0, 0);
#else
    fotaNvmSetZone(FOTA_NVM_ZONE_DELTA, FOTA_NVM_DELTA_ADDR, FOTA_NVM_DELTA_SIZE, FOTA_NVM_DELTA_BACKUP_SIZE, FOTA_NVM_A2AP_XIP_ADDR);
#endif
    fotaNvmSetZone(FOTA_NVM_ZONE_BKUP, FOTA_NVM_REAL_BACKUP_ADDR, FOTA_NVM_REAL_BACKUP_SIZE, FOTA_NVM_BACKUP_MUX_SIZE, FOTA_NVM_A2AP_XIP_ADDR);
#if defined CHIP_EC618 || defined CHIP_EC618_Z0 || defined CHIP_EC718 || defined CHIP_EC716
    fotaNvmSetZone(FOTA_NVM_ZONE_AP, FOTA_NVM_AP_LOAD_ADDR, FOTA_NVM_AP_LOAD_SIZE, 0, FOTA_NVM_A2AP_XIP_ADDR);
    fotaNvmSetZone(FOTA_NVM_ZONE_CP, FOTA_NVM_CP_LOAD_ADDR, FOTA_NVM_CP_LOAD_SIZE, 0, FOTA_NVM_A2CP_XIP_ADDR);
    fotaNvmSetZone(FOTA_NVM_ZONE_SYSH, FOTA_NVM_SYSH_LOAD_ADDR, FOTA_NVM_SYSH_LOAD_SIZE, 0, FOTA_NVM_A2AP_XIP_ADDR);
#endif
#ifdef FOTA_CUST_APP_ENABLE
    fotaNvmSetZone(FOTA_NVM_ZONE_APP, FOTA_NVM_APP_LOAD_ADDR, FOTA_NVM_APP_LOAD_SIZE, 0, FOTA_NVM_A2AP_XIP_ADDR);
#endif

    return FOTA_EOK;
}

/******************************************************************************
 * @brief : fotaNvmDeinit
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
PLAT_BL_CIRAM_FLASH_TEXT int32_t fotaNvmDeinit(void)
{
    BSP_QSPI_DISABLE_CP_FLASH();

    /* CAUTION:
     * some zones will be acessed later, e.g. getting DFU result,
     * therefore, NEVER deinit the flags of these zones.
     */
    //gFotaNvmZoneMan.bmZoneId = FOTA_NVM_BM_ZONE_NONE;

    return FOTA_EOK;
}

/******************************************************************************
 * @brief : fotaNvmClear
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
PLAT_BL_CIRAM_FLASH_TEXT int32_t fotaNvmClear(uint32_t zid, uint32_t offset, uint32_t len)
{
#ifdef FOTA_NVM_FS_ENABLE
    if(zid == FOTA_NVM_ZONE_DELTA)
    {
        return fotaNvmFsRemove(FOTA_DELTA_PAR_NAME);
    }
    else
#endif
    {
        return fotaNvmNfsClear(zid, offset, len);
    }
}

/******************************************************************************
 * @brief : fotaNvmWrite
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
PLAT_BL_CIRAM_FLASH_TEXT int32_t fotaNvmWrite(uint32_t zid, uint32_t offset, uint8_t *buf, uint32_t bufLen)
{
#ifdef FOTA_NVM_FS_ENABLE
    if(zid == FOTA_NVM_ZONE_DELTA)
    {
    #ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_WRITE_0, P_WARNING, "dont wr delta! wr remap instead!\n");
        return FOTA_EPERM;
    #else
        int32_t   fd = gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_DELTA].handle;

        fotaNvmFsSeek(fd, offset, SEEK_SET);
        return fotaNvmFsWrite(buf, bufLen, 1, fd);
    #endif
    }
    else
#endif
    {
        return fotaNvmNfsWrite(zid, offset, buf, bufLen);
    }
}

/******************************************************************************
 * @brief : fotaNvmRead
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
PLAT_BL_CIRAM_FLASH_TEXT int32_t fotaNvmRead(uint32_t zid, uint32_t offset, uint8_t *buf, uint32_t bufLen)
{
#ifdef FOTA_NVM_FS_ENABLE
    int32_t   fd = gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_DELTA].handle;

    if(zid == FOTA_NVM_ZONE_DELTA)
    {
        fotaNvmFsSeek(fd, offset, SEEK_SET);
        return fotaNvmFsRead(buf, bufLen, 1, fd);
    }
    else
#endif
    {
        return fotaNvmNfsRead(zid, offset, buf, bufLen);
    }
}

/******************************************************************************
 * @brief : fotaNvmGetZoneId
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
uint32_t fotaNvmGetZoneId(uint32_t addr, uint32_t *size, uint32_t *offset)
{
    uint32_t   zid = FOTA_NVM_ZONE_DELTA;
    uint32_t   zsz = FOTA_NVM_DELTA_DOWNLOAD_SIZE;

    if(!size)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_ZID_1, P_ERROR, "get zid: null size ptr!\n");
        goto GET_ZID_END;
    }

    if(!addr && !(*size))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_ZID_2, P_SIG, "get zid: set default zone(%d: %d)!\n", zid, zsz);
        goto GET_ZID_END;
    }

    /* no xip offset */
    addr &= (~FLASH_XIP_ADDR);

    switch(addr)
    {
        case FOTA_NVM_DELTA_ADDR:
        {
            zid = FOTA_NVM_ZONE_DELTA;
            zsz = FOTA_NVM_DELTA_DOWNLOAD_SIZE;
            break;
        }
        case FOTA_NVM_DELTA_BACKUP_ADDR:
        {
            zid = FOTA_NVM_ZONE_BKUP;
            zsz = FOTA_NVM_DELTA_BACKUP_SIZE;
            break;
        }
    #if defined CHIP_EC618 || defined CHIP_EC618_Z0 || defined CHIP_EC718 || defined CHIP_EC716
        case FOTA_NVM_AP_LOAD_ADDR:
        {
            zid = FOTA_NVM_ZONE_AP;
            zsz = FOTA_NVM_AP_LOAD_SIZE;
            break;
        }
        case FOTA_NVM_CP_LOAD_ADDR:
        {
            zid = FOTA_NVM_ZONE_CP;
            zsz = FOTA_NVM_CP_LOAD_SIZE;
            break;
        }
    #endif

        default:
        {
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_ZID_3, P_WARNING, "get zid: unknown starting addr(0x%x)!\n", addr);
            break;
        }
    }

GET_ZID_END:
    if(!(*size))
    {
        *size = zsz;
    }
    else
    {
        if(*size != zsz)
        {
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_ZID_4, P_WARNING, "get zid: in_size(%d) != zone_size(%d)!\n", *size, zsz);
        }
    }

    if(offset) *offset = 0;

    return zid;
}


/******************************************************************************
 * @brief : fotaNvmGetSize
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
PLAT_BL_CIRAM_FLASH_TEXT uint32_t fotaNvmGetSize(uint32_t zid, uint8_t isOvhdExcl)
{
    if(zid >= FOTA_NVM_ZONE_MAXNUM)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_SZ_1, P_WARNING, "get size: invalid zid(%d)! max(%d)\n",
                                                                  zid, FOTA_NVM_ZONE_MAXNUM);
        return 0;
    }

    if(!(gFotaNvmZoneMan.bmZoneId & (1 << zid)))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_SZ_2, P_WARNING, "get size: no fota zone(%d)! bmZid(0x%x)\n",
                                                                  zid, gFotaNvmZoneMan.bmZoneId);
        return 0;
    }

    return (gFotaNvmZoneMan.zone[zid].size - (isOvhdExcl ? gFotaNvmZoneMan.zone[zid].overhead : 0));
}

/******************************************************************************
 * @brief : fotaNvmGetHandle
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
PLAT_BL_CIRAM_FLASH_TEXT int32_t fotaNvmGetHandle(uint32_t zid)
{
    if(zid >= FOTA_NVM_ZONE_MAXNUM)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_HNDL_1, P_WARNING, "get handle: invalid zid(%d)! max(%d)\n",
                                                                    zid, FOTA_NVM_ZONE_MAXNUM);
        return FOTA_EARGS;
    }

    if(!(gFotaNvmZoneMan.bmZoneId & (1 << zid)))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_HNDL_2, P_WARNING, "get handle: no fota zone(%d)! bmZid(0x%x)\n",
                                                                    zid, gFotaNvmZoneMan.bmZoneId);
        return FOTA_EUNFOUND;
    }

    return gFotaNvmZoneMan.zone[zid].handle;
}

/******************************************************************************
 * @brief : fotaNvmGetExtras
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
PLAT_BL_CIRAM_FLASH_TEXT int32_t fotaNvmGetExtras(uint32_t zid)
{
    if(zid >= FOTA_NVM_ZONE_MAXNUM)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_XTRAS_1, P_WARNING, "get xtras: invalid zid(%d)! max(%d)\n",
                                                                     zid, FOTA_NVM_ZONE_MAXNUM);
        return FOTA_EARGS;
    }

    if(!(gFotaNvmZoneMan.bmZoneId & (1 << zid)))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_NVM_GET_XTRAS_2, P_WARNING, "get xtras: no fota zone(%d)! bmZid(0x%x)\n",
                                                                     zid, gFotaNvmZoneMan.bmZoneId);
        return FOTA_EUNFOUND;
    }

    return gFotaNvmZoneMan.zone[zid].extras;
}

/******************************************************************************
 * @brief : fotaNvmVerifyDelta
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
int32_t fotaNvmVerifyDelta(uint32_t zid, uint8_t *hash, uint32_t pkgSize, uint32_t *deltaState)
{
    FotaDefChkDeltaState_t    chkDelta = {0};
    FotaDefChkBaseImage_t      chkBase = {0};

    fotaNvmCheckDeltaState(&chkDelta);
    if(!chkDelta.isValid)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, VERIFY_DELTA_1, P_ERROR, "validate delta err! errno(%d)", chkDelta.state);
        return FOTA_EPAR;
    }

    ECPLAT_PRINTF(UNILOG_FOTA, VERIFY_DELTA_2, P_SIG, "validate delta ok!");

    fotaNvmCheckBaseImage(&chkBase);
    if(!chkBase.isMatched)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, VERIFY_DELTA_3, P_WARNING, "however, base fw is unmatched!");
        chkDelta.state = FOTA_DCS_DELTA_UNMATCHB;
        return FOTA_EFWNIDENT;
    }

    if(deltaState)
    {
        *deltaState = chkDelta.state;
    }

    return FOTA_EOK;
}

/******************************************************************************
 * @brief : fotaNvmGetOtaResult
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
int32_t  fotaNvmGetOtaResult(uint32_t zid, int32_t *deltaState)
{
    FotaDefDfuResult_t  dfuResult = {0};

    if(FOTA_EOK != fotaNvmGetDfuResult(&dfuResult))
    {
        dfuResult.deltaState = FOTA_DCS_DELTA_UNDEF;
    }

    if(deltaState)
    {
        *deltaState = dfuResult.deltaState;
    }

    return dfuResult.dfuResult;
}

/******************************************************************************
 * @brief : fotaNvmDoExtension
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
int32_t fotaNvmDoExtension(uint32_t flags, void *args)
{
    int32_t retCode = FOTA_EUNDEF;

    switch(flags)
    {
        case FOTA_DEF_CHK_REMAP_ZONE:
            retCode = fotaNvmCheckRemapZone((FotaDefChkRemapZone_t*)args);
            break;
        case FOTA_DEF_CHK_DELTA_STATE:
            retCode = fotaNvmCheckDeltaState((FotaDefChkDeltaState_t*)args);
            break;
        case FOTA_DEF_CHK_BASE_IMAGE:
            retCode = fotaNvmCheckBaseImage((FotaDefChkBaseImage_t*)args);
            break;
        case FOTA_DEF_IS_IMAGE_IDENTICAL:
            retCode = fotaNvmIsImageIdentical((FotaDefIsImageIdentical_t*)args);
            break;
        case FOTA_DEF_SET_DOWNLOAD_OVER:
            break;
        case FOTA_DEF_SET_DFU_RESULT:
            retCode = fotaNvmSetDfuResult((FotaDefDfuResult_t*)args);
            break;
        case FOTA_DEF_GET_DFU_RESULT:
            retCode = fotaNvmGetDfuResult((FotaDefDfuResult_t*)args);
            break;
        case FOTA_DEF_PREPARE_DFU:
            retCode = fotaNvmPrepareDfu((FotaDefPrepareDfu_t*)args);
            break;
        case FOTA_DEF_CLOSING_DFU:
            retCode = fotaNvmClosingDfu((FotaDefClosingDfu_t*)args);
            break;
        case FOTA_DEF_ADJ_ZONE_SIZE:
            retCode = fotaNvmAdjustZoneSize((FotaDefAdjZoneSize_t*)args);

        default:
            break;
    }

    return retCode;
}

