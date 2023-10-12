/****************************************************************************
 *
 * Copy right:   2019-, Copyrigths of AirM2M Ltd.
 * File name:    hal_i2s.c
 * Description:  EC618 i2s hal driver source file
 * History:      Rev1.0   2021-9-18
 *
 ****************************************************************************/
#include "audioDrv.h"

extern I2sDrvInterface_t    i2sDrvInterface0;
extern I2sDrvInterface_t    i2sDrvInterface1;

static I2sDrvInterface_t   *i2sDrv = NULL;

extern I2sDataFmt_t         i2sDataFmt;
extern I2sSlotCtrl_t        i2sSlotCtrl;
extern I2sBclkFsCtrl_t      i2sBclkFsCtrl;
extern I2sCtrl_t            i2sCtrl;
extern I2sDmaCtrl_t         i2sDmaCtrl;


static CodecType_e          codecType; // Record codec type
static I2sMode_e            i2sMode;
bool   i2sHasBeenStoped = false;


// Register 
void HAL_I2sInit(i2sCbFunc_fn txCb, i2sCbFunc_fn rxCb)
{
#if (RTE_I2S0)
    i2sDrv = &CREATE_SYMBOL(i2sDrvInterface, 0); // Choose i2s0
#elif (RTE_I2S1)
    i2sDrv = &CREATE_SYMBOL(i2sDrvInterface, 1); // Choose i2s1
#endif

    i2sDrv->init(txCb, rxCb);
    i2sDrv->powerCtrl(I2S_POWER_FULL);
}

void HAL_I2sSetDmaDescriptorNum(I2sDirectionSel_e direc, uint8_t num)
{
    switch (direc)
    {
        case I2S_TX:
            i2sDrv->ctrl(I2S_CTRL_TX_DESCRIPTOR , num);
            break;

        case I2S_RX:
            i2sDrv->ctrl(I2S_CTRL_RX_DESCRIPTOR , num);
            break;

         default:
            break;         
    }
}



void HAL_I2sDeInit()
{
    i2sDrv->powerCtrl(I2S_POWER_OFF);
    i2sDrv->deInit();
}

void HAL_I2sSetChannel(I2sChannelSel_e channelSel)
{
    switch(channelSel)
    {
		case MONO:
			{
				i2sSlotCtrl.slotEn  = 1;
				i2sSlotCtrl.slotNum = 1;
			}
			break;

		case DUAL_CHANNEL:
			{
				i2sSlotCtrl.slotEn  = 3;
				i2sSlotCtrl.slotNum = 1;
			}
			break;

		default:
			break;
    }
    
    i2sDrv->ctrl(I2S_CTRL_SLOT_CTRL , 0);
}

void HAL_I2SSetTotalNum(uint32_t totalNum)
{
    i2sDrv->ctrl(I2S_CTRL_SET_TOTAL_NUM , totalNum);
}

uint32_t HAL_I2SGetTotalNum()
{
	return i2sDrv->getTotalCnt();
}

uint32_t HAL_I2SGetTrunkNum()
{
	return i2sDrv->getTrunkCnt();
}


void HAL_i2sSetSampleRate(I2sRole_e i2sRole, uint32_t sampleRate)
{
    I2sSampleRate_e sampleRateIndex = 0;
    
    switch (sampleRate)
    {
        case 8000:
            sampleRateIndex = SAMPLERATE_8K;
        break;

        case 16000:
            sampleRateIndex = SAMPLERATE_16K;
        break;

        case 32000:
            sampleRateIndex = SAMPLERATE_32K;
        break;

        case 22050:
            sampleRateIndex = SAMPLERATE_22_05K;
        break;

        case 44100:
            sampleRateIndex = SAMPLERATE_44_1K;
        break;

        case 48000:
            sampleRateIndex = SAMPLERATE_48K;
        break;

        case 96000:
            sampleRateIndex = SAMPLERATE_96K;
        break;

        default:
            break;
    }
    
    if (i2sRole == CODEC_MASTER_MODE) // Codec act as master
    {
        i2sDrv->ctrl(I2S_CTRL_SAMPLE_RATE_SLAVE, sampleRateIndex); // I2S Set sample rate in slave role
    }
    else
    {
        i2sDrv->ctrl(I2S_CTRL_SAMPLE_RATE_MASTER, sampleRateIndex); // I2S Set sample rate in master role
    }
}

