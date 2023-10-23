#include "common_api.h"

#include "luat_base.h"
#include "luat_spi.h"
#include "luat_gpio.h"

#include "spi_lcd_common.h"

const uint32_t spi_lcd_list[SPI_LCD_DRIVER_QTY] =
{
		0x00069300,
};

static void spi_lcd_read(uint8_t sda_pin, uint8_t scl_pin, uint8_t cs_pin, uint8_t dc_pin, uint8_t cmd, uint8_t *buf, uint8_t len)
{
	luat_gpio_cfg_t gpio_cfg;
	int i,bit;
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pin = sda_pin;
	luat_gpio_open(&gpio_cfg);
	gpio_cfg.pin = scl_pin;
	luat_gpio_open(&gpio_cfg);
	luat_gpio_set(scl_pin, LUAT_GPIO_HIGH);
	luat_gpio_set(cs_pin, LUAT_GPIO_HIGH);
	luat_gpio_set(dc_pin, LUAT_GPIO_LOW);
	luat_gpio_set(cs_pin, LUAT_GPIO_LOW);
	for(bit = 7; bit >= 0; bit--)
	{
		luat_gpio_set(scl_pin, LUAT_GPIO_LOW);
		luat_gpio_set(sda_pin, cmd & (1 << bit));
		luat_gpio_set(scl_pin, LUAT_GPIO_HIGH);
	}
	luat_gpio_set(dc_pin, LUAT_GPIO_HIGH);
	gpio_cfg.pin = sda_pin;
	gpio_cfg.mode = LUAT_GPIO_INPUT;
	luat_gpio_open(&gpio_cfg);
	for(i = 0; i < len; i++)
	{
		buf[i] = 0;
		for(bit = 7; bit >= 0; bit--)
		{
			luat_gpio_set(scl_pin, LUAT_GPIO_LOW);
			luat_gpio_set(scl_pin, LUAT_GPIO_HIGH);
			if (luat_gpio_get(sda_pin))
			{
				buf[i] |= (1 << bit);
			}
			luat_gpio_set(scl_pin, LUAT_GPIO_LOW);
		}
	}
	luat_gpio_set(cs_pin, LUAT_GPIO_HIGH);
}

int spi_lcd_detect(uint8_t sda_pin, uint8_t scl_pin, uint8_t cs_pin, uint8_t dc_pin, uint8_t rst_pin)
{
	uint32_t i;
//	uint8_t rx_buffer[5] = {0xff};
	PV_Union uID;
	uID.u32 = 0;
	luat_gpio_set(rst_pin, LUAT_GPIO_LOW);
	luat_rtos_task_sleep(10);
	luat_gpio_set(rst_pin, LUAT_GPIO_HIGH);
	luat_rtos_task_sleep(10);
//	spi_lcd_read(sda_pin, scl_pin, cs_pin, dc_pin, SPI_LCD_CMD_READ_STATUS, rx_buffer, 5);
//	DBG("%x,%x,%x,%x", rx_buffer[1], rx_buffer[2], rx_buffer[3], rx_buffer[4]);

	spi_lcd_read(sda_pin, scl_pin, cs_pin, dc_pin, SPI_LCD_CMD_READ_ID1, uID.u8, 1);
	spi_lcd_read(sda_pin, scl_pin, cs_pin, dc_pin, SPI_LCD_CMD_READ_ID2, uID.u8 + 1, 1);
	spi_lcd_read(sda_pin, scl_pin, cs_pin, dc_pin, SPI_LCD_CMD_READ_ID3, uID.u8 + 2, 1);

	for(i = 0; i < SPI_LCD_DRIVER_QTY; i++)
	{
		if (uID.u32 == spi_lcd_list[i])
		{
			return i;
		}
	}
	return -1;
}



void spi_lcd_write(spi_lcd_ctrl_t *spi_lcd, uint8_t cmd, uint8_t *data, uint8_t len)
{
	luat_gpio_set(spi_lcd->cs_pin, LUAT_GPIO_HIGH);
	luat_gpio_set(spi_lcd->dc_pin, LUAT_GPIO_LOW);
	luat_gpio_set(spi_lcd->cs_pin, LUAT_GPIO_LOW);
	luat_spi_send(spi_lcd->spi_id, &cmd, 1);
	luat_gpio_set(spi_lcd->dc_pin, LUAT_GPIO_HIGH);
	luat_spi_send(spi_lcd->spi_id, data, len);
	luat_gpio_set(spi_lcd->cs_pin, LUAT_GPIO_HIGH);
}

void spi_lcd_full_blank(spi_lcd_ctrl_t *spi_lcd)
{
	uint8_t temp[4];
	uint16_t *data = calloc(spi_lcd->w_max * 8, 2);
	uint32_t size = spi_lcd->w_max * 8 * 2;
	BytesPutBe16(temp, 0);
	BytesPutBe16(temp + 2, spi_lcd->w_max - 1);
	spi_lcd_write(spi_lcd, SPI_LCD_CMD_COLUMN_ADDRESS_SET, temp, 4);
	BytesPutBe16(temp, 0);
	BytesPutBe16(temp + 2, spi_lcd->h_max - 1);
	spi_lcd_write(spi_lcd, SPI_LCD_CMD_PAGE_ADDRESS_SET, temp, 4);
	temp[0] = SPI_LCD_CMD_MEMORY_WRITE;
	luat_gpio_set(spi_lcd->dc_pin, LUAT_GPIO_LOW);
	luat_gpio_set(spi_lcd->cs_pin, LUAT_GPIO_LOW);
	luat_spi_send(spi_lcd->spi_id, temp, 1);
	luat_gpio_set(spi_lcd->dc_pin, LUAT_GPIO_HIGH);
	uint16_t pos = 0;
	while(pos < spi_lcd->h_max)
	{
		luat_spi_send(spi_lcd->spi_id, data, size);
		pos+=8;
	}
	free(data);
}

void spi_lcd_init_gc9306x(spi_lcd_ctrl_t *spi_lcd)
{
	spi_lcd_driver_gc9306x_init(spi_lcd);
	spi_lcd_write(spi_lcd, SPI_LCD_CMD_DISPLAY_OFF, NULL, 0);
	spi_lcd_write(spi_lcd, SPI_LCD_CMD_WAKEUP_MODE, NULL, 0);
	spi_lcd_full_blank(spi_lcd);
	spi_lcd_write(spi_lcd, SPI_LCD_CMD_DISPLAY_ON, NULL, 0);
	luat_gpio_set(spi_lcd->blk_pin, LUAT_GPIO_HIGH);
}
