#include <atmel_start.h>
#include <hal_gpio.h>
#include <hal_delay.h>
#include <driver_examples.h>
#include <peripheral_clk_config.h>
#include <ethernet_phy_main.h>
#include <lwip/netif.h>
#include <lwip/dhcp.h>
#include <lwip/udp.h>
#include <lwip/timers.h>
#include <lwip_demo_config.h>
#include <stdio.h>
#include <string.h>
#include <udpserver.h>
#include <robot_motor_pic_instructions.h>


/* Saved total time in mS since timer was enabled */
volatile static u32_t systick_timems;
volatile static bool  gmac_recv_flag = false;
static bool           link_up   = false;

struct udp_pcb *udpserver_pcb; //udp server
//extern struct mac_async_descriptor MACIF; //is declared as ETHERNET_MAC_0 in driver_init.c
extern struct mac_async_descriptor ETHERNET_MAC_0;
extern struct netif LWIP_MACIF_desc;

u32_t sys_now(void)
{
	return systick_timems;
}

void SysTick_Handler(void)
{
	systick_timems++;
}

void systick_enable(void)
{
	systick_timems = 0;
	SysTick_Config((CONF_CPU_FREQUENCY) / 1000);
}

void mac_receive_cb(struct mac_async_descriptor *desc)
{
	gmac_recv_flag = true;
	//printf("rx ");
	gpio_set_pin_level(PHY_YELLOW_LED_PIN,false);
	//delay_ms(1);
	//gpio_set_pin_level(PHY_YELLOW_LED_PIN,false);
	//gpio_set_pin_level(PHY_YELLOW_LED_PIN,true);
}

void mac_transmit_cb(struct mac_async_descriptor *desc)
{
	//gmac_tx_flag = true;
	//printf("tx ");
}

static void status_callback(struct netif *n)
{
	if (n->flags & NETIF_FLAG_UP) {
		printf("Interface Up %s:\n",
		n->flags & NETIF_FLAG_DHCP ? "(DHCP)" : "(STATIC)");

		#if 0
		printf("  IP Address: " IP_F "\n", IP_ARGS(&n->ip_addr));
		printf("  Net Mask:   " IP_F "\n", IP_ARGS(&n->netmask));
		printf("  Gateway:    " IP_F "\n", IP_ARGS(&n->gw));

		const char *speed = "10Mb/s";
		if (ETH->MACCR & ETH_MACCR_FES)
		speed = "100Mb/s";

		const char *duplex = "Half";
		if (ETH->MACCR & ETH_MACCR_DM)
		duplex = "Full";

		printf("  Mode:       %s  %s Duplex\n", speed, duplex);
		#endif
		} else {
		printf("Interface Down.\n");
	}
}

static void link_callback(struct netif *n)
{

	if (n->flags & NETIF_FLAG_LINK_UP) {
		printf("Link Up.\n");

		if (n->flags & NETIF_FLAG_DHCP) {
			printf("Restarting DHCP\n");
			dhcp_start(n);
		}

		} else {
		printf("Link Down.\n");
	}
}


