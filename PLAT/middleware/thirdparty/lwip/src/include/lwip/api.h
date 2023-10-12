/**
 * @file
 * netconn API (to be used from non-TCPIP threads)
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
 *
 */
#ifndef LWIP_HDR_API_H
#define LWIP_HDR_API_H

#include "lwip/opt.h"

#if LWIP_NETCONN || LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */
/* Note: Netconn API is always available when sockets are enabled -
 * sockets are implemented on top of them */

#include "lwip/arch.h"
#include "lwip/netbuf.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"

#ifdef __cplusplus
extern "C" {
#endif

#if ENABLE_PSIF
typedef enum {
    SOCK_STREAM_TYPE = 1,
    SOCK_DGRAM_TYPE = 2,
    SOCK_RAW_TYPE = 3,
}SockInfoType;

/*
 * the data flag with the socket data
*/
typedef struct sockdataflag{
    u32_t  imsflag:2; //indicate the ul data flag with ims related attribute. then UE will do some special process with the ul socket data. please reference the define "sockimsdatatype_t"
    u32_t dataRai:2;
    u32_t bExceptData:1;
    u32_t rsvd:27;
}sockdataflag_t;
#endif

/* Throughout this file, IP addresses and port numbers are expected to be in
 * the same byte order as in the corresponding pcb.
 */

/* Flags for netconn_write (u8_t) */
#define NETCONN_NOFLAG    0x00
#define NETCONN_NOCOPY    0x00 /* Only for source code compatibility */
#define NETCONN_COPY      0x01
#define NETCONN_MORE      0x02
#define NETCONN_DONTBLOCK 0x04

/* Flags for struct netconn.flags (u8_t) */
/** Should this netconn avoid blocking? */
#define NETCONN_FLAG_NON_BLOCKING             0x02
/** Was the last connect action a non-blocking one? */
#define NETCONN_FLAG_IN_NONBLOCKING_CONNECT   0x04
/** If a nonblocking write has been rejected before, poll_tcp needs to
    check if the netconn is writable again */
#define NETCONN_FLAG_CHECK_WRITESPACE         0x10
#if LWIP_IPV6
/** If this flag is set then only IPv6 communication is allowed on the
    netconn. As per RFC#3493 this features defaults to OFF allowing
    dual-stack usage by default. */
#define NETCONN_FLAG_IPV6_V6ONLY              0x20

#if ENABLE_PSIF

/* if the flag is set, UL UDP will not delay and retry after UL high water*/
#define NETCONN_FLAG_HIGH_WATER_WITHOUT_DELAY_RETRY      0x40

#endif

#endif /* LWIP_IPV6 */


/* Helpers to process several netconn_types by the same code */
#define NETCONNTYPE_GROUP(t)         ((t)&0xF0)
#define NETCONNTYPE_DATAGRAM(t)      ((t)&0xE0)
#if LWIP_IPV6
#define NETCONN_TYPE_IPV6            0x08
#define NETCONNTYPE_ISIPV6(t)        (((t)&NETCONN_TYPE_IPV6) != 0)
#define NETCONNTYPE_ISUDPLITE(t)     (((t)&0xF3) == NETCONN_UDPLITE)
#define NETCONNTYPE_ISUDPNOCHKSUM(t) (((t)&0xF3) == NETCONN_UDPNOCHKSUM)
#else /* LWIP_IPV6 */
#define NETCONNTYPE_ISIPV6(t)        (0)
#define NETCONNTYPE_ISUDPLITE(t)     ((t) == NETCONN_UDPLITE)
#define NETCONNTYPE_ISUDPNOCHKSUM(t) ((t) == NETCONN_UDPNOCHKSUM)
#endif /* LWIP_IPV6 */

/** @ingroup netconn_common
 * Protocol family and type of the netconn
 */
enum netconn_type {
  NETCONN_INVALID     = 0,
  /** TCP IPv4 */
  NETCONN_TCP         = 0x10,
#if LWIP_IPV6
  /** TCP IPv6 */
  NETCONN_TCP_IPV6    = NETCONN_TCP | NETCONN_TYPE_IPV6 /* 0x18 */,
#endif /* LWIP_IPV6 */
  /** UDP IPv4 */
  NETCONN_UDP         = 0x20,
  /** UDP IPv4 lite */
  NETCONN_UDPLITE     = 0x21,
  /** UDP IPv4 no checksum */
  NETCONN_UDPNOCHKSUM = 0x22,

#if LWIP_IPV6
  /** UDP IPv6 (dual-stack by default, unless you call @ref netconn_set_ipv6only) */
  NETCONN_UDP_IPV6         = NETCONN_UDP | NETCONN_TYPE_IPV6 /* 0x28 */,
  /** UDP IPv6 lite (dual-stack by default, unless you call @ref netconn_set_ipv6only) */
  NETCONN_UDPLITE_IPV6     = NETCONN_UDPLITE | NETCONN_TYPE_IPV6 /* 0x29 */,
  /** UDP IPv6 no checksum (dual-stack by default, unless you call @ref netconn_set_ipv6only) */
  NETCONN_UDPNOCHKSUM_IPV6 = NETCONN_UDPNOCHKSUM | NETCONN_TYPE_IPV6 /* 0x2a */,
#endif /* LWIP_IPV6 */

  /** Raw connection IPv4 */
  NETCONN_RAW         = 0x40
#if LWIP_IPV6
  /** Raw connection IPv6 (dual-stack by default, unless you call @ref netconn_set_ipv6only) */
  , NETCONN_RAW_IPV6    = NETCONN_RAW | NETCONN_TYPE_IPV6 /* 0x48 */
#endif /* LWIP_IPV6 */
};

/** Current state of the netconn. Non-TCP netconns are always
 * in state NETCONN_NONE! */
enum netconn_state {
  NETCONN_NONE,
  NETCONN_WRITE,
  NETCONN_LISTEN,
  NETCONN_CONNECT,
  NETCONN_CLOSE
};

/** Used to inform the callback function about changes
 *
 * Event explanation:
 *
 * In the netconn implementation, there are three ways to block a client:
 *
 * - accept mbox (sys_arch_mbox_fetch(&conn->acceptmbox, &accept_ptr, 0); in netconn_accept())
 * - receive mbox (sys_arch_mbox_fetch(&conn->recvmbox, &buf, 0); in netconn_recv_data())
 * - send queue is full (sys_arch_sem_wait(LWIP_API_MSG_SEM(msg), 0); in lwip_netconn_do_write())
 *
 * The events have to be seen as events signaling the state of these mboxes/semaphores. For non-blocking
 * connections, you need to know in advance whether a call to a netconn function call would block or not,
 * and these events tell you about that.
 *
 * RCVPLUS events say: Safe to perform a potentially blocking call call once more.
 * They are counted in sockets - three RCVPLUS events for accept mbox means you are safe
 * to call netconn_accept 3 times without being blocked.
 * Same thing for receive mbox.
 *
 * RCVMINUS events say: Your call to to a possibly blocking function is "acknowledged".
 * Socket implementation decrements the counter.
 *
 * For TX, there is no need to count, its merely a flag. SENDPLUS means you may send something.
 * SENDPLUS occurs when enough data was delivered to peer so netconn_send() can be called again.
 * A SENDMINUS event occurs when the next call to a netconn_send() would be blocking.
 */
enum netconn_evt {
  NETCONN_EVT_RCVPLUS,
  NETCONN_EVT_RCVMINUS,
  NETCONN_EVT_SENDPLUS,
  NETCONN_EVT_SENDMINUS,
#if LWIP_SOCKET_CALL_BACK_ENABLE
  NETCONN_EVT_CONNECTED,
  NETCONN_EVT_ACCEPTPLUS,
  NETCONN_EVT_SENDACKED,
  NETCONN_EVT_ERROR_CLSD,
  NETCONN_EVT_ERROR_RST,
  NETCONN_EVT_ERROR_ABORT,
#endif
  NETCONN_EVT_ERROR
};

#if LWIP_IGMP || (LWIP_IPV6 && LWIP_IPV6_MLD)
/** Used for netconn_join_leave_group() */
enum netconn_igmp {
  NETCONN_JOIN,
  NETCONN_LEAVE
};
#endif /* LWIP_IGMP || (LWIP_IPV6 && LWIP_IPV6_MLD) */

#if LWIP_DNS
/* Used for netconn_gethostbyname_addrtype(), these should match the DNS_ADDRTYPE defines in dns.h */
#define NETCONN_DNS_DEFAULT   NETCONN_DNS_IPV4_IPV6
#define NETCONN_DNS_IPV4      0
#define NETCONN_DNS_IPV6      1
#define NETCONN_DNS_IPV4_IPV6 2 /* try to resolve IPv4 first, try IPv6 if IPv4 fails only */
#define NETCONN_DNS_IPV6_IPV4 3 /* try to resolve IPv6 first, try IPv4 if IPv6 fails only */
#endif /* LWIP_DNS */

/* forward-declare some structs to avoid to include their headers */
struct ip_pcb;
struct tcp_pcb;
struct udp_pcb;
struct raw_pcb;
struct netconn;
struct api_msg;

/** A callback prototype to inform about events for a netconn */
typedef void (* netconn_callback)(struct netconn *, enum netconn_evt, u16_t len);

/** A netconn descriptor */
struct netconn {
  /** type of the netconn (TCP, UDP or RAW) */
  enum netconn_type type;
  /** current state of the netconn */
  enum netconn_state state;
  /** the lwIP internal protocol control block */
  union {
    struct ip_pcb  *ip;
    struct tcp_pcb *tcp;
    struct udp_pcb *udp;
    struct raw_pcb *raw;
  } pcb;
  /** the last error this netconn had */
  err_t last_err;
#if !LWIP_NETCONN_SEM_PER_THREAD || !LWIP_NETCONN_SEM_PER_OPERATION
  /** sem that is used to synchronously execute functions in the core context */
  sys_sem_t op_completed;
#endif
  /** mbox where received packets are stored until they are fetched
      by the netconn application thread (can grow quite big) */
  sys_mbox_t recvmbox;
#if LWIP_TCP
  /** mbox where new connections are stored until processed
      by the application thread */
  sys_mbox_t acceptmbox;
#endif /* LWIP_TCP */
  /** only used for socket layer */
#if LWIP_SOCKET
  int socket;
#endif /* LWIP_SOCKET */
#if LWIP_SO_SNDTIMEO
  /** timeout to wait for sending data (which means enqueueing data for sending
      in internal buffers) in milliseconds */
  s32_t send_timeout;
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVTIMEO
  /** timeout in milliseconds to wait for new data to be received
      (or connections to arrive for listening netconns) */
  int recv_timeout;
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVBUF
  /** maximum amount of bytes queued in recvmbox
      not used for TCP: adjust TCP_WND instead! */
  int recv_bufsize;
  /** number of bytes currently in recvmbox to be received,
      tested against recv_bufsize to limit bytes on recvmbox
      for UDP and RAW, used for FIONREAD */
  int recv_avail;
#endif /* LWIP_SO_RCVBUF */
#if LWIP_SO_LINGER
   /** values <0 mean linger is disabled, values > 0 are seconds to linger */
  s16_t linger;
#endif /* LWIP_SO_LINGER */
  /** flags holding more netconn-internal state, see NETCONN_FLAG_* defines */
  u8_t flags;
#if LWIP_TCP
  /** TCP: when data passed to netconn_write doesn't fit into the send buffer,
      this temporarily stores how much is already sent. */
  size_t write_offset;
  /** TCP: when data passed to netconn_write doesn't fit into the send buffer,
      this temporarily stores the message.
      Also used during connect and close. */
  struct api_msg *current_msg;
#endif /* LWIP_TCP */
  /** A callback function that is informed about events for this netconn */
  netconn_callback callback;
#if LWIP_SOCKET_CALL_BACK_ENABLE
  void *socket_callback;
#endif
};

/** Register an Network connection event */
#define API_EVENT(c,e,l) if (c->callback) {         \
                           (*c->callback)(c, e, l); \
                         }

