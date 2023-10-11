#include "common_api.h"
#include "luat_base.h"
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "mbedtls/cipher.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/md5.h"
#include "luat_network_adapter.h"
#include "networkmgr.h"

uint32_t luat_get_utc(uint32_t *tamp)
{
	uint32_t sec = soc_get_utc();
	if (tamp)
	{
		*tamp = sec;
	}
	return sec;
}

void luat_os_entry_cri(void) {
  portENTER_CRITICAL();
}

void luat_os_exit_cri(void) {
  portEXIT_CRITICAL();
}

// delay_us 是系统函数
extern void delay_us(uint32_t us);
void luat_timer_us_delay(size_t us) {
  if (us > 0)
    delay_us(us);
}

extern void soc_vsprintf(uint8_t no_print, const char *fmt, va_list ap);
extern void soc_debug_out(char *string, uint32_t size);
void DBG_Printf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	soc_vsprintf(0, fmt, ap);
	va_end(ap);
}

extern const uint8_t ByteToAsciiTable[16];
void DBG_HexPrintf(void *Data, unsigned int len)
{
	uint8_t *data = (uint8_t *)Data;
	uint8_t *uart_buf;
    uint32_t i,j;
    j = 0;
    if (!len) return;
    uart_buf = malloc(len * 3 + 2);
    if (!uart_buf) return;
    memset(uart_buf, 0, len * 3 + 2);
    for (i = 0; i < len; i++){
		uart_buf[j++] = ByteToAsciiTable[(data[i] & 0xf0) >> 4];
		uart_buf[j++] = ByteToAsciiTable[data[i] & 0x0f];
		uart_buf[j++] = ' ';
    }
    uart_buf[j++] = '\r';
    uart_buf[j++] = '\n';
    soc_debug_out((char*)uart_buf, len * 3 + 2);
	free(uart_buf);
}

struct tm *mbedtls_platform_gmtime_r( const mbedtls_time_t *tt,
                                     struct tm *tm_buf )
{
	Date_UserDataStruct Date;
	Time_UserDataStruct Time;
	Tamp2UTC(*tt, &Date, &Time, 0);
	tm_buf->tm_year = Date.Year - 1900;
	tm_buf->tm_mon = Date.Mon - 1;
	tm_buf->tm_mday = Date.Day;
	tm_buf->tm_hour = Time.Hour;
	tm_buf->tm_min = Time.Min;
	tm_buf->tm_sec = Time.Sec;
	return tm_buf;

}

#include "osasys.h"
extern int soc_mobile_get_default_pdp_part_info(uint8_t *ip_type, uint8_t *apn,uint8_t *apn_len, uint8_t *dns_num, ip_addr_t *dns_ip);

time_t luat_time(time_t *_Time) {
  utc_timer_value_t *timeUtc = OsaSystemTimeReadUtc();
  if (_Time != NULL) {
    *_Time = timeUtc->UTCsecs;
  }
  return timeUtc->UTCsecs;
}

int luat_timer_mdelay(size_t ms) {
    if (ms > 0) {
        vTaskDelay(ms);
    }
    return 0;
}

void luat_socket_check_ready(uint32_t param, uint8_t *is_ipv6)
{
	ip_addr_t dns_ip[2];
	uint8_t type, dns_num, ipv6;
	dns_num = 2;
	if (!is_ipv6) is_ipv6 = &ipv6;
	*is_ipv6 = 0;
	soc_mobile_get_default_pdp_part_info(&type, NULL, NULL, &dns_num, dns_ip);

	if (type & 0x80)
	{
		if (param != 4)
		{
			return;
		}
		else
		{
			NmAtiNetifInfo *pNetifInfo = malloc(sizeof(NmAtiNetifInfo));
			NetMgrGetNetInfo(0xff, pNetifInfo);
			if (pNetifInfo->ipv6Cid != 0xff)
			{
				net_lwip_set_local_ip6(&pNetifInfo->ipv6Info.ipv6Addr);
				*is_ipv6 = 1;
			}
			free(pNetifInfo);
		}
	}
	if (dns_num > 0)
	{
		network_set_dns_server(NW_ADAPTER_INDEX_LWIP_GPRS, 2, &dns_ip[0]);
		if (dns_num > 1)
		{
			network_set_dns_server(NW_ADAPTER_INDEX_LWIP_GPRS, 3, &dns_ip[1]);
		}
	}
	net_lwip_set_link_state(NW_ADAPTER_INDEX_LWIP_GPRS, 1);
}
