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
#include <main.h>
#include <motor.h>  //motor data structures (motor port values and masks, duration, and program data structure)
#include <utils_ringbuffer.h> //needed for esp-01 usart

#define PCB_NAME_LENGTH 5
static const char *PCB_Name = "Motor";
#define UDP_PORT 53510  //port used for UDP communication

//Motor variables
uint8_t NumMotors;
MotorStatus Motor[MAX_NUM_MOTORS];  //status of each motor
int NumClocksInMotorDutyCycle; //the number of timer interrupt clocks in the motor duty cycle
int MotorDutyCycleClock; //the time (in us) of each timer2 interrupt when the motor pins are updated

/* Saved total time in mS since timer was enabled */
volatile static u32_t systick_timems;
volatile static bool  gmac_recv_flag = false;
static bool           link_up   = false;

struct udp_pcb *udpserver_pcb; //udp server
//extern struct mac_async_descriptor MACIF; //is declared as ETHERNET_MAC_0 in driver_init.c
extern struct mac_async_descriptor ETHERNET_MAC_0;
extern struct netif LWIP_MACIF_desc;
extern u8_t LWIP_MACIF_hwaddr[6];
//extern struct io_descriptor *stdio_io; 

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


// Last entry in ring buffer (ring buffer found at io_descr->rx in callback).
// Can be used for parsing in the call back.
uint8_t ringbuffer_last(struct ringbuffer const *const rb)
{
    ASSERT(rb);

    uint8_t data = rb->buf[(rb->write_index-1) & rb->size];

    return data;
}

volatile bool line_available = false;
//int NumCharRx=0;
void usart1_receive_cb(const struct usart_async_descriptor *const io_descr)
{
//	struct io_descriptor *io_in,*io_out; 
//	u8_t buffer[256];
//	int NumChars;
	
	/* Read transfer completed */
	//if (io_descr->rx.size>14) {
	//	line_available=true;
	//}
	//Note that the ring buffer can add characters after a new line before being read from
    if( ringbuffer_last(&io_descr->rx) == 0x0a ) {
        line_available = true;
		//NumCharRx=io_descr->rx.size;
	}
	
	//printf("u1 ");
//	usart_async_get_io_descriptor(&USART_1, &io_in);
//	usart_sync_get_io_descriptor(&USART_1, &io_out);
	//CRITICAL_SECTION_ENTER()
//	if (usart_async_is_rx_not_empty(&USART_1)) {
//	printf("%s",&io_descr->rx.buf[io_descr->rx.read_index]);
		//NumChars=ringbuffer_num(&io_descr->rx);
//		NumChars=io_read(io_in, (uint8_t *)&buffer,255);
//		buffer[NumChars]=0; //terminal string
		//io_write(io_out, (uint8_t *)&buffer,1);
//		printf("%s",buffer);
//	} //if (usart_async_is_rx_not_empty(USART_1)) {
	//CRITICAL_SECTION_LEAVE()

}

