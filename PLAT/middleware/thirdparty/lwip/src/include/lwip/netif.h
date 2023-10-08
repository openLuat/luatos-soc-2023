/**
 * @file
 * netif API (to be used from TCPIP thread)
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
#ifndef LWIP_HDR_NETIF_H
#define LWIP_HDR_NETIF_H

#include "lwip/opt.h"

#define ENABLE_LOOPBACK (LWIP_NETIF_LOOPBACK || LWIP_HAVE_LOOPIF)

#include "lwip/err.h"

#include "lwip/ip_addr.h"

#include "lwip/def.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"

#if ENABLE_PSIF
#include "pspdu.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* Throughout this file, IP addresses are expected to be in
 * the same byte order as in IP_PCB. */

/** Must be the maximum of all used hardware address lengths
    across all types of interfaces in use.
    This does not have to be changed, normally. */
#ifndef NETIF_MAX_HWADDR_LEN
#define NETIF_MAX_HWADDR_LEN 6U
#endif

/**
 * @defgroup netif_flags Flags
 * @ingroup netif
 * @{
 */

/** Whether the network interface is 'up'. This is
 * a software flag used to control whether this network
 * interface is enabled and processes traffic.
 * It must be set by the startup code before this netif can be used
 * (also for dhcp/autoip).
 */
#define NETIF_FLAG_UP           0x01U
/** If set, the netif has broadcast capability.
 * Set by the netif driver in its init function. */
#define NETIF_FLAG_BROADCAST    0x02U
/** If set, the interface has an active link
 *  (set by the network interface driver).
 * Either set by the netif driver in its init function (if the link
 * is up at that time) or at a later point once the link comes up
 * (if link detection is supported by the hardware). */
#define NETIF_FLAG_LINK_UP      0x04U
/** If set, the netif is an ethernet device using ARP.
 * Set by the netif driver in its init function.
 * Used to check input packet types and use of DHCP. */
#define NETIF_FLAG_ETHARP       0x08U
/** If set, the netif is an ethernet device. It might not use
 * ARP or TCP/IP if it is used for PPPoE only.
 */
#define NETIF_FLAG_ETHERNET     0x10U
/** If set, the netif has IGMP capability.
 * Set by the netif driver in its init function. */
#define NETIF_FLAG_IGMP         0x20U
/** If set, the netif has MLD6 capability.
 * Set by the netif driver in its init function. */
#define NETIF_FLAG_MLD6         0x40U

/**
 * @}
 */
#define NETIF_REPORT_TYPE_IPV4  0x01
#define NETIF_REPORT_TYPE_IPV6  0x02

#if ENABLE_PSIF


typedef enum lwip_netif_type
{
    LWIP_NETIF_TYPE_INVALID = 0,

    /* LAN netif type */
    LWIP_NETIF_TYPE_LAN_ETH,    /* If ETH LAN not link up(ECM/RNDIS), set to it,
                                 * and when LAN linkup, this type should change to RNDIS/ECM type */
    LWIP_NETIF_TYPE_LAN_ETH_RNDIS,
    LWIP_NETIF_TYPE_LAN_ETH_ECM,
    LWIP_NETIF_TYPE_LAN_PPP,

    /* WAN netif type */
    LWIP_NETIF_TYPE_WAN_INTERNET,
    LWIP_NETIF_TYPE_WAN_DEFAULT = LWIP_NETIF_TYPE_WAN_INTERNET,
    LWIP_NETIF_TYPE_WAN_IMS,    /* Not support now */
    LWIP_NETIF_TYPE_WAN_OTHER,

    LWIP_NETIF_TYPE_MAX = 0x0F
}lwip_netif_type_t;

#endif

enum lwip_internal_netif_client_data_index
{
#if LWIP_DHCP
   LWIP_NETIF_CLIENT_DATA_INDEX_DHCP,
#endif
#if LWIP_AUTOIP
   LWIP_NETIF_CLIENT_DATA_INDEX_AUTOIP,
#endif
#if LWIP_IGMP
   LWIP_NETIF_CLIENT_DATA_INDEX_IGMP,
#endif
#if LWIP_IPV6_MLD
   LWIP_NETIF_CLIENT_DATA_INDEX_MLD6,
#endif
#if LWIP_DHCPD
    LWIP_NETIF_SERVER_DATA_INDEX_DHCP,
#endif

#if LWIP_ENABLE_IPV6_RA_SERVER
    LWIP_NETIF_SERVER_RA_IPV6,
#endif

#if LWIP_LAN_RNDIS_PRIVATE_IP_DNS_RELAY
    LWIP_NETIF_DNS_RELAY,
#endif

