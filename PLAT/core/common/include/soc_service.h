/*
 * am_service.h
 *
 *  Created on: 2023年8月10日
 *      Author: Administrator
 */

#ifndef CORE_INCLUDE_SOC_SERVICE_H_
#define CORE_INCLUDE_SOC_SERVICE_H_
#include "common_api.h"

typedef enum SOC_MISC_EVENT
{
	SOC_MISC_VBUS_OFF,
	SOC_MISC_VBUS_ON,
	SOC_MISC_WIFI_SCAN_DONE,
}SOC_MISC_EVENT_E;

typedef enum SOC_MOBILE_EVENT
{
	SOC_MOBILE_EVENT_CFUN = 0,
	/*!< SIM卡消息*/
	SOC_MOBILE_EVENT_SIM,
    /*!< 移动网络注册消息*/
	SOC_MOBILE_EVENT_REGISTER_STATUS,
	/*!< 小区基站信号变更消息*/
	SOC_MOBILE_EVENT_CELL_INFO,
	/*!< PDP状态消息*/
	SOC_MOBILE_EVENT_PDP,
	/*!< internet状态*/
	SOC_MOBILE_EVENT_NETIF,
	/*!< 通过基站同步时间完成*/
	SOC_MOBILE_EVENT_TIME_SYNC,
	SOC_MOBILE_EVENT_CSCON,
	SOC_MOBILE_EVENT_BEARER,
	SOC_MOBILE_EVENT_SMS,
	SOC_MOBILE_EVENT_FATAL_ERROR = 0xff,
}SOC_MOBILE_EVENT_E;

typedef enum SOC_MOBILE_CFUN_STATUS
{
	SOC_MOBILE_CFUN_OFF = 0,
	SOC_MOBILE_CFUN_ON,
	SOC_MOBILE_CFUN_NO_RF = 4,
}SOC_MOBILE_CFUN_STATUS_E;

typedef enum SOC_MOBILE_SIM_STATUS
{
	SOC_MOBILE_SIM_READY = 0,
	SOC_MOBILE_NO_SIM,
	SOC_MOBILE_SIM_NEED_PIN,
	SOC_MOBILE_SIM_ENTER_PIN_RESULT,
	SOC_MOBILE_SIM_NUMBER,
	SOC_MOBILE_SIM_NEED_PUK,
	SOC_MOBILE_SIM_ENTER_PIN_RESULT_OK,
}SOC_MOBILE_SIM_STATUS_E;

typedef enum SOC_MOBILE_REGISTER_STATUS
{

    /*!< 网络未注册*/
	SOC_MOBILE_STATUS_UNREGISTER,
    /*!< 网络已注册*/
	SOC_MOBILE_STATUS_REGISTERED,
	SOC_MOBILE_STATUS_SEARCHING,
	/*!< 网络注册被拒绝*/
	SOC_MOBILE_STATUS_DENIED,
	/*!< 网络状态未知*/
	SOC_MOBILE_STATUS_UNKNOW,
	/*!< 网络已注册，漫游*/
	SOC_MOBILE_STATUS_REGISTERED_ROAMING,
	SOC_MOBILE_STATUS_SMS_ONLY_REGISTERED,
	SOC_MOBILE_STATUS_SMS_ONLY_REGISTERED_ROAMING,
	SOC_MOBILE_STATUS_EMERGENCY_REGISTERED,
	SOC_MOBILE_STATUS_CSFB_NOT_PREFERRED_REGISTERED,
	SOC_MOBILE_STATUS_CSFB_NOT_PREFERRED_REGISTERED_ROAMING,
}SOC_MOBILE_REGISTER_STATUS_E;

typedef enum SOC_MOBILE_CELL_INFO_STATUS
{
	SOC_MOBILE_CELL_INFO_UPDATE = 0,
	SOC_MOBILE_SIGNAL_UPDATE,
}SOC_MOBILE_CELL_INFO_STATUS_E;

typedef enum SOC_MOBILE_PDP_STATUS
{
	SOC_MOBILE_PDP_ACTIVED = 0,
	SOC_MOBILE_PDP_DEACTIVING,
	SOC_MOBILE_PDP_DEACTIVED,
}SOC_MOBILE_PDP_STATUS_E;

