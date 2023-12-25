/******************************************************************************

*(C) Copyright 2018 AirM2M International Ltd.

* All Rights Reserved

******************************************************************************
*  Filename: ccio_base.h
*
*  Description:
*
*  History: 2021/1/19 created by xuwang
*
*  Notes: basic elements of Channel Centre for Input/Output(CCIO) service
*
******************************************************************************/
#ifndef CCIO_BASE_H
#define CCIO_BASE_H

/*----------------------------------------------------------------------------*
 *                    INCLUDES                                                *
 *----------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include DEBUG_LOG_HEADER_FILE


#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*
 *                    MACROS                                                  *
 *----------------------------------------------------------------------------*/
/* buffer size */
#define CCIO_BUF_SIZE_8            (8)
#define CCIO_BUF_SIZE_16           (16)
#define CCIO_BUF_SIZE_32           (32)
#define CCIO_BUF_SIZE_64           (64)
#define CCIO_BUF_SIZE_128          (128)
#define CCIO_BUF_SIZE_256          (256)
#define CCIO_BUF_SIZE_512          (512)
#define CCIO_BUF_SIZE_1K           (1024)
#define CCIO_BUF_SIZE_2K           (2048)
#define CCIO_BUF_SIZE_4K           (4096)
#define CCIO_BUF_SIZE_1_28K        (1280)
#define CCIO_BUF_SIZE_8K           (CCIO_BUF_SIZE_4K << 1)
#define CCIO_BUF_SIZE_16K          (CCIO_BUF_SIZE_4K << 2)
#define CCIO_BUF_SIZE_32K          (CCIO_BUF_SIZE_4K << 3)

/* alignment size */
#define CCIO_ALIGN_SIZE_1          (1 << 0)
#define CCIO_ALIGN_SIZE_2          (1 << 1)
#define CCIO_ALIGN_SIZE_4          (1 << 2)
#define CCIO_ALIGN_SIZE_8          (1 << 3)

#define CCIO_INFINITE_CYCLE_CNT     0xffff
/* mac addr length */
#define CCIO_MAC_ADDR_MAXSIZE       6
/* ipv4 addr length */
#define CCIO_IP4_ADDR_MAXSIZE       4
/* ipv6 addr length */
#define CCIO_IP6_ADDR_MAXSIZE       16
/* ipv6 interface id length */
#define CCIO_IP6_ID_MAXSIZE         8

/* 5bits */
#define CCIO_CHAN_MAXNUM            32
#define CCIO_CHAN_USB_MAXNUM        6
#define CCIO_CHAN_UART_MAXNUM       4
#define CCIO_CHAN_SPI_MAXNUM        2
#define CCIO_CHAN_I2S_MAXNUM        2
#define CCIO_CHAN_I2C_MAXNUM        2
#define CCIO_CHAN_RADIO_MAXNUM      16

/* lan channel: usb/uart */
#define CCIO_CHAN_LAN_MAXNUM       (CCIO_CHAN_USB_MAXNUM + CCIO_CHAN_UART_MAXNUM)
#define CCIO_CHAN_WAN_MAXNUM       (CCIO_CHAN_RADIO_MAXNUM)

#define CCIO_CHAN_ID_ANY            0xFFFF
#define CCIO_CHAN_ID_UNDEF          CCIO_DEV_ID_UNDEF
#define CCIO_CHAN_NO_GET(chanId)    CCIO_DEV_NO_GET(chanId)

/* chanId & devId has the same definition,
 * whose format is described as follows.
 * *******************************************
 * 16   15     12    9        5           0
 *   |rsvd+-type-+-st-+--llsn--+----No.----|
 *   |====+======+====+========+===========|
 *   |          Channel/Device ID          |
 *   |====+======+====+========+===========|
 */
#define CCIO_DEV_NO_MAX(base, cnt)  ((base) + ((cnt) - 1))

/* USB: 0-5 */
#define CCIO_DEV_NO_BASE_USB        0
#define CCIO_DEV_NO_MAX_USB         CCIO_DEV_NO_MAX(CCIO_DEV_NO_BASE_USB, CCIO_CHAN_USB_MAXNUM)
/* UART: 6-9 */
#define CCIO_DEV_NO_BASE_UART       (CCIO_DEV_NO_MAX_USB + 1)
#define CCIO_DEV_NO_MAX_UART        CCIO_DEV_NO_MAX(CCIO_DEV_NO_BASE_UART, CCIO_CHAN_UART_MAXNUM)
/* SPI: 10-11 */
#define CCIO_DEV_NO_BASE_SPI        (CCIO_DEV_NO_MAX_UART + 1)
#define CCIO_DEV_NO_MAX_SPI         CCIO_DEV_NO_MAX(CCIO_DEV_NO_BASE_SPI, CCIO_CHAN_SPI_MAXNUM)
/* I2S: 12-13 */
#define CCIO_DEV_NO_BASE_I2S        (CCIO_DEV_NO_MAX_SPI + 1)
#define CCIO_DEV_NO_MAX_I2S         CCIO_DEV_NO_MAX(CCIO_DEV_NO_BASE_I2S, CCIO_CHAN_I2S_MAXNUM)

