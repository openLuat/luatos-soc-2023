#include "psdial_ps_ctrl.h"
#include "cms_comm.h"
#include "common_api.h"
#include "pscommtype.h"
#include "usbd_clscdc.h"
#include "usbd_func_cc.h"
#include "usbd_multi_usrcfg_common.h"
#include "system_ec7xx.h"
#include "fota_utils.h"
#define PS_DIAL_PS_UP_MEM_SIZE      (UP_BUF_MAX_SIZE)
ALIGNED_4BYTE CAT_PSPHY_SHAREDATA   UINT8   psUpMem[PS_DIAL_PS_UP_MEM_SIZE];
void *psDialGetUpMemAndSize(UINT32 *pUpMemSize)
{
    if (pUpMemSize != PNULL)
    {
        *pUpMemSize     = sizeof(psUpMem);
    }

    return (void *)psUpMem;
}

/**
  \fn        sysROAddrCheck(uint32_t addr)
  \brief     This function is called in flash erase or write apis to prevent
             unexpected access the bootloader image read only area..
  \param[in] Addr: flash erase or write addr
  \note      Be careful if you want change this function.
 */
static uint8_t sysROAddrCheck(uint32_t addr)
{
    //protect 2 hdr
    if(addr <FUSE_FLASH_MIRROR_ADDR)
    {
          return 1;
    }

    //protect bl
    if((addr >=(BOOTLOADER_FLASH_LOAD_ADDR-AP_FLASH_XIP_ADDR))
          && (addr <(BOOTLOADER_FLASH_LOAD_ADDR+BOOTLOADER_FLASH_LOAD_SIZE-AP_FLASH_XIP_ADDR)))
    {
          return 1;
    }

    //protect app
    if((addr >=(AP_FLASH_LOAD_ADDR-AP_FLASH_XIP_ADDR))
        && (addr <(AP_FLASH_LOAD_ADDR+AP_FLASH_LOAD_SIZE-AP_FLASH_XIP_ADDR)))
    {
        return 1;
    }

    return 0;
}



 /*----------------------------------------------------------------------------*
  *                      GLOBAL FUNCTIONS                                      *
  *----------------------------------------------------------------------------*/


 /**
  \fn        sysROSpaceCheck(uint32_t addr, uint32_t size)
  \brief     This function is called in flash erase or write apis to prevent
             unexpected access the bootloader image read only area..
  \param[in] Addr: flash erase or write addr
             Addr: flash erase or write size
  \note      Be careful if you want change this function.
 */
uint8_t sysROSpaceCheck(uint32_t addr, uint32_t size)

{
    if(sysROAddrCheck(addr))
    {
        return 1;
    }
    if (sysROAddrCheck(addr+size - 1))
    {
        return 1;
    }
    return 0;
}


/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/

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
#define FOTA_NVM_DELTA_BACKUP_SIZE     (FOTA_BUF_SIZE_1K * 44)

#else  /* raw flash */
#define FOTA_NVM_DELTA_ADDR            (FLASH_FOTA_REGION_START)
#define FOTA_NVM_DELTA_SIZE            (FLASH_FOTA_REGION_LEN)

#define FOTA_NVM_DELTA_DOWNLOAD_ADDR   (FOTA_NVM_DELTA_ADDR)
#define FOTA_NVM_DELTA_DOWNLOAD_SIZE   (FOTA_NVM_DELTA_SIZE - FOTA_NVM_DELTA_BACKUP_SIZE)

#define FOTA_NVM_DELTA_BACKUP_ADDR     (FOTA_NVM_DELTA_ADDR + FOTA_NVM_DELTA_DOWNLOAD_SIZE)
#ifdef __USER_CODE__
#define FOTA_NVM_DELTA_BACKUP_SIZE     (FOTA_BUF_SIZE_1K * 96)
#else
#define FOTA_NVM_DELTA_BACKUP_SIZE     (FOTA_BUF_SIZE_1K * 44)
#endif

#endif

#define FOTA_NVM_REAL_BACKUP_ADDR      (FOTA_NVM_DELTA_BACKUP_ADDR)
#define FOTA_NVM_REAL_BACKUP_SIZE      (FOTA_NVM_DELTA_BACKUP_SIZE + FOTA_NVM_BACKUP_MUX_SIZE)
#ifdef __USER_CODE__
#define FOTA_NVM_BACKUP_MUX_SIZE       (0)
#else
#define FOTA_NVM_BACKUP_MUX_SIZE       (NVRAM_PHYSICAL_SIZE)
#endif

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

#define FOTA_NVM_SECTOR_SIZE           FOTA_BUF_SIZE_4K
#define FOTA_NVM_BLOCK_SIZE            FOTA_BUF_SIZE_32K

