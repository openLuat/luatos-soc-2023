/******************************************************************************

*(C) Copyright 2018 AirM2M International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: fota_utils.h
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/

#ifndef FOTA_UTILS_H
#define FOTA_UTILS_H

#include <stdio.h>
#include <stdint.h>
#include "fota_pub.h"

#define FOTA_BUF_SIZE_32       (32)
#define FOTA_BUF_SIZE_64       (64)
#define FOTA_BUF_SIZE_128      (128)
#define FOTA_BUF_SIZE_256      (256)
#define FOTA_BUF_SIZE_512      (512)
#define FOTA_BUF_SIZE_1K       (1024)
#define FOTA_BUF_SIZE_2K       (FOTA_BUF_SIZE_1K << 1)
#define FOTA_BUF_SIZE_4K       (FOTA_BUF_SIZE_1K << 2)
#define FOTA_BUF_SIZE_8K       (FOTA_BUF_SIZE_1K << 3)
#define FOTA_BUF_SIZE_16K      (FOTA_BUF_SIZE_1K << 4)
#define FOTA_BUF_SIZE_32K      (FOTA_BUF_SIZE_1K << 5)
#define FOTA_BUF_SIZE_64K      (FOTA_BUF_SIZE_1K << 6)
#define FOTA_BUF_SIZE_128K     (FOTA_BUF_SIZE_1K << 7)
#define FOTA_BUF_SIZE_256K     (FOTA_BUF_SIZE_1K << 8)
#define FOTA_BUF_SIZE_512K     (FOTA_BUF_SIZE_1K << 9)

/*
 *|=====================================|===============================================================|
 *|               268.5K                |                           745.5K                              |
 *|-------------------------------------|---------------------------------------------------------------|
 *|               rsvd                  |          prior  memory usage (e.g. sha256)                    |
 *|=====================================|================================================================
 *|    172K     |   4K    |     92K   | |        243K       |    130K     |     200K      |    172K   | |
 *|-------------------------------------|----------------------------------------------------------------
 *| unzip new |G|  pgzh |G|  zip sect |P| lzma decompr zone | zip delta |G| unzip delta |G| unzip old |P|
 *|=====================================|================================================================
 *|    172K     |        96K          | |        243K       |    130K     |     200K      |    172K   | |
 *|-------------------------------------|----------------------------------------------------------------
 *| unzip new |G|   bspat tmp deps    |P| bzip decompr zone | zip delta |G| unzip delta |G| unzip old |P|
 *|-------------------------------------|----------------------------------------------------------------
 *| unzip new |G|        zip new      |P| bzip decompr zone | zip delta |G| unzip delta |G| unzip old |P|
 *|=====================================|================================================================
 *|            ^                       ^|          lzma compress zone for zip sections                  |
 *|            |                       ||===============================================================|
 *|           GAP               PADDINGS|          following  memory usage (e.g. sha256)                |
 *|=====================================|================================================================
 *|    172K     |        96K          | |        243K       |    130K     |     200K      |    172K   | |
 *|-------------------------------------|----------------------------------------------------------------
 *| zip nvram |G| unzip nvram         |P| lzma decompr zone |                                           |
 *|=====================================|================================================================
 *| Abbreviations:
 *| 1) G-Gap
 *| 2) P-Paddings
 *|*****************************************************************************************************
 *| CAUTION:
 *| 1) max compress ratio = 0.57
 *| 2) max P&G size = 1K
 *| 3) zip delta is occupied only when FS @bl is enabled
 *|*****************************************************************************************************
 */

/* Patch-Mux-Buffer */
#define FOTA_PMB_GAP_SIZE                  FOTA_BUF_SIZE_32

#define FOTA_PMB_NEXT_OFFS                 0
#define FOTA_PMB_NEXT_SIZE                (268 * FOTA_BUF_SIZE_1K + FOTA_BUF_SIZE_512)

/* >>> multiplex @next zone */
#define FOTA_PMB_UNZIP_NEXT_OFFS           FOTA_PMB_NEXT_OFFS
#define FOTA_PMB_UNZIP_NEXT_SIZE          (172 * FOTA_BUF_SIZE_1K + FOTA_PMB_GAP_SIZE)

