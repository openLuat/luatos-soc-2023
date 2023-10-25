/******************************************************************************

*(C) Copyright 2018 AirM2M International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename:
*
*  Description:
*
*  History: initiated by xxxx
*
*  Notes:
*
******************************************************************************/

/*----------------------------------------------------------------------------*
 *                    INCLUDES                                                *
 *----------------------------------------------------------------------------*/


#include <stdint.h>
#include "string.h"
#ifdef CORE_IS_AP
#include "hal_misc.h"
#include "flash_rt.h"
#if (defined CHIP_EC618)
#include "cpxip.h"
#elif (defined TYPE_EC718H)
#include "cp_flash.h"
#include "pcache.h"
#endif
#include "cache.h"

#ifndef NVRAM_READONLY
#ifdef FEATURE_AT_ENABLE
#include "atec_rf.h"
#endif
#endif

#endif
#include "nvram_flash.h"
#include "nvram.h"


#include "mem_map.h"
#include "cmsis_os2.h"
#include "stdio.h"
#include "exception_process.h"
#ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
#include "common.h"
#else
#include DEBUG_LOG_HEADER_FILE
#endif

/*
A= factory/backup region
B= normal used region

cali stage: fill B then sync to A

error recovry:
case1: A = OK B=OK, always use B
case2: A = OK B=NOK, recovry B from A when read/boot(nvram_after_init)
case3: A = NOK B = OK, same as case1
case4: A = NOK B = NOK, DO NOT format B, keep A as broken one, no method to recovry.
       could boot up, header is fine, but part of  content is invalid
       RF read invalid data, RF should handle it( e.g. use default val)

*/


/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/

#define FLASH_BASE_ADDRESS              AP_FLASH_XIP_ADDR
#define NVRAM_FACTORY_BASE             (AP_FLASH_XIP_ADDR+NVRAM_FACTORY_PHYSICAL_BASE)
#define NVRAM_AP_SECTOR_FACTORY_BASE    NVRAM_FACTORY_PHYSICAL_BASE
#define NVRAM_CP_SECTOR_FACTORY_BASE    CP_NVRAM_FACTORY_PHYSICAL_BASE
#define NVRAM_AP_SECTOR_BASE            NVRAM_PHYSICAL_BASE
#define NVRAM_CP_SECTOR_BASE            CP_NVRAM_PHYSICAL_BASE

#define NVM_ASSERT(a) EC_ASSERT(a,0,0,0);

extern uint32_t ShareInfoAPSetPhyCaliMem(void);
#define  RF_NST_A2C_BUFF_ADDR                     ShareInfoAPSetPhyCaliMem()  // 0xC800 : 50K
#define  RF_NST_C2A_BUFF_ADDR                     (RF_NST_A2C_BUFF_ADDR+0xC800)
#define  RF_NST_TEMP_BUFF_ADDR1                   (RF_NST_C2A_BUFF_ADDR+0x3C00)
#define  RF_NST_TEMP_BUFF_ADDR2                   (RF_NST_TEMP_BUFF_ADDR1+0x7800)
#define  RF_NST_TABLE_BUFF_ADDR                   (RF_NST_TEMP_BUFF_ADDR2+0x3C00)

/**/
#define NVM_PROBE(a)
//do{;}while(a)
#define NVM_TRAP(a)
//do{;}while(a)

extern uint8_t apmuGetImageType(void);
#define NVM_Print(fmt,...)  /* do{   \
    if(apmuGetImageType() == 1)    \
        printf(fmt,##__VA_ARGS__); \
    }while(0)                  //do not print in paging image
*/      //do not printf, may cause memory leak

// #define NVM_Print(fmt,...) printf(fmt,##__VA_ARGS__)
#define NV_FLAG 0x45494736
#define POLY32 0x4C11DB7


/*----------------------------------------------------------------------------*
 *                    DATA TYPE DEFINITION                                    *
 *----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*
 *                      GLOBAL VARIABLES                                      *
 *----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*
 *                      PRIVATE FUNCTION DECLEARATION                         *
 *----------------------------------------------------------------------------*/
uint32_t nvramGetnvAddr(NvType_t nvt);
uint32_t nvramGetnvLength(NvType_t nvt);

/*----------------------------------------------------------------------------*
 *                      PRIVATE FUNCTIONS                                     *
 *----------------------------------------------------------------------------*/


#ifdef CORE_IS_AP


static uint32_t nvramCrc32(void* input, int len)
{
    return halCrc32(input,len);
}


static uint16_t nvramChksum(void *dataptr, uint16_t len)
{
  uint32_t acc;
  uint16_t src;
  uint8_t *octetptr;

  acc = 0;
  octetptr = (uint8_t*)dataptr;
  while (len > 1) {
    src = (*octetptr) << 8;
    octetptr++;
    src |= (*octetptr);
    octetptr++;
    acc += src;
    len -= 2;
  }
  if (len > 0) {
    src = (*octetptr) << 8;
    acc += src;
  }

  acc = (acc >> 16) + (acc & 0x0000ffffUL);
  if ((acc & 0xffff0000UL) != 0) {
    acc = (acc >> 16) + (acc & 0x0000ffffUL);
  }

  src = (uint16_t)acc;
  return ~src;
}


/*static*/ NvErr_e formatNv(NvType_t nvt)
{
    NvErr_e ret=NVRAM_OK;
    NvHeader_t nvHeader;
    NvHeader_t *pNvHeader=NULL;
    uint32_t nv_addr=0;
    uint16_t header_chk=0;

    // format all data

    nv_addr = nvramGetnvAddr(nvt);

    // init nvram header
    pNvHeader = &nvHeader;
    memset(pNvHeader,0xFF,sizeof(NvHeader_t));

    nvHeader.nv_flag=NV_FLAG;
    nvHeader.used_size=0;
    nvHeader.time_counter=0;
    nvHeader.size_dummy=0xFFFF;
    nvHeader.dummy1=0xFFFFFFFF;
    nvHeader.dummy2=0xFFFFFFFF;
    nvHeader.dummy3=0xFFFFFFFF;
    nvHeader.header_chk_dummy=0xFFFF;
    nvHeader.chksum=0;
    nvHeader.header_chk=0;

    header_chk = nvramChksum(pNvHeader,sizeof(NvHeader_t)-4);
    ((NvHeader_t *)pNvHeader)->header_chk = header_chk;

    if(nvt>=CPNV1)
    {
        ret=(NvErr_e)cp_nvram_flash_earse(nv_addr&0x007FFFFF);
        if(ret != 0)
        {
            ret=NVRAM_FORMAT_ERR;
            return ret;
        }
        // program nvram header
        ret=(NvErr_e)cp_nvram_flash_write((uint8_t *)pNvHeader,(nv_addr&0x007FFFFF),sizeof(NvHeader_t));
        if(ret != 0)
        {
            ret=NVRAM_FORMAT_ERR;
            NVM_ASSERT(0);
        }
    }else
    {
        ret=(NvErr_e)nvram_flash_earse(nv_addr&0x007FFFFF);
        if(ret != 0)
        {
            ret=NVRAM_FORMAT_ERR;
            return ret;
        }
        // program nvram header
        ret=(NvErr_e)nvram_flash_write((uint8_t *)pNvHeader,(nv_addr&0x007FFFFF),sizeof(NvHeader_t));
        if(ret != 0)
        {
            ret=NVRAM_FORMAT_ERR;
            NVM_ASSERT(0);
        }
    }
    return ret;
}




