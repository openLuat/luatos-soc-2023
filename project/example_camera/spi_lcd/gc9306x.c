#include "spi_lcd_common.h"
#include "luat_gpio.h"
#include "luat_spi.h"
static const uint8_t data_for_cmd_f0[] = {0x02, 0x00, 0x00, 0x1b, 0x1f, 0x0b};
static const uint8_t data_for_cmd_f1[] = {0x01, 0x03, 0x00, 0x28, 0x2b, 0x0e};
static const uint8_t data_for_cmd_f2[] = {0x0b, 0x08, 0x34, 0x04, 0x03, 0x4c};
static const uint8_t data_for_cmd_f3[] = {0x0e, 0x07, 0x46, 0x04, 0x05, 0x51};
static const uint8_t data_for_cmd_f4[] = {0x08, 0x15, 0x15, 0x1f, 0x22, 0x0f};
static const uint8_t data_for_cmd_f5[] = {0x0b, 0x13, 0x11, 0x1f, 0x21, 0x0f};

static const spi_lcd_cmd_t cmd_list[] =
{
		{
				.cmd = 0xfe,
				.len = 0,
				.queue = NULL,

		},
		{
				.cmd = 0xef,
				.len = 0,
				.queue = NULL,
		},
		{
				.cmd = 0x3a,
				.len = 1,
				.buf = {0x05},
		},
		{
				.cmd = 0xad,
				.len = 1,
				.buf = {0x33},
		},
		{
				.cmd = 0xaf,
				.len = 1,
				.buf = {0x55},
		},
		{
				.cmd = 0xae,
				.len = 1,
				.buf = {0x2b},
		},
		{
				.cmd = 0xa4,
				.len = 2,
				.buf = {0x44,0x44},
		},
		{
				.cmd = 0xa5,
				.len = 2,
				.buf = {0x42,0x42},
		},
		{
				.cmd = 0xaa,
				.len = 2,
				.buf = {0x88,0x88},
		},
		{
				.cmd = 0xae,
				.len = 1,
				.buf = {0x2b},
		},
		{
				.cmd = 0xe8,
				.len = 2,
				.buf = {0x11,0x0b},
		},
		{
				.cmd = 0xe3,
				.len = 2,
				.buf = {0x01,0x10},
		},
		{
				.cmd = 0xff,
				.len = 1,
				.buf = {0x61},
		},
		{
				.cmd = 0xac,
				.len = 1,
				.buf = {0},
		},
		{
				.cmd = 0xaf,
				.len = 1,
				.buf = {0x67},
		},
		{
				.cmd = 0xa6,
				.len = 2,
				.buf = {0x2a,0x2a},
		},
		{
				.cmd = 0xa7,
				.len = 2,
				.buf = {0x2b,0x2b},
		},
		{
				.cmd = 0xa8,
				.len = 2,
				.buf = {0x18,0x18},
		},
		{
				.cmd = 0xa9,
				.len = 2,
				.buf = {0x2a,0x2a},
		},
		{
				.cmd = 0xf0,
				.len = sizeof(data_for_cmd_f0),
				.queue = data_for_cmd_f0,
		},
		{
				.cmd = 0xf1,
				.len = sizeof(data_for_cmd_f1),
				.queue = data_for_cmd_f1,
		},
		{
				.cmd = 0xf2,
				.len = sizeof(data_for_cmd_f2),
				.queue = data_for_cmd_f2,
		},
		{
				.cmd = 0xf3,
				.len = sizeof(data_for_cmd_f3),
				.queue = data_for_cmd_f3,
		},
		{
				.cmd = 0xf4,
				.len = sizeof(data_for_cmd_f4),
				.queue = data_for_cmd_f4,
		},
		{
				.cmd = 0xf5,
				.len = sizeof(data_for_cmd_f5),
				.queue = data_for_cmd_f5,
		},

};

void spi_lcd_driver_gc9306x_init(spi_lcd_ctrl_t *spi_lcd)
{
	int i;
	uint8_t temp[4] = {0x48,0xE8,0x28,0xf8};
	luat_gpio_set(spi_lcd->blk_pin, LUAT_GPIO_LOW);
	luat_gpio_set(spi_lcd->rst_pin, LUAT_GPIO_LOW);
	luat_rtos_task_sleep(10);
	luat_gpio_set(spi_lcd->rst_pin, LUAT_GPIO_HIGH);
	luat_rtos_task_sleep(10);
	for(i = 0; i < sizeof(cmd_list)/sizeof(spi_lcd_cmd_t); i++)
	{
		if (cmd_list[i].len > 4)
		{
			spi_lcd_write(spi_lcd, cmd_list[i].cmd, cmd_list[i].queue, cmd_list[i].len);
		}
		else
		{
			spi_lcd_write(spi_lcd, cmd_list[i].cmd, cmd_list[i].buf, cmd_list[i].len);
		}
	}
	if (spi_lcd->direction >= 4)
	{
		spi_lcd->direction = 0;
	}
	spi_lcd_write(spi_lcd, SPI_LCD_CMD_MEMORY_ACCESS_CONTROL, &temp[spi_lcd->direction], 1);

}
