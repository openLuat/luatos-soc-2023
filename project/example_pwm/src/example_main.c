#include "common_api.h"
#include "luat_rtos.h"
#include "luat_debug.h"

#include "luat_pwm.h"

/*
    1.PWM的3、5通道不能使用
    2.PWM引脚复用使用说明https://doc.openluat.com/wiki/37?wiki_page_id=4785
*/
luat_rtos_task_handle pwm_task_handle;

static int32_t pwm_test_callback(void *pdata, void *param)
{
	LUAT_DEBUG_PRINT("pwm done!");
}

static void task_test_pwm(void *param)
{
	/*
		出现异常后默认为死机重启
		demo这里设置为LUAT_DEBUG_FAULT_HANG_RESET出现异常后尝试上传死机信息给PC工具，上传成功或者超时后重启
		如果为了方便调试，可以设置为LUAT_DEBUG_FAULT_HANG，出现异常后死机不重启
		但量产出货一定要设置为出现异常重启！！！！！！！！！1
	*/
	luat_debug_set_fault_mode(LUAT_DEBUG_FAULT_HANG_RESET);
	luat_rtos_task_sleep(2000);

	uint8_t channel = 4;
	luat_pwm_set_callback(channel, pwm_test_callback, NULL);
	//测试13M, 50%占空比连续输出，看示波器
	LUAT_DEBUG_PRINT("测试13MHz, 50占空比连续输出，看示波器");
	luat_pwm_open(channel, 13000000, 500, 0);
	luat_rtos_task_sleep(10000);
	LUAT_DEBUG_PRINT("测试1Hz, 50占空比连续输出，输出10个波形停止");
	luat_pwm_open(channel, 1, 500, 10);
	luat_rtos_task_sleep(20000);
	LUAT_DEBUG_PRINT("测试26KHz, 连续输出，占空比每5秒增加1，从0循环到100");
	luat_pwm_open(channel, 26000, 0, 0);
	uint32_t pulse_rate = 0;
    while(1)
	{
        luat_rtos_task_sleep(5000);
        pulse_rate += 10;
        if (pulse_rate > 1000)
        {
        	pulse_rate = 0;
        }
        LUAT_DEBUG_PRINT("当前占空比%u", pulse_rate/10);
        luat_pwm_update_dutycycle(channel, pulse_rate);
	}
    
}

static void task_demo_pwm(void)
{
    luat_rtos_task_create(&pwm_task_handle, 2048, 20, "pwm", task_test_pwm, NULL, 0);
}

INIT_TASK_EXPORT(task_demo_pwm,"1");