static uint32_t checkNvValid(NvBank_e nv_bank,NvType_t nvt)
//uint32_t checkNvValid(NvType_t nvt)
{
    uint32_t ret=0;
    uint32_t nv_addr=0,nv_flag=0,time_count=0;
    uint16_t header_chk=0,org_header_chk=0,used_size=0;
    uint32_t chksum=0;

    NVM_Print("checkNvValid.nvt: %d\n",nvt);

    //parameter check
    if((int)nvt<0 || (int)nvt>=NV_MAX)
        NVM_ASSERT(0);

    if(nv_bank == NV_FAC)
        nv_addr=nvramGetnvFacAddr(nvt);
    else
        nv_addr=nvramGetnvAddr(nvt);


    nv_flag=((NvHeader_t *)(nv_addr))->nv_flag;
    NVM_Print("nv_flag %08X.nvt: %d\n",nv_flag,nvt);
    if(nv_flag == NV_FLAG)
    {
        NVM_Print("[NVM_INFO]checkNvValid:NV_FLAG pass \n");
    }

    org_header_chk = ((NvHeader_t *)(nv_addr))->header_chk;
    header_chk= nvramChksum((void *)(nv_addr),sizeof(NvHeader_t)-4);
    NVM_Print("org_header_chk org: %04X ,calc: %04X\n",org_header_chk,header_chk);
    if(org_header_chk!=header_chk)
    {
        NVM_Print("[NVM_ERROR]checkNvValid:nv_flag header check fail\n");
        return 0;
    }

    time_count=((NvHeader_t *)(nv_addr))->time_counter;
    NVM_Print("time_count %08X.nvt: %d\n",time_count,nvt);
    if(time_count == 0xFFFFFFFF)
    {
        NVM_Print("[NVM_ERROR]checkNvValid:time_count fail \n");
        return 0;
    }

    // get nv size
    used_size = ((NvHeader_t *)(nv_addr))->used_size;
    NVM_Print("used_size %d.nvt: %d\n",used_size,nvt);
    if(used_size == 0)  //format
        return 1;

    NVM_Print("pdata %08X.nvt: %d\n",nv_addr,nvt);

    NVM_Print("(pdata+sizeof(NvHeader_t)) %08X.nvt: %d\n",(nv_addr+sizeof(NvHeader_t)),nvt);
    chksum = nvramCrc32((void *)(nv_addr+sizeof(NvHeader_t)),used_size);

    NVM_Print("chksum org: %08X ,calc: %08X\n",(((NvHeader_t *)(nv_addr))->chksum),chksum);
    if(((NvHeader_t *)(nv_addr))->chksum == chksum)
    {
        ret = 1;
    }
    else
    {
        NVM_Print("[NVM_ERROR]checkNvValid:chksum fail \n");
        ret = 0;
    }

    NVM_Print("checkNvValid over \n");


    return ret;
}

static NvErr_e restoreFactory(NvType_t nvt)
{
    uint8_t *pdata=NULL;
    uint32_t nv_fac_addr=0,nv_addr=0;
    NvErr_e ret=NVRAM_OK;
    uint32_t valid=0;
    int sec_counter=0;
    int sec_num=0;

    NVM_Print("restoreFactory \n");

    #ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
    extern uint8_t* FOTA_getUnzipNvRamBuf(void);
    pdata = FOTA_getUnzipNvRamBuf();
    #else
    pdata = (uint8_t *)malloc(NVRAM_SECTOR_SIZE);
    if( pdata == NULL )
        NVM_ASSERT(0);
    #endif

    NVM_Print("clear all broken data in NVRAM area\n");

    nv_fac_addr = nvramGetnvFacAddr(nvt);
    nv_addr = nvramGetnvAddr(nvt);

    //check Fac area is correct or not
    valid = checkNvValid(NV_FAC,nvt);
    if(valid != 1)
    {
        //if bakeup broken
        #if 0// if factory is broken, do not format, since RF still could use remained data
        formatNv(nvt);
        #endif

        ret = NVRAM_FAC_BROKEN;
        goto RESTORE_FAC_END;
    }

    sec_num = nvramGetnvLength(nvt);
    NVM_Print("sec_num %08X.nvt: %d\n",sec_num,nvt);
    for(sec_counter=0;sec_counter<sec_num;sec_counter++)
    {
        if(nvt>=CPNV1)
            ret=(NvErr_e)cp_nvram_flash_earse((nv_addr+sec_counter*NVRAM_SECTOR_SIZE)&0x007FFFFF);
        else
            ret=(NvErr_e)nvram_flash_earse((nv_addr+sec_counter*NVRAM_SECTOR_SIZE)&0x007FFFFF);

        memcpy(pdata,(void *)(nv_fac_addr+sec_counter*NVRAM_SECTOR_SIZE),NVRAM_SECTOR_SIZE);

        if(nvt>=CPNV1)
            ret=(NvErr_e)cp_nvram_flash_write(pdata,((nv_addr+sec_counter*NVRAM_SECTOR_SIZE)&0x007FFFFF),NVRAM_SECTOR_SIZE);
        else
            ret=(NvErr_e)nvram_flash_write(pdata,((nv_addr+sec_counter*NVRAM_SECTOR_SIZE)&0x007FFFFF),NVRAM_SECTOR_SIZE);

        NVM_Print("write nv_addr:0x%08X.\n",nv_addr);
        if(ret != 0)
        {
            ret=NVRAM_WRITE_FLASH_ERR;
            goto RESTORE_FAC_END;
        }
    }

RESTORE_FAC_END:
    NVM_Print("write data to NVRAM area over.\n");

#ifndef FEATURE_BOOTLOADER_PROJECT_ENABLE
    free(pdata);
#endif

    return ret;
}