#define FOTA_NVM_AP_LOAD_ADDR          (AP_FLASH_LOAD_ADDR & (~AP_FLASH_XIP_ADDR))
#define FOTA_NVM_AP_LOAD_SIZE          (AP_FLASH_LOAD_SIZE)

#define FOTA_NVM_CP_LOAD_ADDR          (CP_FLASH_LOAD_ADDR & (~CP_FLASH_XIP_ADDR))
#define FOTA_NVM_CP_LOAD_SIZE          (CP_FLASH_LOAD_SIZE)

#define FOTA_NVM_SYSH_LOAD_ADDR        (SYS_SEC_HAED_ADDR)
#define FOTA_NVM_SYSH_LOAD_SIZE        (SYS_FLASH_LOAD_SIZE)


int32_t soc_fota_nvm_init(void)
{

#if 1
#ifdef FOTA_NVM_FS_ENABLE
    fotaNvmSetZone(FOTA_NVM_ZONE_REMAP, FOTA_NVM_REMAP_ADDR, FOTA_NVM_REMAP_SIZE, 0, FOTA_NVM_A2AP_XIP_ADDR);
    fotaNvmSetZone(FOTA_NVM_ZONE_DELTA, fotaNvmFsOpen(FOTA_DELTA_PAR_NAME, "rd"), FOTA_DELTA_PAR_MAXSIZE, 0, 0);
#else
    fotaNvmSetZone(FOTA_NVM_ZONE_DELTA, FOTA_NVM_DELTA_ADDR, FOTA_NVM_DELTA_SIZE, FOTA_NVM_DELTA_BACKUP_SIZE, FOTA_NVM_A2AP_XIP_ADDR);
#endif
    fotaNvmSetZone(FOTA_NVM_ZONE_BKUP, FOTA_NVM_REAL_BACKUP_ADDR, FOTA_NVM_REAL_BACKUP_SIZE, FOTA_NVM_BACKUP_MUX_SIZE, FOTA_NVM_A2AP_XIP_ADDR);
    fotaNvmSetZone(FOTA_NVM_ZONE_AP, FOTA_NVM_AP_LOAD_ADDR, FOTA_NVM_AP_LOAD_SIZE, 0, FOTA_NVM_A2AP_XIP_ADDR);
    fotaNvmSetZone(FOTA_NVM_ZONE_CP, FOTA_NVM_CP_LOAD_ADDR, FOTA_NVM_CP_LOAD_SIZE, 0, FOTA_NVM_A2CP_XIP_ADDR);
#ifdef FOTA_CUST_APP_ENABLE
    fotaNvmSetZone(FOTA_NVM_ZONE_APP, FOTA_NVM_APP_LOAD_ADDR, FOTA_NVM_APP_LOAD_SIZE, 0, FOTA_NVM_A2AP_XIP_ADDR);
#endif
    fotaNvmSetZone(FOTA_NVM_ZONE_SYSH, FOTA_NVM_SYSH_LOAD_ADDR, FOTA_NVM_SYSH_LOAD_SIZE, 0, FOTA_NVM_A2AP_XIP_ADDR);

#else
#ifdef FOTA_NVM_FS_ENABLE
    gFotaNvmZoneMan.bmZoneId |= FOTA_NVM_BM_ZONE_REMAP ;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_REMAP].handle   = FOTA_NVM_REMAP_ADDR;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_REMAP].size     = FOTA_NVM_REMAP_SIZE;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_REMAP].overhead = 0;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_REMAP].extras   = FOTA_NVM_A2AP_XIP_ADDR;

    gFotaNvmZoneMan.bmZoneId |= FOTA_NVM_BM_ZONE_DELTA ;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_DELTA].handle   = fotaNvmFsOpen(FOTA_DELTA_PAR_NAME, "rd");
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_DELTA].size     = FOTA_DELTA_PAR_MAXSIZE;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_DELTA].overhead = 0;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_DELTA].extras   = 0;

#else
    gFotaNvmZoneMan.bmZoneId |= FOTA_NVM_BM_ZONE_DELTA ;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_DELTA].handle   = FOTA_NVM_DELTA_ADDR;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_DELTA].size     = FOTA_NVM_DELTA_SIZE;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_DELTA].overhead = FOTA_NVM_DELTA_BACKUP_SIZE;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_DELTA].extras   = FOTA_NVM_A2AP_XIP_ADDR;

