#include "ec7xx.h"
#include "stdint.h"
#include "mem_map.h"
#define AP_BODY_MAX_SIZE           AP_FLASH_LOAD_SIZE
uint32_t soc_ap_body_max_size(void)
{
	return AP_BODY_MAX_SIZE;
}

__attribute__((weak)) void user_code_run(void) {;}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "7zAlloc.h"
#include "7zFile.h"
#include "LzFind.h"
#include "LzmaEnc.h"
#include "LzmaDec.h"
#include "LzmaEc.h"
#include "mem_map.h"
#ifdef CORE_IS_CP
#include "shareinfo.h"
#endif
#ifdef CORE_IS_AP
#include "tls.h"
#include "apmu_external.h"
#endif

extern UINT32 flashXIPLimit;

#ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
extern UINT32 _compress_buf_start;
extern UINT32 _decompress_buf_start;
#endif

extern LZMA_BSS_SECTION ecCompBinSectionInfo *pCompBinSectionInfoAddr;
extern LZMA_BSS_SECTION ecCompBinHeader compBinHeader;
extern LZMA_BSS_SECTION unsigned int hashBuffStartAddr;
extern LZMA_BSS_SECTION unsigned int decompBuffStartAddr;
extern LZMA_BSS_SECTION unsigned int decompBuffEndAddr;

#ifdef CORE_IS_AP
extern void sctInitLzma();
extern void sctDeInitLzma();
extern int32_t shaUpdateLzma(shaType_e shaMode, uint32_t srcAddr, uint32_t dstAddr, uint32_t length, uint32_t lastFlag);
extern int32_t dbusCacheForceOff( BOOL isForceOff );
extern uint32_t ShareInfoAPGetCPFlashXIPLimit(void);

#endif

PLAT_BL_UNCOMP_FLASH_TEXT void decompressGetAddrInfo(void)
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