#ifdef REL_COMPRESS_EN
static uint32_t checkNvValidCprs(uint32_t dcprsBuf)
//uint32_t checkNvValid(NvType_t nvt)
{
    uint32_t ret=0;
    uint32_t time_count=0;
    uint16_t header_chk=0,org_header_chk=0,used_size=0;
    uint32_t chksum=0;
    uint32_t nv_addr = 0;

    nv_addr = dcprsBuf;

    org_header_chk = ((NvHeader_t *)(nv_addr))->header_chk;
    header_chk= nvramChksum((void *)(nv_addr),sizeof(NvHeader_t)-4);
    if(org_header_chk!=header_chk)
    {
        return 0;
    }

    time_count=((NvHeader_t *)(nv_addr))->time_counter;
    if(time_count == 0xFFFFFFFF)
    {
        return 0;
    }

    // get nv size
    used_size = ((NvHeader_t *)(nv_addr))->used_size;
    if(used_size == 0)  //format
        return 1;

    chksum = nvramCrc32((void *)(nv_addr+sizeof(NvHeader_t)),used_size);

    if(((NvHeader_t *)(nv_addr))->chksum == chksum)
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    return ret;
}


NvErr_e restoreFactoryCprs( uint32_t bitmap )
{
    uint8_t *pdata=NULL;
    uint32_t nv_addr=0,nv_fac_addr=0;
    volatile NvErr_e ret=NVRAM_OK;
    uint32_t valid=0;
    unsigned int dCprsDataLen = 0,cprsDataSize = 0;
    unsigned int decompDataLenTemp = 0;
    int sec_counter=0, i =0;
    int sec_num=0, sec_offset=0;
    uint8_t *  data = NULL;
    uint32_t decprsBuf = 0;
    NvRfCompHeader_t rfCompHeader;


#ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
    extern uint8_t* FOTA_getUnzipNvRamBuf(void);
    decprsBuf = (uint32_t)FOTA_getUnzipNvRamBuf();
#else
    decprsBuf = up_buf_start + 0x20000;//hardcode here, yunran need to check and fix
#endif


    nv_fac_addr = nvramGetnvFacAddr(CPNV1);

    nvram_flash_read((uint8_t*)&rfCompHeader, (nv_fac_addr&0x007FFFFF), sizeof(NvRfCompHeader_t));  //read compress data header

    if(rfCompHeader.magic != NVRAM_COMPRESS_HEADER_MAGIC)
    {
        return NVRAM_FORMAT_ERR;
    }

    cprsDataSize = rfCompHeader.compressedSize;
#ifdef FEATURE_BOOTLOADER_PROJECT_ENABLE
    extern uint8_t* FOTA_getZipNvRamBuf(void);
    data = FOTA_getZipNvRamBuf();
#else
    data = malloc(cprsDataSize+1);
#endif
    memset(data, 0, (cprsDataSize+1));
    nvram_flash_read(data, (nv_fac_addr&0x007FFFFF)+sizeof(NvRfCompHeader_t), cprsDataSize);  //read compress data

    //step1 decompress the CP NV, decompress size should be 48KB
    extern void decompressRamCodeGetAddrInfo(void);
    decompressRamCodeGetAddrInfo();


    extern int lzmaDecodeEc(unsigned char *outStream, unsigned int *outStreamLen, unsigned char *inStream, unsigned int inStreamLen, unsigned int dataType, unsigned int *lzmaUnpackLen);
    lzmaDecodeEc((unsigned char *)decprsBuf, &dCprsDataLen, (unsigned char *)data, cprsDataSize, 0, &decompDataLenTemp); //decompress all data to deCompressBuf
    ECPLAT_PRINTF(UNILOG_NVRAM, restoreFactoryCprs_0, P_INFO, "outStream=0x%x,outStreamLen=%d..inStream=0x%x,inStreamLen=%d..",decprsBuf, dCprsDataLen, data, cprsDataSize);

#ifndef FEATURE_BOOTLOADER_PROJECT_ENABLE
    free(data);
#endif
    if(dCprsDataLen != NVRAM_DECOMPRESS_FAC_MAX_SIZE)
    {
        ECPLAT_PRINTF(UNILOG_NVRAM, restoreFactoryCprs_1, P_ERROR, "restoreFactoryCprs len error!! len= %d",dCprsDataLen);
        return NVRAM_FAC_BROKEN;
    }

    //step 2 check if decompress fac region is valid, then recovery except fac nv is not valid
    for(i=CPNV1; i<NV_MAX; i++ )
    {
        if((bitmap&(0x1<<i)) !=0)// nv i is broken
        {
            //check Fac area is correct or not
            valid = checkNvValidCprs( decprsBuf + sec_offset*NVRAM_SECTOR_SIZE);
            pdata = (uint8_t *)(decprsBuf + sec_offset*NVRAM_SECTOR_SIZE);

            sec_num = nvramGetnvLength((NvType_t)i);
            sec_offset+=sec_num;

            if(valid != 1)
            {
                // if factory is broken, do not format, since RF still could use remained data
                ECPLAT_PRINTF(UNILOG_NVRAM, restoreFactoryCprs_2, P_ERROR, "fac nv %d is invalid, bitmap 0x%x!!",i,bitmap);
                ret = NVRAM_FAC_BROKEN;
                continue;//try next nv
            }

            nv_addr = nvramGetnvAddr((NvType_t)i);
            for(sec_counter=0;sec_counter<sec_num;sec_counter++)
            {
                ret=(NvErr_e)cp_nvram_flash_earse((nv_addr+sec_counter*NVRAM_SECTOR_SIZE)&0x007FFFFF);
                ret=(NvErr_e)cp_nvram_flash_write((pdata+sec_counter*NVRAM_SECTOR_SIZE),((nv_addr+sec_counter*NVRAM_SECTOR_SIZE)&0x007FFFFF),NVRAM_SECTOR_SIZE);
                if(ret != 0)
                {
                    ret=NVRAM_WRITE_FLASH_ERR;
                    return ret;
                }
            }
        }
        else
        {   //no need to recover, but still need to calc offset
            sec_num = nvramGetnvLength((NvType_t)i);
            sec_offset+=sec_num;
        }
    }

    return ret;
}

#endif


