#include "lcdDev_7789.h"

extern lspiDrvInterface_t lspiDrvInterface2;
lspiDrvInterface_t *lspiDrv7789 = &lspiDrvInterface2; // We use lspi2 by default

DmaDescriptor_t __ALIGNED(16) lcdDmaTxDesc7789[20]; // here set maximum dma descriptor will be no more then 20
int8_t lcdDmaTxCh7789; // dma tx channel
DmaTransferConfig_t lcdDmaTxCfg7789 =
{
    NULL,
    (void *)&(LSPI2->TFIFO),
    DMA_FLOW_CONTROL_TARGET,
    DMA_ADDRESS_INCREMENT_SOURCE,
    DMA_DATA_WIDTH_ONE_BYTE, // DMA_DATA_WIDTH_ONE_BYTE
    DMA_BURST_8_BYTES, 
    0
};

/* 
    sx: start x
    sy: start y
    ex: end x
    ey: end y
*/
void st7789AddrSet(lcdDev_t *lcd, uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey)
{
	lcdWriteCmd(0x2B); // row addr set, y
	lcdWriteData(sx>>8);
	lcdWriteData(sx);
	lcdWriteData(ex>>8);
	lcdWriteData(ex);
    lspiDrv7789->send(NULL, 0);

	lcdWriteCmd(0x2A); // column addr set, x
	lcdWriteData(sy>>8);
	lcdWriteData(sy);
	lcdWriteData(ey>>8);
	lcdWriteData(ey);
    lspiDrv7789->send(NULL, 0);

	lcdWriteCmd(0x2C);
}

void st7789StartStop(lcdDev_t* lcd, bool startOrStop)
{
    if (startOrStop)
    {
        DMA_loadChannelDescriptorAndRun(DMA_INSTANCE_MP, lcdDmaTxCh7789, lcdDmaTxDesc7789);
    }
    else
    {    
        extern void DMA_stopChannelNoWait(DmaInstance_e instance, uint32_t channel);
        DMA_stopChannelNoWait(DMA_INSTANCE_MP, lcdDmaTxCh7789);
    }
}


void st7789CamPreviewStartStop(lcdDev_t* lcd, camPreviewStartStop_e previewStartStop)
{   
    if (previewStartStop)
    {
        // preview
        lcdWriteCmd(0x36);
        lcdWriteData(0x00);// 0: normal;   0x20: reverse, mirror image   0x40: x mirror
        lspiDrv7789->send(NULL, 0);
        
        lcdWriteCmd(0x2C);
        
        st7789AddrSet(lcd, 40, 0, 240+40-1, 240-1);

        lspiDataFmt.wordSize = 7;
        lspiDataFmt.txPack = 0;
        lspiDrv7789->ctrl(LSPI_CTRL_DATA_FORMAT, 0);
        
        // lcd size
        lspiInfo.frameHeight = 480; // frame input height
        lspiInfo.frameWidth = 640; // frame input width
        lspiDrv7789->ctrl(LSPI_CTRL_FRAME_INFO, 0);
        
        lspiFrameInfoOut.frameHeightOut =240;//lcd->pra->height;//320; // frame output height
        lspiFrameInfoOut.frameWidthOut = 240;//lcd->pra->width;//240; // frame output width
        lspiDrv7789->ctrl(LSPI_CTRL_FRAME_INFO_OUT, 0);

        lspiScaleInfo.rowScaleFrac = 64;
        lspiScaleInfo.colScaleFrac = 64;
        lspiDrv7789->ctrl(LSPI_CTRL_SCALE_INFO, 0);

        lspiTailorInfo.tailorLeft = 80; // frame output height
        lspiTailorInfo.tailorRight = 80; // frame output width
        lspiDrv7789->ctrl(LSPI_CTRL_TAILOR_INFO, 0);

        lspiCtrl.enable = 1;
        lspiCtrl.line4 = 1;
        lspiCtrl.datSrc = 0; // 0: data from camera; 1: data from memory
        lspiCtrl.colorModeIn = 0; // YUV422, every item is 8bit
        lspiCtrl.colorModeOut = 1; // RGB565
        lspiCtrl.busType = 1; // Interface II
        lspiDrv7789->ctrl(LSPI_CTRL_CTRL, 0);

        lspiCmdCtrl.wrRdn = 1; // 1: wr   0: rd
        lspiCmdCtrl.ramWr = 1; // start ramwr
        lspiCmdCtrl.dataLen = 0x3ffff; // infinite data, used in camera to lspi
        lspiDrv7789->ctrl(LSPI_CTRL_CMD_CTRL, 0);
        
        mDelay (120); //Delay 120ms
    }
}