/*
 *  * 14~15 is reserved for other hw type, such as i2c, ...
 */

/* RADIO: 16-31 */
#define CCIO_DEV_NO_BASE_RADIO      16
#define CCIO_DEV_NO_MAX_RADIO       CCIO_DEV_NO_MAX(CCIO_DEV_NO_BASE_RADIO, CCIO_CHAN_RADIO_MAXNUM)

#define CCIO_DEV_ID_BIT_MAXNUM      16
#define CCIO_DEV_NO_BIT_MAXNUM      5
#define CCIO_DEV_LLSN_BIT_MAXNUM    4
#define CCIO_DEV_SUBTYPE_BIT_MAXNUM 3
#define CCIO_DEV_TYPE_BIT_MAXNUM    3

#define CCIO_DEV_NO_BIT_BEGIN       0
#define CCIO_DEV_LLSN_BIT_BEGIN     5
#define CCIO_DEV_SUBTYPE_BIT_BEGIN  9
#define CCIO_DEV_TYPE_BIT_BEGIN     12

#define CCIO_DEV_ID_UNDEF           CCIO_BITMASK(CCIO_DEV_ID_BIT_MAXNUM,0)
#define CCIO_DEV_NO_UNDEF           CCIO_BITMASK(CCIO_DEV_NO_BIT_MAXNUM,0)
#define CCIO_DEV_LLSN_UNDEF         CCIO_BITMASK(CCIO_DEV_LLSN_BIT_MAXNUM,0)
#define CCIO_DEV_SUBTYPE_UNDEF      CCIO_BITMASK(CCIO_DEV_SUBTYPE_BIT_MAXNUM,0)
#define CCIO_DEV_TYPE_UNDEF         CCIO_BITMASK(CCIO_DEV_TYPE_BIT_MAXNUM,0)

#define CCIO_DEV_ID_MASK            CCIO_BITMASK(CCIO_DEV_ID_BIT_MAXNUM,0)
#define CCIO_DEV_NO_MASK            CCIO_BITMASK(CCIO_DEV_NO_BIT_MAXNUM,CCIO_DEV_NO_BIT_BEGIN)
#define CCIO_DEV_LLSN_MASK          CCIO_BITMASK(CCIO_DEV_LLSN_BIT_MAXNUM,CCIO_DEV_LLSN_BIT_BEGIN)
#define CCIO_DEV_SUBTYPE_MASK       CCIO_BITMASK(CCIO_DEV_SUBTYPE_BIT_MAXNUM,CCIO_DEV_SUBTYPE_BIT_BEGIN)
#define CCIO_DEV_TYPE_MASK          CCIO_BITMASK(CCIO_DEV_TYPE_BIT_MAXNUM,CCIO_DEV_TYPE_BIT_BEGIN)

#define CCIO_DEV_NO_GET(id)         ( (id) & CCIO_DEV_NO_MASK)
#define CCIO_DEV_LLSN_GET(id)       (((id) & CCIO_DEV_LLSN_MASK) >> CCIO_DEV_LLSN_BIT_BEGIN)
#define CCIO_DEV_SUBTYPE_GET(id)    (((id) & CCIO_DEV_SUBTYPE_MASK) >> CCIO_DEV_SUBTYPE_BIT_BEGIN)
#define CCIO_DEV_TYPE_GET(id)       (((id) & CCIO_DEV_TYPE_MASK) >> CCIO_DEV_TYPE_BIT_BEGIN)

#define CCIO_DEV_ID_SET(dt,st,llsn,no)   ((((dt) << CCIO_DEV_TYPE_BIT_BEGIN) & CCIO_DEV_TYPE_MASK) | \
                                          (((st) << CCIO_DEV_SUBTYPE_BIT_BEGIN) & CCIO_DEV_SUBTYPE_MASK) | \
                                          (((llsn) << CCIO_DEV_LLSN_BIT_BEGIN) & CCIO_DEV_LLSN_MASK) | \
                                          ((no)  & CCIO_DEV_NO_MASK))

#define CCIO_BITMASK(nbits,lsh)     (((1 << (nbits)) - 1) << lsh)

