/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "luat_base.h"
#include "luat_mcu.h"
#include "luat_malloc.h"
#include "luat_spi.h"
#include "luat_fota.h"
#include "common_api.h"
#include "platform_define.h"
#ifdef LUAT_USE_TLS
#include "mbedtls/md5.h"
#else
#include "md5.h"
#endif
#include "fota_nvm.h"
#include "mem_map.h"
#include "flash_rt.h"
#define __SOC_OTA_COMMON_DATA_LOAD_ADDRESS__	(FULL_OTA_SAVE_ADDR + AP_FLASH_XIP_ADDR)
#define __SOC_OTA_COMMON_DATA_SAVE_ADDRESS__	(FULL_OTA_SAVE_ADDR)
#define __SOC_OTA_SDK_DATA_LOAD_ADDRESS__	(FLASH_FOTA_REGION_START + AP_FLASH_XIP_ADDR)
#define __SOC_OTA_SDK_DATA_SAVE_ADDRESS__	(FLASH_FOTA_REGION_START)
#define __SOC_OTA_COMMON_DATA_LOAD_ADDRESS_LIMIT__	(AP_FLASH_LOAD_ADDR + AP_FLASH_LOAD_SIZE)

#ifdef LUAT_USE_TLS
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
#define mbedtls_md5_starts_ret mbedtls_md5_starts
#define mbedtls_md5_update_ret mbedtls_md5_update
#define mbedtls_md5_finish_ret mbedtls_md5_finish
#endif
#endif

typedef struct
{
	Buffer_Struct data_buffer;
#ifdef LUAT_USE_TLS
	mbedtls_md5_context *md5_ctx;
#endif
	CoreUpgrade_FileHeadCalMD5Struct *p_fota_file_head;
	uint32_t *crc32_table;
	uint32_t ota_done_len;
	uint8_t ota_state;
	uint8_t ota_type;
}luat_fota_ctrl_t;

enum
{
	OTA_STATE_IDLE,
	OTA_STATE_WRITE_COMMON_DATA,
	OTA_STATE_WRITE_SDK_DATA,
	OTA_STATE_OK,
};

static luat_fota_ctrl_t g_s_fota;

#ifdef LUAT_USE_TLS
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
int mbedtls_sha256_starts_ret( mbedtls_sha256_context *ctx, int is224 ) {
	mbedtls_sha256_starts(ctx, is224);
}
int mbedtls_sha256_update_ret( mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen ) {
	mbedtls_sha256_update(ctx, input, ilen);
}
int mbedtls_sha256_finish_ret( mbedtls_sha256_context *ctx, unsigned char output[32]) {
	mbedtls_sha256_finish(ctx, output);
}
#endif
#endif

static void luat_fota_finish(void)
{
	CoreUpgrade_HeadCalMD5Struct Head = {0};
	PV_Union uPV;
	Head.MaigcNum = __APP_START_MAGIC__;
	Head.DataStartAddress = __SOC_OTA_COMMON_DATA_LOAD_ADDRESS__;
	uPV.u8[0] = g_s_fota.ota_type;
	uPV.u8[1] = CORE_OTA_IN_FLASH;
	Head.Param1 = uPV.u32;
	Head.DataLen = g_s_fota.p_fota_file_head->CommonDataLen;
	memcpy(Head.CommonMD5, g_s_fota.p_fota_file_head->CommonMD5, 16);
	Head.CRC32 = CRC32_Cal(g_s_fota.crc32_table, (uint8_t *)&Head.Param1, sizeof(Head) - 8, 0xffffffff);
	FLASH_eraseSafe(__SOC_OTA_INFO_DATA_SAVE_ADDRESS__, __FLASH_SECTOR_SIZE__);
	FLASH_writeSafe((uint8_t *)&Head, __SOC_OTA_INFO_DATA_SAVE_ADDRESS__, sizeof(Head));
	g_s_fota.ota_state = OTA_STATE_OK;
	free(g_s_fota.crc32_table);
	g_s_fota.crc32_table = NULL;
#ifdef LUAT_USE_TLS
	free(g_s_fota.md5_ctx);
	g_s_fota.md5_ctx = NULL;
#endif
	free(g_s_fota.p_fota_file_head);
	g_s_fota.p_fota_file_head = NULL;
	OS_DeInitBuffer(&g_s_fota.data_buffer);
	DBG("fota ok!, wait reboot");
}

