/*
 * udpserver.h
 *
 * Created: 11/4/2018 11:55:12 AM
 *  Author: tedhu
 */ 


#ifndef UDPSERVER_H_
#define UDPSERVER_H_

void udp_echo_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port);
void udp_sample(int sample);
void start_udp(void);




#endif /* UDPSERVER_H_ */