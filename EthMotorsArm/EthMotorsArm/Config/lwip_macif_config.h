/* Auto-generated config file lwip_macif_config.h */
#ifndef LWIP_MACIF_CONFIG_H
#define LWIP_MACIF_CONFIG_H

// <<< Use Configuration Wizard in Context Menu >>>

// <h> Basic Configuration

// <q> Enable DHCP
// <i> If set, the interface is configured using DHCP.
// <i> Set by the DHCP code when starting or stopping DHCP.
// <id> macif_dhcp
#ifndef CONF_LWIP_MACIF_DHCP
#define CONF_LWIP_MACIF_DHCP 1 //tph 0
#endif

// <q> Enable Broadcast
// <i> If set, the netif has broadcast capability.
// <id> macif_broadcast
#ifndef CONF_LWIP_MACIF_BROADCAST
#define CONF_LWIP_MACIF_BROADCAST 1
#endif

// <q> Enable ARP
// <i> If set, the netif is an ethernet device using ARP.
// <id> macif_arp
#ifndef CONF_LWIP_MACIF_ETHARP
#define CONF_LWIP_MACIF_ETHARP 1
#endif

// <q> Enable IGMP
// <i> If set, the netif has IGMP capability.
// <id> macif_igmp
#ifndef CONF_LWIP_MACIF_IGMP
#define CONF_LWIP_MACIF_IGMP 0
#endif

// <q> Enable Point to Point
// <i> If set, the netif is one end of a point-to-point connection.
// <id> macif_p2p
#ifndef CONF_LWIP_MACIF_P2P
#define CONF_LWIP_MACIF_P2P 0
#endif

#ifndef CONF_LWIP_MACIF_FLAG
#define CONF_LWIP_MACIF_FLAG                                                                                           \
	((CONF_LWIP_MACIF_DHCP ? NETIF_FLAG_DHCP : 0) | (CONF_LWIP_MACIF_BROADCAST ? NETIF_FLAG_BROADCAST : 0)             \
	 | (CONF_LWIP_MACIF_P2P ? NETIF_FLAG_POINTTOPOINT : 0) | (CONF_LWIP_MACIF_ETHARP ? NETIF_FLAG_ETHARP : 0)          \
	 | (CONF_LWIP_MACIF_IGMP ? NETIF_FLAG_IGMP : 0) | NETIF_FLAG_ETHERNET)
#endif

// <o> MTU Size <64-1600>
// <i> maximum transfer unit (in bytes)
// <id> macif_mtu
#ifndef CONF_LWIP_MACIF_MTU
#define CONF_LWIP_MACIF_MTU 1536
#endif

// <s> Host Name
// <i> the hostname for this ethernet interface
// <id> macif_hostname
#ifndef CONF_LWIP_MACIF_HOSTNAME
#define CONF_LWIP_MACIF_HOSTNAME "lwip"
#endif

// <s> Host Name descriptive abbreviation
// <i> Define those to better describe your network interface, only 2 bytes needed.
// <id> macif_hostname_abbr
#ifndef CONF_LWIP_MACIF_HOSTNAME_ABBR
#define CONF_LWIP_MACIF_HOSTNAME_ABBR "en"
#endif

// </h>

// <e> Enable Static IP Address
// <i> Enable Static IP Address Configuration
// <id> macif_enable_static_ip
#ifndef CONF_LWIP_MACIF_STATIC_IP
#define CONF_LWIP_MACIF_STATIC_IP 0//tph1
#endif

#if CONF_LWIP_MACIF_DHCP == 1 && CONF_LWIP_MACIF_STATIC_IP == 1
#warning The Static IP Address setting will not setup since DHCP already enabled
#endif

// <s> IP Address
// <i> Static IP address
// <id> macif_ip
#ifndef CONF_LWIP_MACIF_IP
#if CONF_LWIP_MACIF_DHCP == 1
#define CONF_LWIP_MACIF_IP "0"//"192.168.1.89"//"0" //tph "192.168.0.100"
#else 
#define CONF_LWIP_MACIF_IP "192.168.1.89"//"0" //tph "192.168.0.100"
#endif
#endif

// <s> Netmask Address
// <i> Netmask Address
// <id> macif_netmask
#ifndef CONF_LWIP_MACIF_NETMASK
#if CONF_LWIP_MACIF_DHCP == 1
#define CONF_LWIP_MACIF_NETMASK "0"
#else
#define CONF_LWIP_MACIF_NETMASK "255.255.255.0"
#endif 
#endif

// <s> Gateway Address
// <i> Static Gateway Address
// <id> macif_gateway
#ifndef CONF_LWIP_MACIF_GATEWAY
#if CONF_LWIP_MACIF_DHCP == 1
#define CONF_LWIP_MACIF_GATEWAY "0"//"192.168.1.254"  //"0" //tph "192.168.0.1"
#else
#define CONF_LWIP_MACIF_GATEWAY "192.168.1.254"  //"0" //tph "192.168.0.1"
#endif
#endif

// </e>

// <<< end of configuration section >>>

#endif // LWIP_MACIF_CONFIG_H