// Control I2S to start or stop. Note: when app wakeup from sleep1, need to call this api to start MCLK and i2s
void HAL_I2sStartSop(I2sStartStop_e startStop)
{
	i2sDrv->ctrl(I2S_CTRL_START_STOP , startStop);

	if (startStop == STOP_I2S)
	{
        i2sHasBeenStoped = true;
	}
	else
	{
        i2sHasBeenStoped = false;
	}
}

void HAL_I2SSetPlayRecord(I2sPlayRecord_e playRecord)
{
    if (playRecord == PLAY)
    {
        i2sCtrl.i2sMode = 0x1; // Set I2S controller to send
        i2sDmaCtrl.txDmaReqEn = 1;
        i2sDmaCtrl.rxDmaReqEn = 0;
    }
    else if (playRecord == RECORD)
    {
        i2sCtrl.i2sMode = 0x2; // Set I2S controller to receive
        i2sDmaCtrl.rxDmaReqEn = 1; // Enable I2S controller RX DMA
        i2sDmaCtrl.txDmaReqEn = 0;
    }
    else if (playRecord == PLAY_RECORD)
    {
        i2sCtrl.i2sMode = 0x3; // Set I2S controller to full duplex sned/receive
        i2sDmaCtrl.rxDmaReqEn = 1; // Enable I2S controller RX DMA
        i2sDmaCtrl.txDmaReqEn = 1;
    }   
    
	i2sDrv->ctrl(I2S_CTRL_DMA_CTRL , 0);

	if (i2sHasBeenStoped)
	{
        HAL_I2sStartSop((I2sStartStop_e)i2sCtrl.i2sMode);
        i2sHasBeenStoped = false;
	}
	else
	{
        i2sDrv->ctrl(I2S_CTRL_I2SCTL , 0);
	}
}

void HAL_I2sConfig(I2sParamCtrl_t paramCtrl)
{   
    // 1. Setting parameters per the I2S working mode    
    i2sMode = paramCtrl.mode;
    switch (i2sMode)
    {
        case MSB_MODE:
            i2sDataFmt.dataDly          = 0;
            i2sBclkFsCtrl.bclkPolarity  = 1; 
            i2sBclkFsCtrl.fsPolarity    = 0;
            
            break;

        case LSB_MODE:
            i2sDataFmt.dataDly          = 1;
            i2sBclkFsCtrl.bclkPolarity  = 1;
            i2sBclkFsCtrl.fsPolarity    = 0;

            break;

        case I2S_MODE:
            i2sDataFmt.dataDly          = 1;
            if (paramCtrl.codecType == ES8311)
            {
                i2sBclkFsCtrl.bclkPolarity  = 1;
            }
            else
            {
                i2sBclkFsCtrl.bclkPolarity  = 0;
            }
            i2sBclkFsCtrl.fsPolarity    = 1;

            break;

        case PCM_MODE:    
            // Configure codec to PCM mode
            i2sBclkFsCtrl.bclkPolarity  = 1;
            break;

        default:
            break;
    }

    
    // 2. Init codec and I2S controller
    switch (paramCtrl.codecType)
    {
        case ES8388:
        {
            codecType = ES8388;
            if (paramCtrl.role == CODEC_MASTER_MODE) // Codec act as master
            {
                i2sDrv->ctrl(I2S_CTRL_SAMPLE_RATE_SLAVE , paramCtrl.sampleRate); // I2S Set sample rate in slave role 
                codecInit(ES8388, CODEC_MASTER_MODE);
            }
            else // Codec act as slave
            {
                i2sDataFmt.slaveModeEn = 0; // Master mode
                i2sDrv->ctrl(I2S_CTRL_DATA_FORMAT , 0); 
                i2sDrv->ctrl(I2S_CTRL_SAMPLE_RATE_MASTER , paramCtrl.sampleRate); // I2S Set sample rate in master role
                codecInit(ES8388, CODEC_SLAVE_MODE);
            }
                
            break;
        }

        case ES7148:
        case ES7149:
        case TM8211:
        {	
        	// now we only can act as master mode
         	i2sDataFmt.slaveModeEn = 0; // Master mode
            i2sDrv->ctrl(I2S_CTRL_DATA_FORMAT , 0); 
            i2sDrv->ctrl(I2S_CTRL_SAMPLE_RATE_MASTER , paramCtrl.sampleRate); // I2S Set sample rate in master role
                
			break;
        }

        case ES8311:
        {
            codecType = ES8311;
            
            if (paramCtrl.role == CODEC_MASTER_MODE) // Codec act as master
            {
                i2sDrv->ctrl(I2S_CTRL_SAMPLE_RATE_SLAVE , paramCtrl.sampleRate); // I2S Set sample rate in slave role 
                codecInit(ES8311, CODEC_MASTER_MODE);
            }
            else // Codec act as slave
            {
                i2sDataFmt.slaveModeEn = 0; // Master mode
                i2sDrv->ctrl(I2S_CTRL_DATA_FORMAT , 0); 
                i2sDrv->ctrl(I2S_CTRL_SAMPLE_RATE_MASTER , paramCtrl.sampleRate); // I2S Set sample rate in master role
                codecInit(ES8311, CODEC_SLAVE_MODE);
            } 
            break;
        }

        default:
            break;
    }

    // 3. Set frame size
    switch (paramCtrl.frameSize)
    {
        case FRAME_SIZE_16_16:
            i2sDataFmt.slotSize     = 0xf;
            i2sBclkFsCtrl.fsWidth   = 0xf;
            i2sDataFmt.wordSize     = 0xf;

            break;

        case FRAME_SIZE_16_32:
            // I2S controller part
            i2sDataFmt.slotSize     = 0x1f;
            i2sDataFmt.wordSize     = 0xf;
            i2sBclkFsCtrl.fsWidth   = 0x1f;

            break;

        case FRAME_SIZE_24_32:
            // I2S controller part
            i2sDataFmt.slotSize     = 0x1f;
            i2sDataFmt.wordSize     = 0x17;
            i2sBclkFsCtrl.fsWidth   = 0x1f;

            break;

        case FRAME_SIZE_32_32:
            // I2S controller part
            i2sDataFmt.slotSize     = 0x1f;
            i2sDataFmt.wordSize     = 0x1f;
            i2sBclkFsCtrl.fsWidth   = 0x1f;

            break;

        default:
            break;
    }

	// 4. Select mono or dual-channel
    switch(paramCtrl.channelSel)
    {
		case MONO:
			{
				i2sSlotCtrl.slotEn  = 1;
				i2sSlotCtrl.slotNum = 1;
			}
			break;

		case DUAL_CHANNEL:
			{
				i2sSlotCtrl.slotEn  = 3;
				i2sSlotCtrl.slotNum = 1;
			}
			break;

		default:
			break;
    }

    // Init part of I2S controller
    i2sDrv->ctrl(I2S_CTRL_DATA_FORMAT , 0);
    i2sDrv->ctrl(I2S_CTRL_BCLK_FS_CTRL , 0);
    i2sDrv->ctrl(I2S_CTRL_SLOT_CTRL , 0);
    i2sDrv->ctrl(I2S_CTRL_INT_CTRL , 0);

    // 5. Set play or record
	HAL_I2SSetPlayRecord(paramCtrl.playRecord); 
}