   LWIP_NETIF_CLIENT_DATA_INDEX_MAX
};

#if LWIP_CHECKSUM_CTRL_PER_NETIF
#define NETIF_CHECKSUM_GEN_IP       0x0001
#define NETIF_CHECKSUM_GEN_UDP      0x0002
#define NETIF_CHECKSUM_GEN_TCP      0x0004
#define NETIF_CHECKSUM_GEN_ICMP     0x0008
#define NETIF_CHECKSUM_GEN_ICMP6    0x0010
#define NETIF_CHECKSUM_CHECK_IP     0x0100
#define NETIF_CHECKSUM_CHECK_UDP    0x0200
#define NETIF_CHECKSUM_CHECK_TCP    0x0400
#define NETIF_CHECKSUM_CHECK_ICMP   0x0800
#define NETIF_CHECKSUM_CHECK_ICMP6  0x1000
#define NETIF_CHECKSUM_ENABLE_ALL   0xFFFF
#define NETIF_CHECKSUM_DISABLE_ALL  0x0000
#endif /* LWIP_CHECKSUM_CTRL_PER_NETIF */

struct netif;

/** MAC Filter Actions, these are passed to a netif's igmp_mac_filter or
 * mld_mac_filter callback function. */
enum netif_mac_filter_action {
  /** Delete a filter entry */
  NETIF_DEL_MAC_FILTER = 0,
  /** Add a filter entry */
  NETIF_ADD_MAC_FILTER = 1
};

/** Function prototype for netif init functions. Set up flags and output/linkoutput
 * callback functions in this function.
 *
 * @param netif The netif to initialize
 */
typedef err_t (*netif_init_fn)(struct netif *netif);
/** Function prototype for netif->input functions. This function is saved as 'input'
 * callback function in the netif struct. Call it when a packet has been received.
 *
 * @param p The received packet, copied into a pbuf
 * @param inp The netif which received the packet
 */
typedef err_t (*netif_input_fn)(struct pbuf *p, struct netif *inp);

#if ENABLE_PSIF
/** Function prototype for netif->input functions. This function is saved as 'input'
 * callback function in the netif struct. Call it when a packet has been received.
 *
 * @param lcid the context_id with the packet
 * @param pPduHdr the packet which received
 */
typedef err_t (*psif_input_fn)(u8_t lcid, DlPduBlock *pPduHdr);

typedef err_t (*psif_pending_input_fn)(u8_t lcid);

#endif

typedef err_t (*lanif_input_fn)(u8_t lan_type, UlPduBlock *pPduHdr);




#if LWIP_IPV4
/** Function prototype for netif->output functions. Called by lwIP when a packet
 * shall be sent. For ethernet netif, set this to 'etharp_output' and set
 * 'linkoutput'.
 *
 * @param netif The netif which shall send a packet
 * @param p The packet to send (p->payload points to IP header)
 * @param ipaddr The IP address to which the packet shall be sent
 */
typedef err_t (*netif_output_fn)(struct netif *netif, struct pbuf *p,
       const ip4_addr_t *ipaddr);
#endif /* LWIP_IPV4*/

#if LWIP_IPV6
/** Function prototype for netif->output_ip6 functions. Called by lwIP when a packet
 * shall be sent. For ethernet netif, set this to 'ethip6_output' and set
 * 'linkoutput'.
 *
 * @param netif The netif which shall send a packet
 * @param p The packet to send (p->payload points to IP header)
 * @param ipaddr The IPv6 address to which the packet shall be sent
 */
typedef err_t (*netif_output_ip6_fn)(struct netif *netif, struct pbuf *p,
       const ip6_addr_t *ipaddr);
#endif /* LWIP_IPV6 */

/** Function prototype for netif->linkoutput functions. Only used for ethernet
 * netifs. This function is called by ARP when a packet shall be sent.
 *
 * @param netif The netif which shall send a packet
 * @param p The packet to send (raw ethernet packet)
 */
