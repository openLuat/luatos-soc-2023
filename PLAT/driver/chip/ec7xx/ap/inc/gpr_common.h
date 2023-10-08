#ifndef _GPR_COMMON_EC7XX_H
#define _GPR_COMMON_EC7XX_H

#define RMI_GPR_TOP_CLKEN_REG_INDEX          (0)
#define RMI_GPR_XPSYS_CLKEN_REG_INDEX        (1)

#define APB_GPR_TOP_MP_CLKEN_REG_INDEX       (2)
#define APB_GPR_TOP_MP_CLKEN_EXT_REG_INDEX   (3)
#define APB_GPR_TOP_AP_CLKEN_REG_INDEX       (4)

#define APB_GPR_APB_GP_PCLK_EN_REG_INDEX     (5)
#define APB_GPR_APB_MP_PCLK_EN_REG_INDEX     (6)
#define APB_GPR_APB_AP_PCLK_EN_REG_INDEX     (7)

#define APB_GPR_APB_MP_FCLK_EN_REG_INDEX     (8)
#define APB_GPR_APB_AP_FCLK_EN_REG_INDEX     (9)


#define CLKEN_REG_INDEX_MAX                  (0xF)

/**
|-----registerAccessIndex-----|-----bitPosition-----|---------index---------|
|------------4 bit------------|--------5 bit--------|---------8 bit---------|
*/
#define CONSTRUCT_CLOCK_ID(registerAccessIndex, bitPosition, index)    \
                  (((registerAccessIndex) << 13) | ((bitPosition) << 8) | (index))


/** Macro to extract index from clock id value */
#define GET_INDEX_FROM_CLOCK_ID(value)   ((value) & 0xFFU)

/** Macro to extract bitPosition from clock id value */
#define GET_BP_FROM_CLOCK_ID(value)      ((value >> 8U) & 0x1FU)

/** Macro to extract registerAccessIndex from clock id value */
#define GET_RAI_FROM_CLOCK_ID(value)     ((value >> 13U) & 0xFU)

/**
 |-----clockIdIndex-----|-----value-----|-----parentClockId-----|
 |---------8 bit--------|-----4 bit-----|--------17 bit---------|
 */
/** Macro to compose clock select value */
#define MAKE_CLOCK_SEL_VALUE(clockIdIndex, value, parentClockId)   \
                  ((clockIdIndex << 21U) | value << 17U | parentClockId)

/** Macro to extract clockId from clock select value */
#define GET_CLOCKIDINDEX_FROM_CLOCK_SEL_VALUE(value)   ((value >> 21U) & 0xFFUL)

/** Macro to extract value from clock select value */
#define GET_VALUE_FROM_CLOCK_SEL_VALUE(value)   ((value >> 17U) & 0xFUL)

/** Macro to extract parentClockId from clock select value */
#define GET_PARENTCLOCKID_FROM_CLOCK_SEL_VALUE(value)   (value & 0x1FFFFUL)


#define APB_GPR_TOP_AP_RST_REQ_REG_INDEX      (0)
#define APB_GPR_APB_GP_PRST_REQ_REG_INDEX     (1)
#define APB_GPR_APB_MP_PRST_REQ_REG_INDEX     (2)

#define APB_GPR_APB_AP_PRST_REQ_REG_INDEX     (3)
#define APB_GPR_APB_MP_FRST_REQ_REG_INDEX     (4)
#define APB_GPR_APB_AP_FRST_REQ_REG_INDEX     (5)

#define RMI_GPR_TOP_RSTREQ_REG_INDEX          (6)
#define RMI_GPR_XPSYS_RSTREQ_REG_INDEX        (7)


/**
|-----registerAccessIndex-----|-----bitPosition-----|
|------------4 bit------------|--------8 bit--------|
*/
#define CONSTRUCT_CLOCK_RESET_ID(registerAccessIndex, bitPosition)    \
                  (((registerAccessIndex) << 8) | (bitPosition))