static uint32_t cpNvramWrite(NvType_t nvt,uint8_t * data,uint32_t size)
{
    uint32_t ret=0;
    NvHeader_t nvHeader;
    uint8_t *pdata=NULL;
    //uint8_t *prdata=NULL;
    uint16_t header_chk=0;
    uint32_t chksum=0;
    uint32_t nv_addr=0;
    int sec_counter=0;
    int sec_num=0;
    NVM_Print("nvramWrite num: %d\n",nvt);

    pdata = data;

    nv_addr = nvramGetnvAddr(nvt);

    nvHeader.nv_flag=NV_FLAG;
    nvHeader.used_size=size;
    nvHeader.time_counter=0;
    nvHeader.size_dummy=0xFFFF;
    nvHeader.dummy1=0xFFFFFFFF;
    nvHeader.dummy2=0xFFFFFFFF;
    nvHeader.dummy3=0xFFFFFFFF;
    nvHeader.header_chk_dummy=0xFFFF;
    nvHeader.chksum=0;
    nvHeader.header_chk=0;

    memcpy(pdata,&nvHeader,sizeof(NvHeader_t));

    chksum = nvramCrc32((pdata+sizeof(NvHeader_t)),size);
    ((NvHeader_t *)pdata)->chksum = chksum;

    header_chk = nvramChksum(pdata,sizeof(NvHeader_t)-4);
    ((NvHeader_t *)pdata)->header_chk = header_chk;

    // erase
    sec_num = nvramGetnvLength(nvt);
    NVM_Print("sec_num %08X.nvt: %d\n",sec_num,nvt);
    for(sec_counter=0;sec_counter<sec_num;sec_counter++)
    {
        ret=cp_nvram_flash_earse((nv_addr+sec_counter*NVRAM_SECTOR_SIZE)&0x007FFFFF);
    }

    header_chk = nvramChksum(pdata,sizeof(NvHeader_t)-4);
    ((NvHeader_t *)pdata)->header_chk = header_chk;

    // program
    ret=cp_nvram_flash_write(pdata,(nv_addr&0x007FFFFF),size+sizeof(NvHeader_t));

    if(ret != 0)
    {
        return ret;
    }
    //before read from cp flash, need to flush the pcache
    #if (defined TYPE_EC718H)
    PCacheFlushAll();
    #else//for other case AP/CP share flash, should flush fcache:CP may read NV before, dirty val in fcache
    DisableFCache();
    EnableFCache();
    #endif

    ret = memcmp(pdata,(void *)nv_addr,size+sizeof(NvHeader_t));
    if(ret != 0)
    {
        return ret;
    }

    if(ret == 0)
        NVM_Print("nv write read back is OK !\n");
    else
        NVM_Print("nv write read back is ERR !\n");

    NVM_Print("nvramWrite over\n");

    return ret;

}





static uint32_t cpNvramRead(NvType_t nvt,uint8_t * data,uint32_t size,uint32_t offset)
{
    uint32_t ret=0;
    NvHeader_t nvHeader;
    uint32_t valid=0;
    uint32_t nv_addr=0;


    //parameter check
    if((int)nvt<0 || (int)nvt>=NV_MAX)
        NVM_ASSERT(0);

    NVM_Print("cp_nvram_read num: %d\n",nvt);

    nv_addr = nvramGetnvAddr(nvt);


    memcpy(&nvHeader,(void *)nv_addr,sizeof(NvHeader_t));
    // need to format
    if(nvHeader.time_counter==0xFFFFFFFF)
    {
        //NVM_ASSERT(0);
        NVM_Print("nvHeader: 0xFFFFFFFF\n");
        NVM_PROBE(1);
    }

    valid = checkNvValid(NV,nvt);
    if(valid == 0)  //broken
    {

        #if (defined TYPE_EC716S) || (defined TYPE_EC718S) || (defined TYPE_EC718P)
        //cp nv factory is compressed, trigger assert, in production, slient reset to let init stage to recover
        NVM_ASSERT(0);
        #else
        NVM_Print("restoreFactory\n");
        ret = restoreFactory(nvt);
        if(ret != NVRAM_OK)
            return NVRAM_FAC_RESTORE_ERR;
        #endif
    }else{  //ok
        NVM_Print("checkNvValid OK\n");
    }

    // XIP mode access flash directly

    memcpy(data,(void *)((nv_addr+sizeof(NvHeader_t))+offset),size);
    ret = ((NvHeader_t *)(nv_addr))->used_size;
    NVM_Print("(nv_addr))->used_size %d\n",ret);
    return ret;
}




/*----------------------------------------------------------------------------*
 *                      GLOBAL FUNCTIONS                                      *
 *----------------------------------------------------------------------------*/


#if (defined TYPE_EC716S) || (defined TYPE_EC718S) || (defined TYPE_EC718P)
uint32_t apNvramWrite(ApNvPart_t partType,uint8_t * data,uint32_t size)
{
    uint32_t ret=NVRAM_OK;
    uint32_t part_offset = 0;
    uint8_t *pdata=NULL;
    uint32_t nv_addr=0;
    int i=0;
    NvHeader_t nvHeader;
    uint16_t header_chk=0;
    uint32_t chksum=0;

    if(partType == APNV_IMEISN_PART)
    {
        if(size > AP_NV_IMEISN_MAX_SZIE)
            return NVRAM_PART_LEN_ERR;

        part_offset = AP_NV_IMEISN_OFFSET;
    }
    else if(partType == APNV_NPI_PART)
    {
        if(size > AP_NV_NPI_MAX_SZIE)
            return NVRAM_PART_LEN_ERR;

        part_offset = AP_NV_NPI_OFFSET;
    }
    else if(partType == APNV_DCXO_PART)
    {
        if(size > AP_NV_DCXO_MAX_SZIE)
            return NVRAM_PART_LEN_ERR;

        part_offset = AP_NV_DXCO_OFFSET;
    }
#ifdef __USER_CODE__
    else if(partType == APNV_RESV_PART)
    {
        if(size > AP_NV_RESV_MAX_SZIE)
            return NVRAM_PART_LEN_ERR;

        part_offset = AP_NV_RESV_OFFSET;
    }
#endif
    //step1 read back all 4K from AP NV1
    nv_addr = nvramGetnvAddr(APNV1);

    pdata = (uint8_t *)malloc(NVRAM_SECTOR_SIZE);
    if( pdata == NULL )
        NVM_ASSERT(0);

    memcpy(pdata,(void *)nv_addr,NVRAM_SECTOR_SIZE);//read back total 4K, include hdr

    nvHeader.nv_flag=NV_FLAG;
    nvHeader.used_size=NVRAM_SECTOR_SIZE - AP_NV_RESV_MAX_SZIE;//always check all except resv area
    nvHeader.time_counter=0;
    nvHeader.size_dummy=0xFFFF;
    nvHeader.dummy1=0xFFFFFFFF;
    nvHeader.dummy2=0xFFFFFFFF;
    nvHeader.dummy3=0xFFFFFFFF;
    nvHeader.header_chk_dummy=0xFFFF;
    nvHeader.chksum=0;
    nvHeader.header_chk=0;

    //step 2 modify the dedicate part in ram buffer
    memcpy(pdata,&nvHeader,sizeof(NvHeader_t));
    for(i=0;i<size;i++)
    {
        pdata[sizeof(NvHeader_t)+part_offset+i]=*(uint8_t *)(data+i);
    }

    chksum = nvramCrc32((pdata+sizeof(NvHeader_t)),NVRAM_SECTOR_SIZE - AP_NV_RESV_MAX_SZIE);//always check all except resv area
    ((NvHeader_t *)pdata)->chksum = chksum;

    header_chk = nvramChksum(pdata,sizeof(NvHeader_t)-4);
    ((NvHeader_t *)pdata)->header_chk = header_chk;


    //step3: write back the ram buffer to APNV1
    // erase
    ret=nvram_flash_earse(nv_addr&0x007FFFFF);

    if(ret != 0)
    {
        free(pdata);
        return ret;
    }

    // program
#ifdef __USER_CODE__
    ret=nvram_flash_write(pdata,(nv_addr&0x007FFFFF),NVRAM_SECTOR_SIZE);
#else
    ret=nvram_flash_write(pdata,(nv_addr&0x007FFFFF),NVRAM_SECTOR_SIZE-AP_NV_RESV_MAX_SZIE);//write back all except resv area
#endif
    if(ret != 0)
    {
        free(pdata);
        return ret;
    }

    ret = memcmp(pdata,(void *)nv_addr,size+sizeof(NvHeader_t));

    free(pdata);

    

    return ret;
}



