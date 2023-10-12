/**************************************************************************//**
 * @file     system_ARMCM3.c
 * @brief    CMSIS Device System Source File for
 *           ARMCM3 Device Series
 * @version  V5.00
 * @date     07. September 2016
 ******************************************************************************/
/*
 * Copyright (c) 2009-2016 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*----------------------------------------------------------------------------*
 *                    INCLUDES                                                *
 *----------------------------------------------------------------------------*/

#include "ec7xx.h"
#include "mpu_armv7.h"
#include "Driver_Common.h"
#include "cache.h"
#include "mem_map.h"
#include "slpman.h"
#include "clock.h"
#include "apmu_external.h"
#include "reset.h"
#ifdef FEATURE_LZMA_ENABLE
#include "LzmaEc.h"
#endif
#if defined (PSRAM_FEATURE_ENABLE) && (PSRAM_EXIST==1)
#include "psram.h"
#endif


/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/
#define SYS_ADDRESS  AP_FLASH_LOAD_ADDR

/*----------------------------------------------------------------------------*
 *                    DATA TYPE DEFINITION                                    *
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*
 *                      GLOBAL VARIABLES                                      *
 *----------------------------------------------------------------------------*/

#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
#if defined(__CC_ARM)
       extern uint32_t __Vectors;
#elif defined(__GNUC__)
       extern uint32_t __isr_vector;
#endif
#endif

/*----------------------------------------------------------------------------
  System Core Clock Variable
 *----------------------------------------------------------------------------*/
#define  CORE_CLOCK_SEL_ADDRESS    (0x4f000020)
uint32_t SystemCoreClock = DEFAULT_SYSTEM_CLOCK;


/*----------------------------------------------------------------------------*
 *                      PRIVATE FUNCTION DECLEARATION                         *
 *----------------------------------------------------------------------------*/

extern void CopyRWDataFromBin(void);
extern void SetZIDataToZero(void);
extern void CopyDataRamtoImage(void);
extern void CopyRamCodeForDeepSleep(void);
extern void CopyDataRWZIForDeepSleep(void);
extern void CopyPsramCodeAndData(void);
extern void mpu_init(void);
extern void mpu_deinit(void);
static uint8_t sysROAddrCheck(uint32_t addr);

extern void usblpw_retwkup_bootstat_reset(void);
extern uint32_t usblpw_retwkup_boot_start(void);
extern void usblpw_wr_reg_usb_wkdetflag(uint32_t  usb_wkdetflag);
extern void usblpw_retwkup_clear_ctxstat_var(void);
extern void delay_us(uint32_t us);
extern void FLASH_qspiInit(BOOL needInitReg );
extern uint8_t fcacheBypass(bool isBypass);
extern uint8_t fcacheTestSetting( uint32_t setNum);
extern uint8_t dbusCacheForceOff( BOOL isForceOff );




/*----------------------------------------------------------------------------*
 *                      PRIVATE FUNCTIONS                                     *
 *----------------------------------------------------------------------------*/

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




/*----------------------------------------------------------------------------
  System Core Clock update function
 *----------------------------------------------------------------------------*/
void SystemCoreClockUpdate (void)
{
#ifdef FPGA_TEST
    SystemCoreClock = 102400000;
#else
    switch((*((uint32_t *)CORE_CLOCK_SEL_ADDRESS)) & 0x3)
    {
        case 0:
            SystemCoreClock = 26000000U;
            break;
        case 1:
            if((*((uint32_t *)CORE_CLOCK_SEL_ADDRESS)) & (0x1 << 15))
            {
                SystemCoreClock = 307200000U;
            }
            else
            {
                SystemCoreClock = 204800000U;
            }
            break;
        case 2:
            SystemCoreClock = 102400000U;
            break;
        case 3:
            SystemCoreClock = 32768U;
            break;
    }
#endif
}

void flashFclkDivSmooth(uint8_t div)
{

    volatile uint32_t *gprFlhFclkCtrl = (volatile uint32_t *)0x4f000120;
    volatile uint32_t *gprTopClkSel = (volatile uint32_t *)0x4f000020;
    volatile uint32_t *gprTopClkDiv = (volatile uint32_t *)0x4f000024;
    //step1 set flash_fclk_div_mode =1 bit 1
    *gprFlhFclkCtrl |=  (0x1<<1);
    //step2 delay 3 cycles
    delay_us(1);
    //step3: change source and div
   *gprTopClkSel = (*gprTopClkSel &~ 0x40) | (0x1 << 6);// bit 6 set as 1 to select 102M

   *gprTopClkDiv = (*gprTopClkDiv &~ (0xf<<16)) | (div << 16);//bit 19:16 set as 0x1 to div 2, 102/2=51M

   //step4: flash_fclk_div_smid_trg =1
   *gprFlhFclkCtrl |=  (0x1<<0);

   //step5: loop until flash_fclk_div_smid_trg =0
   while((*gprFlhFclkCtrl&0x1));

   //step6:
   #if 0
   if((*gprFlhFclkCtrl&(0xf<<8))==div)
   {

   }
   else
   {
    while(1);
   }
   #endif



}

