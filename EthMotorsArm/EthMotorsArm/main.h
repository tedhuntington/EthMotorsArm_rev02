/*
 * main.h
 *
 * Created: 2/15/2019 9:56:10 AM
 *  Author: Ted Huntington
 */ 


#ifndef MAIN_H_
#define MAIN_H_


//Possible ports for instructions to be recd/sent from/to:
#define INST_TYPE_ETHERNET 1
#define INST_TYPE_USART 2
#define INST_TYPE_USART_ESP 3

typedef struct Instruction
{
	uint8_t inbuffer[256];  //actual bytes of instruction received in (from ethernet or uart)
	uint8_t outbuffer[256];  //actual bytes of return instruction to send out (to ethernet or uart)
	int InLength; //length of inbuffer
	int OutLength; //length of outbuffer
	uint8_t Type; //instruction recv/send port ETHERNET, UART, UART_ESP
	struct udp_pcb *pcb; //(needed for UDP only)
	struct ip_addr *addr; //IP address (needed for ethernet UDP only)
}Instruction;



#endif /* MAIN_H_ */