#define FOTA_PMB_ZIP_NEXT_OFFS            (FOTA_PMB_UNZIP_NEXT_OFFS + FOTA_PMB_UNZIP_NEXT_SIZE)
#define FOTA_PMB_ZIP_NEXT_SIZE            (96 * FOTA_BUF_SIZE_1K + FOTA_PMB_GAP_SIZE)
/* <<< multiplex @next zone */

#define FOTA_PMB_SHA256_DATA_OFFS         (FOTA_PMB_NEXT_OFFS + FOTA_PMB_NEXT_SIZE)
#define FOTA_PMB_SHA256_DATA_SIZE          FOTA_BUF_SIZE_64K

#define FOTA_PMB_COMPR_OFFS               (FOTA_PMB_NEXT_OFFS + FOTA_PMB_NEXT_SIZE)
#define FOTA_PMB_COMPR_SIZE               (745 * FOTA_BUF_SIZE_1K + FOTA_BUF_SIZE_512)

#define FOTA_PMB_DECOMPR_OFFS             (FOTA_PMB_NEXT_OFFS + FOTA_PMB_NEXT_SIZE)
#define FOTA_PMB_DECOMPR_SIZE             (243 * FOTA_BUF_SIZE_1K)

/* >>> multiplex @decompress zone */
#define FOTA_PMB_DECOMPR_BZIP_FBUFF_OFFS   FOTA_PMB_DECOMPR_OFFS
#define FOTA_PMB_DECOMPR_BZIP_FBUFF_SIZE  (5 * FOTA_BUF_SIZE_1K + FOTA_PMB_GAP_SIZE)

#define FOTA_PMB_DECOMPR_BZIP_DSTATE_OFFS (FOTA_PMB_DECOMPR_BZIP_FBUFF_OFFS + FOTA_PMB_DECOMPR_BZIP_FBUFF_SIZE)
#define FOTA_PMB_DECOMPR_BZIP_DSTATE_SIZE (64 * FOTA_BUF_SIZE_1K + FOTA_PMB_GAP_SIZE)

#define FOTA_PMB_DECOMPR_BZIP_LL16_OFFS   (FOTA_PMB_DECOMPR_BZIP_DSTATE_OFFS + FOTA_PMB_DECOMPR_BZIP_DSTATE_SIZE)
#define FOTA_PMB_DECOMPR_BZIP_LL16_SIZE   (137 * FOTA_BUF_SIZE_1K + FOTA_PMB_GAP_SIZE)

#define FOTA_PMB_DECOMPR_BZIP_LL4_OFFS    (FOTA_PMB_DECOMPR_BZIP_LL16_OFFS + FOTA_PMB_DECOMPR_BZIP_LL16_SIZE)
#define FOTA_PMB_DECOMPR_BZIP_LL4_SIZE    (36 * FOTA_BUF_SIZE_1K + FOTA_PMB_GAP_SIZE)
/* <<< multiplex @decompress zone */

#define FOTA_PMB_ZIP_DELTA_OFFS           (FOTA_PMB_DECOMPR_OFFS + FOTA_PMB_DECOMPR_SIZE)
#define FOTA_PMB_ZIP_DELTA_SIZE           (130 * FOTA_BUF_SIZE_1K + FOTA_PMB_GAP_SIZE)

#define FOTA_PMB_UNZIP_DELTA_OFFS         (FOTA_PMB_ZIP_DELTA_OFFS + FOTA_PMB_ZIP_DELTA_SIZE)
#define FOTA_PMB_UNZIP_DELTA_SIZE         (200 * FOTA_BUF_SIZE_1K + FOTA_PMB_GAP_SIZE)

#define FOTA_PMB_UNZIP_BASE_OFFS          (FOTA_PMB_UNZIP_DELTA_OFFS + FOTA_PMB_UNZIP_DELTA_SIZE)
#define FOTA_PMB_UNZIP_BASE_SIZE          (172 * FOTA_BUF_SIZE_1K + FOTA_PMB_GAP_SIZE)

/* >>> multiplex @zip next zone */
#define FOTA_PMB_DELTA_PZGH_OFFS           FOTA_PMB_ZIP_NEXT_OFFS
#define FOTA_PMB_DELTA_PZGH_SIZE          (4 * FOTA_BUF_SIZE_1K + FOTA_PMB_GAP_SIZE)