/*----------------------------------------------------------------------------
  System initialization function
 *----------------------------------------------------------------------------*/
void SystemFullImageInit (void)
{
    apmuRestoreBootFlag();

    uint32_t ResetStat = apmuGetAPBootFlag();       //areg0

    if(ResetStat == AP_BOOT_FROM_AS1)           //sleep1
        return;

    mpu_deinit();
    #ifdef RAMCODE_COMPRESS_EN
    decompressRamCodeGetAddrInfo();
    #endif

    /*move the RW data in the image to its excution region*/
    CopyRWDataFromBin();

    /*append the ZI region. TODO: maybe ZI data need not to be 0,
    random value maybe aslo fine for application, if so we could
    remove this func call, since it takes a lot of time*/
    SetZIDataToZero();

    #ifdef RAMCODE_COMPRESS_EN
    decompressRamCodeGetAddrInfo();
    #endif

    CopyDataRWZIForDeepSleep();

#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
#if defined(__CC_ARM)
    SCB->VTOR = (uint32_t) &__Vectors;
#elif defined (__GNUC__)
    SCB->VTOR = (uint32_t) &__isr_vector;
#endif
#endif

  /*set NVIC priority group as 4(bits 7,6,5 is group priority bits ),
    3 bits for group priority. Since our CM3 core only implemented 3
    bits for priority and freertos recommend all bits should be
    group priority bits*/
    NVIC_SetPriorityGrouping(4);

    //enable div 0 trap,then div 0 operation will trigger hardfault
    SCB->CCR|=SCB_CCR_DIV_0_TRP_Msk;


    SystemCoreClockUpdate();

    mpu_init();

    FLASH_qspiInit(false);

    #if defined (PSRAM_FEATURE_ENABLE) && (PSRAM_EXIST==1)
    PSRAM_init();
    #endif

}


void SystemInit (void)
{
    DisableICache();

    EnableICache();

#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
#if defined(__CC_ARM)
    SCB->VTOR = (uint32_t) &__Vectors;
#elif defined (__GNUC__)
    SCB->VTOR = (uint32_t) &__isr_vector;
#endif
#endif

    ResetApplyAPLockupCfg();

    BOOT_TIMESTAMP_SET(0, 0);

    GPR_bootSetting();

    usblpw_retwkup_bootstat_reset();

    apmuRestoreBootFlag();

    APBootFlag_e apBootFlag = apmuGetAPLLBootFlag();        //aon reg0
    usblpw_wr_reg_usb_wkdetflag(0);

    if(apBootFlag != AP_BOOT_FROM_POWER_ON)
    {
        usblpw_retwkup_boot_start();
    }
    else
    {
        usblpw_retwkup_clear_ctxstat_var();
    }

    BOOT_TIMESTAMP_SET(0, 1);

    if(apBootFlag == AP_BOOT_FROM_AH)
        apmuSetSwWakeupSlowCntFlash();

    if(apBootFlag == AP_BOOT_FROM_AS1)           //sleep1
    {
        fcacheBypass(FALSE);
        fcacheTestSetting(1);
        dbusCacheForceOff(TRUE);
        DisableFCache();
        EnableFCache();
        #if defined (PSRAM_FEATURE_ENABLE) && (PSRAM_EXIST==1)
        PSRAM_init();
        #endif
        return;
    }

    #ifdef RAMCODE_COMPRESS_EN
    decompressRamCodeGetAddrInfo();
    #endif
    CopyRamCodeForDeepSleep();

    CopyDataRWZIForDeepSleep();

    mpu_init();

    BOOT_TIMESTAMP_SET(0, 2);

    // should call after ramcode init
    fcacheBypass(FALSE);//not bypass flh cache
    fcacheTestSetting(1);
    dbusCacheForceOff(TRUE);
    DisableFCache();
    EnableFCache();

#ifdef FEATURE_DUMP_CHECK
    EXCEP_CHECK_POINT(1);
#endif

  /*set NVIC priority group as 4(bits 7,6,5 is group priority bits ),
    3 bits for group priority. Since our CM3 core only implemented 3
    bits for priority and freertos recommend all bits should be
    group priority bits*/
    NVIC_SetPriorityGrouping(4);

    //enable div 0 trap,then div 0 operation will trigger hardfault
    SCB->CCR|=SCB_CCR_DIV_0_TRP_Msk;

    SystemCoreClockUpdate();

    FLASH_qspiInit(false);

}

