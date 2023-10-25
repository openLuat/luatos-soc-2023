/******************************************************************************

*(C) Copyright 2018 AirM2M International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename:
*
*  Description:
*
*  History: initiated by xuwang
*
*  Notes:
*
******************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include "mw_aal_hash.h"
#include "fota_pub.h"
#include "fota_sal.h"
#include "fota_chksum.h"



/*----------------------------------------------------------------------------*
 *                      GLOBAL VARIABLES                                      *
 *----------------------------------------------------------------------------*/
static MwAalSha256Ctx_t  gFotaSha256Ctx;


/*----------------------------------------------------------------------------*
 *                      GLOBAL FUNCTIONS                                      *
 *----------------------------------------------------------------------------*/

/******************************************************************************
 * @brief : fotaInitChksum
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
int32_t fotaInitChksum(uint8_t flags, void *args)
{
    switch(flags)
    {
        case FOTA_CA_SHA256SUM:
            mwAalInitSha256(&gFotaSha256Ctx, 0);
            break;

        default:
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_INIT_CHKSUM, P_WARNING, "chksum: undef algo(%d)!\n", flags);
            break;
    }

    return FOTA_EOK;
}

/******************************************************************************
 * @brief : fotaDeinitChksum
 * @author: Xu.Wang
 * @note  :
 ******************************************************************************/
int32_t fotaDeinitChksum(uint8_t flags, void *args)
{
    switch(flags)
    {
        case FOTA_CA_SHA256SUM:
            mwAalDeinitSha256(&gFotaSha256Ctx);
            break;

        default:
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_DEINIT_CHKSUM, P_WARNING, "chksum: undef algo(%d)!\n", flags);
            break;
    }

    return FOTA_EOK;
}

/******************************************************************************
 * @brief : fotaCalcChksum
 * @author: Xu.Wang
 * @note  : fotaInitChksum should be invoked before this API is called!
 ******************************************************************************/
int32_t fotaCalcChksum(uint8_t flags, void *args)
{
    int32_t           retCode = FOTA_EUNDEF;
    FotaCaSha256Sum_t *sha256 = (FotaCaSha256Sum_t*)args;

    switch(flags)
    {
        case FOTA_CA_SHA256SUM:
            retCode = mwAalUpdateSha256(&gFotaSha256Ctx, sha256->input, sha256->output, sha256->inLen, sha256->isLast);
            break;

        default:
            ECPLAT_PRINTF(UNILOG_FOTA, FOTA_CALC_CHKSUM, P_WARNING, "chksum: undef algo(%d)!\n", flags);
            break;
    }

    return retCode;
}