/** Macro to extract bitPosition from clock reset id value */
#define GET_BP_FROM_CLOCK_RESET_ID(value)      ((value) & 0xFFU)

/** Macro to extract registerAccessIndex from clock reset id value */
#define GET_RAI_FROM_CLOCK_RESET_ID(value)     ((value >> 8U) & 0xFU)


/** \brief List of IDs used for sw reset control */
typedef enum
{
    /*                   Top AP Reset                                              */
    RST_AP_FABSYS_HCLK    = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 0),            /**< Ap fab sys AHB reset             */
    RST_AP_RMI_HCLK       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 1),            /**< Ap RMI AHB reset                 */
    RST_MFAB_HCLK         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 2),            /**< MFAB AHB reset                   */
    RST_AFBBR_HCLK        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 3),            /**< AFBBR AHB reset                  */
    RST_CFBBR_HCLK        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 4),            /**< CFBBR AHB reset                  */
    RST_MSMB_HCLK         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 5),            /**< MSMB AHB reset                   */
    RST_FLASH_HCLK        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 6),            /**< Flash AHB reset                  */
    RST_PSRAM_HCLK        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 7),            /**< Psram AHB reset                  */
    RST_ULOG_HCLK         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 8),            /**< Unilog AHB reset                 */
    RST_USBC_HCLK         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 9),            /**< Usbc AHB reset                   */
    RST_CLK_FLASH         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 10),           /**< Flash Func reset                 */
    RST_CLK_PSRAM         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 11),           /**< Psram Func reset                 */
    // reserved             = 12,
    RST_LPUA              = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 13),           /**< Lpua reset                       */
    RST_CLK_FRACDIV       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 14),           /**< Fracdiv reset                    */
    RST_ULOG_SMP          = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 15),           /**< Unilog Smp reset                 */
    RST_FCLK_FUSE         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 16),           /**< Fuse Func reset                  */
    RST_UTFC_HCLK         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 17),           /**< Utfc AHB reset                   */
    RST_ULDP_HCLK         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 18),           /**< Uldp AHB reset                   */
    RST_UPMU              = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 19),           /**< Upmu reset                       */
    RST_TMU_SMP           = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 20),           /**< TMU SMP reset                    */
    RST_TMU_CLKCAL        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 21),           /**< TMU CLKCAL  reset                */
    RST_FCACHE_HCLK       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 22),           /**< Fcache AHB reset                 */
    RST_PCACHE_HCLK       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 23),           /**< Pcache AHB reset                 */
    RST_UTFW_HCLK         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 24),           /**< UTFW AHB reset                 */
    RST_SCT_HCLK          = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 25),           /**< SCT AHB reset                 */

    // reserved             = 26,
    RST_USBC_PHY          = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 27),           /**< Usbc PHY reset                   */
    RST_USBP_POR          = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 28),           /**< USBP Por reset                   */
    RST_USBP_UTMI         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 29),           /**< USBP Utmi reset                  */
    RST_VPU_HCLK          = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 30),           /**< VPU AHB reset                 */
    RST_EPU_HCLK          = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_TOP_AP_RST_REQ_REG_INDEX, 31),           /**< EPU AHB reset                 */

    /*                   APB GP Reset                                              */
    // reserved                         = 0,
    RST_PCLK_SIPC         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_GP_PRST_REQ_REG_INDEX, 1),           /**< Sipc APB reset                   */
    RST_PCLK_AON          = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_GP_PRST_REQ_REG_INDEX, 2),           /**< Aon APB reset                    */
    RST_PCLK_CPMU         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_GP_PRST_REQ_REG_INDEX, 3),           /**< Cpmu APB reset                   */
    RST_PCLK_PMDIG        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_GP_PRST_REQ_REG_INDEX, 4),           /**< Pmdig APBreset                   */
    RST_PCLK_RFDIG        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_GP_PRST_REQ_REG_INDEX, 5),           /**< Rfdig APB reset                  */
    RST_PCLK_PAD          = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_GP_PRST_REQ_REG_INDEX, 6),           /**< Pad APB reset                    */
    RST_PCLK_TMU          = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_GP_PRST_REQ_REG_INDEX, 7),           /**< Tmu APB reset                    */
    RST_PCLK_FUSE         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_GP_PRST_REQ_REG_INDEX, 8),           /**< Fuse APB reset                   */
    RST_PCLK_TRNG         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_GP_PRST_REQ_REG_INDEX, 9),           /**< Trng APB reset                   */
    RST_PCLK_USBP         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_GP_PRST_REQ_REG_INDEX, 10),          /**< Usbp APB reset                   */

    /*                   APB MP Reset                                              */
    RST_PCLK_UART0        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_PRST_REQ_REG_INDEX, 0),           /**< Uart0 APB reset                  */
    RST_PCLK_UART1        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_PRST_REQ_REG_INDEX, 1),           /**< Uart1 APB reset                  */
    RST_PCLK_UART2        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_PRST_REQ_REG_INDEX, 2),           /**< Uart2 APB reset                  */
    RST_PCLK_UART3        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_PRST_REQ_REG_INDEX, 3),           /**< Uart3 APB reset                  */
    RST_PCLK_I2C0         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_PRST_REQ_REG_INDEX, 4),           /**< I2c 0 APB reset                  */
    RST_PCLK_I2C1         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_PRST_REQ_REG_INDEX, 5),           /**< I2c 1 APB reset                  */
    RST_PCLK_SPI0         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_PRST_REQ_REG_INDEX, 6),           /**< Ssp 0 APB reset                  */
    RST_PCLK_SPI1         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_PRST_REQ_REG_INDEX, 7),           /**< Ssp 1 APB reset                  */
    RST_PCLK_USP0         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_PRST_REQ_REG_INDEX, 8),           /**< Usp 0 APB reset                  */
    RST_PCLK_USP1         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_PRST_REQ_REG_INDEX, 9),           /**< Usp 1 APB reset                  */
    RST_PCLK_USP2         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_PRST_REQ_REG_INDEX, 10),          /**< Usp 2 APB reset                  */

    /*                   APB AP Reset                                              */
    RST_PCLK_WDG          = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 0),           /**< WDG APB reset                    */
    RST_PCLK_TIMER0       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 1),           /**< Timer0 APB reset                 */
    RST_PCLK_TIMER1       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 2),           /**< Timer1 APBreset                  */
    RST_PCLK_TIMER2       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 3),           /**< Timer2 APB reset                 */
    RST_PCLK_TIMER3       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 4),           /**< Timer3 APB reset                 */
    RST_PCLK_TIMER4       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 5),           /**< Timer4 APB reset                 */
    RST_PCLK_TIMER5       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 6),           /**< Timer5 APB reset                 */
    RST_PCLK_IPC          = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 7),           /**< IPC APB reset                    */
    RST_PCLK_USIM0        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 8),           /**< USIM0 APB reset                  */
    RST_PCLK_USIM1        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 9),           /**< USIM1 APB reset                  */
    RST_PCLK_KPC          = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 10),          /**< KPC APB reset                    */
    RST_PCLK_ONEW         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 11),          /**< ONEW APB reset                   */
    RST_PCLK_DPMU         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_PRST_REQ_REG_INDEX, 12),          /**< DPMU APB reset                   */


    /*                   APB MP Func Reset                                         */
    RST_FCLK_UART0       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_FRST_REQ_REG_INDEX, 0),            /**< Uart0 Func reset                 */
    RST_FCLK_UART1       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_FRST_REQ_REG_INDEX, 1),            /**< Uart1 Func reset                 */
    RST_FCLK_UART2       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_FRST_REQ_REG_INDEX, 2),            /**< Uart2 Func reset                 */
    RST_FCLK_UART3       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_FRST_REQ_REG_INDEX, 3),            /**< Uart3 Func reset                 */
    RST_FCLK_I2C0        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_FRST_REQ_REG_INDEX, 4),            /**< I2C0 Func reset                  */
    RST_FCLK_I2C1        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_FRST_REQ_REG_INDEX, 5),            /**< I2C1 Func reset                  */
    RST_FCLK_SPI0        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_FRST_REQ_REG_INDEX, 6),            /**< SSP0 Func reset                  */
    RST_FCLK_SPI1        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_FRST_REQ_REG_INDEX, 7),            /**< SSP1 Func reset                  */
    RST_FCLK_USP0        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_FRST_REQ_REG_INDEX, 8),            /**< USP0 Func reset                  */
    RST_FCLK_USP1        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_FRST_REQ_REG_INDEX, 9),            /**< USP1 Func reset                  */
    RST_FCLK_USP2        = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_MP_FRST_REQ_REG_INDEX, 10),           /**< USP2 Func reset                  */

    /*                   APB AP Func Reset                                         */
    RST_FCLK_WDG         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_FRST_REQ_REG_INDEX, 0),           /**< WDG Func reset                    */
    RST_FCLK_TIMER0      = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_FRST_REQ_REG_INDEX, 1),           /**< TIMER0 Func reset                 */
    RST_FCLK_TIMER1      = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_FRST_REQ_REG_INDEX, 2),           /**< TIMER1 Func reset                 */
    RST_FCLK_TIMER2      = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_FRST_REQ_REG_INDEX, 3),           /**< TIMER2 Func reset                 */
    RST_FCLK_TIMER3      = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_FRST_REQ_REG_INDEX, 4),           /**< TIMER3 Func reset                 */
    RST_FCLK_TIMER4      = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_FRST_REQ_REG_INDEX, 5),           /**< TIMER4 Func reset                 */
    RST_FCLK_TIMER5      = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_FRST_REQ_REG_INDEX, 6),           /**< TIMER5 Func reset                 */
    RST_FCLK_USIM0       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_FRST_REQ_REG_INDEX, 7),           /**< USIM0 Func reset                  */
    RST_FCLK_USIM1       = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_FRST_REQ_REG_INDEX, 8),           /**< USIM1 Func reset                  */
    RST_FCLK_KPC         = CONSTRUCT_CLOCK_RESET_ID(APB_GPR_APB_AP_FRST_REQ_REG_INDEX, 9),           /**< KPC Func reset                    */

    /*                   RMI TOP Reset                                             */
    RST_TOP_PBRG_HCLK    = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_TOP_RSTREQ_REG_INDEX, 0),                /**< DMA AHB domain reset              */
    RST_TOP_PBRG_PCLK    = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_TOP_RSTREQ_REG_INDEX, 1),                /**< DMA APB domain reset              */
    RST_TOP_GPR_PCLK     = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_TOP_RSTREQ_REG_INDEX, 2),                /**< GPR APB domain reset              */
    RST_TOP_APBGP_HCLK   = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_TOP_RSTREQ_REG_INDEX, 3),                /**< APBGP AHB domain reset            */
    RST_TOP_APBGP_PCLK   = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_TOP_RSTREQ_REG_INDEX, 4),                /**< APBGP APB domain reset            */

    /*                   RMI XPSYS Reset                                           */
    RST_CACHE_HCLK      = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 0),              /**< GPR CACHE RMI domain reset        */
    RST_FABSUB_HCLK     = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 1),              /**< GPR FABSUB RMI domain reset       */
    RST_SBU_HCLK        = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 2),              /**< GPR SBU RMI domain reset          */
    RST_PBRG_HCLK       = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 3),              /**< GPR PDMA RMI domain reset         */
    RST_SPIS_HCLK       = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 4),              /**< GPR SPIS RMI domain reset         */
    RST_XIC_RMI_HCLK    = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 5),              /**< GPR XIC RMI domain reset          */
    RST_ULOG_RMI_HCLK   = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 6),              /**< GPR ULOG RMI domain reset         */
    RST_GPIO_RMI_HCLK   = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 7),              /**< GPR GPIO RMI domain reset         */
    RST_SCT_RMI_HCLK    = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 8),              /**< GPR SCT RMI domain reset          */

    RST_UTFC_RMI_HCLK   = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 11),             /**< GPR UTFC RMI domain reset         */
    RST_ULDP_RMI_HCLK   = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 12),             /**< GPR ULDP RMI domain reset         */

    RST_PCLK_DMA        = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 15),             /**< GPR PDMA APB reset                */

    RST_UTFW_RMI_HCLK   = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 28),             /**< GPR UTFW RMI domain reset                 */

    RST_VPU_RMI_HCLK    = CONSTRUCT_CLOCK_RESET_ID(RMI_GPR_XPSYS_RSTREQ_REG_INDEX, 30),             /**< GPR VPU RMI domain reset                 */

} ClockResetId_e;