typedef err_t (*netif_linkoutput_fn)(struct netif *netif, struct pbuf *p);
/** Function prototype for netif status- or link-callback functions. */
typedef void (*netif_status_callback_fn)(struct netif *netif);
#if LWIP_IPV4 && LWIP_IGMP
/** Function prototype for netif igmp_mac_filter functions */
typedef err_t (*netif_igmp_mac_filter_fn)(struct netif *netif,
       const ip4_addr_t *group, enum netif_mac_filter_action action);
#endif /* LWIP_IPV4 && LWIP_IGMP */
#if LWIP_IPV6 && LWIP_IPV6_MLD
/** Function prototype for netif mld_mac_filter functions */
typedef err_t (*netif_mld_mac_filter_fn)(struct netif *netif,
       const ip6_addr_t *group, enum netif_mac_filter_action action);
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

#if LWIP_DHCP || LWIP_AUTOIP || LWIP_IGMP || LWIP_IPV6_MLD || (LWIP_NUM_NETIF_CLIENT_DATA > 0) || LWIP_DHCPD
u8_t netif_alloc_client_data_id(void);
/** @ingroup netif_cd
 * Set client data. Obtain ID from netif_alloc_client_data_id().
 */
#define netif_set_client_data(netif, id, data) netif_get_client_data(netif, id) = (data)
/** @ingroup netif_cd
 * Get client data. Obtain ID from netif_alloc_client_data_id().
 */
#define netif_get_client_data(netif, id)       (netif)->client_data[(id)]
#endif /* LWIP_DHCP || LWIP_AUTOIP || (LWIP_NUM_NETIF_CLIENT_DATA > 0) */

/** Generic data structure used for all lwIP network interfaces.
 *  The following fields should be filled in by the initialization
 *  function for the device driver: hwaddr_len, hwaddr[], mtu, flags */
struct netif {
  /** pointer to next in linked list */
  struct netif *next;

#if LWIP_IPV4
  /** IP address configuration in network byte order */
  ip_addr_t ip_addr;
  ip_addr_t netmask;
  ip_addr_t gw;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  /** Array of IPv6 addresses for this netif.
   * [0]: ipv6 local address: perfix: 0xFE80://64 + ID(64 bits) assigned by NW
   * [1]: ipv6 global address
  */
  ip_addr_t ip6_addr[LWIP_IPV6_NUM_ADDRESSES];
  /** The state of each IPv6 address (Tentative, Preferred, etc).
   * @see ip6_addr.h */
  u8_t ip6_addr_state[LWIP_IPV6_NUM_ADDRESSES];