#endif

    gFotaNvmZoneMan.bmZoneId |= FOTA_NVM_BM_ZONE_BKUP ;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_BKUP].handle    = FOTA_NVM_REAL_BACKUP_ADDR;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_BKUP].size      = FOTA_NVM_REAL_BACKUP_SIZE;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_BKUP].overhead  = FOTA_NVM_BACKUP_MUX_SIZE;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_BKUP].extras    = FOTA_NVM_A2AP_XIP_ADDR;

    gFotaNvmZoneMan.bmZoneId |= FOTA_NVM_BM_ZONE_AP ;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_AP].handle      = FOTA_NVM_AP_LOAD_ADDR;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_AP].size        = FOTA_NVM_AP_LOAD_SIZE;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_AP].overhead    = 0;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_AP].extras      = FOTA_NVM_A2AP_XIP_ADDR;

    gFotaNvmZoneMan.bmZoneId |= FOTA_NVM_BM_ZONE_CP ;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_CP].handle      = FOTA_NVM_CP_LOAD_ADDR;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_CP].size        = FOTA_NVM_CP_LOAD_SIZE;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_CP].overhead    = 0;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_CP].extras      = FOTA_NVM_A2CP_XIP_ADDR;

#ifdef FOTA_CUST_APP_ENABLE
    gFotaNvmZoneMan.bmZoneId |= FOTA_NVM_BM_ZONE_APP ;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_APP].handle     = FOTA_NVM_APP_LOAD_ADDR;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_APP].size       = FOTA_NVM_APP_LOAD_SIZE;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_APP].overhead   = 0;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_APP].extras     = FOTA_NVM_A2AP_XIP_ADDR;
#endif

    gFotaNvmZoneMan.bmZoneId |= FOTA_NVM_BM_ZONE_SYSH ;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_SYSH].handle      = FOTA_NVM_SYSH_LOAD_ADDR;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_SYSH].size        = FOTA_NVM_SYSH_LOAD_SIZE;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_SYSH].overhead    = 0;
    gFotaNvmZoneMan.zone[FOTA_NVM_ZONE_SYSH].extras      = FOTA_NVM_A2AP_XIP_ADDR;

#endif

    return FOTA_EOK;
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
    addr &= (~AP_FLASH_XIP_ADDR);

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

#include "7zAlloc.h"
#include "7zFile.h"
#include "LzFind.h"
#include "LzmaEnc.h"
#include "LzmaDec.h"
#include "LzmaEc.h"
#include "mem_map.h"
extern LZMA_BSS_SECTION ecCompBinHeader compBinHeader;
extern LZMA_BSS_SECTION ecCompBinSectionInfo *pCompBinSectionInfoAddr;
extern LZMA_BSS_SECTION unsigned int hashBuffStartAddr;
extern LZMA_BSS_SECTION unsigned int decompBuffStartAddr;
extern LZMA_BSS_SECTION unsigned int decompBuffEndAddr;
PLAT_BL_UNCOMP_FLASH_TEXT void decompressRamCodeGetAddrInfo(void)
{
    unsigned int pFlashXIPAddr = 0;

    #ifdef CORE_IS_AP
    pFlashXIPAddr = *(int *)((AP_FLASH_LOAD_SIZE-8)+AP_FLASH_LOAD_ADDR)+AP_FLASH_LOAD_ADDR;
    #endif
    #ifdef CORE_IS_CP
    pFlashXIPAddr = *(int *)((CP_FLASH_LOAD_SIZE-8)+CP_FLASH_LOAD_ADDR)+CP_FLASH_LOAD_ADDR;
    #endif
    #ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
    pFlashXIPAddr = *(int *)((BOOTLOADER_FLASH_LOAD_SIZE-8)+BOOTLOADER_FLASH_LOAD_ADDR)+BOOTLOADER_FLASH_LOAD_ADDR;
    #endif

    compBinHeader.magicHdr = *(unsigned int *)pFlashXIPAddr;
    compBinHeader.numOfSec = *(unsigned int *)(pFlashXIPAddr+4);
    pCompBinSectionInfoAddr = (ecCompBinSectionInfo *)(pFlashXIPAddr+8);

    #ifdef CORE_IS_AP
    decompBuffStartAddr  = up_buf_start;
    decompBuffEndAddr    = decompBuffStartAddr + AP_DECOMP_MEM_RF_CALIB_MAX_LEN;
    hashBuffStartAddr    = decompBuffStartAddr + AP_HASH_MEM_OFFSET;
    #endif

    #ifdef CORE_IS_CP
    decompBuffStartAddr = ShareInfoCPGetCpdprsBufAddr();
    decompBuffEndAddr   = decompBuffStartAddr + CP_DECOMP_MEM_RF_CALIB_MAX_LEN;
    hashBuffStartAddr   = decompBuffStartAddr + CP_HASH_MEM_OFFSET;
    #endif

    #ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
    decompBuffStartAddr  = (unsigned int)&_decompress_buf_start;
    decompBuffEndAddr    = decompBuffStartAddr + AP_DECOMP_MEM_RF_CALIB_MAX_LEN;
    hashBuffStartAddr    = decompBuffStartAddr + AP_HASH_MEM_OFFSET;

    #endif
}