/** \brief typedef for reset whole module */
typedef struct
{
    uint32_t idNumber;                               /**< number of ids this module contains */
    uint32_t delayCpuCycles;                         /**< the cycles need to delay after release */
    const ClockResetId_e* resetReleaseQueue;         /**< pointer to the queue(array) of ids,  reset & release operation sequence
                                                          depends on the position of id in the array, so arrange it properly */
} ClockResetVector_t;

#define RESET_RELEASE_DELAY_US                       (204)

#define MAKE_RESET_VECTOR(delay, ...)                                                                            \
                         {                                                                                       \
                            (sizeof((ClockResetId_e [])__VA_ARGS__) / sizeof(ClockResetId_e)),                   \
                            (delay),                                                                             \
                            (const ClockResetId_e [])__VA_ARGS__                                                 \
                         }

#define RESET_VECTOR_PTR(v) ((ClockResetVector_t [])v)

#define UART0_RESET_VECTOR               MAKE_RESET_VECTOR(RESET_RELEASE_DELAY_US, {RST_PCLK_UART0, RST_FCLK_UART0, RST_PCLK_UART0, RST_FCLK_UART0})
#define UART1_RESET_VECTOR               MAKE_RESET_VECTOR(RESET_RELEASE_DELAY_US, {RST_PCLK_UART1, RST_FCLK_UART1, RST_PCLK_UART1, RST_FCLK_UART1})
#define UART2_RESET_VECTOR               MAKE_RESET_VECTOR(RESET_RELEASE_DELAY_US, {RST_PCLK_UART2, RST_FCLK_UART2, RST_PCLK_UART2, RST_FCLK_UART2})
#define UART3_RESET_VECTOR               MAKE_RESET_VECTOR(RESET_RELEASE_DELAY_US, {RST_PCLK_UART3, RST_FCLK_UART3, RST_PCLK_UART3, RST_FCLK_UART3})