/** Set conn->last_err to err but don't overwrite fatal errors */
#define NETCONN_SET_SAFE_ERR(conn, err) do { if ((conn) != NULL) { \
  SYS_ARCH_DECL_PROTECT(netconn_set_safe_err_lev); \
  SYS_ARCH_PROTECT(netconn_set_safe_err_lev); \
  if (!ERR_IS_FATAL((conn)->last_err)) { \
    (conn)->last_err = err; \
  } \
  SYS_ARCH_UNPROTECT(netconn_set_safe_err_lev); \
}} while(0);

/* Network connection functions: */

/** @ingroup netconn_common
 * Create new netconn connection
 * @param t @ref netconn_type */
#define netconn_new(t)                  netconn_new_with_proto_and_callback(t, 0, NULL)
#define netconn_new_with_callback(t, c) netconn_new_with_proto_and_callback(t, 0, c)
struct netconn *netconn_new_with_proto_and_callback(enum netconn_type t, u8_t proto,
                                             netconn_callback callback);
err_t   netconn_delete(struct netconn *conn);
/** Get the type of a netconn (as enum netconn_type). */
#define netconn_type(conn) (conn->type)

err_t   netconn_getaddr(struct netconn *conn, ip_addr_t *addr,
                        u16_t *port, u8_t local);
