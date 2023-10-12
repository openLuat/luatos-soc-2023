/******************************************************************************

*(C) Copyright 2018 AirM2M International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: atec_cust_cmd_table.h
*
*  Description:
*
*  History:
*
*  Notes:
*
******************************************************************************/
#ifndef _ATEC_CUSTOMER_CMD_TABLE_H
#define _ATEC_CUSTOMER_CMD_TABLE_H

#include "at_util.h"

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

AtCmdPreDefInfoC* atcGetATCustCommandsSeqPointer(void);

uint32_t atcGetATCustCommandsSeqNumb(void);


#endif

/* END OF FILE */