#define UART_RESET_VECTORS               {UART0_RESET_VECTOR, UART1_RESET_VECTOR, UART2_RESET_VECTOR, UART3_RESET_VECTOR}

#define SPI0_RESET_VECTOR                MAKE_RESET_VECTOR(RESET_RELEASE_DELAY_US, {RST_PCLK_SPI0, RST_FCLK_SPI0, RST_PCLK_SPI0, RST_FCLK_SPI0})
#define SPI1_RESET_VECTOR                MAKE_RESET_VECTOR(RESET_RELEASE_DELAY_US, {RST_PCLK_SPI1, RST_FCLK_SPI1, RST_PCLK_SPI1, RST_FCLK_SPI1})

#define SPI_RESET_VECTORS                {SPI0_RESET_VECTOR, SPI1_RESET_VECTOR}

#define I2C0_RESET_VECTOR                MAKE_RESET_VECTOR(RESET_RELEASE_DELAY_US, {RST_PCLK_I2C0, RST_FCLK_I2C0, RST_PCLK_I2C0, RST_FCLK_I2C0})
#define I2C1_RESET_VECTOR                MAKE_RESET_VECTOR(RESET_RELEASE_DELAY_US, {RST_PCLK_I2C1, RST_FCLK_I2C1, RST_PCLK_I2C1, RST_FCLK_I2C1})