void st7789v2Reginit(lcdDev_t *lcd)
{
    //--------------------------------ST7789V reset sequence------------------------------------//
    LSPI_RST_HIGH;
    mDelay(50); //Delay 100ms
    LSPI_RST_LOW;
    mDelay(150); //Delay 200ms
    LSPI_RST_HIGH;
    mDelay(250); //Delay 500ms

    //-------------------------------Color Mode---------------------------------------------//
    lcdWriteCmd(0x11);
    lspiDrv7789->send(NULL, 0);
    mDelay (120); //Delay 120ms
    
    //--------------------------------Display Setting------------------------------------------//
    lcdWriteCmd(0x36);
    lcdWriteData(0x20);// 0: normal;   0x20: reverse, mirror image   0x40: x mirror
    lspiDrv7789->send(NULL, 0);

    lcdWriteCmd(0x3a);
    lcdWriteData(0x05); //TOOO.....0x5:RGB565;   0x3:RGB444;   0x6:RGB666
    lspiDrv7789->send(NULL, 0);
    
    //--------------------------------ST7789V Frame rate setting----------------------------------//
    lcdWriteCmd(0xb2);
    lcdWriteData(0x0c);
    lcdWriteData(0x0c);
    lcdWriteData(0x00);
    lcdWriteData(0x33);
    lcdWriteData(0x33);
    lspiDrv7789->send(NULL, 0);

    lcdWriteCmd(0xb7);
    lcdWriteData(0x35);
    lspiDrv7789->send(NULL, 0);
    
    //--------------------------------ST7789V Power setting--------------------------------------//
    lcdWriteCmd(0xbb);
    lcdWriteData(0x20);
    lspiDrv7789->send(NULL, 0);

    lcdWriteCmd(0xc0);
    lcdWriteData(0x2c);
    lspiDrv7789->send(NULL, 0);

    lcdWriteCmd(0xc2);
    lcdWriteData(0x01);
    lspiDrv7789->send(NULL, 0);

    lcdWriteCmd(0xc3);
    lcdWriteData(0x0b);
    lspiDrv7789->send(NULL, 0);

    lcdWriteCmd(0xc4);
    lcdWriteData(0x20);
    lspiDrv7789->send(NULL, 0);

    lcdWriteCmd(0xc6);
    lcdWriteData(0x0f);
    lspiDrv7789->send(NULL, 0);

    lcdWriteCmd(0xd0);
    lcdWriteData(0xa4);
    lcdWriteData(0xa1);
    lspiDrv7789->send(NULL, 0);
    
    //--------------------------------ST7789V gamma setting---------------------------------------//
    lcdWriteCmd(0xe0);
    lcdWriteData(0xd0);
    lcdWriteData(0x03);
    lcdWriteData(0x09);
    lcdWriteData(0x0e);
    lcdWriteData(0x11);
    lcdWriteData(0x3d);
    lcdWriteData(0x47);
    lcdWriteData(0x55);
    lcdWriteData(0x53);
    lcdWriteData(0x1a);
    lcdWriteData(0x16);
    lcdWriteData(0x14);
    lcdWriteData(0x1f);
    lcdWriteData(0x22);
    lspiDrv7789->send(NULL, 0);

    lcdWriteCmd(0xe1);
    lcdWriteData(0xd0);
    lcdWriteData(0x02);
    lcdWriteData(0x08);
    lcdWriteData(0x0d);
    lcdWriteData(0x12);
    lcdWriteData(0x2c);
    lcdWriteData(0x43);
    lcdWriteData(0x55);
    lcdWriteData(0x53);
    lcdWriteData(0x1e);
    lcdWriteData(0x1b);
    lcdWriteData(0x19);
    lcdWriteData(0x20);
    lcdWriteData(0x22);
    lspiDrv7789->send(NULL, 0);

    lcdWriteCmd(0x29);
    lspiDrv7789->send(NULL, 0);
    mDelay (120); //Delay 120ms
}


