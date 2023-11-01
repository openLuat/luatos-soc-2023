#ifndef _EXAMPLE_SOCKET_H
#define _EXAMPLE_SOCKET_H

#include "commontypedef.h"

#define SERVER_UDP_HOSTNAME "bs.openluat.com" // 基站定位网址
#define SERVER_UDP_PORT 12411

#define AM_LOCATION_SERVICE_LOCATION_BCD_LEN 5
#define AM_LOCATION_SERVICE_RCV_TIMEOUT 15 

#ifdef PACK_STRUCT_USE_INCLUDES
#include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct am_location_service_rsp_data_t
{
    PACK_STRUCT_FLD_8(u8_t result);
    PACK_STRUCT_FIELD(u8_t latitude[AM_LOCATION_SERVICE_LOCATION_BCD_LEN]);
    PACK_STRUCT_FIELD(u8_t longitude[AM_LOCATION_SERVICE_LOCATION_BCD_LEN]);
    PACK_STRUCT_FLD_8(u8_t year);
    PACK_STRUCT_FLD_8(u8_t month);
    PACK_STRUCT_FLD_8(u8_t day);
    PACK_STRUCT_FLD_8(u8_t hour);
    PACK_STRUCT_FLD_8(u8_t minute);
    PACK_STRUCT_FLD_8(u8_t second);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#include "arch/epstruct.h"
#endif


typedef struct
{
    UINT16 result;
    UINT8 latitude[20];
    UINT8 longitude[20];
    UINT16 year;
    UINT8 month;
    UINT8 day;
    UINT8 hour;
    UINT8 minute;
    UINT8 second;
}luat_location_service_result_t;

#endif