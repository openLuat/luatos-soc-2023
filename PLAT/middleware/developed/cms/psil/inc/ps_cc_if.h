/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of AirM2M Ltd.
 * File name:    PS_cc_if.h
 * Description:  API interface implementation header file for IMS CC service
 * History:      Rev1.0
 *
 ****************************************************************************/
#ifndef __PS_CC_IF_H__
#define __PS_CC_IF_H__

#ifdef FEATURE_IMS_ENABLE

#include "cms_util.h"
#include "imicc.h"
#include "imimedia.h"


CmsRetId ccSetATD(UINT32 atHandle, UINT8 *dialNumStr, UINT16 dialNumStrLen);
CmsRetId ccSetATA(UINT32 atHandle);
CmsRetId ccSetHangupCall(UINT32 atHandle);
CmsRetId ccSetCSTA(UINT32 atHandle, UINT16 type);
CmsRetId ccGetCSTA(UINT32 atHandle);
CmsRetId ccSetCVHU(UINT32 atHandle, UINT8 mode);
CmsRetId ccGetCVHU(UINT32 atHandle);
CmsRetId ccSetCLCC(UINT32 atHandle);
CmsRetId ccSetCRC(UINT32 atHandle, UINT8 mode);
CmsRetId ccGetCRC(UINT32 atHandle);
CmsRetId ccSetCCWA(UINT32 atHandle, UINT8 n, UINT8 mode, UINT8 class);
CmsRetId ccGetCCWA(UINT32 atHandle);
CmsRetId ccSetCHLD(UINT32 atHandle, UINT8 n);
CmsRetId ccSetVTS(UINT32 atHandle, UINT8 *dtmfStr, UINT16 dtmfStrLen, UINT16 duration);
CmsRetId ccSetVTD(UINT32 atHandle, UINT16 duration);
CmsRetId ccGetVTD(UINT32 atHandle);

#endif
#endif