int32_t nread = 0;
static uint8_t buffer[256];
void USART_1_input(void)
{
    struct io_descriptor *io,*io_out;
    bool willRead = false;
    usart_async_get_io_descriptor(&USART_1, &io);	// Get the pointer to statically allocated io_descriptor structure.
	uint8_t MotorInst[4];

    if( nread <= 0 ) {
        // Here need a semaphore (counts only to 1 in interrupt):
        CRITICAL_SECTION_ENTER()
        if(line_available) {
            willRead = true;			// Accepting the semaphore count.
            line_available = false;		// Decrementing the semaphore count from 1 to 0.
        }								// Fast in and out with interrupts disabled.
        CRITICAL_SECTION_LEAVE()
        if (willRead) {
            //nread = io_read(io, buffer, sizeof(buffer));	// Returns count of characters read (or negative error flag).
			nread = io_read(io, buffer, sizeof(buffer));	// Returns count of characters read (or negative error flag).
			//nread=io_read(io,buffer,NumCharRx); //only read to 0x0a- leave anything else in ring buffer
			if (nread>0) {
				//usart_sync_get_io_descriptor(&USART_0, &io_out);	// Get the pointer to statically allocated io_descriptor structure.
				//io_write(io_out,buffer,1);
				//printf("W: %s",buffer); //each USART packet ends with 0x0a
				buffer[nread]=0; //terminate string
				printf("W: %s:W\n",buffer); //each USART packet ends with 0x0a
				//note that there is no transmit buffer (even for the synchronous driver) printf calls can overwrite the buffer currently

				//tread+=nread; //incremement total read count
				nread=0;
				//if (tread>=9) { //end of motor instruction length
				//Process any motor instructions
				if (buffer[4]==ROBOT_MOTORS_SEND_4BYTE_INST) {
					//got send a 4 byte Instruction over USART from ESP-01
					MotorInst[0]=buffer[5];  //Motor Num<<4
					MotorInst[1]=buffer[6];  //Dir+Strength
					MotorInst[2]=buffer[7];  //duration low byte
					MotorInst[3]=buffer[8];  //duration high byte
					SendMotorInst(MotorInst);
					//tread-=9;
				} //if (buffer[4]==ROBOT_MOTORS_SEND_4BYTE_INST) {
				//} //if (tread==9) {
			} //if (nread>0) {
		} //if(willRead) {
    } //if( nread <= 0 ) {
}

void usart0_receive_cb(const struct usart_async_descriptor *const io_descr)
{
}


