#ifndef __LUAT_FULL_OTA_H__
#define __LUAT_FULL_OTA_H__
#include "bsp_common.h"
#include "platform_define.h"
#include "mbedtls/md5.h"

typedef struct
{
	Buffer_Struct data_buffer;
	mbedtls_md5_context md5_ctx;
	CoreUpgrade_FileHeadCalMD5Struct fota_file_head;
	uint32_t crc32_table[256];
	uint32_t ota_done_len;
	uint8_t ota_state;
}luat_full_ota_ctrl_t;


enum
{
	OTA_STATE_IDLE,
	OTA_STATE_WRITE_COMMON_DATA,
	OTA_STATE_OK,
	OTA_STATE_ERROR,
};

/**
 * @defgroup luatos_full_ota 整包升级接口
 * @{
 */

/**
 * @brief 用于初始化fota,创建写入升级包数据的上下文结构体
 *
 * @param 所有param都是无效的
 * @return void *fota句柄
 */
luat_full_ota_ctrl_t *luat_full_ota_init(uint32_t start_address, uint32_t len, void* spi_device, const char *path, uint32_t pathlen);
/**
 * @brief 用于向本地 Flash 中写入升级包数据
 *
 * @param handle - fota句柄
 * @param data - 升级包数据地址
 * @param len - 升级包数据长度。 单位：字节
 * @return int =0成功，其他失败；
 */
int luat_full_ota_write(luat_full_ota_ctrl_t *handle, uint8_t *data, uint32_t len);

/**
 * @brief 检查升级包是否写入完整
 *
 * @param handle - fota句柄
 * @return int =0成功，>0还需要写入更多数据 <0发生异常已经失败
 */
int luat_full_ota_is_done(luat_full_ota_ctrl_t *handle);

/**
 * @brief 用于结束升级包下载
 *
 * @param handle - fota句柄
 * @param is_ok 废弃无效
 * @return int =0成功，其他失败
 */
void luat_full_ota_end(luat_full_ota_ctrl_t *handle, uint8_t is_ok);
/** @}*/
#endif