uint32_t apNvramRead(ApNvPart_t partType,uint8_t * data,uint32_t size,uint32_t offset)
{
    uint32_t ret=0;
    NvHeader_t nvHeader;
    uint32_t valid=0;
    uint32_t nv_addr=0;
    uint32_t part_offset = 0;


    if(partType == APNV_IMEISN_PART)
    {
        if(size > AP_NV_IMEISN_MAX_SZIE)
            return NVRAM_PART_LEN_ERR;

        part_offset = AP_NV_IMEISN_OFFSET;
    }
    else if(partType == APNV_NPI_PART)
    {
        if(size > AP_NV_NPI_MAX_SZIE)
            return NVRAM_PART_LEN_ERR;

        part_offset = AP_NV_NPI_OFFSET;
    }
    else if(partType == APNV_DCXO_PART)
    {
        if(size > AP_NV_DCXO_MAX_SZIE)
            return NVRAM_PART_LEN_ERR;

        part_offset = AP_NV_DXCO_OFFSET;
    }
#ifdef __USER_CODE__
    else if(partType == APNV_RESV_PART)
    {
        if(size > AP_NV_RESV_MAX_SZIE)
            return NVRAM_PART_LEN_ERR;

        part_offset = AP_NV_RESV_OFFSET;
    }
#endif
    nv_addr = nvramGetnvAddr(APNV1);


    memcpy(&nvHeader,(void *)nv_addr,sizeof(NvHeader_t));
    // need to format
    if(nvHeader.time_counter==0xFFFFFFFF)
    {
        //NVM_ASSERT(0);
        NVM_Print("nvHeader: 0xFFFFFFFF\n");
        NVM_PROBE(1);
    }

    valid = checkNvValid(NV,APNV1);
    if(valid == 0)  //broken
    {
        NVM_Print("restoreFactory\n");
        ret = restoreFactory(APNV1);
        if(ret != NVRAM_OK)
            return NVRAM_FAC_RESTORE_ERR;
    }else{  //ok
        NVM_Print("checkNvValid pass\n");
    }

    // XIP mode access flash directly
    memcpy(data,(void *)((nv_addr+sizeof(NvHeader_t))+part_offset+offset),size);
    ret = ((NvHeader_t *)(nv_addr))->used_size;
    return ret;
}

/**
 \fn        uint32_t nvramSave2CprsFac()
 \brief     used to sync CP NV from working area to compressed factory area
            only valid when factory area is compressed
 para[0]:   data  compressed data to save to factory
 para[1]:   size  saved data size
 para[2]    decprsBuf global buffer ptr for decompress API
 \note      called by  AT used by EMAT to send all compressed data( cp nv only) to UE after cali

*/
#ifdef REL_COMPRESS_EN
uint32_t nvramSave2CprsFac(uint8_t * data,uint32_t size, uint8_t *decprsBuf)
{
    uint32_t nv_addr = 0 , fac_nv_addr = 0;
    uint8_t numofBlk = 0,sec_counter = 0;
    unsigned int decompDataLen = (unsigned int)size;
    unsigned int decompDataLenTemp = 0;
    uint32_t nvRFDataReadLen = 4096; //4K
    uint32_t nvRFDataCmpCount = 12; //48K
    volatile uint32_t ret = 0;
    uint8_t *nvData = NULL;
    NvRfCompHeader_t rfCompHeader;

    if(size > NVRAM_COMPRESS_FAC_MAX_SIZE)
        NVM_ASSERT(0);//fatal error

    //step 1 save data to factory nv
    fac_nv_addr = nvramGetnvFacAddr(CPNV1);
    // num of 4096 block of compressed data
    numofBlk =((size+(sizeof(NvRfCompHeader_t)))%NVRAM_SECTOR_SIZE) ? (((size+(sizeof(NvRfCompHeader_t)))/NVRAM_SECTOR_SIZE) +1) : ((size+(sizeof(NvRfCompHeader_t)))/NVRAM_SECTOR_SIZE);

    for(sec_counter=0;sec_counter<numofBlk;sec_counter++)
    {
        nvram_flash_earse((fac_nv_addr+sec_counter*NVRAM_SECTOR_SIZE)&0x007FFFFF);
    }

    rfCompHeader.magic = NVRAM_COMPRESS_HEADER_MAGIC;
    rfCompHeader.compressedSize = size;
    rfCompHeader.crc = 0;
    rfCompHeader.resv = 0;
    nvData = malloc(size+(sizeof(NvRfCompHeader_t)));
    memcpy(nvData, &rfCompHeader, (sizeof(NvRfCompHeader_t)));
    memcpy(nvData+sizeof(NvRfCompHeader_t), data, size);
    nvram_flash_write(nvData,(fac_nv_addr&0x007FFFFF),(size+(sizeof(NvRfCompHeader_t))));
    free(nvData);

    //step 2 read back and decompress
    memset(data, 0, size);
    nvram_flash_read(data, (fac_nv_addr&0x007FFFFF)+sizeof(NvRfCompHeader_t), size);  //read compress data

    extern void decompressRamCodeGetAddrInfo(void);
    decompressRamCodeGetAddrInfo();

    ECPLAT_PRINTF(UNILOG_NVRAM, nvramSave2CprsFac_0, P_INFO, "outStream=0x%x,outStreamLen=%d..inStream=0x%x,inStreamLen=%d..",decprsBuf, decompDataLen, data, size);
    extern int lzmaDecodeEc(unsigned char *outStream, unsigned int *outStreamLen, unsigned char *inStream, unsigned int inStreamLen, unsigned int dataType, unsigned int *lzmaUnpackLen);
    lzmaDecodeEc((unsigned char *)decprsBuf, &decompDataLen, (unsigned char *)data, (unsigned int)size, 0, &decompDataLenTemp); //decompress all data to deCompressBuf
    ECPLAT_PRINTF(UNILOG_NVRAM, nvramSave2CprsFac_1, P_INFO, "nvramSave2CprsFac decompDataLen= %d",decompDataLen);

    //step3 check with working area
    nv_addr = nvramGetnvAddr(CPNV1);
    nvData = malloc(nvRFDataReadLen);
    for(sec_counter=0; sec_counter<nvRFDataCmpCount; sec_counter++)
    {
        memset(nvData, 0, nvRFDataReadLen);
        nvram_flash_read(nvData, ((nv_addr+sec_counter*nvRFDataReadLen)&0x007FFFFF), nvRFDataReadLen);  //read compress data 4096 bytes once and compare
        if(memcmp(nvData, (decprsBuf+sec_counter*nvRFDataReadLen), nvRFDataReadLen) != 0)
        {
            ret = 1;
            ECPLAT_PRINTF(UNILOG_NVRAM, nvramSave2CprsFac_2, P_ERROR, "nvramSave2CprsFac decomp error !!");
            break;
        }
    }
    free(nvData);
    return ret;
}
#endif