  u8_t ip6_prefix_len[LWIP_IPV6_NUM_ADDRESSES];//1->32 bits,2->64 bits,3->96 biits
#endif /* LWIP_IPV6 */
  /** This function is called by the network device driver
   *  to pass a packet up the TCP/IP stack. */
  netif_input_fn input;
#if LWIP_IPV4
  /** This function is called by the IP module when it wants
   *  to send a packet on the interface. This function typically
   *  first resolves the hardware address, then sends the packet.
   *  For ethernet physical layer, this is usually etharp_output() */
  netif_output_fn output;
#endif /* LWIP_IPV4 */
  /** This function is called by ethernet_output() when it wants
   *  to send a packet on the interface. This function outputs
   *  the pbuf as-is on the link medium. */
  netif_linkoutput_fn linkoutput;
#if LWIP_IPV6
  /** This function is called by the IPv6 module when it wants
   *  to send a packet on the interface. This function typically
   *  first resolves the hardware address, then sends the packet.
   *  For ethernet physical layer, this is usually ethip6_output() */
  netif_output_ip6_fn output_ip6;
#endif /* LWIP_IPV6 */
#if LWIP_NETIF_STATUS_CALLBACK
  /** This function is called when the netif state is set to up or down
   */
  netif_status_callback_fn status_callback;
#endif /* LWIP_NETIF_STATUS_CALLBACK */
#if LWIP_NETIF_LINK_CALLBACK
  /** This function is called when the netif link is set to up or down
   */
  netif_status_callback_fn link_callback;
#endif /* LWIP_NETIF_LINK_CALLBACK */
#if LWIP_NETIF_REMOVE_CALLBACK
  /** This function is called when the netif has been removed */
  netif_status_callback_fn remove_callback;
#endif /* LWIP_NETIF_REMOVE_CALLBACK */
  /** This field can be set by the device driver and could point
   *  to state information for the device. */
  void *state;
#ifdef netif_get_client_data
  void* client_data[LWIP_NETIF_CLIENT_DATA_INDEX_MAX + LWIP_NUM_NETIF_CLIENT_DATA];
#endif
#if LWIP_IPV6_AUTOCONFIG
  /** is this netif enabled for IPv6 autoconfiguration */
  u8_t ip6_autoconfig_enabled;
#endif /* LWIP_IPV6_AUTOCONFIG */
#if LWIP_IPV6_SEND_ROUTER_SOLICIT
  /** Number of Router Solicitation messages that remain to be sent. */
  u8_t rs_count;
#endif /* LWIP_IPV6_SEND_ROUTER_SOLICIT */
#if LWIP_NETIF_HOSTNAME
  /* the hostname for this netif, NULL is a valid value */
  const char*  hostname;
#endif /* LWIP_NETIF_HOSTNAME */
#if LWIP_CHECKSUM_CTRL_PER_NETIF
  u16_t chksum_flags;
#endif /* LWIP_CHECKSUM_CTRL_PER_NETIF*/
  /** maximum transfer unit (in bytes) */
  u16_t mtu;
  /** number of bytes used in hwaddr */
  u8_t hwaddr_len;
  /** link level hardware address of this interface */
  u8_t hwaddr[NETIF_MAX_HWADDR_LEN];
  /** flags (@see @ref netif_flags) */
  u8_t flags;
  /** descriptive abbreviation */
  char name[2];
  /** number of this interface */
  u8_t num;
#if MIB2_STATS
  /** link type (from "snmp_ifType" enum from snmp_mib2.h) */
  u8_t link_type;
  /** (estimate) link speed */
  u32_t link_speed;
  /** timestamp at last change made (up/down) */
  u32_t ts;
  /** counters */
  struct stats_mib2_netif_ctrs mib2_counters;
#endif /* MIB2_STATS */
#if LWIP_IPV4 && LWIP_IGMP
  /** This function could be called to add or delete an entry in the multicast
      filter table of the ethernet MAC.*/
  netif_igmp_mac_filter_fn igmp_mac_filter;
#endif /* LWIP_IPV4 && LWIP_IGMP */
#if LWIP_IPV6 && LWIP_IPV6_MLD
  /** This function could be called to add or delete an entry in the IPv6 multicast
      filter table of the ethernet MAC. */
  netif_mld_mac_filter_fn mld_mac_filter;
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
#if LWIP_NETIF_HWADDRHINT
  u8_t *addr_hint;
#endif /* LWIP_NETIF_HWADDRHINT */
#if ENABLE_LOOPBACK
  /* List of packets to be queued for ourselves. */
  struct pbuf *loop_first;
  struct pbuf *loop_last;
#if LWIP_LOOPBACK_MAX_PBUFS
  u16_t loop_cnt_current;
#endif /* LWIP_LOOPBACK_MAX_PBUFS */
#endif /* ENABLE_LOOPBACK */

 /*record cid for ps netif*/
#if ENABLE_PSIF
  u8_t primary_ipv4_cid;
  u8_t primary_ipv6_cid;
  u8_t netif_type;      //lwip_netif_type_t
  u8_t bPublic;

  /* ded_cid_bitmap (dedicated bearer CID) binded to this netif,
   * example: if bit0 is set to 1, just means CID=0 is a dedicated bearer, and binded to this netif */
  u16_t ded_cid_bitmap;
  /*header compress channel*/
  u16_t rohc_enable_bitmap;

  /* PacketFilterList linked list, in order of "packet filter evaluation ID"
   * 1> Higher the value of the packet filter evaluation precedence field, lower the
   *    precedence of that packet filter is. ==> pfEpId: 0 > pfEpId: 255
  */
  void *tft_list;       /* PacketFilterList, all packet filter allocated in heap */

//add netif ipv6 timer active mask
#if LWIP_TIMER_ON_DEMOND
  u32_t netif_timer_active_mask;
#endif

//the flag whether the netif is suspend or not. 0->unsuspend, 1->suspend, 0->default value
  u8_t is_suspend;
  u8_t is_oos;