#define I2C_RESET_VECTORS                {I2C0_RESET_VECTOR, I2C1_RESET_VECTOR}

#define KPC_RESET_VECTOR                 MAKE_RESET_VECTOR(RESET_RELEASE_DELAY_US, {RST_PCLK_KPC, RST_FCLK_KPC, RST_PCLK_KPC, RST_FCLK_KPC})

typedef enum
{
    FRACDIV_ROOT_CLK_26M,
    FRACDIC_ROOT_CLK_612M
} FracDivRootClk_e;

typedef enum
{
    FRACDIV_0,
    FRACDIV_1
} FracDivSel_e;


typedef struct
{
    FracDivRootClk_e source;
    FracDivSel_e     fracdivSel;
    uint32_t fracDiv0DivRatioInteger;
    uint32_t fracDiv0DivRatioFrac;
    uint32_t fracDiv1DivRatioInteger;
    uint32_t fracDiv1DivRatioFrac;
} FracDivConfig_t;

typedef enum
{
    FRACDIV0_OUT0,
    FRACDIV0_OUT1,
    FRACDIV0_OUT2,
    FRACDIV0_OUT3,

    FRACDIV1_OUT0,
    FRACDIV1_OUT1,
    FRACDIV1_OUT2,
    FRACDIV1_OUT3
} FracDivOutClkId_e;