/** @ingroup netconn_common */
#define netconn_peer(c,i,p) netconn_getaddr(c,i,p,0)
/** @ingroup netconn_common */
#define netconn_addr(c,i,p) netconn_getaddr(c,i,p,1)

err_t   netconn_bind(struct netconn *conn, const ip_addr_t *addr, u16_t port);

#if ENABLE_PSIF
err_t   netconn_bind_cid(struct netconn *conn, u8_t cid);
err_t
netconn_alloca_server_port(u8_t type, int32_t *server_port);

#endif

err_t   netconn_connect(struct netconn *conn, const ip_addr_t *addr, u16_t port);
err_t   netconn_disconnect (struct netconn *conn);
err_t   netconn_listen_with_backlog(struct netconn *conn, u8_t backlog);
/** @ingroup netconn_tcp */
#define netconn_listen(conn) netconn_listen_with_backlog(conn, TCP_DEFAULT_LISTEN_BACKLOG)
err_t   netconn_accept(struct netconn *conn, struct netconn **new_conn);
err_t   netconn_recv(struct netconn *conn, struct netbuf **new_buf);
err_t   netconn_recv_tcp_pbuf(struct netconn *conn, struct pbuf **new_buf);
err_t   netconn_sendto(struct netconn *conn, struct netbuf *buf,
                             const ip_addr_t *addr, u16_t port);
