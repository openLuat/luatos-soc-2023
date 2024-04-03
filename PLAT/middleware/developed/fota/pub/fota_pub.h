/******************************************************************************

*(C) Copyright 2018 AirM2M International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: fota_pub.h
*
*  Description:
*
*  History: 2021/10/24 created by xuwang
*
*  Notes:
*
******************************************************************************/
#ifndef FOTA_PUB_H
#define FOTA_PUB_H

/*----------------------------------------------------------------------------*
 *                    INCLUDES                                                *
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>



/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/
#if defined CHIP_EC718 || defined CHIP_EC716
#ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
#define FOTA_PRESET_RAM_ENABLE   1
#else
#define FOTA_PRESET_RAM_ENABLE   0
#endif

#else
#define FOTA_PRESET_RAM_ENABLE   0

#endif


#define FOTA_EOK          0
#define FOTA_EUNDEF      -1   /* undefined error */
#define FOTA_EARGS       -2   /* arguments invalid */
#define FOTA_EBATT       -3   /* battery low */
#define FOTA_EPAR        -4   /* parchive invalid */
#define FOTA_EPARSZ      -5   /* parchive size err */
#define FOTA_EFLQUERY    -6   /* flash query fail */
#define FOTA_EFLERASE    -7   /* flash erase fail */
#define FOTA_EFLWRITE    -8   /* flash write fail */
#define FOTA_EFLREAD     -9   /* flash read fail */
#define FOTA_EMINIT      -10  /* memory init fail */
#define FOTA_EMALLOC     -11  /* memory alloc fail */
#define FOTA_EMFREE      -12  /* memory free fail */
#define FOTA_EPVER       -13  /* parver uncompatible */
#define FOTA_EPMETH      -14  /* pmethod unsupported */
#define FOTA_EFWNIDENT   -15  /* fw non-identical */
#define FOTA_EPATCH      -16  /* fw patch fail */
#define FOTA_EUNFOUND    -17  /* item not found */
#define FOTA_EOVERFLOW   -18  /* resource ran out */
#define FOTA_EPERM       -19  /* scenario not permitted */
#define FOTA_ECHKSUM     -20  /* checksum calc fail*/
#define FOTA_EDCONV      -21  /* data convertion fail*/


#define FOTA_TICK_RATE_MS         (1)

#define FOTA_SHA256_HASH_LEN      32

#define FOTA_ALIGN_UP(x,sz)      (((x) + ((sz) - 1)) & (~((sz) - 1)))
#define FOTA_ALIGN_DOWN(x,sz)    ((x) & (~((sz) - 1)))

#if (FOTA_PRESET_RAM_ENABLE == 1)
extern int EC_Printf(const char * pFormat, ...);
extern int EC_Sprintf(char *pBuf, const char * pFormat, ...);

#define FOTA_TRACE(tag,fmt,args...)    EC_Printf(fmt, ##args)
#define FOTA_SPRINTF(buf,args...)      EC_Sprintf(buf, ##args)
#define FOTA_RAND()                    (0)

#else
#define FOTA_TRACE(tag,fmt,args...)    printf(fmt, ##args)
#define FOTA_SPRINTF(buf,args...)      sprintf(buf, ##args)
#define FOTA_RAND()                    rand()

#endif


/*----------------------------------------------------------------------------*
 *                   DATA TYPE DEFINITION                                     *
 *----------------------------------------------------------------------------*/
/*-definition of 'fwAttr:3' */
typedef enum
{
    FOTA_FA_UNDEF = 0,
    FOTA_FA_BL,
    FOTA_FA_AP,
    FOTA_FA_CP,
    FOTA_FA_APP,
    FOTA_FA_APP2,
    FOTA_FA_XOBJ,
    FOTA_FA_SYSH,

    FOTA_FA_MAXNUM
}FotaFwAttr_e;

typedef enum
{
    FOTA_NVM_ZONE_REMAP = 0,  /* remap zone <rsvd for customers>  */
    FOTA_NVM_ZONE_DELTA,      /* delta zone for downloading & patching */
    FOTA_NVM_ZONE_BKUP,
    FOTA_NVM_ZONE_BLU,        /* bl updater <rsvd> */
    FOTA_NVM_ZONE_AP,
    FOTA_NVM_ZONE_CP,
    FOTA_NVM_ZONE_APP,
    FOTA_NVM_ZONE_APP2,
    FOTA_NVM_ZONE_XOBJ,
    FOTA_NVM_ZONE_SYSH,

    FOTA_NVM_ZONE_MAXNUM,
    FOTA_NVM_ZONE_UNDEF = 0xff,
    FOTA_NVM_ZONE_ANY = FOTA_NVM_ZONE_UNDEF
}FotaNvmZoneId_e;