int st7789Init(lcdDev_t *lcd, lspiCbEvent_fn cb)
{   
    lcdGpioInit(lcd);

    lspiDrv7789->init(NULL);
    lspiDrv7789->powerCtrl(LSPI_POWER_FULL);
    lspiDrv7789->ctrl(LSPI_CTRL_BUS_SPEED, 0); // bus speed

    lspiDataFmt.wordSize = 7;
    lspiDrv7789->ctrl(LSPI_CTRL_DATA_FORMAT, 0);
    
    lspiBusSel.lspiBusEn = 1;
    lspiDrv7789->ctrl(LSPI_CTRL_BUS_SEL, 0);
    
    lspiCtrl.enable = 1;
    lspiCtrl.line4 = 1;
    lspiCtrl.datSrc = 0; // 0: data from camera; 1: data from memory
    lspiCtrl.colorModeIn = 0; // YUV422, every item is 8bit
    lspiCtrl.colorModeOut = 1; // RGB565
    lspiCtrl.busType = 1; // Interface II
    lspiDrv7789->ctrl(LSPI_CTRL_CTRL, 0);

    lspiScaleInfo.rowScaleFrac = 64;
    lspiScaleInfo.colScaleFrac = 64;
    lspiDrv7789->ctrl(LSPI_CTRL_SCALE_INFO, 0);
   
    lspiInfo.frameHeight = 480; // frame input height
    lspiInfo.frameWidth = 640; // frame input width
    lspiDrv7789->ctrl(LSPI_CTRL_FRAME_INFO, 0);

    lspiFrameInfoOut.frameHeightOut =320;//320; // frame output height
    lspiFrameInfoOut.frameWidthOut = 240;//240; // frame output width
    lspiDrv7789->ctrl(LSPI_CTRL_FRAME_INFO_OUT, 0);

    lspiIntCtrl.lspiRamWrEndEn = 1;
    lspiDrv7789->ctrl(LSPI_CTRL_INT_CTRL, 0);
    XIC_SetVector(PXIC0_USP2_IRQn, cb);
    XIC_EnableIRQ(PXIC0_USP2_IRQn);	
    
    st7789v2Reginit(lcd);
    st7789AddrSet(lcd, 0, 0, lcd->pra->width-1, lcd->pra->height-1);
    st7789CamPreviewStartStop(lcd, stopPreview);
    

    lspiDmaCtrl.txDmaReqEn = 1;
    lspiDrv7789->ctrl(LSPI_CTRL_DMA_CTRL, 0);

    DMA_init(DMA_INSTANCE_MP);
    lcdDmaTxCh7789 = DMA_openChannel(DMA_INSTANCE_MP);
    DMA_setChannelRequestSource(DMA_INSTANCE_MP, lcdDmaTxCh7789, (DmaRequestSource_e)DMA_REQUEST_USP2_TX);
    DMA_rigisterChannelCallback(DMA_INSTANCE_MP, lcdDmaTxCh7789, NULL); // here dma cb is quicker than frame cb

    return 0;
}

static int st7789DrawPoint(lcdDev_t *lcd, uint16_t x, uint16_t y, uint32_t dataWrite)
{
    LSPI_TypeDef* lspi2 = LSPI2;
    st7789AddrSet(lcd, x, y, x, y);
    //static uint16_t times = 0;

    //if (times == 0)
    {    
        lspiDmaCtrl.txDmaReqEn = 0;
        lspiDrv7789->ctrl(LSPI_CTRL_DMA_CTRL, 0);
    
        lspiDataFmt.wordSize = 15;
        lspiDrv7789->ctrl(LSPI_CTRL_DATA_FORMAT, 0);
            
        lspiDataFmt.txPack = 1;
        lspiDrv7789->ctrl(LSPI_CTRL_DATA_FORMAT, 0);
        
        lspiCtrl.datSrc = 1; // 0: data from camera; 1: data from memory
        lspiCtrl.colorModeIn = 3; // RGB565
        lspiCtrl.colorModeOut = 1; // RGB565
        lspiDrv7789->ctrl(LSPI_CTRL_CTRL, 0);
        
        lspiInfo.frameHeight = 1;//320; // frame input height
        lspiInfo.frameWidth = 1;//240; // frame input width
        lspiDrv7789->ctrl(LSPI_CTRL_FRAME_INFO, 0);
        
        lspiScaleInfo.rowScaleFrac = 0;
        lspiScaleInfo.colScaleFrac = 0;
        lspiDrv7789->ctrl(LSPI_CTRL_SCALE_INFO, 0);

        lspiTailorInfo.tailorLeft = 0; // frame output height
        lspiTailorInfo.tailorRight = 0; // frame output width
        lspiDrv7789->ctrl(LSPI_CTRL_TAILOR_INFO, 0);
        
        lspiFrameInfoOut.frameHeightOut = 1; // frame output height
        lspiFrameInfoOut.frameWidthOut = 1; // frame output width
        lspiDrv7789->ctrl(LSPI_CTRL_FRAME_INFO_OUT, 0);

        //times++;
    }

    
	lspi2->STAS = lspi2->STAS | (0x1<<30); // clear
	//lspi2->TFIFO = dataWrite;

    lspiCmdCtrl.wrRdn = 1; // 1: wr   0: rd
    lspiCmdCtrl.ramWr = 1; // start ramwr
    lspiCmdCtrl.dataLen = 2; // 1 word
    lspiDrv7789->ctrl(LSPI_CTRL_CMD_CTRL, 0);

	// txfifo
	
    //while ((((lspi2->STAS) >> 13) & 0x3f) > 15);
    lspi2->TFIFO = dataWrite;
    //while ((((lspi2->STAS) >> 13) & 0x3f) > 1);
    #if 1
    while (1)
    {
        if ((((lspi2->STAS) >> 30) & 1) == 1)
        {
            lspi2->STAS = lspi2->STAS | (0x1<<30); // clear
            break;
        }
    }
    #endif

    return 0;
}