void HAL_I2sTransfer(I2sPlayRecord_e playRecord, uint8_t* memAddr, uint32_t trunkNum)
{
    // 5. After other parameters are ready, start the I2S controller
    if (playRecord == PLAY) // Play audio
    {
        i2sDrv->send(true, true, false, memAddr, trunkNum);
    }
    else if (playRecord == RECORD) // Record audio
    {
        i2sDrv->recv(true, true, false, memAddr, trunkNum);
    }
    else if (playRecord == PLAY_LOOP)
    {
        i2sDrv->send(false, false, true, memAddr, trunkNum);
    }
    else if (playRecord == PLAY_LOOP_IRQ)
    {
        i2sDrv->send(false, true, true, memAddr, trunkNum);
    }
    else if (playRecord == RECORD_LOOP_IRQ)
    {
        i2sDrv->recv(false, true, true, memAddr, trunkNum);
    }
}

void HAL_I2sSrcAdjustVolumn(int16_t* srcBuf, uint32_t srcTotalNum, uint16_t volScale)
{
	int integer = volScale / 10;
	int decimal = volScale % 10;
	int scale = 0;
	int32_t tmp = 0;
	uint32_t totalNum = srcTotalNum;
	uint32_t step = 0;
	
	while (totalNum)
	{
		if (volScale < 10)
		{
			tmp = ((*(srcBuf + step)) * (256 * integer + 26 * decimal)) >> 8;
		}
		else
		{
			scale = (256 * integer + 26 * decimal) >> 8;
			tmp = (*(srcBuf + step)) * scale;
		}
		
		if (tmp > 32767)
		{
			tmp = 32767;
		}
		else if (tmp < -32768)
		{
			tmp = -32768;
		}
			
		*(srcBuf + step) = (int16_t)tmp;
		step += 1;
		totalNum -= 2;
	}
}

void HAL_I2sSetInt()
{
	//i2sDrv->ctrl(I2S_CTRL_INT_CTRL , startStop);
}

