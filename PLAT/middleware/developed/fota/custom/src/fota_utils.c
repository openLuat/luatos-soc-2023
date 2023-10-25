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

#include "fota_sal.h"
#include "fota_utils.h"
#include "fota_mem.h"
#include "fota_nvm.h"
#include "fota_chksum.h"

#define FOTA_SHA256_BLOCK_MIN_SIZE     FOTA_BUF_SIZE_1K
#define FOTA_SHA256_BLOCK_MAX_SIZE     FOTA_BUF_SIZE_16K

#if (FOTA_PRESET_RAM_ENABLE == 1)
extern uint32_t _fota_mux_buf_start;
extern uint32_t _fota_mux_buf_end;

#endif

/******************************************************************************
 * @brief : FOTA_getUnzipNextBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
int32_t FOTA_getUnzipNextBuf(FotaBufferDesc_t *desc)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    FOTA_BUF_DESC_SET(desc, (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_UNZIP_NEXT_OFFS), FOTA_PMB_UNZIP_NEXT_SIZE, 0);
#endif
    return FOTA_EOK;
}

/******************************************************************************
 * @brief : FOTA_getZipNextBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
int32_t FOTA_getZipNextBuf(FotaBufferDesc_t *desc)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    FOTA_BUF_DESC_SET(desc, (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_ZIP_NEXT_OFFS), FOTA_PMB_ZIP_NEXT_SIZE, 0);
#endif
    return FOTA_EOK;
}

/******************************************************************************
 * @brief : FOTA_getSha256DataBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
uint8_t* FOTA_getSha256DataBuf(void)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    return (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_SHA256_DATA_OFFS);
#else
    return NULL;
#endif
}

/******************************************************************************
 * @brief : FOTA_getComprBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
uint8_t* FOTA_getComprBuf(void)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    return (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_COMPR_OFFS);
#else
    return NULL;
#endif
}

/******************************************************************************
 * @brief : FOTA_getDecomprBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
uint8_t* FOTA_getDecomprBuf(void)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    return (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_DECOMPR_OFFS);
#else
    return NULL;
#endif
}

/******************************************************************************
 * @brief : FOTA_getDecomprBzipFileBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
uint8_t* FOTA_getDecomprBzipFileBuf(void)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    return (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_DECOMPR_BZIP_FBUFF_OFFS);
#else
    return NULL;
#endif
}

/******************************************************************************
 * @brief : FOTA_getDecomprBzipDstateBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
uint8_t* FOTA_getDecomprBzipDstateBuf(void)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    return (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_DECOMPR_BZIP_DSTATE_OFFS);
#else
    return NULL;
#endif
}

/******************************************************************************
 * @brief : FOTA_getDecomprBzipLl16Buf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
uint8_t* FOTA_getDecomprBzipLl16Buf(void)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    return (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_DECOMPR_BZIP_LL16_OFFS);
#else
    return NULL;
#endif
}

/******************************************************************************
 * @brief : FOTA_getDecomprBzipLl4Buf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
uint8_t* FOTA_getDecomprBzipLl4Buf(void)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    return (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_DECOMPR_BZIP_LL4_OFFS);
#else
    return NULL;
#endif
}

/******************************************************************************
 * @brief : FOTA_getZipDeltaBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
int32_t FOTA_getZipDeltaBuf(FotaBufferDesc_t *desc)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    FOTA_BUF_DESC_SET(desc, (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_ZIP_DELTA_OFFS), FOTA_PMB_ZIP_DELTA_SIZE, 0);
#endif
    return FOTA_EOK;
}

/******************************************************************************
 * @brief : FOTA_getUnzipDeltaBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
int32_t FOTA_getUnzipDeltaBuf(FotaBufferDesc_t *desc)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    FOTA_BUF_DESC_SET(desc, (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_UNZIP_DELTA_OFFS), FOTA_PMB_UNZIP_DELTA_SIZE, 0);
#endif
    return FOTA_EOK;
}

/******************************************************************************
 * @brief : FOTA_getUnzipBaseBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
int32_t FOTA_getUnzipBaseBuf(FotaBufferDesc_t *desc)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    FOTA_BUF_DESC_SET(desc, (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_UNZIP_BASE_OFFS), FOTA_PMB_UNZIP_BASE_SIZE, 0);
#endif
    return FOTA_EOK;
}

/******************************************************************************
 * @brief : FOTA_getDeltaPzghBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
uint8_t* FOTA_getDeltaPzghBuf(void)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    return (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_DELTA_PZGH_OFFS);
#else
    return NULL;
#endif
}

/******************************************************************************
 * @brief : FOTA_getZipSectBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
int32_t FOTA_getZipSectBuf(FotaBufferDesc_t *desc)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    FOTA_BUF_DESC_SET(desc, (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_ZIP_SECT_OFFS), FOTA_PMB_ZIP_SECT_SIZE, 0);
#endif
    return FOTA_EOK;
}

/******************************************************************************
 * @brief : FOTA_getBspatDepsBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
int32_t FOTA_getBspatDepsBuf(FotaBufferDesc_t *desc)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    FOTA_BUF_DESC_SET(desc, (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_BSPAT_DEPS_OFFS), FOTA_PMB_BSPAT_DEPS_SIZE, 0);
#endif
    return FOTA_EOK;
}

/******************************************************************************
 * @brief : FOTA_getZipNvRamBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
uint8_t* FOTA_getZipNvRamBuf(void)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    return (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_ZIP_NVRAM_OFFS);
#else
    return NULL;
#endif
}

/******************************************************************************
 * @brief : FOTA_getUnzipNvRamBuf
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
uint8_t* FOTA_getUnzipNvRamBuf(void)
{
#if (FOTA_PRESET_RAM_ENABLE == 1)
    return (uint8_t*)((uint32_t)&_fota_mux_buf_start + FOTA_PMB_UNZIP_NVRAM_OFFS);
#else
    return NULL;
#endif
}


/******************************************************************************
 * @brief : FOTA_convToZoneId
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
uint32_t FOTA_convToZoneId(uint32_t fwAttr)
{
    uint32_t   zoneId = FOTA_NVM_ZONE_UNDEF;

    switch(fwAttr)
    {
        case FOTA_FA_BL:
            zoneId = FOTA_NVM_ZONE_BLU;
            break;
        case FOTA_FA_AP:
            zoneId = FOTA_NVM_ZONE_AP;
            break;
        case FOTA_FA_CP:
            zoneId = FOTA_NVM_ZONE_CP;
            break;
        case FOTA_FA_APP:
            zoneId = FOTA_NVM_ZONE_APP;
            break;
        case FOTA_FA_APP2:
            zoneId = FOTA_NVM_ZONE_APP2;
            break;
        case FOTA_FA_APP3:
            zoneId = FOTA_NVM_ZONE_APP3;
            break;
        case FOTA_FA_SYSH:
            zoneId = FOTA_NVM_ZONE_SYSH;
            break;

        default:
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CONVTO_ZONEID, P_WARNING, "undef fwAttr(%d)!\n", fwAttr);
            break;
    }

    return zoneId;
}

/******************************************************************************
 * @brief : FOTA_convToFwAttr
 * @author: Xu.Wang
 * @note  :
******************************************************************************/
uint32_t FOTA_convToFwAttr(uint32_t zoneId)
{
    uint32_t   fwAttr = FOTA_FA_UNDEF;

    switch(zoneId)
    {
        case FOTA_NVM_ZONE_BLU:
            fwAttr = FOTA_FA_BL;
            break;
        case FOTA_NVM_ZONE_AP:
            fwAttr = FOTA_FA_AP;
            break;
        case FOTA_NVM_ZONE_CP:
            fwAttr = FOTA_FA_CP;
            break;
        case FOTA_NVM_ZONE_APP:
            fwAttr = FOTA_FA_APP;
            break;
        case FOTA_NVM_ZONE_APP2:
            fwAttr = FOTA_FA_APP2;
            break;
        case FOTA_NVM_ZONE_APP3:
            fwAttr = FOTA_FA_APP3;
            break;
        case FOTA_NVM_ZONE_SYSH:
            fwAttr = FOTA_FA_SYSH;
            break;

        default:
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CONVTO_FWATTR, P_WARNING, "invalid zoneId(%d)!\n", zoneId);
            break;
    }

    return fwAttr;
}