int luat_fota_init(uint32_t start_address, uint32_t len, luat_spi_device_t* spi_device, const char *path, uint32_t pathlen)
{
	if (!g_s_fota.crc32_table)
	{
		g_s_fota.p_fota_file_head = malloc(sizeof(CoreUpgrade_FileHeadCalMD5Struct));
		g_s_fota.crc32_table = malloc(256 * sizeof(uint32_t));
#ifdef LUAT_USE_TLS
		g_s_fota.md5_ctx = malloc(sizeof(mbedtls_md5_context));
#endif
#ifdef LUAT_USE_TLS
		if (g_s_fota.p_fota_file_head == NULL || 
			g_s_fota.crc32_table == NULL ||

			g_s_fota.md5_ctx == NULL) {
#else
		if (g_s_fota.p_fota_file_head == NULL ||
				g_s_fota.crc32_table == NULL ) {
#endif
			DBG("fota init fail, out of sys memory!!!");
			if (g_s_fota.p_fota_file_head) {
				free(g_s_fota.p_fota_file_head);
				g_s_fota.p_fota_file_head = NULL;
			}
			if (g_s_fota.crc32_table) {
				free(g_s_fota.crc32_table);
				g_s_fota.crc32_table = NULL;
			}
#ifdef LUAT_USE_TLS
			if (g_s_fota.md5_ctx) {
				free(g_s_fota.md5_ctx);
				g_s_fota.md5_ctx = NULL;
			}
#endif
			return -1;
		}

		memset(g_s_fota.crc32_table, 0, 1024);
		CRC32_CreateTable(g_s_fota.crc32_table, CRC32_GEN);
	}
	memset(g_s_fota.p_fota_file_head, 0, sizeof(CoreUpgrade_FileHeadCalMD5Struct));
	g_s_fota.ota_state = OTA_STATE_IDLE;
	g_s_fota.ota_type = CORE_OTA_MODE_FULL;
	FLASH_eraseSafe(__SOC_OTA_INFO_DATA_SAVE_ADDRESS__, __FLASH_SECTOR_SIZE__);
	OS_ReInitBuffer(&g_s_fota.data_buffer, __FLASH_SECTOR_SIZE__ * 4);
	return 0;
}

int luat_fota_write(uint8_t *data, uint32_t len)
{
	uint32_t save_len;
	if (!g_s_fota.crc32_table) {
		DBG("fota need init");
		return -1;
	}
	OS_BufferWrite(&g_s_fota.data_buffer, data, len);
REPEAT:
	switch(g_s_fota.ota_state)
	{
	case OTA_STATE_IDLE:
		if (g_s_fota.data_buffer.Pos > sizeof(CoreUpgrade_FileHeadCalMD5Struct))
		{
			memcpy(g_s_fota.p_fota_file_head, g_s_fota.data_buffer.Data, sizeof(CoreUpgrade_FileHeadCalMD5Struct));
			OS_BufferRemove(&g_s_fota.data_buffer, sizeof(CoreUpgrade_FileHeadCalMD5Struct));
			if (g_s_fota.p_fota_file_head->MaigcNum != __APP_START_MAGIC__)
			{
				DBG("magic num error %x", g_s_fota.p_fota_file_head->MaigcNum);
				g_s_fota.data_buffer.Pos = 0;
				return -1;
			}
			uint32_t crc32 = CRC32_Cal(g_s_fota.crc32_table, g_s_fota.p_fota_file_head->MainVersion, sizeof(CoreUpgrade_FileHeadCalMD5Struct) - 8, 0xffffffff);
			if (crc32 != g_s_fota.p_fota_file_head->CRC32)
			{
				DBG("file head crc32 error %x,%x", crc32, g_s_fota.p_fota_file_head->CRC32);
				g_s_fota.data_buffer.Pos = 0;
				return -1;
			}
			g_s_fota.ota_done_len = 0;
#ifdef LUAT_USE_TLS
			mbedtls_md5_init(g_s_fota.md5_ctx);
			mbedtls_md5_starts_ret(g_s_fota.md5_ctx);
#endif
			if (g_s_fota.p_fota_file_head->CommonDataLen)
			{
				g_s_fota.ota_state = OTA_STATE_WRITE_COMMON_DATA;
				DBG("write common data");
				goto REPEAT;
			}
			else
			{
				g_s_fota.ota_state = OTA_STATE_WRITE_SDK_DATA;
				g_s_fota.ota_type = CORE_OTA_MODE_DIFF;
				DBG("write core data");
				goto REPEAT;
			}
		}
		break;
	case OTA_STATE_WRITE_COMMON_DATA:
		save_len = ((g_s_fota.ota_done_len + __FLASH_SECTOR_SIZE__) < g_s_fota.p_fota_file_head->CommonDataLen)?__FLASH_SECTOR_SIZE__:(g_s_fota.p_fota_file_head->CommonDataLen - g_s_fota.ota_done_len);
		if (g_s_fota.data_buffer.Pos >= save_len)
		{
			FLASH_eraseSafe(__SOC_OTA_COMMON_DATA_SAVE_ADDRESS__ + g_s_fota.ota_done_len, __FLASH_SECTOR_SIZE__);
			FLASH_writeSafe(g_s_fota.data_buffer.Data, __SOC_OTA_COMMON_DATA_SAVE_ADDRESS__ + g_s_fota.ota_done_len, save_len);
			size_t loadaddr = __SOC_OTA_COMMON_DATA_LOAD_ADDRESS__;
#ifdef LUAT_USE_TLS
			mbedtls_md5_update_ret(g_s_fota.md5_ctx, (uint8_t *)(loadaddr + g_s_fota.ota_done_len), save_len );
#endif
			OS_BufferRemove(&g_s_fota.data_buffer, save_len);
		}
		else
		{
			break;
		}
		g_s_fota.ota_done_len += save_len;
		if (g_s_fota.ota_done_len >= g_s_fota.p_fota_file_head->CommonDataLen)
		{
			DBG("common data done, now checking");
			uint8_t md5[16];
#ifdef LUAT_USE_TLS
			mbedtls_md5_finish_ret(g_s_fota.md5_ctx, md5);
#endif
			if (memcmp(md5, g_s_fota.p_fota_file_head->CommonMD5, 16))
			{
				DBG("common data md5 check failed");
				g_s_fota.ota_state = OTA_STATE_IDLE;
				g_s_fota.data_buffer.Pos = 0;
				return -1;
			}
			else
			{
				DBG("common data md5 ok");
				if (g_s_fota.p_fota_file_head->SDKDataLen)
				{
					g_s_fota.ota_state = OTA_STATE_WRITE_SDK_DATA;
					g_s_fota.ota_done_len = 0;
					DBG("write core data");
					goto REPEAT;
				}
				else
				{
					DBG("common data md5 ok");
					g_s_fota.ota_type = CORE_OTA_MODE_FULL;
					luat_fota_finish();
					return 0;
				}
			}
		}
		else
		{
			goto REPEAT;
		}
		break;
	case OTA_STATE_WRITE_SDK_DATA:
		save_len = ((g_s_fota.ota_done_len + __FLASH_SECTOR_SIZE__) < (g_s_fota.p_fota_file_head->SDKDataLen))?__FLASH_SECTOR_SIZE__:(g_s_fota.p_fota_file_head->SDKDataLen - g_s_fota.ota_done_len);
		if (g_s_fota.data_buffer.Pos >= save_len)
		{
			FLASH_eraseSafe(__SOC_OTA_SDK_DATA_SAVE_ADDRESS__ + g_s_fota.ota_done_len, __FLASH_SECTOR_SIZE__);
			FLASH_writeSafe(g_s_fota.data_buffer.Data, __SOC_OTA_SDK_DATA_SAVE_ADDRESS__ + g_s_fota.ota_done_len, save_len);
			OS_BufferRemove(&g_s_fota.data_buffer, save_len);
			//			mbedtls_md5_update_ret(g_s_fota.md5_ctx, (uint8_t *)(__SOC_OTA_COMMON_DATA_SAVE_ADDRESS__ + g_s_fota.ota_done_len), save_len );
		}
		else
		{
			break;
		}
		g_s_fota.ota_done_len += save_len;
		if (g_s_fota.ota_done_len >= g_s_fota.p_fota_file_head->SDKDataLen)
		{
			fotaNvmInit();
		    FotaDefChkDeltaState_t    chkDelta = {0};
		    FotaDefChkBaseImage_t      chkBase = {0};

			fotaNvmDoExtension(FOTA_DEF_CHK_DELTA_STATE, (void*)&chkDelta);
			if(!chkDelta.isValid)
			{
				DBG("validate delta err! errno(%d)", chkDelta.state);
				g_s_fota.ota_state = OTA_STATE_IDLE;
				g_s_fota.data_buffer.Pos = 0;
				return -1;
			}
			else
			{
				DBG("validate delta ok!");
				fotaNvmDoExtension(FOTA_DEF_CHK_BASE_IMAGE, (void*)&chkBase);
				if(!chkBase.isMatched)
				{
					DBG("however, base fw is unmatched!");
					fotaNvmClearDelta(0, 4096);
					g_s_fota.ota_state = OTA_STATE_IDLE;
					g_s_fota.data_buffer.Pos = 0;
					return -1;
				}
			}
			DBG("sdk data ok!");
			luat_fota_finish();
			return 0;
		}
		else
		{
			goto REPEAT;
		}
		break;
	default:
		return 0;
		break;
	}
	return 1;
}

int luat_fota_done(void)
{
	switch(g_s_fota.ota_state)
	{
	case OTA_STATE_IDLE:
		return -1;
	case OTA_STATE_OK:
		return 0;
	default:
		return 1;
	}
	return 1;
}

int luat_fota_end(uint8_t is_ok)
{
	if (g_s_fota.ota_state != OTA_STATE_OK)
	{
		if (g_s_fota.crc32_table) {
			free(g_s_fota.crc32_table);
			g_s_fota.crc32_table = NULL;
		}
#ifdef LUAT_USE_TLS
		if (g_s_fota.md5_ctx) {
			free(g_s_fota.md5_ctx);
			g_s_fota.md5_ctx = NULL;
		}
#endif
		if (g_s_fota.p_fota_file_head) {
			free(g_s_fota.p_fota_file_head);
			g_s_fota.p_fota_file_head = NULL;
		}
		OS_DeInitBuffer(&g_s_fota.data_buffer);
		OS_DeInitBuffer(&g_s_fota.data_buffer);
		FLASH_eraseSafe(__SOC_OTA_INFO_DATA_SAVE_ADDRESS__, __FLASH_SECTOR_SIZE__);
		DBG("fota failed");
		return -1;
	}
	return 0;
}

uint8_t luat_fota_wait_ready(void)
{
	return 1;
}