err_t   netconn_send(struct netconn *conn, struct netbuf *buf);
#if ENABLE_PSIF
err_t   netconn_write_partly(struct netconn *conn, const void *dataptr, size_t size,
                             u8_t apiflags, size_t *bytes_written, sockdataflag_t dataflags, u8_t sequence);
#else
err_t   netconn_write_partly(struct netconn *conn, const void *dataptr, size_t size,
                             u8_t apiflags, size_t *bytes_written);
#endif
/** @ingroup netconn_tcp */

#if ENABLE_PSIF
#define netconn_write(conn, dataptr, size, apiflags) \
          netconn_write_partly(conn, dataptr, size, apiflags, NULL, 0, 0)
#else
#define netconn_write(conn, dataptr, size, apiflags) \
          netconn_write_partly(conn, dataptr, size, apiflags, NULL)
#endif
err_t   netconn_close(struct netconn *conn);
err_t   netconn_shutdown(struct netconn *conn, u8_t shut_rx, u8_t shut_tx);

#if LWIP_IGMP || (LWIP_IPV6 && LWIP_IPV6_MLD)
err_t   netconn_join_leave_group(struct netconn *conn, const ip_addr_t *multiaddr,
                             const ip_addr_t *netif_addr, enum netconn_igmp join_or_leave);
#endif /* LWIP_IGMP || (LWIP_IPV6 && LWIP_IPV6_MLD) */
#if LWIP_DNS

#if LWIP_IPV4 && LWIP_IPV6
err_t   netconn_gethostbyname_addrtype(const char *name, ip_addr_t *addr, u8_t dns_addrtype, uint8_t cid);
#define netconn_gethostbyname(name, addr, cid) netconn_gethostbyname_addrtype(name, addr, NETCONN_DNS_DEFAULT, cid)
err_t   netconn_gethostbyname_addrtype_async(const char *name, u8_t dns_addrtype, uint8_t cid, uint16_t src_handler);
#define netconn_gethostbyname_async(name, cid, src_handler) netconn_gethostbyname_addrtype_async(name, NETCONN_DNS_DEFAULT, cid, src_handler)
err_t   netconn_gethostbyname_addrtype_async_by_cmsSockMgr(const char *name, u8_t dns_addrtype, uint8_t cid, uint8_t source);
#define netconn_gethostbyname_async_by_cmsSockMgr(name, cid, source) netconn_gethostbyname_addrtype_async_by_cmsSockMgr(name, NETCONN_DNS_DEFAULT, cid, source)
#else /* LWIP_IPV4 && LWIP_IPV6 */
err_t   netconn_gethostbyname(const char *name, ip_addr_t *addr, uint8_t cid);
#define netconn_gethostbyname_addrtype(name, addr, dns_addrtype, cid) netconn_gethostbyname(name, addr, cid)
err_t   netconn_gethostbyname_async(const char *name, uint8_t cid, uint16_t src_handler);
#define netconn_gethostbyname_addrtype_async(name, dns_addrtype, cid, src_handler) netconn_gethostbyname_async(name, cid, src_handler)
err_t   netconn_gethostbyname_async_by_cmsSockMgr(const char *name, uint8_t cid, uint8_t source);
#define netconn_gethostbyname_addrtype_async_by_cmsSockMgr(name, dns_addrtype, cid, source) netconn_gethostbyname_async_by_cmsSockMgr(name, cid, source)

#endif /* LWIP_IPV4 && LWIP_IPV6 */
#endif /* LWIP_DNS */

#define netconn_err(conn)               ((conn)->last_err)
#define netconn_recv_bufsize(conn)      ((conn)->recv_bufsize)

