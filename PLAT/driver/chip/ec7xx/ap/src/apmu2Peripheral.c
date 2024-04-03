
#include "apmu2Peripheral.h"
#include "RTE_Device.h"
#include "timer.h"
#include "clock.h"
#include DEBUG_LOG_HEADER_FILE
#if (RTE_USB_EN == 1)
void usblpw_enter_start_proc(bool force_cfg_pwr_down);
void usblpw_enter_abort_proc(bool force_cfg_pwr_down);
void usblpw_susp2vbustbl_guard_dlychk(uint32_t cur_tick);
void usblpw_susp2hib_guard_dlychk(uint32_t cur_tick);
void usblpw_retwkup_sleep1_later_recovery(void);
void usblpw_retwkup_sleep1_pre_recovery(void);
#endif

#if (RTE_LPUART_EN == 1)
#include "bsp_lpusart.h"
#endif

// to wakeup in paging wfi for hib timer, should only use in paging.(timer4 can use in app)
#define AP_NEARHIBTIME_INSTANCE                 (4)
#define AP_NEARHIBTIME_IRQ                      (PXIC0_TIMER4_IRQn)
#define AP_NEARHIBTIME_MAXIMUM                  (4294967)     // 0xffffffff/1000 ms

// do not use timer5 in app
#define CP_STARTTIME_INSTANCE                   (5)
#define CP_STARTTIME_IRQ                        (PXIC0_TIMER5_IRQn)
#define CP_STARTTIME_MAXIMUM                    (4294967)     // 0xffffffff/1000 ms
#ifdef __USER_CODE__
#else
static uint32_t cpPowerOnMask = 0;
static bool cpPowerOnUartMaskMode = false;

extern bool getCPWakeupType(void);
#endif

void apmuPeriUsbEnterStartProc(bool forceCfgPwrDown)
{
#if (RTE_USB_EN == 1)
    usblpw_enter_start_proc(forceCfgPwrDown);
#endif
}

void apmuPeriUsbEnterAbortProc(bool forceCfgPwrDown)
{
#if (RTE_USB_EN == 1)
    usblpw_enter_abort_proc(forceCfgPwrDown);
#endif
}

void apmuPeriUsbSusp2VbusTblGuardDlyChk(uint32_t cur_tick)
{
#if (RTE_USB_EN == 1)
    usblpw_susp2vbustbl_guard_dlychk(cur_tick);
#endif
}

void apmuPeriUsbSusp2HibGuardDlyChk(uint32_t cur_tick)
{
#if (RTE_USB_EN == 1)
    usblpw_susp2hib_guard_dlychk(cur_tick);
#endif
}

void apmuPeriUsbSleep1LateRecoverFlow(bool sleepSuccess)
{
#if (RTE_USB_EN == 1)
    if(sleepSuccess)
    {
        usblpw_retwkup_sleep1_later_recovery();
    }
#endif
}

void apmuPeriUsbSleep1PreRecoverFlow(bool sleepSuccess)
{
#if (RTE_USB_EN == 1)
    if(sleepSuccess)
    {
        usblpw_retwkup_sleep1_pre_recovery();
    }
#endif
}

bool apmuPeriLpuartPreSleepProcess(void)
{
#ifdef __USER_CODE__
extern int soc_uart_sleep_check(uint8_t *result);
	uint8_t result;
	if (!soc_uart_sleep_check(&result))
	{
		return result?true:false;
	}
	else
	{
		return false;
	}
#else
#if (RTE_LPUART_EN == 1)
    LPUSART_SetStopFlag();
    if(LPUSART_IsRxActive())
    {
        ECPLAT_PRINTF(UNILOG_PLA_DRIVER, pmuLpuartPreSleepProcess_lpuart_status, P_SIG, "lpuart status, lsr: 0x%x, isr: 0x%x, fsr: 0x%x", LPUSART_CORE->LSR, LPUSART_CORE->ISR, LPUSART_CORE->FSR);
        LPUSART_ClearStopFlag();
        return true;
    }
    else
    {
        extern void AonRegSetWakeupEdge(uint8_t index, bool posEnable, bool negEnable);
        AonRegSetWakeupEdge(6, false, true);
        NVIC_EnableIRQ(LpuartWakeup_IRQn);
    }
    LPUSART_ClearStopFlag();
    return false;
#else
    return false;
#endif
#endif
}