/******************************************************************************
 * @brief : FOTA_chksumFlashData
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
int32_t FOTA_chksumFlashData(uint32_t zid, uint32_t offset, uint32_t size, uint8_t *hash, uint32_t isLast, buf_handle_callback buf_handle_cb)
{
    uint32_t               loop = 0;
    uint32_t          ajustSize = 0;
    uint32_t          allocSize = 0;
    uint32_t         remainSize = size;
    FotaCaSha256Sum_t    sha256 = {0};

    if((!isLast) && (remainSize % FOTA_SHA256_HWALIGN_SIZE))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHKSUM_FLASH_1, P_WARNING, "err! not aligned by %d bytes @non-last data!\n", FOTA_SHA256_HWALIGN_SIZE);
        return FOTA_ECHKSUM;
    }

    if(remainSize < FOTA_SHA256_BLOCK_MIN_SIZE)  /* 1K- */
    {
        ajustSize = FOTA_SHA256_BLOCK_MIN_SIZE;
    }
    else if(remainSize > FOTA_SHA256_BLOCK_MAX_SIZE)    /* 16K+ */
    {
        ajustSize = FOTA_SHA256_BLOCK_MAX_SIZE;
    }
    else
    {
        ajustSize = remainSize;
    }

    do
    {
        allocSize = ajustSize >> loop;
        if(fotaIsSuppDynMem())
        {
            sha256.input = (uint8_t*)fotaAllocMem(allocSize);
        }
        else
        {
            sha256.input = (uint8_t*)FOTA_getSha256DataBuf();
        }

        if(sha256.input) break;
    }while((allocSize > FOTA_SHA256_BLOCK_MIN_SIZE) && (loop++ < 4));

    if(!sha256.input)
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHKSUM_FLASH_2, P_ERROR, "sha256sum: alloc buffer(%d) failure!\n", allocSize);
        return FOTA_EMALLOC;
    }

    sha256.output = hash;

    for(loop = 0; loop < size; loop += allocSize)
    {
        if(remainSize > allocSize)
        {
            remainSize -= allocSize;

            sha256.inLen  = allocSize;
            sha256.isLast = 0;
        }
        else
        {
            sha256.inLen  = remainSize;
            sha256.isLast = isLast ? 1 : 0;
        }

        if(FOTA_EOK != fotaNvmRead(zid, offset + loop, sha256.input, sha256.inLen))
        {
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHKSUM_FLASH_3, P_WARNING, "zid(%d) flash read failure!\n", zid);
            return FOTA_EFLREAD;
        }

        if(buf_handle_cb)
        {
            buf_handle_cb(sha256.input, sha256.inLen);
        }

        if(FOTA_EOK != fotaCalcChksum(FOTA_CA_SHA256SUM, &sha256))
        {
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHKSUM_FLASH_4, P_ERROR, "zid(%d) flash cs-calc fail!\n", zid);
            return FOTA_ECHKSUM;
        }
    }

    if(fotaIsSuppDynMem() && sha256.input)
    {
        fotaFreeMem(sha256.input);
    }

    return FOTA_EOK;
}