/** Set the blocking status of netconn calls (@todo: write/send is missing) */
#define netconn_set_nonblocking(conn, val)  do { if(val) { \
  (conn)->flags |= NETCONN_FLAG_NON_BLOCKING; \
} else { \
  (conn)->flags &= ~ NETCONN_FLAG_NON_BLOCKING; }} while(0)
/** Get the blocking status of netconn calls (@todo: write/send is missing) */
#define netconn_is_nonblocking(conn)        (((conn)->flags & NETCONN_FLAG_NON_BLOCKING) != 0)

#if ENABLE_PSIF
/** Set the blocking status of netconn calls (@todo: write/send is missing) */
#define netconn_set_highwater_without_delay_retry(conn, val)  do { if(val) { \
  (conn)->flags |= NETCONN_FLAG_HIGH_WATER_WITHOUT_DELAY_RETRY; \
} else { \
  (conn)->flags &= ~ NETCONN_FLAG_HIGH_WATER_WITHOUT_DELAY_RETRY; }} while(0)
/** Get the blocking status of netconn calls (@todo: write/send is missing) */
#define netconn_is_highwater_without_delay_retry(conn)        (((conn)->flags & NETCONN_FLAG_HIGH_WATER_WITHOUT_DELAY_RETRY) != 0)

#endif

#if LWIP_IPV6
/** @ingroup netconn_common
 * TCP: Set the IPv6 ONLY status of netconn calls (see NETCONN_FLAG_IPV6_V6ONLY)
 */
#define netconn_set_ipv6only(conn, val)  do { if(val) { \
  (conn)->flags |= NETCONN_FLAG_IPV6_V6ONLY; \
} else { \
  (conn)->flags &= ~ NETCONN_FLAG_IPV6_V6ONLY; }} while(0)
/** @ingroup netconn_common
 * TCP: Get the IPv6 ONLY status of netconn calls (see NETCONN_FLAG_IPV6_V6ONLY)
 */
#define netconn_get_ipv6only(conn)        (((conn)->flags & NETCONN_FLAG_IPV6_V6ONLY) != 0)
#endif /* LWIP_IPV6 */

#if LWIP_SO_SNDTIMEO
/** Set the send timeout in milliseconds */
#define netconn_set_sendtimeout(conn, timeout)      ((conn)->send_timeout = (timeout))
/** Get the send timeout in milliseconds */
#define netconn_get_sendtimeout(conn)               ((conn)->send_timeout)
#endif /* LWIP_SO_SNDTIMEO */
#if LWIP_SO_RCVTIMEO
/** Set the receive timeout in milliseconds */
#define netconn_set_recvtimeout(conn, timeout)      ((conn)->recv_timeout = (timeout))
/** Get the receive timeout in milliseconds */
#define netconn_get_recvtimeout(conn)               ((conn)->recv_timeout)
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVBUF
/** Set the receive buffer in bytes */
#define netconn_set_recvbufsize(conn, recvbufsize)  ((conn)->recv_bufsize = (recvbufsize))
/** Get the receive buffer in bytes */
#define netconn_get_recvbufsize(conn)               ((conn)->recv_bufsize)
#endif /* LWIP_SO_RCVBUF*/

#if LWIP_NETCONN_SEM_PER_THREAD
void netconn_thread_init(void);
void netconn_thread_cleanup(void);
#else /* LWIP_NETCONN_SEM_PER_THREAD */
#define netconn_thread_init()
#define netconn_thread_cleanup()
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

#if ENABLE_PSIF
err_t netconn_get_net_info_with_cid(u8_t *result, u8_t cid);
err_t netconn_get_sock_info(int fd, ip_addr_t *local_ip, ip_addr_t *remote_ip, u16_t *local_port, u16_t *remote_port, int *type);
err_t netconn_get_hib_sock_id(int *tcp_sockid, int *udp_sockid);
err_t netconn_get_tcp_send_buffer_size(struct netconn *conn, uint16_t *buffer_size);
err_t netconn_get_sock_info_by_pcb(int socket_type, void *pcb, ip_addr_t *local_ip, ip_addr_t *remote_ip, u16_t *local_port, u16_t *remote_port);

err_t netconn_set_rohc_rtp_cfg(u8_t cid, ip_addr_t *dstIp, u16_t srcPort, u16_t dstPort);

err_t netconn_clean_rohc_rtp_cfg(u8_t cid);

#endif

#ifdef __cplusplus
}
#endif

#endif /* LWIP_NETCONN || LWIP_SOCKET */

#endif /* LWIP_HDR_API_H */