void mac_receive_cb(struct mac_async_descriptor *desc)
{
	gmac_recv_flag = true;
	//printf("rx ");
	gpio_set_pin_level(PHY_YELLOW_LED_PIN,false);
	delay_ms(1);
	//gpio_set_pin_level(PHY_YELLOW_LED_PIN,false);
	gpio_set_pin_level(PHY_YELLOW_LED_PIN,true);
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
//Process any kind of instruction - instrucitons are generalized so that this code is the same whether the instruciton came from ethernet, usart, or usart-esp
void Process_Instruction(uint8_t Inst,uint32_t len,uint8_t type)
{

} //void Process_Instruction(struct pbuf *p)
#endif		
		
static struct timer_task MotorTimerTask;

static void MotorTimerTask_cb(const struct timer_task *const timer_task)
{
//Process any motor pwm
	uint8_t temp;
	uint8_t i;


	//go through each motor status and set/reset the correct PORTB and PORTC pins
	for(i=0;i<NumMotors;i++) {//NUM_MOTORS;i++) {
		if (Motor[i].Duration>0) { //this motor is moving, or will start moving
			//set strength duty cycle
			if (Motor[i].StrengthCount<Motor[i].Strength && !(Motor[i].flags&MOTOR_INST_FIRST_RUN)) { //set motor pins
				//in "on" portion of duty cycle
				//clear and set the direction pins
				//(is the same for driver with or without a pulse pin)
				if (Motor[i].Direction) {
					//todo: write directly to correct OUTSET/OUTCLR register
					gpio_set_pin_level(Motor[i].DirPin,true);
					//CCW
										
					//clear clockwise pin
					//LATB&=Motor[i].DirectionCWBitMask;
					//*Motor[i].DirCWPort&=Motor[i].DirectionCWBitMask;
				} else {
					gpio_set_pin_level(Motor[i].DirPin,false);
					//CW
					//clear counter-clockwise pin
					//set clockwise pin
					//*Motor[i].DirCWPort|=Motor[i].DirectionCWBit;
				}//if (Motor[i].Direction) {

				//pulse the pulse pin if necessary
				gpio_set_pin_level(Motor[i].PulsePin,true);
			} else {  //if (Motor[i].StrengthCount<Motor[i].Strength)
			
				//in "off" part of duty cycle (or first run of a motor instruction)

				if (Motor[i].flags&MOTOR_INST_FIRST_RUN) {
					Motor[i].flags&=~MOTOR_INST_FIRST_RUN; //clear first run bit
					if (Motor[i].StrengthCount>0) {
						Motor[i].StrengthCount--; //set back 1 so direction can get set above
					}
				} //if (Motor[i].flags&MOTOR_INST_FIRST_RUN) {
									    
				//Note that there is no need to set direction pins
				//because the "off" duty cycle is never called before the "on"
				//duty cycle, and the "on" duty cycle sets the direction
				//which doesn't need to change for the "off" cycle
					
				//*Motor[i].PulsePort&=Motor[i].PulseBitMask;
				gpio_set_pin_level(Motor[i].PulsePin,false);
			} //if (Motor[i].StrengthCount<motor[i].Strength)

			Motor[i].StrengthCount++;  //increase duty cycle count
			if (Motor[i].StrengthCount>=NumClocksInMotorDutyCycle) {  //reached end of duty cycle, reset count
				Motor[i].StrengthCount=0;
			}

			//decrement Duration
			if (Motor[i].Duration!=0) { //new inst might set Duration=0
				Motor[i].Duration--;  //currently units are timer interrupts
			}
			//which depend on MotorDutyCycleClock
		} else {  //no motor Duration - set motor pins to 0

			Motor[i].StrengthCount=0;
									    
			gpio_set_pin_level(Motor[i].PulsePin,false);
			} //if (Motor[i].Duration>0)
		} //for(i=0;i<NUM_MOTORS;i++)

} //static void MotorTimerTask_cb(const struct timer_task *const timer_task)



//tph - TIMER_0 uses the TC0 peripheral which takes as input the 12MHz general clock divided by 4 = 3Mhz to produce (/75) a 40khz (25us) signal for Motor pwm
void MotorTimer_Initialize(void)
{
	MotorTimerTask.interval = 1; //clock ticks
	MotorTimerTask.cb       = MotorTimerTask_cb;
	MotorTimerTask.mode     = TIMER_TASK_REPEAT;

	timer_add_task(&TIMER_0, &MotorTimerTask);
	timer_start(&TIMER_0);
}
		
		
//this instruction is a motor instruction to execute now
//and contains (in 4 bytes):
//Motor# (0:3),unused(4:7),direction(8),thrust(9:15),duration(16:31)
//was 3 bytes:
//motor# (address):4, direction:1, strength:3,duration:16
void SendMotorInst(uint8_t *MInst)
{
    uint8_t MotorNum,Strength;
    uint32_t Duration;

    //set the motor status so the timer interrupt
    //will find that a motor has a duration and needs to be moved
    //determine which motor
    MotorNum=(MInst[0]&0xf0)>>4;

    Motor[MotorNum].Duration=0; //just in case motor is currently running (pulsing)
    //SYS_TMR_DelayMS(1);  //wait to make sure motor is stopped if running (pulsing)
    Motor[MotorNum].flags|=MOTOR_INST_FIRST_RUN; //first set motor pins to 00 to avoid short circuit
    //for example, motor is turning with 10 then a 01 instruction is sent, and for a nanosecond when LATB is being set, possibly 2 pins might be 1 and cause a short at the h-bridge.

    //if ((MInst[0]&0x08)!=0) {
    if ((MInst[1]&0x80)!=0) {
        //counter-clockwise
       // Motor[MotorNum].DirectionMask=0x40>>(MotorNum*2);  //counter-clockwise
         //Motor[MotorNum].DirectionMask=0x1<<(MotorNum*2);  //counter-clockwise
         Motor[MotorNum].Direction=MOTOR_COUNTERCLOCKWISE;
    } else {
        //clock-wise
        //Motor[MotorNum].DirectionMask=0x80>>(MotorNum*2);  //clockwise
        //Motor[MotorNum].DirectionMask=0x2<<(MotorNum*2);  //clockwise
        Motor[MotorNum].Direction=MOTOR_CLOCKWISE;
    }

    //set motor turn strength- number of timer clocks in "on" part of duty cycle
    //0=stop 20=full speed (NumClkInDutyCycle)
    //this number is multiplied by the NumClocksInMotorDutyCycle/20
    //so for strength=1, Strength*NumClocksInMotorDutyCycle/20 (1 clks of 20 are on)
    //for strength=20 the motor is on for every clock (20 of 20)
    //Motor[MotorNum].Strength=((MInst[0]&0x07)*NumClocksInMotorDutyCycle)/ROBOT_MOTORS_DEFAULT_NUM_CLKS_IN_MOTOR_DUTY_CYCLE;
    Strength=(MInst[1]&0x7f);//max is currently 0 to 127
    //if strength is higher than the number of possible speeds, set at maximum strength
    if (Strength> ROBOT_MOTORS_DEFAULT_NUM_CLKS_IN_MOTOR_DUTY_CYCLE) {
        Strength=ROBOT_MOTORS_DEFAULT_NUM_CLKS_IN_MOTOR_DUTY_CYCLE;
    }

    //set motor turn duration (is 16-bit little endian int)
    //Motor[MotorNum].Duration=(uint16_t)((MInst[2]<<8)+MInst[1]);
    Duration=(uint32_t)((MInst[3]<<8)+MInst[2]);
    //duration from user is in ms, so convert to timer interrupts:
    //Motor[MotorNum].Duration is in timer interrupts 
    //MotorDutyCycleClock is 25us by default (40khz)  = every 25us
    //so convert Duration into time interrupt units (how many 25us units)

    //if Duration*1000 < 127*MotorDutyCycleClock then Strength needs to be scaled down
    //because the entire duty cycle of the motor will be < 127.
    //ex: DurTime=1ms, MDCC=25us DurInts=1000/25=only 40 so if strength=127/2=63, that needs to be scaled down to 63/127=x/40
    //x=63*40/127=19 so 19 timer interrupts will be on, and 21 off for a total of 40x25us=1ms

    //convert ms to 25us units
    //so Duration of 100ms in 25uS units=0.1/0.000025=4000 clocks * 2= 8000
    //in us: (100)*1000/25=4000, so generalizing in us:
    //Duration*1000/MotorDutyCycle = number of TimerInterrupts for 
    //user sent Duration in ms.
    Duration*=1000; //Duration is divided by MotorDutyCycleClock below
    //MotorDutyCycleClock is in ms so multiply Duration x 1000 
	
    //at 40khz (25us) a pulse:
	//duty cycle is 25us*20=500us
	//so 1/20 speed is 25us on 475us off (shortest pulse possible for drv8800- 40khz)
	//   19/20 speed is 475us on 25us off

    Motor[MotorNum].Strength=Strength;
    Motor[MotorNum].StrengthCount=0;

    //Convert duration in us to duration in number of timer interrupts
    //since multiplying by 1000 above, I doubt Duration would ever be < MotorDutyCycleClock
    //but just as a failsafe in case Motor[].Duration somehow will get set to 0]
    //and somebody is trying to set strength=0 - probably not needed
    if ((Duration/MotorDutyCycleClock)==0 && Duration>0) {
      Motor[MotorNum].Duration=1;
    } else {
      Motor[MotorNum].Duration=Duration/MotorDutyCycleClock;  //note: this is the equivalent of enabling the motor
    } 

    Motor[MotorNum].DurationCount=0;

	//was: start timer3 if not already started
} //SendMotorInst

		
		
void udpserver_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
	int i;
	uint8_t *InstData; //pointer to udp data (instruction)
	uint8_t *ReturnInst; //currently just 50 bytes but probably will change
	struct pbuf *retbuf;  //return buffer
	int InstLen,ReturnInstLen;
	uint8_t MotorInst[4];  //motor instruction
	struct io_descriptor *io; //for ESP-01 UART1
	uint8_t buffer[256]; //temporary buffer
	
	//printf("received at %d, echoing to the same port\n",pcb->local_port);
	//dst_ip = &(pcb->remote_ip); // this is zero always
	if (p != NULL) {
		//printf("UDP rcv %d bytes: ", (*p).len);
		printf("%d ", (*p).len);
		//    	  for (i = 0; i < (*p).len; ++i)
		//			printf("%c",((char*)(*p).payload)[i]);
		//    	printf("\n");
		//udp_sendto(pcb, p, IP_ADDR_BROADCAST, 1234); //dest port
				//		udp_sendto(pcb, p, &forward_ip, fwd_port); //dest port
				
		//Process any UDP instructions recognized
		if (pcb->local_port==UDP_PORT) {  //note that currently there could never be a different port because UDP server only listens to this port
			//printf("port: %d\n", pcb->local_port);
			
			InstData=(uint8_t *)(*p).payload;  //shorthand to data
			InstLen=(*p).len;
			switch(InstData[4]) //Robot Instruction
			{
			case ROBOT_MOTORS_TEST: //send back 0x12345678
				retbuf = pbuf_alloc(PBUF_TRANSPORT, 10, PBUF_RAM);
				ReturnInst=retbuf->payload;
				memcpy(ReturnInst,p->payload,5); //copy IP + inst byte to return instruction
				ReturnInst[6]=0x12;
				ReturnInst[7]=0x34;
				ReturnInst[8]=0x56;
				ReturnInst[9]=0x78;
				udp_sendto(pcb, retbuf, addr, UDP_PORT); //dest port
				pbuf_free(retbuf);
				break;
			case ROBOT_MOTORS_PCB_NAME: //01 send back mac+name/id
				retbuf = pbuf_alloc(PBUF_TRANSPORT, 5+sizeof(LWIP_MACIF_hwaddr)+PCB_NAME_LENGTH, PBUF_RAM);
				ReturnInst=retbuf->payload;
				ReturnInstLen=5;				   
				memcpy(ReturnInst,p->payload,5); //copy IP + inst byte to return instruction
				//get the MAC address from the default network interface
				//this presumes that there is only 1 net
				//in the future there could be more than 1 net,
				//for example a wireless net too
				memcpy(ReturnInst+ReturnInstLen,LWIP_MACIF_hwaddr,sizeof(LWIP_MACIF_hwaddr));//copy mac
				ReturnInstLen+=sizeof(LWIP_MACIF_hwaddr);
				memcpy(ReturnInst+ReturnInstLen,PCB_Name,PCB_NAME_LENGTH);//copy name
				ReturnInstLen+=PCB_NAME_LENGTH; //MOTOR
				udp_sendto(pcb, retbuf, addr, UDP_PORT); //dest port
				pbuf_free(retbuf);
			break;
	        case ROBOT_MOTORS_SEND_4BYTE_INST:  //got send a 4 byte Instruction over Net (was USB)
				MotorInst[0]=InstData[5];  //Motor Num<<4
				MotorInst[1]=InstData[6];  //Dir+Strength
				MotorInst[2]=InstData[7];  //duration low byte
				MotorInst[3]=InstData[8];  //duration high byte
				SendMotorInst(MotorInst);
			break; 
			case ROBOT_MOTORS_TEST_WIFI:				
				usart_async_get_io_descriptor(&USART_1, &io);
				memcpy(buffer,&InstData[5],InstLen-5);
				buffer[InstLen-5]=0; //terminate string
				printf("%s",buffer);
				io_write(io, (uint8_t *)&InstData[5], InstLen-5);
			break;
			} //switch

		} //if (pcb->local_port==UDP_PORT) {
		pbuf_free(p);
	} //if (p != NULL) {
} //void udpserver_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)