bool apmuPeriLpuartIsRxActive(void)
{
#ifdef __USER_CODE__
extern int soc_uart_rx_check(uint8_t *result);
	uint8_t result;
	if (!soc_uart_rx_check(&result))
	{
		return result?true:false;
	}
	else
	{
		return false;
	}
#else
#if (RTE_LPUART_EN == 1)
    return LPUSART_IsRxActive();
#else
    return false;
#endif
#endif
}

static void apmuPeriWFITimerExp(void)
{
    ECPLAT_PRINTF(UNILOG_PMU, apmu2PeriWFITimerExp_0, P_VALUE, "apmu2PeriWFITimerExp Enter");
    if (TIMER_getInterruptFlags(CP_STARTTIME_INSTANCE) & TIMER_MATCH0_INTERRUPT_FLAG)
    {
        TIMER_clearInterruptFlags(CP_STARTTIME_INSTANCE, TIMER_MATCH0_INTERRUPT_FLAG);
    }
    TIMER_stop(AP_NEARHIBTIME_INSTANCE);
    TIMER_deInit(AP_NEARHIBTIME_INSTANCE);
}


void apmuPeriStartWFITimer(uint32_t ms)
{
    int32_t clkRet1,clkRet2;
    if(ms<10)
        return;

    CLOCK_clockReset(FCLK_TIMER4);      // to protect setClock error

    //  TIMER config
    //  Config TIMER clock, source from 26MHz and divide by 1
    clkRet1 = CLOCK_setClockSrc(FCLK_TIMER4, FCLK_TIMER4_SEL_26M);
    clkRet2 = CLOCK_setClockDiv(FCLK_TIMER4, 26);

    if((clkRet1 != 0) || (clkRet2 != 0))
    {
        ECPLAT_PRINTF(UNILOG_PMU, apmuPeriStartWFITimer_0, P_ERROR, "clkRet1=%d, clkRet2=%d, CLKFREQ=0x%x", clkRet1, clkRet2, CLOCK_getClockFreq(FCLK_TIMER4));
        EC_ASSERT(0, clkRet1, clkRet2, CLOCK_getClockFreq(FCLK_TIMER4));
    }

    TimerConfig_t timerConfig;
    TIMER_getDefaultConfig(&timerConfig);
    timerConfig.reloadOption = TIMER_RELOAD_ON_MATCH0;

    if(ms > AP_NEARHIBTIME_MAXIMUM)
    {
        ms = AP_NEARHIBTIME_MAXIMUM;
    }
    timerConfig.match0 = (ms-5)*1000;

    ECPLAT_PRINTF(UNILOG_PMU, apmuPeriStartWFITimer_1, P_VALUE, "apmuPeriStartWFITimer set to %u ms", (ms-5));

    TIMER_init(AP_NEARHIBTIME_INSTANCE, &timerConfig);

    TIMER_interruptConfig(AP_NEARHIBTIME_INSTANCE, TIMER_MATCH0_SELECT, TIMER_INTERRUPT_LEVEL);
    TIMER_interruptConfig(AP_NEARHIBTIME_INSTANCE, TIMER_MATCH1_SELECT, TIMER_INTERRUPT_DISABLE);
    TIMER_interruptConfig(AP_NEARHIBTIME_INSTANCE, TIMER_MATCH2_SELECT, TIMER_INTERRUPT_DISABLE);

    // Enable TIMER IRQ
    XIC_SetVector(AP_NEARHIBTIME_IRQ, apmuPeriWFITimerExp);
    XIC_EnableIRQ(AP_NEARHIBTIME_IRQ);

    TIMER_start(AP_NEARHIBTIME_INSTANCE);
}

