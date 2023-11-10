#include "ec7xx.h"
#include "clock.h"
#include "platform_define.h"
#include "timer.h"
#include "bsp_common.h"
#include "driver_gpio.h"
#include "luat_debug.h"
#include "luat_pwm.h"
#include "luat_mcu.h"

#define EIGEN_TIMER(n)             ((TIMER_TypeDef *) (AP_TIMER0_BASE_ADDR + 0x1000*n))
#define PWM_CH_MAX (5)

typedef struct
{
	const int irq_line;
	const ClockId_e id;
	const ClockSelect_e select;
	CBFuncEx_t callback;
	void *user_param;

	uint32_t pulse_total_num;
	uint32_t pulse_cnt;
	uint32_t freq;
	uint32_t last_pulse_rate;
	uint8_t update_period;
	uint8_t reverse;
	int stop_level;
}luat_pwm_ctrl_t;

static luat_pwm_ctrl_t g_s_pwm_table[PWM_CH_MAX] =
{
		{
				PXIC0_TIMER0_IRQn,
				FCLK_TIMER0,
				FCLK_TIMER0_SEL_26M,
				NULL,
				NULL,
				0,0,0,0,0,0
		},
		{
				PXIC0_TIMER1_IRQn,
				FCLK_TIMER1,
				FCLK_TIMER1_SEL_26M,
				NULL,
				NULL,
				0,0,0,0,0,0
		},
		{
				PXIC0_TIMER2_IRQn,
				FCLK_TIMER2,
				FCLK_TIMER2_SEL_26M,
				NULL,
				NULL,
				0,0,0,0,0,0
		},
		{
				-1,
				0,0,
				NULL,
				NULL,
				0,0,0,0,0,0
		},
		{
				PXIC0_TIMER4_IRQn,
				FCLK_TIMER4,
				FCLK_TIMER4_SEL_26M,
				NULL,
				NULL,
				0,0,0,0,0,0
		},
};

__ISR_IN_RAM__ static void s_timer_handle(uint32_t instance)
{
    if (EIGEN_TIMER(instance)->TSR & TIMER_MATCH1_INTERRUPT_FLAG)
    {
    	EIGEN_TIMER(instance)->TSR = TIMER_MATCH1_INTERRUPT_FLAG;
    	g_s_pwm_table[instance].pulse_cnt++;
        if (g_s_pwm_table[instance].pulse_total_num && (g_s_pwm_table[instance].pulse_cnt >= g_s_pwm_table[instance].pulse_total_num))
        {
            TIMER_stop(instance);
            if (g_s_pwm_table[instance].callback)
            {
            	g_s_pwm_table[instance].callback(instance, g_s_pwm_table[instance].user_param);
            }
        }
    }
}

__ISR_IN_RAM__ static void timer0_isr(void)
{
	s_timer_handle(0);
}
__ISR_IN_RAM__ static void timer1_isr(void)
{
	s_timer_handle(1);
}
__ISR_IN_RAM__ static void timer2_isr(void)
{
	s_timer_handle(2);
}
__ISR_IN_RAM__ static void timer4_isr(void)
{
	s_timer_handle(4);
}


int luat_pwm_set_callback(int channel, CBFuncEx_t callback, void *param)
{
	if ((channel >= PWM_CH_MAX) || (channel == 3) || g_s_pwm_table[channel].freq) return -1;
	g_s_pwm_table[channel].callback = callback;
	g_s_pwm_table[channel].user_param = param;
	return 0;
}