/*----------------------------------------------------------------------------*
 *                   DATA TYPE DEFINITION                                     *
 *----------------------------------------------------------------------------*/
typedef bool bool_t;

typedef void CcioPbuf_t;

/* The type which is used to hold the size and the indicies of the buffer. */
typedef uint16_t RbufSize_t;

/* 2bits: ccio speed type definition */
typedef enum
{
    CCIO_ST_SLOW = 0,
    CCIO_ST_MEDIUM,
    CCIO_ST_HIGH,

    CCIO_ST_MAXNUM
}CcioSpeedType_e;

/* 3bits: ccio hardware type definition */
typedef enum
{
    CCIO_HT_USB = 0,
    CCIO_HT_UART,
    CCIO_HT_SPI,
    CCIO_HT_I2S,
    CCIO_HT_RADIO,
    CCIO_HT_HWIND,  /* hw independent */

    CCIO_HT_MAXNUM
}CcioHwType_e;

/* 3bits: ccio device type definition */
typedef enum
{
    CCIO_DT_SERIAL = 0,
    CCIO_DT_RNDIS,
    CCIO_DT_ECM,
    CCIO_DT_EUTRA,
    CCIO_DT_CCID,

    CCIO_DT_MAXNUM
}CcioDevType_e;

/* 3bits: Serial device subtype definition */
typedef enum
{
    CSIO_DT_AT = 0,
    CSIO_DT_PPP,
    CSIO_DT_DIAG,
    CSIO_DT_OPAQ,
    CSIO_DT_AUDIO,
    CSIO_DT_MUX,

    CSIO_DT_MAXNUM
}CsioDevType_e;

/* 3bits: rndis/ecm(Net) device subtype definition */
typedef enum
{
    CNIO_DT_ETHER = 0,

    CNIO_DT_MAXNUM
}CnioDevType_e;

/* 3bits: eUtra device subtype definition: cp&up? */
typedef enum
{
    CUIO_DT_CTRL = 0,
    CUIO_DT_DATA,

    CUIO_DT_MAXNUM
}CuioDevType_e;


/* 3bits:cciD device subtype definition: currently only one */
typedef enum
{
    CDIO_DT_DATA = 0,

    CDIO_DT_MAXNUM
}CdioDevType_e;


/* 2bits: disable/en~ uplink/downlink hardware acceleration mechanism */
typedef enum
{
    CCIO_DEV_HWACM_NONE = 0,
    CCIO_DEV_HWACM_UL = (1 << 0), /* bit0 */
    CCIO_DEV_HWACM_DL = (1 << 1), /* bit1 */

    CCIO_DEV_HWACM_MAXNUM
}CcioDevHwAcm_e;

/* 2bits: ccio device working state definition */
typedef enum
{
    CCIO_DWS_UNDEF = 0,
    CCIO_DWS_INITIAL,
    CCIO_DWS_ASSIGNED,
    CCIO_DWS_WORKING,

    CCIO_DWS_MAXNUM
}CcioDevWorkState_e;

/** \brief List of channel Tx/Rx task create/kill flag */
typedef enum
{
    CCIO_TASK_FLAG_NONE = 0x0,  /**< None of send and recv task is created */
    CCIO_TASK_FLAG_RX   = 0x1,  /**< Will create/kill recv task during device initialization and start to recv after successful call */
    CCIO_TASK_FLAG_TX1  = 0x2,  /**< Will create/kill send task1 during device initialization */
    CCIO_TASK_FLAG_TX2  = 0x4,  /**< Will create/kill send task2 during device initialization */
    CCIO_TASK_FLAG_TX3  = 0x8,  /**< Will create/kill send task3 during device initialization */
    CCIO_TASK_FLAG_TX   = (CCIO_TASK_FLAG_TX1 | CCIO_TASK_FLAG_TX2 | CCIO_TASK_FLAG_TX3)
}CcioTaskOperFlag_e;

typedef enum
{
    CCIO_SERL_STATE_DCD = 0,
    CCIO_SERL_STATE_RI,

    CCIO_SERL_STATE_MAXNUM
}CcioSerlState_e;

typedef enum
{
    CCIO_SERL_DCD_OFF = 0,
    CCIO_SERL_DCD_ON,
}CcioSerlDcdState_e;

typedef struct
{
    uint8_t   stateId;  /* CcioSerlState_e */
    uint8_t   rsvd[3];
    uint32_t  data;
    uint32_t  extras;
    void     *chdev;
}CcioSerlState_t;