 //the ipv6 router priority for the same ipv6 subnet
  u8_t ip6_subnet_router_priority;

  u8_t arp_reply_mode;       // if enable ,UE will reply all request with ip address

//the ipv6 source address select priority
  u8_t ip6_src_router_priority;

  ip4_addr_t arp_reply_ignore_addr;
#endif
};

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE

typedef enum netif_ip6_prefix_state_tag
{
    NETIF_IP6_PREFIX_INACTIVE = 0,
    NETIF_IP6_PREFIX_ACTIVE = 1,
}netif_ip6_prefix_state;

typedef enum netif_ip6_prefix_source_tag
{
    NETIF_IP6_PREFIX_PDP_CONTEXT = 0,
    NETIF_IP6_PREFIX_RA_INFO = 1,
    NETIF_IP6_PREFIX_HIB_RECOVER = 2,
}netif_ip6_prefix_source;


typedef struct netif_ip6_prefix_info_tag
{
    u32_t prefix[2];
    u8_t state; //netif_ip6_prefix_state_tag
    u8_t source; //netif_ip6_prefix_source_tag
    u8_t cid;
    u8_t reserved;
    u32_t life_time;
    u32_t active_time;
    struct netif_ip6_prefix_info_tag *next;
}netif_ip6_prefix_info;

#endif

#if LWIP_CHECKSUM_CTRL_PER_NETIF
#define NETIF_SET_CHECKSUM_CTRL(netif, chksumflags) do { \
  (netif)->chksum_flags = chksumflags; } while(0)
#define IF__NETIF_CHECKSUM_ENABLED(netif, chksumflag) if (((netif) == NULL) || (((netif)->chksum_flags & (chksumflag)) != 0))
#else /* LWIP_CHECKSUM_CTRL_PER_NETIF */
#define NETIF_SET_CHECKSUM_CTRL(netif, chksumflags)
#define IF__NETIF_CHECKSUM_ENABLED(netif, chksumflag)
#endif /* LWIP_CHECKSUM_CTRL_PER_NETIF */

/** The list of network interfaces. */
extern struct netif *netif_list;
/** The default network interface. */
extern struct netif *netif_default;

void netif_init(void);

struct netif *netif_add(struct netif *netif,
#if LWIP_IPV4
                        const ip4_addr_t *ipaddr, const ip4_addr_t *netmask, const ip4_addr_t *gw,
#endif /* LWIP_IPV4 */
                        void *state, netif_init_fn init, netif_input_fn input);


#if LWIP_IPV4
void netif_set_addr(struct netif *netif, const ip4_addr_t *ipaddr, const ip4_addr_t *netmask,
                    const ip4_addr_t *gw);
#endif /* LWIP_IPV4 */
void netif_remove(struct netif * netif);

void netif_remove_type(struct netif *netif, u8_t type);

/* Returns a network interface given its name. The name is of the form
   "et0", where the first two letters are the "name" field in the
   netif structure, and the digit is in the num field in the same
   structure. */
struct netif *netif_find(const char *name);

#if ENABLE_PSIF
struct netif *netif_find_netif_list(void);
struct netif *netif_find_by_cid(u8_t cid);
struct netif *netif_find_by_ip4_cid(u8_t cid);
struct netif *netif_find_by_ip6_cid(u8_t cid);
struct netif *netif_find_by_ded_cid(u8_t ded_cid);

u8_t netif_find_ip6_exist(void);
struct netif *netif_find_default(void);
struct netif *netif_find_netif_list(void);
void netif_set_arp_reply_mode(struct netif *pNetif, u8_t mode);
void netif_set_arp_reply_ignore_addr(struct netif *pNetif, ip4_addr_t *addr);

void netif_set_netif_type(struct netif *pNetif, u8_t type);

u8_t netif_get_netif_type(struct netif *pNetif);

struct netif *netif_get_adpt_netif(u8_t type);

u8_t netif_check_netif_type(struct netif *netif, u8_t type);


#endif

u8_t netif_check_netif_type_is_wan(struct netif *netif);
struct netif *NetGetLanNetif(void);
struct netif *NetGetLanTypeNetif(u8_t type);

struct netif *netif_find_by_wan_netif(void);


