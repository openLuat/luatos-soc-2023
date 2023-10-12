/**
 * @file
 * Timer implementations
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *         Simon Goldschmidt
 *
 */
#ifndef LWIP_HDR_TIMEOUTS_H
#define LWIP_HDR_TIMEOUTS_H

#include "lwip/opt.h"
#include "lwip/err.h"
#if !NO_SYS
#include "lwip/sys.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LWIP_DEBUG_TIMERNAMES
#if 0   //disable timeout name debug info to save flash size
#ifdef LWIP_DEBUG
#define LWIP_DEBUG_TIMERNAMES SYS_DEBUG
#else /* LWIP_DEBUG */
#define LWIP_DEBUG_TIMERNAMES 0
#endif /* LWIP_DEBUG*/
#else
#define LWIP_DEBUG_TIMERNAMES 0
#endif
#endif

/** Function prototype for a stack-internal timer function that has to be
 * called at a defined interval */
typedef void (* lwip_cyclic_timer_handler)(void);

/** This struct contains information about a stack-internal timer function
 that has to be called at a defined interval */
struct lwip_cyclic_timer {
  u32_t interval_ms;
  lwip_cyclic_timer_handler handler;
#if LWIP_DEBUG_TIMERNAMES
  const char* handler_name;
#endif /* LWIP_DEBUG_TIMERNAMES */
};


/** This array contains all stack-internal cyclic timers. To get the number of
 * timers, use LWIP_ARRAYSIZE() */
extern const struct lwip_cyclic_timer lwip_cyclic_timers[];

#if LWIP_TIMERS

/** Function prototype for a timeout callback function. Register such a function
 * using sys_timeout().
 *
 * @param arg Additional argument to pass to the function - set up by sys_timeout()
 */
typedef void (* sys_timeout_handler)(void *arg);

struct sys_timeo {
  struct sys_timeo *next;
  u32_t time;
  sys_timeout_handler h;
  void *arg;
#if LWIP_DEBUG_TIMERNAMES
  const char* handler_name;
#endif /* LWIP_DEBUG_TIMERNAMES */
};

void sys_timeouts_init(void);

#if LWIP_TIMER_ON_DEMOND
enum {
#if LWIP_DNS
    LWIP_SYS_TIMER_TYPE_DNS_RETRY,
    LWIP_SYS_TIMER_TYPE_DNS_CACHE,
#endif
#if LWIP_IPV6
    LWIP_SYS_TIMER_TYPE_ND6_NEIGHBOR_CACHE,
    LWIP_SYS_TIMER_TYPE_ND6_ROUTER_ENTRY,
    LWIP_SYS_TIMER_TYPE_ND6_PREFIX_ENTRY,
    LWIP_SYS_TIMER_TYPE_ND6_ADDRESS_DAD,
    LWIP_SYS_TIMER_TYPE_ND6_RS_RETRY,
    LWIP_SYS_TIMER_TYPE_ND6_RS_REFRESH,
    LWIP_SYS_TIMER_TYPE_ND6_NEIGHBOR_DISCOVERY_RETRY,
#endif
#if LWIP_TCP
    LWIP_SYS_TIMER_TYPE_TCP_RETRY,
    LWIP_SYS_TIMER_TYPE_TCP_DELAY_ACK,
    LWIP_SYS_TIMER_TYPE_TCP_PENDING_FIN,
    LWIP_SYS_TIMER_TYPE_TCP_REFUSE_DATA,
    LWIP_SYS_TIMER_TYPE_TCP_FIN_WAIT2_TIMEOUT,
    LWIP_SYS_TIMER_TYPE_TCP_KEEPALIVE_TIMEOUT,
    LWIP_SYS_TIMER_TYPE_TCP_OOSEQ_TIMEOUT,
    LWIP_SYS_TIMER_TYPE_TCP_SYNCRCV_TIMEOUT,
    LWIP_SYS_TIMER_TYPE_TCP_LASTACK_TIMEOUT,
    LWIP_SYS_TIMER_TYPE_TCP_TIMEWAIT_TIMEOUT,
    LWIP_SYS_TIMER_TYPE_TCP_TOTAL_RETRY_TIMEOUT,
    LWIP_SYS_TIMER_TYPE_TCP_CLOSE_LOCAL_ABORT_TIMEOUT,
    LWIP_SYS_TIMER_TYPE_TCP_POLL_TIMEOUT,
#endif
#if LWIP_IGMP
    LWIP_SYS_TIMER_TYPE_IGMP_REPORT_TIMEOUT,
#endif
#if LWIP_IPV6_MLD
    LWIP_SYS_TIMER_TYPE_MLD6_REPORT_TIMEOUT,
#endif
#if LWIP_IPV4
    LWIP_SYS_TIMER_TYPE_IP4_FRAG_TIMEOUT,
#endif
#if LWIP_IPV6
    LWIP_SYS_TIMER_TYPE_IP6_FRAG_TIMEOUT,
#endif
    LWIP_SYS_TIMER_TYPE_IDLE,
#if LWIP_ARP
    LWIP_SYS_TIMER_TYPE_ARP_TABLE_CACHE_TIMEOUT,
    LWIP_SYS_TIMER_TYPE_ARP_retry_TIMEOUT,
#endif
#if LWIP_ENABLE_IPV6_RA_SERVER
    LWIP_SYS_TIMER_TYPE_IP6_RA_RETRANS_TIMEOUT,
#endif
    LWIP_SYS_TIMER_IPSEC_LIFETIME_TIMEOUT,
    LWIP_SYS_TIMER_TYPE_MAX,
};

extern const sys_timeout_handler lwip_sys_timeout_handler_list[LWIP_SYS_TIMER_TYPE_MAX];

#endif


#if LWIP_DEBUG_TIMERNAMES
void sys_timeout_debug(u32_t msecs, sys_timeout_handler handler, void *arg, const char* handler_name);
#define sys_timeout(msecs, handler, arg) sys_timeout_debug(msecs, handler, arg, #handler)
#else /* LWIP_DEBUG_TIMERNAMES */
void sys_timeout(u32_t msecs, sys_timeout_handler handler, void *arg);
#endif /* LWIP_DEBUG_TIMERNAMES */

void sys_untimeout(sys_timeout_handler handler, void *arg);
void sys_restart_timeouts(void);
#if NO_SYS
void sys_check_timeouts(void);
u32_t sys_timeouts_sleeptime(void);
#else /* NO_SYS */
void sys_timeouts_mbox_fetch(sys_mbox_t *mbox, void **msg);
#endif /* NO_SYS */

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
u8_t sys_check_timeouts_for_hib_sleep2(void);
#endif

#if LWIP_TIMER_ON_DEMOND
void tcpip_idle_timer(void *arg);
#endif


#endif /* LWIP_TIMERS */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_TIMEOUTS_H */