typedef uint32_t FotaNvmZoneId_bm;
#define FOTA_NVM_BM_ZONE_NONE      (0)
#define FOTA_NVM_BM_ZONE_REMAP     (1 << FOTA_NVM_ZONE_REMAP)
#define FOTA_NVM_BM_ZONE_DELTA     (1 << FOTA_NVM_ZONE_DELTA)
#define FOTA_NVM_BM_ZONE_BKUP      (1 << FOTA_NVM_ZONE_BKUP)
#define FOTA_NVM_BM_ZONE_BLU       (1 << FOTA_NVM_ZONE_BLU)
#define FOTA_NVM_BM_ZONE_AP        (1 << FOTA_NVM_ZONE_AP)
#define FOTA_NVM_BM_ZONE_CP        (1 << FOTA_NVM_ZONE_CP)
#define FOTA_NVM_BM_ZONE_APP       (1 << FOTA_NVM_ZONE_APP)
#define FOTA_NVM_BM_ZONE_APP2      (1 << FOTA_NVM_ZONE_APP2)
#define FOTA_NVM_BM_ZONE_XOBJ      (1 << FOTA_NVM_ZONE_XOBJ)
#define FOTA_NVM_BM_ZONE_SYSH      (1 << FOTA_NVM_ZONE_SYSH)


typedef enum
{
    FOTA_CA_SHA256SUM = 0,

    FOTA_CA_ALGO_MAXNUM
}FotaChksumAlgo_e;

/* FOTA_CA_SHA256SUM */
typedef struct
{
    uint8_t  *input;
    uint8_t  *output;
    size_t    inLen;
    uint32_t  isLast;
}FotaCaSha256Sum_t;

/* for fotaNvmDoExtension() */
typedef enum
{
    FOTA_DEF_US_DELAY = 0,
    FOTA_DEF_WDT_KICK,
    FOTA_DEF_WDT_START,
    FOTA_DEF_WDT_STOP,
    FOTA_DEF_GET_RAM_SIZE,
    FOTA_DEF_CHK_BATTERY,
    FOTA_DEF_CHK_DYN_MEM,
    FOTA_DEF_CHK_REMAP_ZONE,
    FOTA_DEF_CHK_DELTA_STATE,
    FOTA_DEF_CHK_BASE_IMAGE,    /* not used any more, call 'fotaNvmVerifyDelta()' instead, just for compatibility here! */
    FOTA_DEF_CHK_TOTAL_PROGRESS,
    FOTA_DEF_IS_IMAGE_IDENTICAL,
    FOTA_DEF_SET_DOWNLOAD_OVER,
    FOTA_DEF_RPT_DFU_PROGRESS,
    FOTA_DEF_GET_DFU_PROGRESS,
    FOTA_DEF_RPT_DFU_RESULT,
    FOTA_DEF_SET_DFU_RESULT,
    FOTA_DEF_GET_DFU_RESULT,   /* not used any more, call 'fotaNvmGetOtaResult()' instead, just for compatibility here! */
    FOTA_DEF_PREPARE_DFU,
    FOTA_DEF_CLOSING_DFU,
    FOTA_DEF_ADJ_ZONE_SIZE,
    FOTA_DEF_SET_XOBJ_ZONE,
    FOTA_DEF_CHK_HLS_STATE,
    FOTA_DEF_CHK_BOOT_STATE
}FotaDoExtensionFlags_e;

/* FOTA_DEF_US_DELAY */
typedef struct
{
    uint32_t usec;
}FotaDefUsDelay_t;

/* FOTA_DEF_GET_RAM_SIZE */
typedef struct
{
    uint32_t avlbSize;
}FotaDefGetRamSize_t;

/* FOTA_DEF_CHK_BATTERY */
typedef struct
{
    uint8_t  isBattLow;   /* 0/1 */
    uint8_t  rsvd[3];
}FotaDefChkBattery_t;

/* FOTA_DEF_CHK_DYN_MEM */
typedef struct
{
    uint8_t  isEnable;   /* 0/1 */
    uint8_t  rsvd[3];
}FotaDefChkDynMem_t;

/* FOTA_DEF_CHK_REMAP_ZONE */
typedef struct
{
    uint8_t  isEnable;   /* 0/1 */
    uint8_t  rsvd[3];
}FotaDefChkRemapZone_t;

/* FOTA_DEF_CHK_DELTA_STATE */
typedef struct
{
    uint8_t  isValid;   /* 0/1 */
    uint8_t  state;     /* FotaDeltaChkState_e */
    uint8_t  rsvd[2];
}FotaDefChkDeltaState_t;

/* FOTA_DEF_CHK_BASE_IMAGE */
typedef struct
{
    uint8_t  isMatched;   /* 0/1 */
    uint8_t  rsvd[3];
}FotaDefChkBaseImage_t;

/* FOTA_DEF_IS_IMAGE_IDENTICAL */
typedef struct
{
    uint32_t  zid;
    uint32_t  size;
    uint8_t  *hash;
}FotaDefIsImageIdentical_t;

