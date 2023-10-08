#ifndef  LCDCOMM_H
#define  LCDCOMM_H

#ifdef __cplusplus
extern "C" {
#endif
#include "stdio.h"
#include "string.h"
#include "ec7xx.h"
#include "bsp.h"
#include "lspi.h"

// The sequences of pins in this enum should be fixed
typedef enum
{
    LCD_RST_PIN,            
    LCD_BACK_LIGHT_PIN,
    LCD_DS_PIN,
    LCD_CS_PIN,
    LCD_GPIO_PIN_NUM,
}lcdGpioPinType_e;

typedef struct
{
    uint16_t            id;             ///< every id points to a series of gpio pins
    lcdGpioPinType_e    lcdGpioPinType;
}lcdGpioPinInfo_t;

typedef struct _lcdDev_t lcdDev_t;


void lcdWriteCmd(uint8_t cmd);
void lcdWriteData(uint8_t data);
void mDelay(uint32_t mDelay);
void lcdGpioInit(lcdDev_t *lcd);



#ifdef __cplusplus
}
#endif
#endif /* LCDCOMM_H */

