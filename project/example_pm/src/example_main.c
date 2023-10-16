/*
 * Copyright (c) 2022 OpenLuat & AirM2M
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
#include "luat_pm.h"
luat_rtos_task_handle task1_handle;


static void task1(void *args)
{
    while(1)
    {
        luat_pm_request(LUAT_PM_SLEEP_MODE_NONE);
        luat_rtos_task_sleep(10000);
        luat_pm_request(LUAT_PM_SLEEP_MODE_IDLE);
        luat_rtos_task_sleep(10000);
        luat_pm_request(LUAT_PM_SLEEP_MODE_LIGHT);
        luat_rtos_task_sleep(10000);
        luat_mobile_set_flymode(0, 1);
        luat_pm_dtimer_start(0, 10000);
        luat_pm_request(LUAT_PM_SLEEP_MODE_STANDBY);
        luat_rtos_task_sleep(10000);
    }
}


static void luat_example_init(void)
{
    luat_rtos_task_create(&task1_handle, 2*1024, 50, "task1", task1, NULL, 0);
}

INIT_TASK_EXPORT(luat_example_init, "1");