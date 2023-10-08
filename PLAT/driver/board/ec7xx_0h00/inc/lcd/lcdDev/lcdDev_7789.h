#ifndef  LCDDEV_7789_H
#define  LCDDEV_7789_H

#include "lcdDrv.h"
#include "lcdComm.h"

#define SPI_DMA_TX_REQID        DMA_REQUEST_SPI0_TX // DMA SPI Request ID
#define LSPI_DMA_TX_REQID       DMA_REQUEST_USP2_TX

#define LCD_DMA_DESCRIPTOR_CHAIN_NUM                    20  // 30w pixel
#define LCD_TRANSFER_SIZE_ONCE                          480//7680 // less than 8k


#endif

