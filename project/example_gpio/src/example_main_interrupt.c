#include "common_api.h"
#include "luat_rtos.h" //luat 头文件 封装FreeRTOS
#include "luat_debug.h"//luat DBUG 库
#include "luat_gpio.h"//luat GPIO 库
#include "platform_define.h"
int led_status=0;
void net_led_init(void)
{
    luat_gpio_cfg_t net_led_struct;
    luat_gpio_set_default_cfg(&net_led_struct);
    net_led_struct.pin=HAL_GPIO_27;
    net_led_struct.pull=Luat_GPIO_PULLDOWN;
    net_led_struct.mode=Luat_GPIO_OUTPUT;
    net_led_struct.output_level=Luat_GPIO_LOW;
    luat_gpio_open(&net_led_struct);
}

int gpio_level_irq(void *data, void* args)
{
	int pin = (int)data;
	LUAT_DEBUG_PRINT("pin:%d, level:%d,", pin, luat_gpio_get(pin));
    if (led_status==0)
    {
        luat_gpio_set(HAL_GPIO_27,1);
        led_status=1;
    }
    else
    {
        luat_gpio_set(HAL_GPIO_27,0);
        led_status=0;
    }
}

void key_init(void)
{
    luat_gpio_cfg_t key_fun_struct;
    luat_gpio_set_default_cfg(&key_fun_struct);
    key_fun_struct.pin=HAL_GPIO_1;
    key_fun_struct.pull=Luat_GPIO_PULLUP;
    key_fun_struct.mode=Luat_GPIO_IRQ;
    key_fun_struct.irq_type=LUAT_GPIO_FALLING_IRQ;
    key_fun_struct.irq_cb=gpio_level_irq;
    luat_gpio_open(&key_fun_struct);
}

static void net_led_task(void *param)
{
    net_led_init();
    key_init();
    while (1)
    {
        luat_rtos_task_sleep(500);
    }
    
}

void net_led_demo(void)
{
    luat_rtos_task_handle net_led_task_handler;
    luat_rtos_task_create(&net_led_task_handler,2*1024,50,"net_led_task",net_led_task,NULL,0);
}


INIT_TASK_EXPORT(net_led_demo,"1");