#else
static uint32_t apNvramWrite(NvType_t nvt,uint8_t * data,uint32_t size)
{
    uint32_t ret=0;
    NvHeader_t nvHeader;
    uint8_t *pdata=NULL;

    int i=0;
    uint16_t header_chk=0;
    uint32_t chksum=0;
    uint32_t nv_addr=0;

    NVM_Print("nvramWrite num: %d\n",nvt);

    pdata = (uint8_t *)malloc(sizeof(NvHeader_t)+size+1);
    if( pdata == NULL )
        NVM_ASSERT(0);

    nv_addr = nvramGetnvAddr(nvt);


    memcpy(pdata,(void *)nv_addr,size+sizeof(NvHeader_t));


    nvHeader.nv_flag=NV_FLAG;
    nvHeader.used_size=size;
    nvHeader.time_counter=0;
    nvHeader.size_dummy=0xFFFF;
    nvHeader.dummy1=0xFFFFFFFF;
    nvHeader.dummy2=0xFFFFFFFF;
    nvHeader.dummy3=0xFFFFFFFF;
    nvHeader.header_chk_dummy=0xFFFF;
    nvHeader.chksum=0;
    nvHeader.header_chk=0;

    memcpy(pdata,&nvHeader,sizeof(NvHeader_t));
    for(i=0;i<size;i++)
    {
        pdata[sizeof(NvHeader_t)+i]=*(uint8_t *)(data+i);
    }

    chksum = nvramCrc32((pdata+sizeof(NvHeader_t)),size);
    ((NvHeader_t *)pdata)->chksum = chksum;

    header_chk = nvramChksum(pdata,sizeof(NvHeader_t)-4);
    ((NvHeader_t *)pdata)->header_chk = header_chk;

    // erase
    ret=nvram_flash_earse(nv_addr&0x007FFFFF);

    if(ret != 0)
    {
        free(pdata);
        return ret;
    }

    // program
    ret=nvram_flash_write(pdata,(nv_addr&0x007FFFFF),size+sizeof(NvHeader_t));

    if(ret != 0)
    {
        free(pdata);
        return ret;
    }

    ret = memcmp(pdata,(void *)nv_addr,size+sizeof(NvHeader_t));



    free(pdata);

    NVM_Print("nvramWrite over\n");

    return ret;
}


uint32_t apNvramRead(NvType_t nvt,uint8_t * data,uint32_t size,uint32_t offset)
{
    uint32_t ret=0;
    NvHeader_t nvHeader;
    uint32_t valid=0;
    uint32_t nv_addr=0;


    //parameter check
    if((int)nvt<0 || (int)nvt>=NV_MAX)
        NVM_ASSERT(0);

    NVM_Print("ap_nvram_read num: %d\n",nvt);

    nv_addr = nvramGetnvAddr(nvt);


    memcpy(&nvHeader,(void *)nv_addr,sizeof(NvHeader_t));
    // need to format
    if(nvHeader.time_counter==0xFFFFFFFF)
    {
        //NVM_ASSERT(0);
        NVM_Print("nvHeader: 0xFFFFFFFF\n");
        NVM_PROBE(1);
    }

    valid = checkNvValid(NV,nvt);
    if(valid == 0)  //broken
    {
        NVM_Print("restoreFactory\n");
        ret = restoreFactory(nvt);
        if(ret != NVRAM_OK)
            return NVRAM_FAC_RESTORE_ERR;
    }else{  //ok
        NVM_Print("checkNvValid pass\n");
    }

    // XIP mode access flash directly
    memcpy(data,(void *)((nv_addr+sizeof(NvHeader_t))+offset),size);
    ret = ((NvHeader_t *)(nv_addr))->used_size;
    return ret;
}