int luat_pwm_open(int channel, size_t freq,  size_t pulse, int pnum) {
	// uint8_t instance = channel % 10;
	if ((channel >= PWM_CH_MAX) || (channel == 3) || (channel < 0) || (freq > 13000000)) return -1;
	CLOCK_setClockSrc(g_s_pwm_table[channel].id, g_s_pwm_table[channel].select);
	CLOCK_setClockDiv(g_s_pwm_table[channel].id, 1);
	XIC_DisableIRQ(g_s_pwm_table[channel].irq_line);
	TIMER_stop(channel);
	TIMER_deInit(channel);
	TimerConfig_t config;
	TIMER_getDefaultConfig(&config);
	TIMER_init(channel, &config);
	if (pulse > 1000) pulse = 1000;
	g_s_pwm_table[channel].freq = freq;
	g_s_pwm_table[channel].last_pulse_rate = pulse;
	g_s_pwm_table[channel].pulse_total_num = pnum;
	g_s_pwm_table[channel].pulse_cnt = 0;
	uint32_t period = 26000000 / freq;
	uint64_t temp = period;
	temp *= pulse;
	uint32_t low_cnt = period - temp / 1000;
	/* Set PWM period */
	EIGEN_TIMER(channel)->TMR[1] = period - 1;
	switch(pulse)
	{
	case 0:
		EIGEN_TIMER(channel)->TMR[0] = period;
		break;
	case 1000:
		EIGEN_TIMER(channel)->TMR[0] = period - 1;
		break;
	default:
		EIGEN_TIMER(channel)->TMR[0] = low_cnt?(low_cnt - 1):0;
		break;
	}

	EIGEN_TIMER(channel)->TIVR = 0;
	uint8_t stop_level = TIMER_PWM_STOP_HOLD;
	switch (g_s_pwm_table[channel].stop_level)
	{
	case 0:
		stop_level = TIMER_PWM_STOP_HOLD;
		break;
	case 1:
		stop_level = TIMER_PWM_STOP_LOW;
		break;
	case 2:
		stop_level = TIMER_PWM_STOP_HIGH;
		break;
	default:
		break;
	}

	/* Enable PWM out */
	EIGEN_TIMER(channel)->TCTLR =  ((stop_level << TIMER_TCTLR_PWM_STOP_VALUE_Pos) & TIMER_TCTLR_PWM_STOP_VALUE_Msk) | ((2 << TIMER_TCTLR_MCS_Pos) | TIMER_TCTLR_PWMOUT_Msk);

    if(0 != pnum)
    {
        uint32_t registerValue = EIGEN_TIMER(channel)->TCTLR;
        registerValue |= TIMER_TCTLR_IE_1_Msk;
        registerValue &= ~TIMER_TCTLR_IT_1_Msk;
    	EIGEN_TIMER(channel)->TCTLR |= registerValue;
        switch(channel)
        {
        case 0:
        	XIC_SetVector(g_s_pwm_table[channel].irq_line,timer0_isr);
        	break;
        case 1:
        	XIC_SetVector(g_s_pwm_table[channel].irq_line,timer1_isr);
        	break;
        case 2:
        	XIC_SetVector(g_s_pwm_table[channel].irq_line,timer2_isr);
        	break;
        case 4:
        	XIC_SetVector(g_s_pwm_table[channel].irq_line,timer4_isr);
        	break;
        }
        XIC_ClearPendingIRQ(g_s_pwm_table[channel].irq_line);
        XIC_EnableIRQ(g_s_pwm_table[channel].irq_line);
        XIC_SuppressOvfIRQ(g_s_pwm_table[channel].irq_line);
    }
	if(0 == g_s_pwm_table[channel].reverse)
	{
    	if (luat_mcu_iomux_is_default(LUAT_MCU_PERIPHERAL_PWM, channel))
    	{
#if defined TYPE_EC716S
			switch (channel)
        	{
        	case 0:
        	    GPIO_IomuxEC7XX(22, 3, 1, 0);
        	    break;
        	case 1:
        	    GPIO_IomuxEC7XX(23, 3, 1, 0);
        	    break;
        	case 2:
        	    GPIO_IomuxEC7XX(24, 3, 1, 0);
        	    break;
        	case 4:
        	    GPIO_IomuxEC7XX(26, 3, 1, 0);
        	    break;
        	default:
        	    break;
        	}
#else
        	switch (channel)
        	{
        	case 0:
        	    GPIO_IomuxEC7XX(16, 5, 1, 0);
        	    break;
        	case 1:
        	    GPIO_IomuxEC7XX(49, 5, 1, 0);
        	    break;
        	case 2:
        	    GPIO_IomuxEC7XX(50, 5, 1, 0);
        	    break;
        	case 4:
        	    GPIO_IomuxEC7XX(52, 5, 1, 0);
        	    break;
        	default:
        	    break;
        	}
#endif
    	}
	}
    TIMER_start(channel);
    return 0;
}