static int st7789Fill(lcdDev_t* lcd, uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint8_t *buf, uint32_t dmaTrunkLength)
{
    //LSPI_TypeDef* lspi2 = LSPI2;

    uint32_t dmaTotalLength = (ey-sy+1)*(ex-sx+1)*2;
    memset(lcdDmaTxDesc7789, 0, sizeof(lcdDmaTxDesc7789));
    
    st7789AddrSet(lcd, sx, sy, ex, ey);

    //lcdBusType_e bus = lcd->obj->bus;

    //lcdWriteCmd(0x36);
    //lcdWriteData(0x20);// 0: normal;   0x20: reverse
    //lspiDrv7789->send(NULL, 0);

    lspiDmaCtrl.txDmaReqEn = 1;
    lspiDrv7789->ctrl(LSPI_CTRL_DMA_CTRL, 0);

    lspiDataFmt.wordSize = 7;
    lspiDrv7789->ctrl(LSPI_CTRL_DATA_FORMAT, 0);

    lspiDataFmt.txPack = 0;
    lspiDrv7789->ctrl(LSPI_CTRL_DATA_FORMAT, 0);

    lspiCtrl.datSrc = 1; // 0: data from camera; 1: data from memory
    lspiCtrl.colorModeIn = 3; // RGB565
    lspiCtrl.colorModeOut = 1; // RGB565
    lspiDrv7789->ctrl(LSPI_CTRL_CTRL, 0);

    lspiInfo.frameHeight = 320;//1; // frame input height
    lspiInfo.frameWidth = 240;//240; // frame input width
    lspiDrv7789->ctrl(LSPI_CTRL_FRAME_INFO, 0);

    lspiScaleInfo.rowScaleFrac = 0;
    lspiScaleInfo.colScaleFrac = 0;
    lspiDrv7789->ctrl(LSPI_CTRL_SCALE_INFO, 0);
    
    lspiFrameInfoOut.frameHeightOut = 0; // frame output height
    lspiFrameInfoOut.frameWidthOut = 0; // frame output width
    lspiDrv7789->ctrl(LSPI_CTRL_FRAME_INFO_OUT, 0);
       
    //uint32_t dataWrite = (color) | (color<<16);//(color1) | (color2<<8) | (color1<<16) | (color2<<24);  
    lspiCmdCtrl.wrRdn = 1; // 1: wr   0: rd
    lspiCmdCtrl.ramWr = 1; // start ramwr
    lspiCmdCtrl.dataLen = dmaTotalLength;
    lspiDrv7789->ctrl(LSPI_CTRL_CMD_CTRL, 0);

    // build dma descriptor chain
    lcdDmaTxCfg7789.sourceAddress = buf;

    if (dmaTotalLength < dmaTrunkLength)
    {
        lcdDmaTxCfg7789.totalLength   = dmaTotalLength; 
        DMA_buildDescriptorChain(lcdDmaTxDesc7789, &lcdDmaTxCfg7789, 1, true, true, true);
    }
    else
    {
        lcdDmaTxCfg7789.totalLength   = dmaTrunkLength; 
        // during building chain
        DMA_buildDescriptorChain(lcdDmaTxDesc7789, &lcdDmaTxCfg7789, dmaTotalLength/dmaTrunkLength, false, false, false);

        // build the last chain
        lcdDmaTxCfg7789.sourceAddress = buf + (dmaTotalLength/dmaTrunkLength) * dmaTrunkLength;
        lcdDmaTxCfg7789.totalLength   = dmaTotalLength % dmaTrunkLength;
        DMA_buildDescriptorChain(&lcdDmaTxDesc7789[dmaTotalLength/dmaTrunkLength], &lcdDmaTxCfg7789, 1, true, true, true);
    }
    
    // start dma
    st7789StartStop(lcd, true);

    return 0;
}