/**
 \fn        uint32_t nvramSave2Fac()
 \brief     used to sync CP NV from working area to factory area
            not valid when factory area is compressed, e.g. ec618s/718s

 \note      called by RfFcWriteTable and nvramSav2FacAt in AP
*/
uint32_t nvramSave2Fac()
{
    uint32_t ret=0;
    uint8_t *pdata=NULL;
    uint32_t nv_addr=0,org_fac_addr=0,fac_addr=0;
    uint32_t ram_offset_addr=0;
    int sec_num=0;
    int i;

    //malloc mem
    pdata = (uint8_t *)RF_NST_TABLE_BUFF_ADDR;

    //dump all memory to ram
    nv_addr = nvramGetnvAddr(CPNV1);
    org_fac_addr = nvramGetnvFacAddr(CPNV1);
    memcpy(pdata,(void *)(nv_addr),NVRAM_SECTOR_SIZE*RF_CALI_NV_SECTOR_BANK_NUM_MAX);
    NVM_Print("nv_addr %08X.\n",nv_addr);
    NVM_Print("org_fac_addr %08X.\n",org_fac_addr);
    //erase all fac nv
    for(i=0;i<RF_CALI_NV_SECTOR_BANK_NUM_MAX;i++)
    {
        ret = cp_nvram_flash_earse((org_fac_addr+i*NVRAM_SECTOR_SIZE)&0x007FFFFF);
        if(ret != 0)
        {
            return ret;
        }
    }

    for(i=CPNV1;i<NV_MAX;i++)
    {
        fac_addr = nvramGetnvFacAddr((NvType_t)i);
        sec_num = nvramGetnvLength((NvType_t)i);
        ram_offset_addr = fac_addr - org_fac_addr;
        NVM_Print("fac_addr %08X.nvt: %d\n",fac_addr,i);
        NVM_Print("sec_num %08X.nvt: %d\n",sec_num,i);
        NVM_Print("ram_offset_addr %08X.nvt: %d\n",ram_offset_addr,i);
        // program
        ret=cp_nvram_flash_write((uint8_t *)(pdata+ram_offset_addr),(fac_addr&0x007FFFFF),sec_num*NVRAM_SECTOR_SIZE);
        NVM_Print("cp_nvram_flash_write %08X.\n",ret);
        if(ret != 0)
        {
            return ret;
        }
    }

    return ret;
}

#endif



NvErr_e nvramInit()
{
    NvErr_e ret=NVRAM_OK;
    //crc32_init(POLY32);
    return ret;
}

uint32_t nvramWrite(NvType_t nvt,uint8_t * data,uint32_t size)
{
    uint32_t ret=0;

    if(nvt>=CPNV1)
    {
        ret=cpNvramWrite(nvt,data,size);
    }
    else
    {
        ret=apNvramWrite(nvt,data,size);
    }


    return ret;

}

uint32_t nvramRead(NvType_t nvt,uint8_t * data,uint32_t size,uint32_t offset)
{
    uint32_t ret=0;
    if(nvt>=CPNV1)
        ret=cpNvramRead(nvt,data,size,offset);
    else
        ret=apNvramRead(nvt,data,size,offset);

    return ret;
}


#ifndef NVRAM_READONLY




/**
 \fn        uint32_t nvramSave2Fac_ap()
 \brief     used to sync AP NV from working area to factory area


 \note      only called by nvramSav2FacAt
            for both 1 APNV and 4 AP NV,should be same, since only copy APNVx to factory area
            if EC618S/718S, only APNV1
*/
uint32_t nvramSave2FacAp()
{
    uint32_t ret=0;
    uint8_t *pdata=NULL;
    uint32_t nv_addr=0,fac_addr=0;
    //int sec_num=0;
    int i;

    //malloc mem
    pdata = (uint8_t *)malloc(NVRAM_SECTOR_SIZE);
    if( pdata == NULL )
        NVM_ASSERT(1);

    for(i=APNV1;i<CPNV1;i++)
    {
        nv_addr = nvramGetnvAddr((NvType_t)i);
        fac_addr = nvramGetnvFacAddr((NvType_t)i);
        //sec_num = nvramGetnvLength(i);
        NVM_Print("fac_addr %08X.nvt: %d\n",fac_addr,i);

        ret = nvram_flash_earse(fac_addr&0x007FFFFF);
        if(ret != 0)
        {
            free(pdata);
            return ret;
        }

        // program
        memcpy(pdata,(void *)(nv_addr),NVRAM_SECTOR_SIZE);
        ret=nvram_flash_write((uint8_t *)pdata,(fac_addr&0x007FFFFF),NVRAM_SECTOR_SIZE);

        if(ret != 0)
        {
            free(pdata);
            return ret;
        }
    }
    free(pdata);
    return ret;
}
#endif




void nvramAfterInit(void)
{
    int i=0;
    uint32_t valid=0;

    #if (defined TYPE_EC718H) || (defined TYPE_EC718U)

    #if (defined TYPE_EC718H)
    CPFLASH_xipInit();
    #endif

    for(i=0;i<NV_MAX;i++)
    {
        NVM_Print("check_nv num: %d\n",i);
        valid = checkNvValid(NV,i);
        if(valid ==0)
        {
            NVM_Print("restoreFactory num: %d\n",i);
            restoreFactory((NvType_t)i);

        }
    }

    #elif (defined TYPE_EC718S) || (defined TYPE_EC716S) || (defined TYPE_EC718P)
    uint32_t cpNVValidBitMp = 0;

    for(i=0;i<CPNV1;i++)// step1 check AP NV which is not compressed
    {
        NVM_Print("check_nv num: %d\n",i);
        valid = checkNvValid(NV,i);
        if(valid ==0)
        {
            NVM_Print("restoreFactory num: %d\n",i);
            restoreFactory((NvType_t)i);

        }
    }

    for(i=CPNV1;i<NV_MAX;i++)// step2 check CP NV which is  compressed
    {
        NVM_Print("check_nv num: %d\n",i);
        valid = checkNvValid(NV,i);
        //encounter one CP NV broken, recover as a whole part, not support one NV recovery
        if(valid ==0)
        {
            cpNVValidBitMp |= (0x1<<i);
        }
    }

    if(cpNVValidBitMp != 0)//at least one nv is broken
    {
        #ifdef REL_COMPRESS_EN
        restoreFactoryCprs(cpNVValidBitMp);
        #endif
    }

    #endif
    NVM_Print("nvram_after_init over.\r\n");
}

#endif





/**
 \fn        uint32_t nvramGetnvLength(NvType_t nvt)
 \brief     used to get length for each nv
 \param[in] nvt nv item

 \note      EC618S/EC718S: only 1 AP NV(4KB), and 4 CP NV(48KB)
            others: 4 AP NV(16KB), and 6 CP NV(100KB)
*/
uint32_t nvramGetnvLength(NvType_t nvt)
{
    uint32_t len=0;
    switch(nvt){

        #if (defined TYPE_EC718S) || (defined TYPE_EC716S) || (defined TYPE_EC718P)
        case APNV1:
            len=1;
            break;
        #else
        case APNV1:
            len=1;
            break;
        case APNV2:
            len=1;
            break;
        case APNV3:
            len=1;
            break;
        case APNV4:
            len=1;
            break;
        #endif

        case CPNV1:
            len=2;
            break;
        case CPNV2:
            len=2;
            break;
        case CPNV3:
            len=2;
            break;

        #if (defined TYPE_EC718S) || (defined TYPE_EC716S) || (defined TYPE_EC718P)
        case CPNV4:
            len=6;
            break;
        #else
        case CPNV4:
            len=2;
            break;
        case CPNV5:
            len=2;
            break;
        case CPNV6:
            len=15;
            break;
        #endif
        default:
            break;
    }
    return len;
}