typedef enum
{
    FOTA_DPR_FREQ_HIGH = 1,
    FOTA_DPR_FREQ_MEDIUM,
    FOTA_DPR_FREQ_LOW
}FotaDfuProgRptFreq_e;

/* FOTA_DEF_CHK_TOTAL_PROGRESS
 * FOTA_DEF_RPT_DFU_PROGRESS
 * FOTA_DEF_GET_DFU_PROGRESS
 */
typedef struct
{
    uint8_t  pkgNum;
    uint8_t  pkgId;
    uint16_t isTotal :1;
    uint16_t rptFreq :2;   /* FotaDfuProgRptFreq_e */
    uint16_t percent :13;
}FotaDefDfuProgress_t;

typedef enum
{
    FOTA_DCS_DELTA_CHECKOK = 0,  /* no error */
    FOTA_DCS_DELTA_UNFOUND,
    FOTA_DCS_DELTA_PARTIAL,
    FOTA_DCS_DELTA_INVALID,
    FOTA_DCS_DELTA_UNMATCHB,     /* unmatched base image */
    FOTA_DCS_DELTA_UNMATCHN,     /* unmatched new image */
    FOTA_DCS_DELTA_PATCHFAIL,    /* patching failure */
    FOTA_DCS_DELTA_UNDEF
}FotaDeltaChkState_e;

typedef enum
{
    FOTA_DRC_DFU_UNDEF = 0,
    FOTA_DRC_DFU_SUCC  = 1,
    FOTA_DRC_DFU_FAIL  = 99
}FotaDfuResultCode_e;

/* FOTA_DEF_SET_DFU_RESULT
 * FOTA_DEF_GET_DFU_RESULT
 */
typedef struct
{
    uint8_t  dfuResult;    /* FotaDfuResultCode_e */
    uint8_t  deltaState;   /* FotaDeltaChkState_e */
    uint8_t  rsvd[2];
}FotaDefDfuResult_t;

/* FOTA_DEF_PREPARE_DFU */
typedef struct
{
    uint8_t  rsvd[4];
}FotaDefPrepareDfu_t;

/* FOTA_DEF_CLOSING_DFU */
typedef struct
{
    uint8_t  rsvd[4];
}FotaDefClosingDfu_t;

/* FOTA_DEF_ADJ_ZONE_SIZE */
typedef struct
{
    uint32_t  zid;
    uint32_t  size;
}FotaDefAdjZoneSize_t;

/* FOTA_DEF_SET_XOBJ_ZONE */
typedef struct
{
    uint32_t  handle;
    uint32_t  size;
    uint32_t  ovhd;
    uint32_t  extras;
}FotaDefSetXobjZone_t;

/* FOTA_DEF_CHK_HLS_STATE */
typedef struct
{
    uint8_t  isEnable;
    uint8_t  rsvd[3];
}FotaDefChkHlsState_t;

/* FOTA_DEF_CHK_BOOT_STATE */
typedef struct
{
    uint8_t  isSigned; /* 0/1: security boot or not? */
    uint8_t  rsvd[3];
}FotaDefChkBootState_t;

/*
 * definition of 'pmagic': ec-delta/diff-fw
 */
#define FOTA_PARCHIVE_MAGIC_0      0xEC
#define FOTA_PARCHIVE_MAGIC_1      0xDF
#define FOTA_CHECK_PAR_MAGIC(magic)    \
             (((FOTA_PARCHIVE_MAGIC_0 == ((uint8_t*)(magic))[0]) && \
               (FOTA_PARCHIVE_MAGIC_1 == ((uint8_t*)(magic))[1])) ? 1 : 0)

#define FOTA_PAR_RETEN_SIZE_4M     40
#define FOTA_PAR_RETEN_SIZE_16M    136

typedef struct
{
    uint8_t   pmagic[2];   /* 0xEC,0xDF */
    uint8_t   rsvd0;
    uint8_t   rsvdBit0:7;
    uint8_t   psigned :1;  /* 0/1: is system signed bin included or not? */
    uint16_t  pcap    :3;  /* fota capacity(total size of images) in every single patching, 2^pcap(MB) */
    uint16_t  rsvdBit1:13;
    uint16_t  rsvd1;
    uint32_t  parLen;      /* including hdr len */
    uint8_t   parHash[FOTA_SHA256_HASH_LEN];
#if !defined(__CC_ARM)
    uint8_t   retention[0];
#endif
}CustFotaParHdr_t;

typedef struct
{
    uint32_t  rsvd0  :5;
    uint32_t  fwAttr :3;    /* FotaFwAttr_e */
    uint32_t  rsvd1  :24;
    uint32_t  pkgLen;       /* including hdr len */
    uint32_t  rsvd2[3];
    uint32_t  baseFwSize;
    uint8_t   baseFwHash[FOTA_SHA256_HASH_LEN];
}CustFotaPkgHdr_t;


/*----------------------------------------------------------------------------*
 *                    GLOBAL FUNCTIONS DECLEARATION                           *
 *----------------------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif
#endif

