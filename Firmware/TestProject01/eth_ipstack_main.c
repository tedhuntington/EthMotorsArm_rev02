/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file or main.c
 * to avoid loosing it when reconfiguring.
 */
#include <atmel_start.h>
#include <eth_ipstack_main.h>

#include <hal_mac_async.h>
#include <lwip_macif_config.h>
#include <ethif_mac.h>
#include <netif/etharp.h>
#include <lwip/dhcp.h>
#include <string.h>

void LWIP_init(void)
{
	lwip_init();
}

struct netif LWIP_MACIF_desc;
static u8_t  LWIP_MACIF_hwaddr[6];

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function mac_low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK  if the loopif is initialized
 */
err_t LWIP_MACIF_stack_init(struct netif *netif)
{
	LWIP_ASSERT("netif != NULL", (netif != NULL));
	LWIP_ASSERT("netif->state != NULL", (netif->state != NULL));

	netif->output     = etharp_output;
	netif->linkoutput = mac_low_level_output;

	/* device capabilities */
	LWIP_MACIF_desc.flags = CONF_LWIP_MACIF_FLAG;
	LWIP_MACIF_desc.mtu   = CONF_LWIP_MACIF_MTU;

	/* set MAC hardware address length */
	memcpy(LWIP_MACIF_desc.hwaddr, LWIP_MACIF_hwaddr, NETIF_MAX_HWADDR_LEN);
	LWIP_MACIF_desc.hwaddr_len = ETHARP_HWADDR_LEN;

#if LWIP_NETIF_HOSTNAME
	/* Initialize interface hostname */
	LWIP_MACIF_desc.hostname = CONF_LWIP_MACIF_HOSTNAME;
#endif
	memcpy(LWIP_MACIF_desc.name, CONF_LWIP_MACIF_HOSTNAME_ABBR, 2);

	/* initialize the mac hardware */
	mac_low_level_init(netif);

	return ERR_OK;
}
void LWIP_MACIF_init(u8_t hwaddr[6])
{
	struct ip_addr ip;
	struct ip_addr nm;
	struct ip_addr gw;
#if CONF_LWIP_MACIF_DHCP
	ip_addr_set_zero(&ip);
	ip_addr_set_zero(&nm);
	ip_addr_set_zero(&gw);
#else
	ipaddr_aton(CONF_LWIP_MACIF_IP, &ip);
	ipaddr_aton(CONF_LWIP_MACIF_NETMASK, &nm);
	ipaddr_aton(CONF_LWIP_MACIF_GATEWAY, &gw);
#endif
	memcpy(LWIP_MACIF_hwaddr, hwaddr, 6);

	netif_add(&LWIP_MACIF_desc, &ip, &nm, &gw, (void *)&MACIF, LWIP_MACIF_stack_init, ethernet_input);
}

void eth_ipstack_init(void)
{
	LWIP_init();
}