typedef enum
{
    BCLK0,
    BCLK1,
    BCLK2
} BclkId_e;

typedef enum
{
    MCLK0,
    MCLK1,
    MCLK2
} MclkId_e;

typedef enum
{
    BCLK_SRC_FRACDIV0_OUT0 = 1,
    BCLK_SRC_FRACDIV0_OUT1 = 2,
    BCLK_SRC_FRACDIV0_OUT2 = 3,
    BCLK_SRC_FRACDIV0_OUT3 = 4,

    BCLK_SRC_FRACDIV1_OUT0 = 5,
    BCLK_SRC_FRACDIV1_OUT1 = 6,
    BCLK_SRC_FRACDIV1_OUT2 = 7,
    BCLK_SRC_FRACDIV1_OUT3 = 8,
    BCLK_SRC_CLK_MFG       = 9,
    BCLK_SRC_19P2M         = 10
} BclkSrc_e;

typedef enum
{
    MCLK_SRC_FRACDIV0_OUT0 = 1,
    MCLK_SRC_FRACDIV0_OUT1 = 2,
    MCLK_SRC_FRACDIV0_OUT2 = 3,
    MCLK_SRC_FRACDIV0_OUT3 = 4,

    MCLK_SRC_FRACDIV1_OUT0 = 5,
    MCLK_SRC_FRACDIV1_OUT1 = 6,
    MCLK_SRC_FRACDIV1_OUT2 = 7,
    MCLK_SRC_FRACDIV1_OUT3 = 8,
    MCLK_SRC_CLK_MFG       = 9,
    MCLK_SRC_19P2M         = 10,
    MCLK_SRC_CLK_SMP       = 11,
    MCLK_SRC_CLK_USBP_12M  = 12,
    MCLK_SRC_CLK_USBP_48M  = 13,
    MCLK_SRC_CLK_USBC_PHY  = 14,
} MclkSrc_e;

typedef enum
{
    GPR_APRST_SWRST = 0,
    GPR_APRST_WDG,
    GPR_APRST_LOCKUP,
    GPR_APRST_EXT,
    GPR_APRST_INVALID,
}ApRstSource_e;

typedef enum
{
    GPR_CPRST_SWRST = 0,
    GPR_CPRST_WDG,
    GPR_CPRST_LOCKUP,
    GPR_CPRST_EXT,
    GPR_CPRST_INVALID,
}CpRstSource_e;

#endif
