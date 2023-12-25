#ifndef  LCDDRV_H
#define  LCDDRV_H

#include "stdio.h"
#include "string.h"
#include "ec7xx.h"
#include "bsp.h"

#include "lspi.h"
#include "lcdDev_7567.h"
#include "lcdDev_7571.h"
#include "lcdDev_7789.h"

#define HEIGHT                  (320)
#define WIDTH                   (240)
#define RED                     (0x001f)//(0xf800)
#define GREEN                   (0x07e0)
#define BLUE                    (0xf800)//(0x001f)
//#define YELLOW                  (0xffe0)
#define WHITE                   (0xffff)
#define BLACK                   (0x0000)
//#define PURPLE                  (0xf81f)

typedef void (*lspiCbEvent_fn) (void);            ///< lspi callback event.


typedef struct
{
    uint32_t        pinInstance : 8;
    uint32_t        pinNum      : 8;
    uint32_t        padAddr     : 8;
    uint32_t        func        : 8;
}lcdPinInfo_t;

typedef struct
{
    lcdPinInfo_t    cs;
    lcdPinInfo_t    ds;
    lcdPinInfo_t    rst;
    lcdPinInfo_t    clk;
    lcdPinInfo_t    mosi;
    lcdPinInfo_t    miso;
}spiPin_t;


typedef struct
{
    uint16_t        id;
    uint32_t        width;
    uint32_t        height;
}lcdDrvPra_t;

typedef enum _lcdBusType_e
{
    BUS_LSPI,
    BUS_SPI,
    BUS_I2C,        ///< OLED will use
}lcdBusType_e;

typedef struct
{
    char            *name;          ///< lcd's name used to configure its id, then use id to find its info, including driver function
    uint16_t        id;             ///< every lcd's id should be different, no matter lcd's type is the same or not
}lcdObj_t;

typedef enum
{
    stopPreview     = 0,     
    startPreview    = 1,
}camPreviewStartStop_e;

typedef struct _lcdDev_t lcdDev_t;

typedef struct 
{
    uint16_t id;

    int (*init)(lcdDev_t *lcd, lspiCbEvent_fn cb);
    int (*drawPoint)(lcdDev_t* lcd, uint16_t x, uint16_t y, uint32_t dataWrite);
    int (*fill)(lcdDev_t* lcd, uint16_t sx, uint16_t ex, uint16_t sy, uint16_t ey, uint8_t *buf, uint32_t dmaTrunkLength);

    int (*prepareDisplay)(lcdDev_t* lcd, uint16_t sx, uint16_t ex, uint16_t sy, uint16_t ey);
    int (*onoff)(lcdDev_t* lcd, uint8_t sta);
    void (*backLight)(lcdDev_t* lcd, uint8_t sta);   
    void (*startStop)(lcdDev_t* lcd, bool startOrStop);
    void (*clear)(lcdDev_t* lcd, uint8_t* buf, uint16_t lcdHeight, uint16_t lcdWidth, uint32_t dmaTrunkLength);
    void (*startStopPreview)(lcdDev_t* lcd, camPreviewStartStop_e previewStartStop);
    //void (*startRamWrite)(lcdDev_t* lcd);
    //void (*addrSet)(lcdDev_t *lcd, uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey);
}lcdDrvFunc_t;

typedef struct _lcdDev_t
{
    int             handle;         ///< lcd descriptor, every opened lcd will return a handle. So need lcd init first, then open it

    lcdObj_t        *obj;           ///< lcd info    
    lcdDrvPra_t     *pra;           ///< lcd parameters
    lcdDrvFunc_t    *drv;           ///< lcd driver functions

    // the parameters that the driver needs
    uint8_t         dir;            ///< vertical: 0;     horizontal: 1;  
    uint16_t        width;          ///< lcd width
    uint16_t        height;         ///< lcd height

    void            *pri;           ///< private parameters that 1-bit screen or oled will use
}lcdDev_t;


/**
  \brief lcd init api, build up the structure of "lcdDev_t".
  \return  status.            
*/
int lcdInit();

/**
  \brief lcd driver api.
  \param[in] name    The name of lcd that need to operate.
  \return  The operated lcd's device information.            
*/
lcdDev_t *lcdOpen(char* name);
void lcdClear(lcdDev_t* lcd, uint8_t* buf, uint16_t lcdHeight, uint16_t lcdWidth, uint32_t dmaTrunkLength);
int lcdFill(lcdDev_t* lcd, uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint8_t* buf, uint32_t dmaTrunkLength);
void st7789StartStop(lcdDev_t* lcd, bool startOrStop);
int lcdDrawPoint(lcdDev_t* lcd, uint16_t x, uint16_t y, uint32_t dataWrite);
void camPreview(lcdDev_t *pdev, camPreviewStartStop_e previewStartStop);


#endif

