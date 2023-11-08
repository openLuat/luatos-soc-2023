/*
 * Copyright (c) 2023 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "luat_network_adapter.h"
#include "common_api.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_mobile.h"
#include "luat_pm.h"
#include "luat_gpio.h"
#include "slpman.h"
luat_rtos_task_handle task1_handle;


static void task1(void *args)
{
	luat_gpio_cfg_t gpio_cfg = {0};
    while(1)
    {
        int lastState, rtcOrPad;
        luat_pm_last_state(&lastState, &rtcOrPad);
        if(rtcOrPad > 0 && rtcOrPad != 0xff)        //深度休眠前若进入飞行模式，并且唤醒后需要进行联网操作的，需要退出飞行模式
        {
            luat_mobile_set_flymode(0, 0);
        }

        luat_pm_request(LUAT_PM_SLEEP_MODE_NONE);
        luat_rtos_task_sleep(10000);
        luat_pm_request(LUAT_PM_SLEEP_MODE_IDLE);
        luat_rtos_task_sleep(10000);
        luat_pm_request(LUAT_PM_SLEEP_MODE_LIGHT);
        luat_rtos_task_sleep(10000);
        luat_mobile_set_flymode(0, 1);
        luat_pm_dtimer_start(0, 20000);
        luat_pm_power_ctrl(LUAT_PM_POWER_USB, 0);	//插着USB的时候需要关闭USB电源
        luat_pm_force(LUAT_PM_SLEEP_MODE_STANDBY);
        //WAKEPAD4设置成上拉关闭wakeup功能，在全IO开发板上功耗最低
        gpio_cfg.pin = HAL_WAKEUP_4;
        gpio_cfg.mode = LUAT_GPIO_INPUT;
        gpio_cfg.pull = LUAT_GPIO_PULLUP;
        luat_gpio_open(&gpio_cfg);
        luat_gpio_close(HAL_WAKEUP_PWRKEY);	//如果powerkey接地了，还需要再关闭powerkey上拉功能
        #ifndef CHIP_EC716
        luat_gpio_close(HAL_GPIO_23);	//关闭能省0.5uA
        #endif
        luat_rtos_task_sleep(30000);
    }
}


static void luat_example_init(void)
{
    luat_rtos_task_create(&task1_handle, 2*1024, 50, "task1", task1, NULL, 0);
}

INIT_TASK_EXPORT(luat_example_init, "1");
