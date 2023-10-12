#include "lcdDrv.h"

extern lcdDrvFunc_t st7567Drv;
extern lcdDrvFunc_t st7789Drv;
extern lcdDrvFunc_t st7571Drv;


// each lcd's parameters
lcdDrvPra_t st7567Pra =
{
    .id     = 0x7567,
    .width  = WIDTH_7567,
    .height = HEIGHT_7567,
};

lcdDrvPra_t st7789Pra =
{
    .id     = 0x7789,
    .width  = WIDTH_7789,
    .height = HEIGHT_7789,
};

lcdDrvPra_t st7571Pra =
{
    .id     = 0x7571,
    .width  = HEIGHT_7571,
    .height = HEIGHT_7571,
};


// lcd parameter list
lcdDrvPra_t* lcdPraList[]  = 
{
    &st7567Pra,
    &st7789Pra,
    &st7571Pra,
};

// lcd driver function list
lcdDrvFunc_t* lcdDrvList[] = 
{
    &st7567Drv,
    &st7789Drv,
    &st7571Drv,
};

// lcd object list
#define TOTAL_LCD_NUM       1
lcdObj_t lcdObjList[] = 
{
    // name,        id
    //{"st7567",  0x7567}, // Lspi2
    {"st7789",    0x7789}, // Lspi2
    //{"st7571",  0x7571}, // Spi0
};

// lcd device list, need to build up in init function
lcdDev_t lcdDevList[TOTAL_LCD_NUM];


static lcdDrvPra_t* utilFindPra(uint16_t id)
{
    uint8_t praNum = sizeof(lcdPraList) / sizeof(lcdPraList[0]);
    
    for (uint8_t i = 0; i < praNum; i++)
    {
        if (lcdPraList[i]->id == id)
        {
            return lcdPraList[i];
        }
    }

    return NULL;
}

static lcdDrvFunc_t* utilFindDrvFunc(uint16_t id)
{
    uint8_t drvNum = sizeof(lcdDrvList) / sizeof(lcdDrvList[0]);
    
    for (uint8_t i = 0; i < drvNum; i++)
    {
        if (lcdDrvList[i]->id == id)
        {
            return lcdDrvList[i];
        }
    }

    return NULL;
}


// build up the structure of "lcdDev_t" per id
int lcdInit(lspiCbEvent_fn cb)
{
    lcdObj_t *pobj;
    lcdDev_t *pdev;
    int ret = -1;

    for (uint8_t i = 0; i < TOTAL_LCD_NUM; i++)
    {
        pobj = &lcdObjList[i];
        pdev = &lcdDevList[i];

        ASSERT(pobj->id != 0);

        // per id number, find its parameter and driver
        pdev->handle = -99; // if init success, will assign -1
        pdev->obj    = pobj;
        pdev->pra    = utilFindPra(pobj->id);
        pdev->drv    = utilFindDrvFunc(pobj->id);
        
        ASSERT(pdev->pra != NULL);
        ASSERT(pdev->drv != NULL);
        
        ret = pdev->drv->init(pdev, cb); // use found parameter to init lcd

        if (ret == 0)
        {
            pdev->handle = -1; // init success, assign -1 to handle
            pdev->height = pdev->pra->height;
            pdev->width  = pdev->pra->width;

            pdev->drv->backLight(pdev, 1); // open backlight
            pdev->drv->onoff(pdev, 1); // lcd on
        }
    }

    return 0;
}

int magNum =  9987;
// lcd driver need init first, then open it. "open" means assign a random value to the "handle"
lcdDev_t* lcdOpen(char* name)
{
    lcdDev_t *pdev;
    
    for (uint8_t i = 0; i < TOTAL_LCD_NUM; i++)
    {
        pdev = &lcdDevList[i];

        if (strcmp(name, pdev->obj->name) == 0)
        {
            if (pdev->handle == -99)
            {
                printf("This lcd hasn't init first.\n");
                return NULL;
            }

            if (pdev->handle == -1)
            {
                pdev->handle = magNum; // assign a random value to handle, then compare it in the close function
                magNum += 77;

                // successfully find a "lcdDev_t" structure
                return pdev;
            }
        }
    }

    return NULL;
}

void camPreview(lcdDev_t *pdev, camPreviewStartStop_e previewStartStop)
{
    pdev->drv->startStopPreview(pdev, previewStartStop);

}

int lcdClose(lcdDev_t *pdev)
{
    if (pdev->handle < 0)
    {
        return -1;
    }
    else 
    {   
        pdev->handle = -1;
        return 0;
    }
}

int lcdDrawPoint(lcdDev_t* lcd, uint16_t x, uint16_t y, uint32_t dataWrite)
{
    if (lcd == NULL)
    {
        return -1;
    }

    return lcd->drv->drawPoint(lcd, x, y, dataWrite);
}

int lcdPrepareDisplay(lcdDev_t* lcd, uint16_t sx, uint16_t ex, uint16_t sy, uint16_t ey)
{
    if (lcd == NULL)
    {
        return -1;
    }

    return lcd->drv->prepareDisplay(lcd, sx, ex, sy, ey);
}

int lcdOnoff(lcdDev_t* lcd, uint8_t sta)
{
    if (lcd == NULL)
    {
        return -1;
    }

    return lcd->drv->onoff(lcd, sta);
}

int lcdFill(lcdDev_t* lcd, uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint8_t* buf, uint32_t dmaTrunkLength)
{
    if (lcd == NULL)
    {
        return -1;
    }

    return lcd->drv->fill(lcd, sx, sy, ex, ey, buf, dmaTrunkLength);
}

void lcdBackLight(lcdDev_t* lcd, uint8_t sta)
{
    if (lcd == NULL)
    {
        return;
    }

    return lcd->drv->backLight(lcd, sta);
}

void lcdClear(lcdDev_t* lcd, uint8_t* buf, uint16_t lcdHeight, uint16_t lcdWidth, uint32_t dmaTrunkLength)
{
    if (lcd == NULL)
    {
        return;
    }

    return lcd->drv->clear(lcd, buf, lcdHeight, lcdWidth, dmaTrunkLength);
}

void line(lcdDev_t* lcd, uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    uint16_t t;
    int deltaX, deltaY, distance, xerr = 0, yerr = 0;
    int incx, incy, uRow, uCol;
    //uint8_t color1 = color>>8, color2 = color;
    uint32_t dataWrite = (color) | (color<<16);//(color1) | (color2<<8) | (color1<<16) | (color2<<24); 
    
    deltaX = ex - sx;
    deltaY = ey - sy;
    uRow = sx;
    uCol = sy;

    // x
    if (deltaX > 0) incx = 1; // set increase direction
    else if (deltaX == 0) incx = 0; // vertical line
    else {incx = -1; deltaX = -deltaX;}

    // y
    if (deltaY > 0) incy = 1; // set increase direction
    else if (deltaY == 0) incy = 0; // horizontal line
    else {incy = -1; deltaY = -deltaY;}

    //
    if (deltaX > deltaY) distance = deltaX;
    else distance = deltaY;

    //lcd->drv->writeSetup(lcd);
    //lcd->drv->addrSet(lcd)

    // draw line
    for (t = 0; t < distance + 1; t = t + 2)
    {
        lcdDrawPoint(lcd, uRow, uCol, dataWrite);
        xerr += deltaX;
        yerr += deltaY;
        if (xerr > distance)
        {
            xerr -= distance;
            uRow += incx;
        }

        if (yerr > distance)
        {
            yerr -= distance;
            uCol += incy;
        }
    }
    
}