#define FOTA_PMB_ZIP_SECT_OFFS            (FOTA_PMB_DELTA_PZGH_OFFS + FOTA_PMB_DELTA_PZGH_SIZE)
#define FOTA_PMB_ZIP_SECT_SIZE            (92 * FOTA_BUF_SIZE_1K + FOTA_PMB_GAP_SIZE)

#define FOTA_PMB_BSPAT_DEPS_OFFS           FOTA_PMB_ZIP_NEXT_OFFS
#define FOTA_PMB_BSPAT_DEPS_SIZE          (96 * FOTA_BUF_SIZE_1K + FOTA_PMB_GAP_SIZE)
/* <<< multiplex @zip next zone */

/* >>> multiplex @next zone */
#define FOTA_PMB_ZIP_NVRAM_OFFS            FOTA_PMB_ZIP_NEXT_OFFS
#define FOTA_PMB_ZIP_NVRAM_SIZE            FOTA_PMB_ZIP_NEXT_SIZE

#define FOTA_PMB_UNZIP_NVRAM_OFFS          FOTA_PMB_UNZIP_NEXT_OFFS
#define FOTA_PMB_UNZIP_NVRAM_SIZE          FOTA_PMB_UNZIP_NEXT_SIZE
/* <<< multiplex @next zone */


#define FOTA_SHA256_HWALIGN_SIZE   64

#define FOTA_MAX(x, y)                (((x) > (y)) ? (x) : (y))
#define FOTA_MIN(x, y)                (((x) < (y)) ? (x) : (y))
#define FOTA_ABS(v)                   (((v) < 0) ? -(v) : (v))
#define FOTA_BITMASK(nbits,lsh)       (((1 << (nbits)) - 1) << lsh)
#define FOTA_OFFSETOF(type, field)    ((uint32_t)&(((type *)0)->field))

#define FOTA_BUF_DESC_INIT(des, buf)  FOTA_BUF_DESC_SET(des, buf, 0, 0)

#define FOTA_BUF_DESC_SET(des, buf, len, offs)  \
        do\
        {\
            ((FotaBufferDesc_t*)(des))->buffer = (buf);  \
            ((FotaBufferDesc_t*)(des))->length = (len);  \
            ((FotaBufferDesc_t*)(des))->offset = (offs); \
        }while(0)

#define FOTA_BUF_DESC_APPEND(des, len)  \
        do\
        {\
            ((FotaBufferDesc_t*)(des))->length += (len); \
            ((FotaBufferDesc_t*)(des))->offset += (len); \
        }while(0)

#define FOTA_BUF_DESC_MEMCPY_APPEND(des, buf, len)  \
        do\
        {\
            memcpy(((FotaBufferDesc_t*)(des))->buffer + ((FotaBufferDesc_t*)(des))->offset, (buf), (len));  \
            ((FotaBufferDesc_t*)(des))->length += (len); \
            ((FotaBufferDesc_t*)(des))->offset += (len); \
        }while(0)


typedef struct
{
    uint8_t *buffer;
    size_t   length;
    size_t   offset;
}FotaBufferDesc_t;

typedef int32_t (*buf_handle_callback)(uint8_t *buf, int32_t buflen);


int32_t  FOTA_getUnzipNextBuf(FotaBufferDesc_t *desc);
int32_t  FOTA_getZipNextBuf(FotaBufferDesc_t *desc);
int32_t  FOTA_getZipDeltaBuf(FotaBufferDesc_t *desc);
int32_t  FOTA_getUnzipDeltaBuf(FotaBufferDesc_t *desc);
int32_t  FOTA_getUnzipBaseBuf(FotaBufferDesc_t *desc);
uint8_t* FOTA_getDeltaPzghBuf(void);
int32_t  FOTA_getZipSectBuf(FotaBufferDesc_t *desc);
int32_t  FOTA_getBspatDepsBuf(FotaBufferDesc_t *desc);

uint32_t FOTA_convToZoneId(uint32_t fwAttr);
uint32_t FOTA_convToFwAttr(uint32_t zoneId);
int32_t  FOTA_chksumFlashData(uint32_t zid, uint32_t offset, uint32_t size, uint8_t *hash, uint32_t isLast, buf_handle_callback buf_handle_cb);
int32_t  FOTA_chksumBufData(uint8_t *buf, uint32_t size, uint8_t *hash, uint32_t isLast, buf_handle_callback buf_handle_cb);
void     FOTA_dumpOctets(uint8_t *octs, int32_t len);

#endif
