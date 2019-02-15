/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file or main.c
 * to avoid loosing it when reconfiguring.
 */
#ifndef ETHERNET_PHY_MAIN_H
#define ETHERNET_PHY_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif
#include <ethernet_phy.h>

#define PHY_RESET_PIN GPIO(GPIO_PORTB, 14) //36  //pin 36 (PB14) is PHY reset  
#define PHY_YELLOW_LED_PIN GPIO(GPIO_PORTC, 14)   //44  //pin 44 (PC14) is PHY YELLOW LED  


struct ethernet_phy_descriptor ETHERNET_PHY_0_desc;

void ethernet_phys_init(void);
void ETHERNET_PHY_0_example(void);

/**
 * \brief Ethernet PHY devices
 */
void ethernet_phys_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ETHERNET_PHY_MAIN_H */
