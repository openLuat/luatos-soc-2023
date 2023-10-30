#include "ec7xx.h"
#include "clock.h"
#include "platform_define.h"
#include "timer.h"
#include "bsp_common.h"
#include "driver_gpio.h"
#include "luat_debug.h"
#include "luat_pwm.h"

#define EIGEN_TIMER(n)             ((TIMER_TypeDef *) (AP_TIMER0_BASE_ADDR + 0x1000*n))
#define PWM_CH_MAX (5)

typedef struct
{
	uint8_t gpio;
	uint8_t gpio_alt;
}luat_pwm_ec618_pin_t;

typedef struct
{
	const int irq_line;
	const luat_pwm_ec618_pin_t pin[5];
	const ClockId_e id;
	const ClockSelect_e select;
	CBFuncEx_t callback;
	void *user_param;

	uint32_t pulse_total_num;
	uint32_t pulse_cnt;
	uint32_t freq;
	uint32_t last_pulse_rate;
	uint8_t update_period;
}luat_pwm_ctrl_t;

static luat_pwm_ctrl_t g_s_pwm_table[PWM_CH_MAX] =
{
		{
				PXIC0_TIMER0_IRQn,
				{
						{HAL_GPIO_23, 5}, {HAL_GPIO_1, 5}, {HAL_GPIO_18, 4}, {HAL_GPIO_14, 5}, {HAL_GPIO_29, 0}
				},
				FCLK_TIMER0,
				FCLK_TIMER0_SEL_26M,
				NULL,
				NULL,
				0,0,0,0
		},
		{
				PXIC0_TIMER1_IRQn,
				{
						{HAL_GPIO_24, 5}, {HAL_GPIO_2, 5}, {HAL_GPIO_30, 5}, {HAL_GPIO_19, 5}, {HAL_GPIO_15, 5}
				},
				FCLK_TIMER1,
				FCLK_TIMER1_SEL_26M,
				NULL,
				NULL,
				0,0,0,0
		},
		{
				PXIC0_TIMER2_IRQn,
				{
						{HAL_GPIO_25, 0}, {HAL_GPIO_3, 0}, {HAL_GPIO_31, 0}, {0xff, 0}, {0xff, 0}
				},
				FCLK_TIMER2,
				FCLK_TIMER2_SEL_26M,
				NULL,
				NULL,
				0,0,0,0
		},
		{
				-1,
				{
						{0xff, 0}, {0xff, 0}, {0xff, 0}, {0xff, 0}, {0xff, 0}
				},
				0,0,
				NULL,
				NULL,
				0,0,0,0
		},
		{
				PXIC0_TIMER4_IRQn,
				{
						{HAL_GPIO_33, 5}, {HAL_GPIO_21, 5}, {HAL_GPIO_27, 5}, {0xff, 0}, {0xff, 0}
				},
				FCLK_TIMER4,
				FCLK_TIMER4_SEL_26M,
				NULL,
				NULL,
				0,0,0,0
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
	uint8_t instance = channel % 10;
	uint8_t alt = channel / 10;
	if (alt > 5) return -1;
	if ((instance >= PWM_CH_MAX) || (instance == 3) || g_s_pwm_table[instance].freq) return -1;
	g_s_pwm_table[instance].callback = callback;
	g_s_pwm_table[instance].user_param = param;
	return 0;
}


int luat_pwm_open(int channel, size_t freq,  size_t pulse, int pnum) {
	uint8_t instance = channel % 10;
	uint8_t alt = channel / 10;
	if (alt > 5) return -1;
	if ((instance >= PWM_CH_MAX) || (instance == 3) || (freq > 13000000) || (g_s_pwm_table[instance].pin[alt].gpio == 0xff)) return -1;
	CLOCK_setClockSrc(g_s_pwm_table[instance].id, g_s_pwm_table[instance].select);
	CLOCK_setClockDiv(g_s_pwm_table[instance].id, 1);
	XIC_DisableIRQ(g_s_pwm_table[instance].irq_line);
	TIMER_stop(instance);
	TIMER_deInit(instance);
	TimerConfig_t config;
	TIMER_getDefaultConfig(&config);
	TIMER_init(instance, &config);
	if (pulse > 1000) pulse = 1000;
	g_s_pwm_table[instance].freq = freq;
	g_s_pwm_table[instance].last_pulse_rate = pulse;
	g_s_pwm_table[instance].pulse_total_num = pnum;
	g_s_pwm_table[instance].pulse_cnt = 0;
	uint32_t period = 26000000 / freq;
	uint64_t temp = period;
	temp *= pulse;
	uint32_t low_cnt = period - temp / 1000;
	/* Set PWM period */
	EIGEN_TIMER(instance)->TMR[1] = period - 1;
	switch(pulse)
	{
	case 0:
		EIGEN_TIMER(instance)->TMR[0] = period;
		break;
	case 1000:
		EIGEN_TIMER(instance)->TMR[0] = period - 1;
		break;
	default:
		EIGEN_TIMER(instance)->TMR[0] = low_cnt?(low_cnt - 1):0;
		break;
	}

	EIGEN_TIMER(instance)->TIVR = 0;

	/* Enable PWM out */
	EIGEN_TIMER(instance)->TCTLR =  (EIGEN_VAL2FLD(TIMER_TCTLR_MODE, TIMER_INTERNAL_CLOCK) ) | (EIGEN_VAL2FLD(TIMER_TCTLR_PWM_STOP_VALUE, TIMER_PWM_STOP_HOLD)) | ((2 << TIMER_TCTLR_MCS_Pos) | TIMER_TCTLR_PWMOUT_Msk);

    if(0 != pnum)
    {
        uint32_t registerValue = EIGEN_TIMER(instance)->TCTLR;
        registerValue |= TIMER_TCTLR_IE_1_Msk;
        registerValue &= ~TIMER_TCTLR_IT_1_Msk;
    	EIGEN_TIMER(instance)->TCTLR |= registerValue;
        switch(instance)
        {
        case 0:
        	XIC_SetVector(g_s_pwm_table[instance].irq_line,timer0_isr);
        	break;
        case 1:
        	XIC_SetVector(g_s_pwm_table[instance].irq_line,timer1_isr);
        	break;
        case 2:
        	XIC_SetVector(g_s_pwm_table[instance].irq_line,timer2_isr);
        	break;
        case 4:
        	XIC_SetVector(g_s_pwm_table[instance].irq_line,timer4_isr);
        	break;
        }
        XIC_ClearPendingIRQ(g_s_pwm_table[instance].irq_line);
        XIC_EnableIRQ(g_s_pwm_table[instance].irq_line);
        XIC_SuppressOvfIRQ(g_s_pwm_table[instance].irq_line);
    }
//    DBG("%x,%x,%x,%x", EIGEN_TIMER(instance)->TCTLR, EIGEN_TIMER(instance)->TMR[0], EIGEN_TIMER(instance)->TMR[1], EIGEN_TIMER(instance)->TMR[2]);
    // if (pulse >= 1000)
    // {
    // 	GPIO_Config(g_s_pwm_table[instance].pin[alt].gpio, 0, 1);
    // 	GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(g_s_pwm_table[instance].pin[alt].gpio, g_s_pwm_table[instance].pin[alt].gpio_alt), g_s_pwm_table[instance].pin[alt].gpio_alt, 0, 0);
    // 	TIMER_start(instance);
    // 	delay_us(100);
    // 	GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(g_s_pwm_table[instance].pin[alt].gpio, g_s_pwm_table[instance].pin[alt].gpio_alt), 5, 1, 0);
    // 	return 0;
    // }
    GPIO_IomuxEC7XX(GPIO_ToPadEC7XX(g_s_pwm_table[instance].pin[alt].gpio, g_s_pwm_table[instance].pin[alt].gpio_alt), 5, 1, 0);
    TIMER_start(instance);
    return 0;
}

int luat_pwm_update_dutycycle(int channel,size_t pulse)
{
	uint8_t instance = channel % 10;
	uint8_t alt = channel / 10;
	if (alt > 5) return -1;
	if ((instance >= PWM_CH_MAX) || (instance == 3)) return -1;
	uint32_t period = 26000000 / g_s_pwm_table[instance].freq;
	uint64_t temp = period;
	temp *= pulse;
	uint32_t low_cnt = period - temp / 1000;
	if (g_s_pwm_table[instance].update_period)
	{
		while (EIGEN_TIMER(instance)->TACR > 5) {;}
		EIGEN_TIMER(instance)->TMR[1] = period - 1;
	}
	//DBG("%u,%u,%u,%u,%u", pulse, g_s_pwm_table[instance].last_pulse_rate, low_cnt, EIGEN_TIMER(instance)->TMR[0], EIGEN_TIMER(instance)->TCAR);
	if (pulse && g_s_pwm_table[instance].last_pulse_rate && (g_s_pwm_table[instance].last_pulse_rate != 1000) && (low_cnt < EIGEN_TIMER(instance)->TMR[0]))
	{
		while (EIGEN_TIMER(instance)->TACR <= EIGEN_TIMER(instance)->TMR[0]) {;}
	}
	switch(pulse)
	{
	case 0:
		EIGEN_TIMER(instance)->TMR[0] = period;
		break;
	case 1000:
		EIGEN_TIMER(instance)->TMR[0] = period - 1;
		break;
	default:
		EIGEN_TIMER(instance)->TMR[0] = low_cnt;
		break;
	}
	g_s_pwm_table[instance].last_pulse_rate = pulse;
    return 0;
}

int luat_pwm_setup(luat_pwm_conf_t* conf)
{
    int channel = conf->channel;
	uint8_t instance = channel % 10;
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
	if (conf->pulse && g_s_pwm_table[instance].freq)
	{
		if (!conf->pnum && !g_s_pwm_table[instance].pulse_total_num)
		{
			if ((g_s_pwm_table[instance].freq == conf->period) && (g_s_pwm_table[instance].last_pulse_rate == conf->pulse))
			{
//				DBG("same pwm, no change");
				return 0;
			}
//			DBG("update pwm period %u->%u rate %u->%u", g_s_pwm_table[instance].freq, conf->period, g_s_pwm_table[instance].last_pulse_rate, conf->pulse);
			if (g_s_pwm_table[instance].freq != conf->period)
			{
				g_s_pwm_table[instance].update_period = 1;
				g_s_pwm_table[instance].freq = conf->period;
			}
			luat_pwm_update_dutycycle(channel, conf->pulse);
			g_s_pwm_table[instance].update_period = 0;
			return 0;
		}
	}

    return luat_pwm_open(conf->channel,conf->period,conf->pulse,conf->pnum);
}

int luat_pwm_capture(int channel,int freq)
{
	return -1;
}

int luat_pwm_close(int channel)
{
	uint8_t instance = channel % 10;
	uint8_t alt = channel / 10;
	if ((instance >= PWM_CH_MAX) || (instance == 3)) return -1;
	XIC_DisableIRQ(g_s_pwm_table[instance].irq_line);
	TIMER_stop(instance);
	TIMER_deInit(instance);
	g_s_pwm_table[instance].freq = 0;
	g_s_pwm_table[instance].pulse_total_num = 0;
	g_s_pwm_table[instance].pulse_cnt = 0;
    return 0;
}
