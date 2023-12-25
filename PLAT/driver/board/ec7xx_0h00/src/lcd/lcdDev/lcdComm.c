#include "lcdComm.h"

extern  void delay_us(uint32_t us);

extern lspiDrvInterface_t lspiDrvInterface2;
lspiDrvInterface_t *lspiDrv2 = &lspiDrvInterface2; // We use lspi2 by default



void mDelay(uint32_t mDelay)
{
    delay_us(mDelay * 1000);
}

void lcdWriteData(uint8_t data)
{
    lspiDrv2->prepareSend(data);
}

void lcdWriteCmd(uint8_t cmd)
{
    LSPI_TypeDef* lspi2 = LSPI2;
    lspi2->LSPI_CADDR   = cmd; 
}

uint8_t lcdReadData()
{
    return 0;
}

void lcdGpioInit(lcdDev_t *lcd)
{   
    PadConfig_t config;
    GpioPinConfig_t gpioCfg;

    PAD_getDefaultConfig(&config);

    // ds and cs pins in LSPI are controlled by HW

    // Rst pin
    config.mux = LSPI_RST_PAD_ALT_FUNC;
    PAD_setPinConfig(LSPI_RST_GPIO_ADDR, &config);

    gpioCfg.pinDirection = GPIO_DIRECTION_OUTPUT;
    //gpioCfg.misc.initOutput = 1;
    GPIO_pinConfig(LSPI_RST_GPIO_INSTANCE, LSPI_RST_GPIO_PIN, &gpioCfg);
}


#define RANGE_LIMIT(x) (x > 255 ? 255 : (x < 0 ? 0 : x))
void yuv422ToRgb565(const void* inbuf, void* outbuf, int width, int height)
{
	int rows, cols;
	int y, u, v, r, g, b;
	unsigned char *yuv_buf;
	unsigned short *rgb_buf;
	int y_pos,u_pos,v_pos;

	yuv_buf = (unsigned char *)inbuf;
	rgb_buf = (unsigned short *)outbuf;

	y_pos = 0;
	u_pos = 1;
	v_pos = 3;

	for (rows = 0; rows < height; rows++)
	{
		for (cols = 0; cols < width; cols++) 
		{
			y = yuv_buf[y_pos];
			u = yuv_buf[u_pos] - 128;
			v = yuv_buf[v_pos] - 128;

			// R = Y + 1.402*(V-128)
			// G = Y - 0.34414*(U-128)
			// B = Y + 1.772*(U-128)
			r = RANGE_LIMIT(y + v + ((v * 103) >> 8));
			g = RANGE_LIMIT(y - ((u * 88) >> 8) - ((v * 183) >> 8));
			b = RANGE_LIMIT(y + u + ((u * 198) >> 8));

			*rgb_buf++ = (((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >> 3));

			y_pos += 2;

			if (cols & 0x01) 
			{
				u_pos += 4;
				v_pos += 4;
			}
		}
	}
}