#if 0
struct netif *NetGetWanIp4Netif(void);
struct netif *NetGetWanIp6Netif(void);
#endif


void netif_set_default(struct netif *netif);

#if LWIP_IPV4
void netif_set_ipaddr(struct netif *netif, const ip4_addr_t *ipaddr);
void netif_set_netmask(struct netif *netif, const ip4_addr_t *netmask);
void netif_set_gw(struct netif *netif, const ip4_addr_t *gw);
/** @ingroup netif_ip4 */
#define netif_ip4_addr(netif)    ((const ip4_addr_t*)ip_2_ip4(&((netif)->ip_addr)))
/** @ingroup netif_ip4 */
#define netif_ip4_netmask(netif) ((const ip4_addr_t*)ip_2_ip4(&((netif)->netmask)))
/** @ingroup netif_ip4 */
#define netif_ip4_gw(netif)      ((const ip4_addr_t*)ip_2_ip4(&((netif)->gw)))
/** @ingroup netif_ip4 */
#define netif_ip_addr4(netif)    ((const ip_addr_t*)&((netif)->ip_addr))
/** @ingroup netif_ip4 */
#define netif_ip_netmask4(netif) ((const ip_addr_t*)&((netif)->netmask))
/** @ingroup netif_ip4 */
#define netif_ip_gw4(netif)      ((const ip_addr_t*)&((netif)->gw))
#endif /* LWIP_IPV4 */

void netif_set_up(struct netif *netif);
void netif_set_down(struct netif *netif);
void netif_set_type_up(struct netif *netif, u8_t type);
void netif_set_type_down(struct netif *netif, u8_t type, u8_t with_flag);


/** @ingroup netif
 * Ask if an interface is up
 */
#define netif_is_up(netif) (((netif)->flags & NETIF_FLAG_UP) ? (u8_t)1 : (u8_t)0)

#if LWIP_NETIF_STATUS_CALLBACK
void netif_set_status_callback(struct netif *netif, netif_status_callback_fn status_callback);
#endif /* LWIP_NETIF_STATUS_CALLBACK */
#if LWIP_NETIF_REMOVE_CALLBACK
void netif_set_remove_callback(struct netif *netif, netif_status_callback_fn remove_callback);
#endif /* LWIP_NETIF_REMOVE_CALLBACK */

void netif_set_link_up(struct netif *netif);
void netif_set_link_down(struct netif *netif);
/** Ask if a link is up */
#define netif_is_link_up(netif) (((netif)->flags & NETIF_FLAG_LINK_UP) ? (u8_t)1 : (u8_t)0)

#if LWIP_NETIF_LINK_CALLBACK
void netif_set_link_callback(struct netif *netif, netif_status_callback_fn link_callback);
#endif /* LWIP_NETIF_LINK_CALLBACK */

#if LWIP_NETIF_HOSTNAME
/** @ingroup netif */
#define netif_set_hostname(netif, name) do { if((netif) != NULL) { (netif)->hostname = name; }}while(0)
/** @ingroup netif */
#define netif_get_hostname(netif) (((netif) != NULL) ? ((netif)->hostname) : NULL)
#endif /* LWIP_NETIF_HOSTNAME */

#if LWIP_IGMP
/** @ingroup netif */
#define netif_set_igmp_mac_filter(netif, function) do { if((netif) != NULL) { (netif)->igmp_mac_filter = function; }}while(0)
#define netif_get_igmp_mac_filter(netif) (((netif) != NULL) ? ((netif)->igmp_mac_filter) : NULL)
#endif /* LWIP_IGMP */

#if LWIP_IPV6 && LWIP_IPV6_MLD
/** @ingroup netif */
#define netif_set_mld_mac_filter(netif, function) do { if((netif) != NULL) { (netif)->mld_mac_filter = function; }}while(0)
#define netif_get_mld_mac_filter(netif) (((netif) != NULL) ? ((netif)->mld_mac_filter) : NULL)
#define netif_mld_mac_filter(netif, addr, action) do { if((netif) && (netif)->mld_mac_filter) { (netif)->mld_mac_filter((netif), (addr), (action)); }}while(0)
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