uint32_t nvramGetnvFacAddr(NvType_t nvt)
{
    uint32_t addr=0;
    switch(nvt){
        #ifdef CORE_IS_AP
        #if (defined TYPE_EC718S) || (defined TYPE_EC716S) || (defined TYPE_EC718P)
        case APNV1:
            addr = NVRAM_AP_SECTOR_FACTORY_BASE+AP_FLASH_XIP_ADDR;
            break;
        #else
        case APNV1:
            addr = NVRAM_AP_SECTOR_FACTORY_BASE+AP_FLASH_XIP_ADDR;
            break;
        case APNV2:
            addr = NVRAM_AP_SECTOR_FACTORY_BASE+NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case APNV3:
            addr = NVRAM_AP_SECTOR_FACTORY_BASE+2*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case APNV4:
            addr = NVRAM_AP_SECTOR_FACTORY_BASE+3*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        #endif
        #endif
#if (defined TYPE_EC718H)
        case CPNV1:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+AP_VIEW_CPFLASH_XIP_ADDR;
            break;
        case CPNV2:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+2*NVRAM_SECTOR_SIZE+AP_VIEW_CPFLASH_XIP_ADDR;
            break;
        case CPNV3:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+4*NVRAM_SECTOR_SIZE+AP_VIEW_CPFLASH_XIP_ADDR;
            break;
        case CPNV4:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+6*NVRAM_SECTOR_SIZE+AP_VIEW_CPFLASH_XIP_ADDR;
            break;
        case CPNV5:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+8*NVRAM_SECTOR_SIZE+AP_VIEW_CPFLASH_XIP_ADDR;
            break;
        case CPNV6:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+10*NVRAM_SECTOR_SIZE+AP_VIEW_CPFLASH_XIP_ADDR;
            break;
#elif (defined TYPE_EC718U)
        case CPNV1:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV2:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+2*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV3:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+4*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV4:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+6*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV5:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+8*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV6:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+10*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;

#elif (defined TYPE_EC718S) || (defined TYPE_EC716S) || (defined TYPE_EC718P)
        case CPNV1:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV2:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+2*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV3:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+4*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV4:
            addr = NVRAM_CP_SECTOR_FACTORY_BASE+6*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;

#endif

        default:
            break;
    }
    return addr;
}



/**
 \fn        void nvramGetnvAddr(NvType_t nvt)
 \brief     used to get xip address for each nv
 \param[in] nvt nv item

 \note      EC618/EC718H as CP flash, CP NV locate in it
            other chip AP/CP share one flash:
            EC718S/EC618S/EC718P: one AP NV, 48K CP NV, and should compress factory region
            EC718U: keep 4 AP NV and 100K CP NV
*/
uint32_t nvramGetnvAddr(NvType_t nvt)
{
    uint32_t addr=0;
    switch(nvt){
        #ifdef CORE_IS_AP
        #if (defined TYPE_EC718S) || (defined TYPE_EC716S) || (defined TYPE_EC718P)
        case APNV1:
            addr = NVRAM_AP_SECTOR_BASE+AP_FLASH_XIP_ADDR;
            break;
        #else
        case APNV1:
            addr = NVRAM_AP_SECTOR_BASE+AP_FLASH_XIP_ADDR;
            break;
        case APNV2:
            addr = NVRAM_AP_SECTOR_BASE+NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case APNV3:
            addr = NVRAM_AP_SECTOR_BASE+2*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case APNV4:
            addr = NVRAM_AP_SECTOR_BASE+3*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        #endif
        #endif
#if (defined TYPE_EC718H)
        case CPNV1:
            addr = NVRAM_CP_SECTOR_BASE+AP_VIEW_CPFLASH_XIP_ADDR;
            break;
        case CPNV2:
            addr = NVRAM_CP_SECTOR_BASE+2*NVRAM_SECTOR_SIZE+AP_VIEW_CPFLASH_XIP_ADDR;
            break;
        case CPNV3:
            addr = NVRAM_CP_SECTOR_BASE+4*NVRAM_SECTOR_SIZE+AP_VIEW_CPFLASH_XIP_ADDR;
            break;
        case CPNV4:
            addr = NVRAM_CP_SECTOR_BASE+6*NVRAM_SECTOR_SIZE+AP_VIEW_CPFLASH_XIP_ADDR;
            break;
        case CPNV5:
            addr = NVRAM_CP_SECTOR_BASE+8*NVRAM_SECTOR_SIZE+AP_VIEW_CPFLASH_XIP_ADDR;
            break;
        case CPNV6:
            addr = NVRAM_CP_SECTOR_BASE+10*NVRAM_SECTOR_SIZE+AP_VIEW_CPFLASH_XIP_ADDR;
            break;
#elif (defined TYPE_EC718U)
        case CPNV1:
            addr = NVRAM_CP_SECTOR_BASE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV2:
            addr = NVRAM_CP_SECTOR_BASE+2*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV3:
            addr = NVRAM_CP_SECTOR_BASE+4*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV4:
            addr = NVRAM_CP_SECTOR_BASE+6*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV5:
            addr = NVRAM_CP_SECTOR_BASE+8*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV6:
            addr = NVRAM_CP_SECTOR_BASE+10*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;

#elif (defined TYPE_EC718S) || (defined TYPE_EC716S) || (defined TYPE_EC718P)
        case CPNV1:
            addr = NVRAM_CP_SECTOR_BASE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV2:
            addr = NVRAM_CP_SECTOR_BASE+2*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV3:
            addr = NVRAM_CP_SECTOR_BASE+4*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;
        case CPNV4:
            addr = NVRAM_CP_SECTOR_BASE+6*NVRAM_SECTOR_SIZE+AP_FLASH_XIP_ADDR;
            break;


#endif
        default:
            break;
    }
    return addr;
}





/**
 \fn        void nvramGetnvAddr(NvType_t nvt)
 \brief     used only by CP to get working nv xip address(exclude header)
 \param[in] nvt nv item

 \note     need confirm this is xip address from cp view!!, maybe CP dedicate flash or shared flash
           nvramGetnvAddr return xip addr from ap view, for cp dedaicate flash case, need transfer to cp view
           ec618 cp flash: 0x08800000 from ap view, 0x00800000 from cp view, need a mask
           ec718S/P/U, shared flash: 0x00800000 from both ap/cp view, no need mask
           ec718H,cp flash: 0x08000000 from both ap/cp view no need mask
*/
uint32_t nvramGetAddr(NvType_t nvt)
{
    uint32_t ret=0;
    ret=(nvramGetnvAddr(nvt)+sizeof(NvHeader_t));
    return ret;

}