static void print_ipaddress(void)
{
	static char tmp_buff[16];
	printf("IP_ADDR    : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *)&(LWIP_MACIF_desc.ip_addr), tmp_buff, 16));
	printf("NET_MASK   : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *)&(LWIP_MACIF_desc.netmask), tmp_buff, 16));
	printf("GATEWAY_IP : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *)&(LWIP_MACIF_desc.gw), tmp_buff, 16));
}

static void read_macaddress(u8_t *mac)
{
	#if CONF_AT24MAC_ADDRESS != 0
	uint8_t addr = 0x9A;
	i2c_m_sync_enable(&I2C_AT24MAC);
	i2c_m_sync_set_slaveaddr(&I2C_AT24MAC, CONF_AT24MAC_ADDRESS, I2C_M_SEVEN);
	io_write(&(I2C_AT24MAC.io), &addr, 1);
	io_read(&(I2C_AT24MAC.io), mac, 6);

	#else
	/* set mac to 0x11 if no EEPROM mounted */
	//memset(mac, 0x11, 6);
	mac[0]=0x74;
	mac[1]=0x27;
	mac[2]=0xea;
	mac[3]=0xda;
	mac[4]=0x89;
	mac[5]=0x85;
	#endif
}

#if 0 
//Process any kind of instruction
void Process_Instruction(struct pbuf *p)
{


#if 0 
uint32_t* MemAddr;
uint32_t MemData;
uint32_t InstValue;
uint8_t *SendData;
uint32_t InstLen;
TCPIP_NET_HANDLE netH;
uint32_t ReturnInstLen;
TCPIP_NET_IF *pNetIf;
uint32_t NumBytes,i;
#endif

//the first 4-bytes of the instruction are the IP of the source of the instruction
//this IP needs to be sent back so the return data can reach the correct requester
//in particular when the instruction involves a long term periodic sending back of data

// Transfer the data out of the TCP RX FIFO and into our local processing buffer.
//NumBytes= TCPIP_UDP_ArrayGet(appData.socket, InstData, InstDataLen);

InstData=(uint8_t *)(*p).payload;

//SYS_CONSOLE_PRINT("\tReceived %d bytes\r\n",NumBytes);    
    
//switch(SetupPkt.bRequest)  //USB
//switch(InstData[0]) //Robot Instruction
switch(InstData[4]) //Robot Instruction
{
case ROBOT_MOTORS_TEST: //send back 0x12345678
    memcpy(ReturnInst,InstData,5); //copy IP + inst byte to return instruction
    ReturnInst[6]=0x12;
    ReturnInst[7]=0x34;
    ReturnInst[8]=0x56;
    ReturnInst[9]=0x78;
	udp_sendto(pcb, p, IP_ADDR_BROADCAST, 53510); //dest port
	
    while (TCPIP_UDP_PutIsReady(appData.socket)<9) {};
    TCPIP_UDP_ArrayPut(appData.socket,ReturnInst,9);  //little endian       
    TCPIP_UDP_Flush(appData.socket); //send the packet        
    //while (UDPIsTxPutReady(UDPSendSock,9)<9) {};
    //UDPPutArray(UDPSendSock,(uint8_t *)ReturnInst,9);  //little endian
    //UDPFlush(UDPSendSock); //send the packet
    break;
case ROBOT_MOTORS_PCB_NAME: //01 send back mac+name/id
    memcpy(ReturnInst,InstData,5); //copy IP + inst byte to return instruction
    ReturnInstLen=5;
    //get the MAC address from the default network interface
    //this presumes that there is only 1 net
    //in the future there could be more than 1 net, 
    //for example a wireless net too
    //netH = TCPIP_STACK_GetDefaultNet();
    app_netH = TCPIP_STACK_IndexToNet(0);  //presumes net 0 is the wired net
    //pMyNetIf = _TCPIPStackHandleToNet(netH);
    pNetIf = _TCPIPStackHandleToNetUp(app_netH);
    memcpy(ReturnInst+ReturnInstLen,(pNetIf)->netMACAddr.v,sizeof(pNetIf->netMACAddr));//copy mac
    ReturnInstLen+=sizeof(pNetIf->netMACAddr);
    memcpy(ReturnInst+ReturnInstLen,PCB_Name,PCB_NAME_LENGTH);//copy name
    ReturnInstLen+=PCB_NAME_LENGTH; //MOTOR
    //while (UDPIsTxPutReady(UDPSendSock,ReturnInstLen)<ReturnInstLen) {};
    //UDPPutArray(UDPSendSock,(uint8_t *)ReturnInst,ReturnInstLen);  //little endian
    //UDPFlush(UDPSendSock); //send the packet
    while (TCPIP_UDP_PutIsReady(appData.socket)<ReturnInstLen) {};
    TCPIP_UDP_ArrayPut(appData.socket,ReturnInst, ReturnInstLen);  //little endian       
    TCPIP_UDP_Flush(appData.socket); //send the packet

    break;
} //void Process_Instruction(struct pbuf *p)
#endif		
		
void udpserver_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
	int i;
	uint8_t *InstData; //pointer to udp data (instruction)
	uint8_t ReturnInst[50]; //currently just 50 bytes but probably will change

	//printf("received at %d, echoing to the same port\n",pcb->local_port);
	//dst_ip = &(pcb->remote_ip); // this is zero always
	if (p != NULL) {
		printf("UDP rcv %d bytes: ", (*p).len);
		//    	  for (i = 0; i < (*p).len; ++i)
		//			printf("%c",((char*)(*p).payload)[i]);
		//    	printf("\n");
		//udp_sendto(pcb, p, IP_ADDR_BROADCAST, 1234); //dest port
				//		udp_sendto(pcb, p, &forward_ip, fwd_port); //dest port
				
		//Process any UDP instructions recognized
		if (pcb->local_port==53510) {  //note that currently there could never be a different port because UDP server only listens to this port
			printf("port: %d ", pcb->local_port);
			
			InstData=(uint8_t *)(*p).payload;  //shorthand to data

			switch(InstData[4]) //Robot Instruction
			{
			case ROBOT_MOTORS_TEST: //send back 0x12345678
				//memcpy(ReturnInst,InstData,5); //copy IP + inst byte to return instruction
				ReturnInst[6]=0x12;
				ReturnInst[7]=0x34;
				ReturnInst[8]=0x56;
				ReturnInst[9]=0x78;
				//udp_sendto(pcb, p, addr, 53510); //dest port
				break;
			} //switch

		} //if (pcb->local_port==53510) {
		pbuf_free(p);
	} //if (p != NULL) {
} //void udpserver_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)



int main(void)
{
	struct io_descriptor *io;
	int count;//,StartDHCP;
	uint8_t OutStr[256];
	int32_t ret;
	u8_t    mac[6];
	u8_t ReadBuffer[256];

	/* Initializes MCU, drivers and middleware - tph - inits phy*/
	atmel_start_init();

	//initialize user gpio pins	
	//gpio_set_pin_level(LED0,true);
	// Set pin direction to output
	//gpio_set_pin_direction(LED0, GPIO_DIRECTION_OUT);
	//gpio_set_pin_function(LED0, GPIO_PIN_FUNCTION_OFF);

	//init usart
	usart_sync_get_io_descriptor(&USART_0, &io);
	usart_sync_enable(&USART_0);
	count=0;
	sprintf((char *)OutStr,"**************************\n");
	io_write(io,OutStr,strlen(OutStr));
	//sprintf((char *)OutStr,"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\n");
	//io_write(io,OutStr,strlen(OutStr));
	sprintf((char *)OutStr,"EthMotorsArm_DRV8800_rev02\n");
	io_write(io,OutStr,strlen(OutStr));
	//sprintf((char *)OutStr,"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\n");
	//io_write(io,OutStr,strlen(OutStr));
	sprintf((char *)OutStr,"**************************\n");
	io_write(io,OutStr,strlen(OutStr));
	//sprintf((char *)OutStr,"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\n");
	//io_write(io,OutStr,strlen(OutStr));

	/* Read MacAddress from EEPROM */  //tph: currently just adding a valid public MAC address
	read_macaddress(mac);

	systick_enable();

	//MACIF_example();
	
	ETHERNET_PHY_0_example();  //restarts autonegotiation



	printf("\r\nHello ATMEL World!\r\n");
	mac_async_register_callback(&ETHERNET_MAC_0, MAC_ASYNC_RECEIVE_CB, (FUNC_PTR)mac_receive_cb);
	mac_async_register_callback(&ETHERNET_MAC_0, MAC_ASYNC_TRANSMIT_CB, (FUNC_PTR)mac_transmit_cb);

	eth_ipstack_init();
	do {
		ret = ethernet_phy_get_link_status(&ETHERNET_PHY_0_desc, &link_up);
		if (ret == ERR_NONE && link_up) {
			break;
		}
	} while (true);
	printf("Ethernet Connection established\n");
	LWIP_MACIF_init(mac);  //tph: add LWIP callback for recvd input: ethernet_input()


	//make this the default interface
	netif_set_default(&LWIP_MACIF_desc);
	
	// Set callback function for netif status change 
	netif_set_status_callback(&LWIP_MACIF_desc, status_callback);

	//Set callback function for link status change
	netif_set_link_callback(&LWIP_MACIF_desc, link_callback);

		
	mac_async_enable(&ETHERNET_MAC_0);

//	//enable interrupts (global)
//__enable_irq();

//enable the GMAC interrupt
//mac_async_enable_irq(&MACIF);





//#if 0 
	//udpecho_init(); //START UDP ECHO THREAD - requires netconn 
	//start_udp();
	udpserver_pcb = udp_new();  //create udp server
	//IP4_ADDR(&forward_ip, 192, 168,   2, 254);
//	udp_bind(udpserver_pcb, IP_ADDR_ANY, 53510);   //port 53510 
	udp_bind(udpserver_pcb, &LWIP_MACIF_desc.ip_addr.addr, 53510);   //port 53510 
	udp_recv(udpserver_pcb, udpserver_recv, NULL);  //set udpserver callback function
	
	const int out_buf_size=4;
	const char buf[]="test";

	//send a udp packet
	struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, out_buf_size * sizeof(char), PBUF_REF);
	if (p!=0) {
		p->payload = buf;
		udp_sendto(udpserver_pcb, p, IP_ADDR_BROADCAST, 1234); //dest port
		//udp_sendto(pcb, p, &forward_ip, fwd_port); //dest port
		pbuf_free(p);
	} //if (p!=0)
//#endif



#if 0 
	//enable interrupts
	hri_gmac_write_NCR_reg(GMAC,GMAC_NCR_MPE|GMAC_NCR_RXEN|GMAC_NCR_TXEN);  //network control register - enable write read and management port
	//hri_gmac_write_NCFGR_reg(GMAC,0xc0000|GMAC_NCFGR_FD|GMAC_NCFGR_SPD);  //network configuration register- /48, FD, SPD
	hri_gmac_write_NCFGR_reg(GMAC,0xc0000|GMAC_NCFGR_FD|GMAC_NCFGR_CAF|GMAC_NCFGR_IRXER|GMAC_NCFGR_IRXFCS|GMAC_NCFGR_SPD);  //network configuration register- /48, FD, SPD
	//hri_gmac_write_IMR_reg(&MACIF,GMAC_IMR_RCOMP);  //network configuration register- /48, FD, SPD
	hri_gmac_write_IMR_reg(GMAC,GMAC_IMR_RCOMP|GMAC_IMR_RXUBR|GMAC_IMR_TCOMP|GMAC_IMR_ROVR|GMAC_IMR_PFNZ|GMAC_IMR_PTZ);  //interrupt mask register
	//hri_gmac_write_IER_reg(GMAC,GMAC_IER_RCOMP|GMAC_IER_TCOMP|GMAC_IER_ROVR|GMAC_IER_PFNZ|GMAC_IER_PTZ);  //interrupt enable register
	((Gmac *)GMAC)->IER.reg = GMAC_IER_RCOMP|GMAC_IER_RXUBR|GMAC_IER_TCOMP|GMAC_IER_ROVR|GMAC_IER_PFNZ|GMAC_IER_PTZ; 


	//enable interrupts (global)
	__enable_irq();

	//enable the GMAC interrupt
	mac_async_enable_irq(&ETHERNET_MAC_0);
#endif

	//enable interrupts
	//hri_nvic_write_ISPR_reg(&MACIF,)
//	NVIC_EnableIRQ(GMAC_IRQn);
//	uint32_t IntStatus;
//	IntStatus=__NVIC_GetEnableIRQ(GMAC_IRQn);

	//enable interrupts (global)
//	__enable_irq();
	


	//bring up the network interface - ned to do here so above interrupts are enabled
	#ifdef LWIP_DHCP
	/* DHCP mode. */
	if (ERR_OK != dhcp_start(&LWIP_MACIF_desc)) {
		LWIP_ASSERT("ERR_OK != dhcp_start", 0);
	}
	printf("DHCP Started\r\n");
	#else
	//needed for lwip 2.0: netif_set_link_up(&LWIP_MACIF_desc);
	/* Static mode. */
	netif_set_up(&LWIP_MACIF_desc);
	printf("Static IP Address Assigned\r\n");
	#endif


	/* Replace with your application code */
	while (true) {

/*
		if (StartDHCP) {
			StartDHCP=0;
			dhcp_start(&LWIP_MACIF_desc); //tph start dhcp
		}
*/

		if (gmac_recv_flag) {
			//printf("gmac_recd");
			//sprintf((char *)OutStr,"recvd2\n");
			//io_write(io,OutStr,strlen(OutStr));
			
			gmac_recv_flag = false;
			ethernetif_mac_input(&LWIP_MACIF_desc);
		}
		/* LWIP timers - ARP, DHCP, TCP, etc. */
		sys_check_timeouts();

		/* Print IP address info */
		if (link_up && LWIP_MACIF_desc.ip_addr.addr) {
			link_up = false;
			print_ipaddress();
		}

		//netif_poll(&LWIP_MACIF_desc);  //tph need?

		//check interface for DHCP
//		if (LWIP_MACIF_desc.dhcp->state == DHCP_BOUND) {
//			sprintf((char *)OutStr,"DHCP bound\n");
//			io_write(io,OutStr,strlen(OutStr));			
//		}
	//autoip_tmr(); //call every 100ms AUTOIP_TMR_INTERVAL msces,

/*		delay_ms(1000);
		gpio_toggle_pin_level(LED0);
		
		sprintf((char *)OutStr,"\b\b\b%03d",count);
		io_write(io, OutStr, strlen(OutStr));
		count++;
		//USART_0_example();
*/

//	GMAC_Handler();
	//mac_async_read(&MACIF, ReadBuffer, 10);

#if 0 
	volatile uint32_t imr,isr,ier,ncr,ncfgr,ur,rsr,dcfgr,nsr,tsr;
	//read GMAC interrupt mask register to confirm which interrupts are enabled (=0, RCOMP: receive complete= bit1)
	imr=hri_gmac_read_IMR_reg(GMAC);  //interrupt mask register
	isr=hri_gmac_read_ISR_reg(GMAC);  //interrupt status register
	//ier=hri_gmac_read_IER_reg(GMAC);  //interrupt enabled register
	ncr=hri_gmac_read_NCR_reg(GMAC);  //network control register
	ncfgr=hri_gmac_read_NCFGR_reg(GMAC);  //network configuration register
	ur=hri_gmac_read_UR_reg(GMAC);  //user register - bit 0=0 for RMII
	dcfgr=hri_gmac_read_DCFGR_reg(GMAC);  //DMA Configuration register 
	rsr=hri_gmac_read_RSR_reg(GMAC);  //user register - bit 0=0 for RMII
	nsr=hri_gmac_read_NSR_reg(GMAC);  //bit 1 and 2
	tsr=hri_gmac_read_TSR_reg(GMAC);  //bit 5 tx complete
#endif	
	//could test loop back send and receive: set LBL bit in NCR

	}  //while(1)

}