#if ENABLE_LOOPBACK
err_t netif_loop_output(struct netif *netif, struct pbuf *p);
void netif_poll(struct netif *netif);
#if !LWIP_NETIF_LOOPBACK_MULTITHREADING
void netif_poll_all(void);
#endif /* !LWIP_NETIF_LOOPBACK_MULTITHREADING */
#endif /* ENABLE_LOOPBACK */

err_t netif_input(struct pbuf *p, struct netif *inp);

#if LWIP_IPV6
/** @ingroup netif_ip6 */
#define netif_ip_addr6(netif, i)  ((const ip_addr_t*)(&((netif)->ip6_addr[i])))
/** @ingroup netif_ip6 */
#define netif_ip6_addr(netif, i)  ((const ip6_addr_t*)ip_2_ip6(&((netif)->ip6_addr[i])))
void netif_ip6_addr_set(struct netif *netif, s8_t addr_idx, const ip6_addr_t *addr6);
void netif_ip6_prefx_length_set(struct netif *netif, s8_t addr_idx, u8_t prefix_length);
u8_t netif_ip6_prefix_length_get(struct netif *netif, s8_t addr_idx);
void ip6_set_subnet_router_priority(struct netif *netif, u8_t priority);
void ip6_set_source_addr_priority(struct netif *netif, u8_t priority);
void netif_ip6_addr_set_parts(struct netif *netif, s8_t addr_idx, u32_t i0, u32_t i1, u32_t i2, u32_t i3);
#define netif_ip6_addr_state(netif, i)  ((netif)->ip6_addr_state[i])
void netif_ip6_addr_set_state(struct netif* netif, s8_t addr_idx, u8_t state);
s8_t netif_get_ip6_addr_match(struct netif *netif, const ip6_addr_t *ip6addr);
void netif_create_ip6_linklocal_address(struct netif *netif, u8_t from_mac_48bit);
void netif_create_ip6_linklocal_address_by_identifier(struct netif *netif, u32_t identifier1, u32_t identifier2);
err_t netif_add_ip6_address(struct netif *netif, const ip6_addr_t *ip6addr, s8_t *chosen_idx);
#define netif_set_ip6_autoconfig_enabled(netif, action) do { if(netif) { (netif)->ip6_autoconfig_enabled = (action); }}while(0)
#endif /* LWIP_IPV6 */

#if ENABLE_PSIF
u16_t netif_get_max_mtu_size(void);
void netif_dump_ul_packet(u8_t *data, u16_t len, u8_t type);
void netif_dump_dl_packet(u8_t *data, u16_t len, u8_t type);

void netif_enable_rohc_bitmap(struct netif* netif, u8_t cid);

void netif_disable_rohc_bitmap(struct netif* netif, u8_t cid);

u8_t netif_get_rohc_set(struct netif* netif, u8_t cid);

void netif_enable_suspend(struct netif* netif);
void netif_disable_suspend(struct netif* netif);
void netif_enable_suspend_all(void);
void netif_disable_suspend_all(void);
void netif_enable_oos_state(struct netif *netif);
void netif_disable_oos_state(struct netif *netif);
void netif_enable_all_psif_oos_state(void);
void netif_disable_all_psif_oos_state(void);
void netif_set_ps_mtu(struct netif *netif, u16_t mtu);

#if LWIP_TIMER_ON_DEMOND

void netif_enable_timer_active_mask(struct netif* netif, u8_t type);

void netif_disable_timer_active_mask(struct netif* netif, u8_t type);

u8_t netif_get_timer_active_state(struct netif* netif, u8_t type);

#endif


#endif

#if PS_ENABLE_TCPIP_HIB_SLEEP2_MODE
void netif_add_ip6_prefix_info(u32_t prefix[2], u8_t state, u8_t source, u32_t life_time, u32_t active_time, u8_t cid);
void netif_remove_ip6_prefix_info(u8_t cid);
struct netif_ip6_prefix_info_tag *netif_find_ip6_prefix_info(u8_t cid);
struct netif_ip6_prefix_info_tag *netif_get_ip6_prefix_info_list(void);
#endif


#if LWIP_NETIF_HWADDRHINT
#define NETIF_SET_HWADDRHINT(netif, hint) ((netif)->addr_hint = (hint))
#else /* LWIP_NETIF_HWADDRHINT */
#define NETIF_SET_HWADDRHINT(netif, hint)
#endif /* LWIP_NETIF_HWADDRHINT */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_NETIF_H */