int luat_pwm_update_dutycycle(int channel,size_t pulse)
{
	if ((channel >= PWM_CH_MAX) || (channel == 3)) return -1;
	uint32_t period = 26000000 / g_s_pwm_table[channel].freq;
	uint64_t temp = period;
	temp *= pulse;
	uint32_t low_cnt = period - temp / 1000;
	if (g_s_pwm_table[channel].update_period)
	{
		while (EIGEN_TIMER(channel)->TACR > 5) {;}
		EIGEN_TIMER(channel)->TMR[1] = period - 1;
	}
	if (pulse && g_s_pwm_table[channel].last_pulse_rate && (g_s_pwm_table[channel].last_pulse_rate != 1000) && (low_cnt < EIGEN_TIMER(channel)->TMR[0]))
	{
		while (EIGEN_TIMER(channel)->TACR <= EIGEN_TIMER(channel)->TMR[0]) {;}
	}
	switch(pulse)
	{
	case 0:
		EIGEN_TIMER(channel)->TMR[0] = period;
		break;
	case 1000:
		EIGEN_TIMER(channel)->TMR[0] = period - 1;
		break;
	default:
		EIGEN_TIMER(channel)->TMR[0] = low_cnt;
		break;
	}
	g_s_pwm_table[channel].last_pulse_rate = pulse;
    return 0;
}

int luat_pwm_setup(luat_pwm_conf_t* conf)
{
    int channel = conf->channel;
    if ((channel >= PWM_CH_MAX) || (channel == 3)) return -1;
	switch(conf->precision)
	{
	case 100:
		conf->pulse *= 10;
		break;
	case 256:
		conf->pulse = (conf->pulse * 1000) >> 8;
		break;
	case 1000:
		break;
	default:
		return -1;
	}
	if (conf->pulse && g_s_pwm_table[channel].freq)
	{
		if (!conf->pnum && !g_s_pwm_table[channel].pulse_total_num)
		{
			if ((g_s_pwm_table[channel].freq == conf->period) && (g_s_pwm_table[channel].last_pulse_rate == conf->pulse))
			{
				return 0;
			}
			if (g_s_pwm_table[channel].freq != conf->period)
			{
				g_s_pwm_table[channel].update_period = 1;
				g_s_pwm_table[channel].freq = conf->period;
			}
			luat_pwm_update_dutycycle(channel, conf->pulse);
			g_s_pwm_table[channel].update_period = 0;
			return 0;
		}
	}
	g_s_pwm_table[channel].reverse = conf->reverse;
	g_s_pwm_table[channel].stop_level = conf->stop_level;
    return luat_pwm_open(conf->channel,conf->period,conf->pulse,conf->pnum);
}

int luat_pwm_capture(int channel,int freq)
{
	return -1;
}

int luat_pwm_close(int channel)
{
	if ((channel >= PWM_CH_MAX) || (channel == 3)) return -1;
	XIC_DisableIRQ(g_s_pwm_table[channel].irq_line);
	TIMER_stop(channel);
	TIMER_deInit(channel);
	g_s_pwm_table[channel].freq = 0;
	g_s_pwm_table[channel].pulse_total_num = 0;
	g_s_pwm_table[channel].pulse_cnt = 0;
    return 0;
}