typedef enum SOC_MOBILE_NETIF_STATUS
{
	SOC_MOBILE_NETIF_LINK_ON = 0,
	SOC_MOBILE_NETIF_LINK_OFF,
	SOC_MOBILE_NETIF_LINK_OOS,	//失去网络连接，尝试恢复中，等同于SOC_MOBILE_NETIF_LINK_OFF
}SOC_MOBILE_NETIF_STATUS_E;

typedef enum SOC_MOBILE_BEARER_STATUS
{
	SOC_MOBILE_BEARER_GET_DEFAULT_APN = 0,
	SOC_MOBILE_BEARER_APN_SET_DONE,
	SOC_MOBILE_BEARER_AUTH_SET_DONE,
	SOC_MOBILE_BEARER_DEL_DONE,
	SOC_MOBILE_BEARER_SET_ACT_STATE_DONE,
}SOC_MOBILE_BEARER_STATUS_E;

typedef enum SOC_MOBILE_SMS_STATUS
{
	SOC_MOBILE_SMS_READY = 0,
	SOC_MOBILE_NEW_SMS,
	SOC_MOBILE_SMS_SEND_DONE,
	SOC_MOBILE_SMS_ACK,
}SOC_MOBILE_SMS_STATUS_E;


void soc_vsprintf(const char * format, va_list ap);


const char *soc_get_sdk_type(void);
const char *soc_get_sdk_version(void);
const char *soc_get_chip_name(void);
int soc_get_model_name(char *model, uint8_t is_full);
char *soc_get_hardware_version(void);

int soc_get_time_zone(void);
void soc_set_time_zone(int time_zone);
int soc_get_muid(char *muid, uint32_t size);

void soc_mobile_event_register_handler(void *handle);
void soc_misc_event_register_handler(void *handle);
#if 0
void soc_mobile_get_iccid(uint8_t sim_id, uint8_t *buf);
void soc_mobile_get_imsi(uint8_t sim_id, uint8_t *buf);
void soc_mobile_set_sim_id(uint8_t sim_id);
void soc_mobile_get_sim_id(uint8_t *sim_id, uint8_t *is_auto);
uint8_t soc_mobile_get_sim_state(uint8_t sim_id);
void soc_mobile_reset_stack(void);
void soc_mobile_fly_mode(uint8_t sim_id, uint8_t on_off);
int soc_mobile_comm_mode(uint8_t sim_id);
void soc_mobile_fatal_error_auto_reset_stack(uint8_t onoff);

void soc_mobile_get_sim_id(uint8_t *sim_id, uint8_t *is_auto);
char *soc_mobile_get_default_apn_info(uint8_t sim_id);
char *soc_mobile_get_apn_info(uint8_t sim_id, uint8_t cid, uint32_t *size);
void soc_mobile_set_ipv6_type(uint8_t on_off);
uint8_t soc_mobile_get_ipv6_type(void);
void soc_mobile_get_ip_data_traffic(uint8_t sim_id, uint64_t *uplink, uint64_t *downlink);
void soc_mobile_clear_ip_data_traffic(uint8_t sim_id, uint8_t clear_uplink, uint8_t clear_downlink);
void soc_mobile_set_user_apn_auto_active(uint8_t sim_id, uint8_t cid,
		uint8_t ip_type,
		uint8_t protocol_type,
		uint8_t *apn_name, uint8_t apn_name_len,
		uint8_t *user, uint8_t user_len,
		uint8_t *password, uint8_t password_len);
void soc_mobile_clear_apn(uint8_t sim_id);

void soc_mobile_set_rrc_release_time(uint8_t s);
void soc_mobile_set_check_sim_period(uint32_t check_sim_period);
void soc_mobile_set_check_network_period(uint32_t check_network_period);
void soc_mobile_set_cell_search_period(uint32_t search_period);

uint8_t soc_mobile_get_csq(void);
int soc_mobile_start_get_cell_list(uint8_t sim_id, uint8_t is_sync);
void soc_mobile_print_cell_info(uint8_t sim_id);
#endif

#endif /* CORE_INCLUDE_SOC_SERVICE_H_ */