void apmuPeriDeleteWFITimer(void)
{
    TIMER_stop(AP_NEARHIBTIME_INSTANCE);
    TIMER_deInit(AP_NEARHIBTIME_INSTANCE);
}

void apmuPeriDeleteCPTimer(void)
{
    TIMER_stop(CP_STARTTIME_INSTANCE);
    TIMER_deInit(CP_STARTTIME_INSTANCE);
}

void apmuPeriClearCPTimerInterrupt(void)
{
    if (TIMER_getInterruptFlags(CP_STARTTIME_INSTANCE) & TIMER_MATCH0_INTERRUPT_FLAG)
    {
        TIMER_clearInterruptFlags(CP_STARTTIME_INSTANCE, TIMER_MATCH0_INTERRUPT_FLAG);
    }
}

void apmuPeriStartCPTimer(uint32_t cpStartTime, void* expFunc)
{
    int32_t clkRet1,clkRet2;

    CLOCK_clockReset(FCLK_TIMER5);      // to protect setClock error, can not reproduce easily

    //  TIMER config
    //  Config TIMER clock, source from 26MHz and divide by 1
    clkRet1 = CLOCK_setClockSrc(FCLK_TIMER5, FCLK_TIMER5_SEL_26M);
    clkRet2 = CLOCK_setClockDiv(FCLK_TIMER5, 26);

    if((clkRet1 != 0) || (clkRet2 != 0))
    {
        ECPLAT_PRINTF(UNILOG_PMU, apmuPeriStartCPTimer_0, P_ERROR, "clkRet1=%d, clkRet2=%d, CLKFREQ=0x%x", clkRet1, clkRet2, CLOCK_getClockFreq(FCLK_TIMER5));
        EC_ASSERT(0, clkRet1, clkRet2, CLOCK_getClockFreq(FCLK_TIMER5));
    }
    TimerConfig_t timerConfig;
    TIMER_getDefaultConfig(&timerConfig);
    timerConfig.reloadOption = TIMER_RELOAD_ON_MATCH0;

    if(cpStartTime > CP_STARTTIME_MAXIMUM)
    {
        cpStartTime = CP_STARTTIME_MAXIMUM;
    }
    timerConfig.match0 = (cpStartTime-1)*1000;

    ECPLAT_PRINTF(UNILOG_PMU, apmuPeriStartCPTimer_1, P_VALUE, "apmuCreateModemStartTimer set to %u ms", (cpStartTime-1));

    TIMER_init(CP_STARTTIME_INSTANCE, &timerConfig);

    TIMER_interruptConfig(CP_STARTTIME_INSTANCE, TIMER_MATCH0_SELECT, TIMER_INTERRUPT_LEVEL);
    TIMER_interruptConfig(CP_STARTTIME_INSTANCE, TIMER_MATCH1_SELECT, TIMER_INTERRUPT_DISABLE);
    TIMER_interruptConfig(CP_STARTTIME_INSTANCE, TIMER_MATCH2_SELECT, TIMER_INTERRUPT_DISABLE);

    // Enable TIMER IRQ
    XIC_SetVector(CP_STARTTIME_IRQ, expFunc);
    XIC_EnableIRQ(CP_STARTTIME_IRQ);
    TIMER_start(CP_STARTTIME_INSTANCE);


}


#ifdef __USER_CODE__	//放在其他地方了
#else
void apmuPeriCPPowerOnSetIrqMask(void)
{
    if(getCPWakeupType())
    {
        // mask uart irq only
        cpPowerOnUartMaskMode = true;
        XIC_DisableAllIRQExceptUart();
    }
    else
    {
        cpPowerOnMask = SaveAndSetIRQMask();
        cpPowerOnUartMaskMode = false;
    }
}


void apmuPeriCPPowerOnSetIrqRestore(void)
{
    if(getCPWakeupType() || (cpPowerOnUartMaskMode == true))
    {
        // restore uart irq
        cpPowerOnUartMaskMode = false;
        XIC_RestoreAllIRQ();
    }
    else if(cpPowerOnUartMaskMode == false)
    {
        RestoreIRQMask(cpPowerOnMask);
    }
}

#endif