static int stPrepareDisplay(lcdDev_t* lcd, uint16_t sx, uint16_t ex, uint16_t sy, uint16_t ey)
{
    return 0;
}

static int st7789OnOff(lcdDev_t* lcd, uint8_t sta)
{
    return 0;
}

static void st7789BackLight(lcdDev_t* lcd, uint8_t sta)
{
}




void st7789Clear(lcdDev_t* lcd,  uint8_t* buf, uint16_t lcdHeight, uint16_t lcdWidth, uint32_t dmaTrunkLength)
{
    //static bool initOnce = false;
    //LSPI_TypeDef* lspi2 = LSPI2;
    uint32_t dmaTotalLength = lcdHeight * lcdWidth * 2;

    lspiDataFmt.wordSize = 7;
    lspiDataFmt.txPack = 0;
    lspiDrv7789->ctrl(LSPI_CTRL_DATA_FORMAT, 0);
            
    lspiCtrl.datSrc = 1; // 0: data from camera; 1: data from memory
    lspiCtrl.colorModeIn = 3; // RGB565
    lspiCtrl.colorModeOut = 1; // RGB565
    lspiDrv7789->ctrl(LSPI_CTRL_CTRL, 0);
    
    lspiInfo.frameHeight = lcdHeight; // frame input height
    lspiInfo.frameWidth = lcdWidth; // frame input width
    lspiDrv7789->ctrl(LSPI_CTRL_FRAME_INFO, 0);
    
    lspiScaleInfo.rowScaleFrac = 0;
    lspiScaleInfo.colScaleFrac = 0;
    lspiDrv7789->ctrl(LSPI_CTRL_SCALE_INFO, 0);

    lspiTailorInfo.tailorLeft = 0; // frame output height
    lspiTailorInfo.tailorRight = 0; // frame output width
    lspiDrv7789->ctrl(LSPI_CTRL_TAILOR_INFO, 0);
    
    lspiFrameInfoOut.frameHeightOut = lcdHeight; // frame output height
    lspiFrameInfoOut.frameWidthOut = lcdWidth; // frame output width
    lspiDrv7789->ctrl(LSPI_CTRL_FRAME_INFO_OUT, 0);

    // shuaping
    lspiCmdCtrl.wrRdn = 1; // 1: wr   0: rd
    lspiCmdCtrl.ramWr = 1; // start ramwr
    lspiCmdCtrl.dataLen = dmaTotalLength;
    lspiDrv7789->ctrl(LSPI_CTRL_CMD_CTRL, 0);

    // build dma descriptor chain
    lcdDmaTxCfg7789.sourceAddress = buf;
    lcdDmaTxCfg7789.totalLength   = dmaTrunkLength; // every dma trunk transfer size
    DMA_buildDescriptorChain(lcdDmaTxDesc7789, &lcdDmaTxCfg7789, dmaTotalLength/dmaTrunkLength, true, true,true);

    // start dma
    st7789StartStop(lcd, true);
}


lcdDrvFunc_t st7789Drv = 
{
    .id             = 0x7789,

    .init           = st7789Init,
    .drawPoint      = st7789DrawPoint,
    .fill           = st7789Fill,
    .prepareDisplay = stPrepareDisplay,
    .onoff          = st7789OnOff,
    .backLight      = st7789BackLight,
    .startStop      = st7789StartStop,
    .clear          = st7789Clear,
    .startStopPreview = st7789CamPreviewStartStop,
    //.startRamWrite  = st7789StartRamWrite,
    //.addrSet        = st7789AddrSet,
};

