#include "common_api.h"
#include "luat_rtos.h" //luat 头文件 封装FreeRTOS
#include "luat_debug.h"//luat DBUG 库
#include "luat_gpio.h"//luat GPIO 库
#include "platform_define.h"
#include "pad.h"

#ifndef CHIP_EC716
#define GPIO_LED_PIN HAL_GPIO_27
#else
#define GPIO_LED_PIN HAL_GPIO_20
#endif

/********************************************GPIO 点亮led GPIO27 start*******************************************/
void net_led_init(void)
{
    luat_gpio_cfg_t Net_led_struct;
    luat_gpio_set_default_cfg(&Net_led_struct);
    Net_led_struct.pin=GPIO_LED_PIN;
    Net_led_struct.pull=Luat_GPIO_PULLDOWN;
    Net_led_struct.mode=Luat_GPIO_OUTPUT;
    Net_led_struct.output_level=Luat_GPIO_LOW;
    luat_gpio_open(&Net_led_struct);
}

static void net_led_task(void *param)
{
    net_led_init();
    while (1)
    {
        luat_gpio_set(GPIO_LED_PIN,0);
        luat_rtos_task_sleep(500);
        luat_gpio_set(GPIO_LED_PIN,1);
        luat_rtos_task_sleep(500);
    }
    
}

void net_led_demo(void)
{
    luat_rtos_task_handle net_led_task_handler;
    luat_rtos_task_create(&net_led_task_handler,2*1024,50,"net_led_task",net_led_task,NULL,0);
}
/********************************************GPIO 点亮led GPIO27 end *******************************************/

/********************************************GPIO 复用 功能演示 start*******************************************/

//GPIO 18 19 原本是I2C 接口（780P） 这里复用为GPIO
// //GPIO 18 19 复用对应的是ALT_Func4 

void gpio_fun_task_init(void)
{
    luat_gpio_cfg_t gpio_fun_struct;
    luat_gpio_set_default_cfg(&gpio_fun_struct);
    gpio_fun_struct.pin=HAL_GPIO_18;
    gpio_fun_struct.pull=Luat_GPIO_PULLDOWN;
    gpio_fun_struct.mode=Luat_GPIO_OUTPUT;
    gpio_fun_struct.alt_fun=PAD_MUX_ALT4;
    gpio_fun_struct.output_level=Luat_GPIO_LOW;
    luat_gpio_open(&gpio_fun_struct);
    gpio_fun_struct.pin=HAL_GPIO_19;
    gpio_fun_struct.pull=Luat_GPIO_PULLDOWN;
    gpio_fun_struct.mode=Luat_GPIO_OUTPUT;
    gpio_fun_struct.alt_fun=PAD_MUX_ALT4;
    gpio_fun_struct.output_level=Luat_GPIO_LOW;
    luat_gpio_open(&gpio_fun_struct);
}

static void gpio_fun_task(void *param)
{
    gpio_fun_task_init();
    while (1)
    {
        luat_gpio_set(HAL_GPIO_18,0);
        luat_gpio_set(HAL_GPIO_19,1);
        luat_rtos_task_sleep(500);
        luat_gpio_set(HAL_GPIO_19,0);
        luat_gpio_set(HAL_GPIO_18,1);
        luat_rtos_task_sleep(500);
    }
    
}

void gpio_fun_demo(void)
{
    luat_rtos_task_handle gpio_fun_task_handler;
    luat_rtos_task_create(&gpio_fun_task_handler,4*1024,50,"gpio_fun_task",gpio_fun_task,NULL,0);
}

/********************************************GPIO 复用 功能演示 end *******************************************/

INIT_TASK_EXPORT(net_led_demo,"1");
INIT_TASK_EXPORT(gpio_fun_demo,"2");