/* lan media state between DTE & DCE */
typedef enum
{
    CCIO_LMS_DISCONNECTED = 0,
    CCIO_LMS_NEGOTIATING,
    CCIO_LMS_CONNECTED
}CcioLanMediaState_e;

typedef enum
{
    CCIO_LAN_MEDIA_UNDEF = 0,
    CCIO_LAN_MEDIA_RNDIS,
    CCIO_LAN_MEDIA_ECM,
    CCIO_LAN_MEDIA_PPP,
    CCIO_LAN_MEDIA_AT,

    CCIO_LAN_MEDIA_MAXNUM = CCIO_LAN_MEDIA_AT
}CcioLanMediaType_e;

typedef struct
{
    uint8_t   lanType;    /* refer to 'CcioLanMediaType_e' */
    uint8_t   lanState;   /* refer to 'CcioLanMediaState_e' */
    uint8_t   rsvd[2];
    void     *chdev;
}CcioLanMedia_t;

typedef struct
{
    uint8_t   isAvlb;
    uint8_t   isPppType;  /* 0-eth, 1-ppp */
    uint8_t   pdp4Cid;    /* bit7: 0/1 exist or not; others: cid info! */
    uint8_t   pdp6Cid;    /* bit7: 0/1 exist or not; others: cid info! */
}CcioDataPathCap_t;

typedef enum
{
    CCIO_IP4_TYPE = (1 << 0),
    CCIO_IP6_TYPE = (1 << 1)
}CcioIpType_e;

typedef struct
{
    uint8_t   ipType;     /* CcioIpType_e */
    uint8_t   pdp4Cid;    /* bit7: 0/1 exist or not; others: cid info! */
    uint8_t   pdp6Cid;    /* bit7: 0/1 exist or not; others: cid info! */
    uint8_t   rsvd;

    uint8_t   ip4Addr[CCIO_IP4_ADDR_MAXSIZE];
    uint8_t   ip6Id[CCIO_IP6_ID_MAXSIZE];

    uint8_t   ip4Dns[2][CCIO_IP4_ADDR_MAXSIZE];
    uint8_t   ip6Dns[2][CCIO_IP6_ADDR_MAXSIZE];
}CcioDataPathHostConf_t;

typedef struct
{
    uint8_t mac[CCIO_MAC_ADDR_MAXSIZE];
}CcioMacAddr_t;

typedef enum
{
    CCIO_AUTH_NONE = 0,
    CCIO_AUTH_PAP,
    CCIO_AUTH_CHAP
}CcioAuthType_e;

typedef struct
{
    uint16_t  userLen;
    uint16_t  pwdLen;
    uint8_t   user[CCIO_BUF_SIZE_64];
    uint8_t   pwd[CCIO_BUF_SIZE_64];
}CcioAuthPap_t;

typedef struct
{
    uint16_t  challLen;
    uint16_t  respLen;
    uint8_t   chall[CCIO_BUF_SIZE_64];
    uint8_t   resp[CCIO_BUF_SIZE_64];
}CcioAuthChap_t;

typedef union
{
    CcioAuthPap_t  pap;
    CcioAuthChap_t chap;
}CcioAuthInfo_t;

typedef enum
{
    CCIO_REMOTE_FLAGS_WKUP =  0,
    CCIO_REMOTE_FLAGS_VBUS
}CcioRemoteFlags_e;

typedef uint8_t CcioRemoteFlags_bm;
#define CCIO_REMOTE_BM_FLAGS_WKUP   (1 << CCIO_REMOTE_FLAGS_WKUP)
#define CCIO_REMOTE_BM_FLAGS_VBUS   (1 << CCIO_REMOTE_FLAGS_VBUS)

typedef enum
{
    CCIO_EOK         =  0,    /* operation completed successfull */
    CCIO_ERR         = -1,    /* unspecified error: no other errno fits */
    CCIO_EARGS       = -2,    /* invalid argument(s) */
    CCIO_EFOUND      = -3,    /* item not found */
    CCIO_EEXIST      = -4,    /* item already exists */
    CCIO_EPERM       = -5,    /* operation/scenario not permitted */
    CCIO_EMALLOC     = -6,    /* memory alloc failure */
    CCIO_EMFREE      = -7,    /* memory free failure */
    CCIO_EMATCHED    = -8,    /* unmatched value/item */
    CCIO_EEMPTY      = -9,    /* table/list is empty */
    CCIO_EMORE       = -10,   /* wanted more data/info */

    CCIO_STATUS_RSVD = 0x7FFFFFFF
}CcioStatus_e;

/*----------------------------------------------------------------------------*
 *                    GLOBAL FUNCTIONS DECLEARATION                           *
 *----------------------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif
#endif