int InitializeMotors(void) 
{
	
	
	//set number of clocks in motor duty cycle
	//is 7 (but was 14), 7 timer2 interrupts make 1 full motor duty cycle
	NumClocksInMotorDutyCycle=ROBOT_MOTORS_DEFAULT_NUM_CLKS_IN_MOTOR_DUTY_CYCLE;
	MotorDutyCycleClock=ROBOT_MOTORS_DEFAULT_MOTOR_DUTY_CYCLE_CLK;
	
	
	NumMotors=1;
	//Clear the robot status array
	memset(Motor,sizeof(MotorStatus)*NumMotors,0);

	//Motor[0].flags|=MOTOR_DRIVER_USES_PULSE_PIN;
	Motor[0].DirPin=GPIO(GPIO_PORTB, 13);
	Motor[0].PulsePin=GPIO(GPIO_PORTB, 12);
	gpio_set_pin_direction(Motor[0].DirPin,GPIO_DIRECTION_OUT);
	gpio_set_pin_level(Motor[0].DirPin,false);
	gpio_set_pin_direction(Motor[0].PulsePin,GPIO_DIRECTION_OUT);
	gpio_set_pin_level(Motor[0].PulsePin,false);
	
//	Motor[1].DirPin=GPIO(GPIO_PORTB, 11); //13);
//	Motor[1].PulsePin=GPIO(GPIO_PORTB, 10);//12);

	return(1);
} //int InitializeMotors(void)


