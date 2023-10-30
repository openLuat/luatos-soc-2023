#include "common_api.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_pwm.h"


luat_rtos_task_handle pwm_task_handle;



static void task_test_pwm(void *param)
{
    luat_pwm_open(0, 8000, 500, 1000);
    luat_pwm_open(1, 8000, 500, 0);
    luat_pwm_open(2, 8000, 500, 0);
    luat_pwm_open(4, 8000, 500, 0);
    size_t pl = 0;
    while (1)
    {
        LUAT_DEBUG_PRINT("test pwm");
        luat_rtos_task_sleep(1000);
    }
    
}

static void task_demo_pwm(void)
{
    luat_rtos_task_create(&pwm_task_handle, 1024, 20, "rtc", task_test_pwm, NULL, 0);
}

INIT_TASK_EXPORT(task_demo_pwm,"1");

