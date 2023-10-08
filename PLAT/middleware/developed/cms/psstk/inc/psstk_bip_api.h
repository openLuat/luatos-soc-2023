#ifndef __PS_STK_BIP_API_H__
#define __PS_STK_BIP_API_H__
/******************************************************************************
 ******************************************************************************
 Copyright:      - 2017- Copyrights of AirM2M Ltd.
 File name:      - psstk_bip_api.h
 Description:    - API called by psproxy task
 ******************************************************************************
******************************************************************************/
#include <osasys.h>

/******************************************************************************
 *****************************************************************************
 * MARCO
 *****************************************************************************
******************************************************************************/

/******************************************************************************
 *****************************************************************************
 * ENUM
 *****************************************************************************
******************************************************************************/

/******************************************************************************
 ******************************************************************************
 * External global variable
 ******************************************************************************
******************************************************************************/

/******************************************************************************
 *****************************************************************************
 * Functions
 *****************************************************************************
******************************************************************************/

void psStkBipProcCmiInd(const SignalBuf *indSignalPtr);
void psStkBipProcCmiCnf(const SignalBuf *cnfSignalPtr);

/*
 * processed the signal send to STK CMS, if processed, return TRUE
*/
BOOL psStkBipProcSimBipSig(const SignalBuf *pSig);



#endif