int main(void)
{
	struct io_descriptor *io;
	int count;//,StartDHCP;
	uint8_t OutStr[256];
	int32_t ret;
	u8_t    mac[6];
	u8_t ReadBuffer[256];
	//struct usart_async_status iostat;  //currently needed for usart async

	/* Initializes MCU, drivers and middleware - tph - inits phy and uarts*/
	atmel_start_init();

	//initialize user gpio pins	
	//gpio_set_pin_level(LED0,true);
	// Set pin direction to output
	//gpio_set_pin_direction(LED0, GPIO_DIRECTION_OUT);
	//gpio_set_pin_function(LED0, GPIO_PIN_FUNCTION_OFF);

//USART_ASYNC_TXC_CB
	usart_async_register_callback(&USART_1, USART_ASYNC_RXC_CB, usart1_receive_cb);
	usart_async_enable(&USART_1);



	/* Read MacAddress from EEPROM */  //tph: currently just adding a valid public MAC address
	read_macaddress(mac);

	systick_enable();

	//MACIF_example();
	
	ETHERNET_PHY_0_example();  //restarts autonegotiation

	//init usart
	usart_sync_get_io_descriptor(&USART_0, &io);
	usart_sync_enable(&USART_0);
	//usart_async_get_io_descriptor(&USART_0, &io);
	//usart_async_enable(&USART_0);
	count=0;
	sprintf((char *)OutStr,"**************************\n");
	io_write(io,OutStr,strlen(OutStr));
	
	//while (usart_async_get_status(&USART_0, &iostat)==ERR_BUSY);
	//sprintf((char *)OutStr,"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\n");
	//io_write(io,OutStr,strlen(OutStr));
	sprintf((char *)OutStr,"EthMotorsArm_DRV8800_rev02\n");
	io_write(io,OutStr,strlen(OutStr));
	//while (usart_async_get_status(&USART_0, &iostat)==ERR_BUSY);

	//sprintf((char *)OutStr,"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\n");
	//io_write(io,OutStr,strlen(OutStr));
	sprintf((char *)OutStr,"**************************\n");
	io_write(io,OutStr,strlen(OutStr));
	//while (usart_async_get_status(&USART_0, &iostat)==ERR_BUSY);

	//sprintf((char *)OutStr,"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\n");
	//io_write(io,OutStr,strlen(OutStr));



	printf("\r\nHello ATMEL World!\r\n");
	//fflush(stdio_io);

	//below does not work for printf because printf calls _puts_r which must send one char at a time 
	//while (usart_async_get_status(&USART_0, &iostat)==ERR_BUSY); 

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
//	udp_bind(udpserver_pcb, IP_ADDR_ANY, UDP_PORT);   //port UDP_PORT 
	udp_bind(udpserver_pcb, &LWIP_MACIF_desc.ip_addr.addr, UDP_PORT);   //port UDP_PORT 
	udp_recv(udpserver_pcb, udpserver_recv, NULL);  //set udpserver callback function


#if 0 	
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
#endif



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

	InitializeMotors(); //set initial settings of all motors
	//currently motor timer stop DHCP from working
	MotorTimer_Initialize();  //start timer for motor pwm

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


	USART_1_input();  //check for usart1 input

	}  //while(1)

}