/******************************************************************************
 * @brief : FOTA_chksumBufData
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
int32_t FOTA_chksumBufData(uint8_t *buf, uint32_t size, uint8_t *hash, uint32_t isLast, buf_handle_callback buf_handle_cb)
{
    FotaCaSha256Sum_t    sha256 = {0};

    if(!buf || !size || !hash) return FOTA_EARGS;

    if((!isLast) && (size % FOTA_SHA256_HWALIGN_SIZE))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHKSUM_BUF_1, P_WARNING, "err! not aligned by %d bytes @non-last data!\n", FOTA_SHA256_HWALIGN_SIZE);
        return FOTA_ECHKSUM;
    }

    sha256.input  = buf;
    sha256.inLen  = size;
    sha256.output = hash;
    sha256.isLast = isLast;

    if(buf_handle_cb)
    {
        buf_handle_cb(sha256.input, sha256.inLen);
    }

    if(FOTA_EOK != fotaCalcChksum(FOTA_CA_SHA256SUM, &sha256))
    {
        ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CHKSUM_BUF_2, P_ERROR, "buf(0x%x/%d) chksum fail!\n", buf, size);
        return FOTA_ECHKSUM;
    }

    return FOTA_EOK;
}

#define FOTA_DUMP_BUF_SIZE  FOTA_BUF_SIZE_256
void FOTA_dumpOctets(uint8_t *octs, int32_t len)
{
    int8_t  buffer[FOTA_DUMP_BUF_SIZE],
              *ptr = buffer;
    int32_t   loop = 0,
            nBlock = 1,
          maxBlock = (len*2 + FOTA_DUMP_BUF_SIZE - 1)/FOTA_DUMP_BUF_SIZE;

    memset(buffer, '\0', FOTA_DUMP_BUF_SIZE);

    ECPLAT_PRINTF(UNILOG_FOTA, FOTA_DUMP_OCTETS_1, P_INFO, "total length(%d): \n", len);

    for(loop = 0; loop < len; loop ++)
    {
        /* buf end */
        if(!((2 * (loop + 1)) % FOTA_DUMP_BUF_SIZE))
        {
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_DUMP_OCTETS_2, P_INFO, "[%d/%d] %s\n", nBlock++, maxBlock, buffer);

            /* reset buffer */
            memset(buffer, '\0', FOTA_DUMP_BUF_SIZE);

            ptr = buffer;
        }

        ptr += FOTA_SPRINTF((char*)ptr, "%02x", octs[loop]);

        /* pdu end */
        if(loop + 1 >= len)
        {
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_DUMP_OCTETS_3, P_INFO, "[%d/%d] %s\n", nBlock, maxBlock, buffer);
            break;
        }
    }

    return;
}



