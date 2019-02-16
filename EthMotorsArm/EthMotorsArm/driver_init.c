/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_init.h"
#include <peripheral_clk_config.h>
#include <utils.h>
#include <hal_init.h>

struct timer_descriptor TIMER_0;

struct usart_sync_descriptor USART_0;

struct usart_sync_descriptor USART_1;

struct mac_async_descriptor ETHERNET_MAC_0;

void USART_0_PORT_init(void)
{

	gpio_set_pin_function(PC27, PINMUX_PC27C_SERCOM1_PAD0);

	gpio_set_pin_function(PC28, PINMUX_PC28C_SERCOM1_PAD1);
}

void USART_0_CLOCK_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM1_GCLK_ID_CORE, CONF_GCLK_SERCOM1_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM1_GCLK_ID_SLOW, CONF_GCLK_SERCOM1_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	hri_mclk_set_APBAMASK_SERCOM1_bit(MCLK);
}

void USART_0_init(void)
{
	USART_0_CLOCK_init();
	usart_sync_init(&USART_0, SERCOM1, (void *)NULL);
	USART_0_PORT_init();
}

void USART_1_PORT_init(void)
{

	gpio_set_pin_function(PB16, PINMUX_PB16C_SERCOM5_PAD0);

	gpio_set_pin_function(PB17, PINMUX_PB17C_SERCOM5_PAD1);
}

void USART_1_CLOCK_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM5_GCLK_ID_CORE, CONF_GCLK_SERCOM5_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM5_GCLK_ID_SLOW, CONF_GCLK_SERCOM5_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	hri_mclk_set_APBDMASK_SERCOM5_bit(MCLK);
}

void USART_1_init(void)
{
	USART_1_CLOCK_init();
	usart_sync_init(&USART_1, SERCOM5, (void *)NULL);
	USART_1_PORT_init();
}

/**
 * \brief Timer initialization function
 *
 * Enables Timer peripheral, clocks and initializes Timer driver
 */
static void TIMER_0_init(void)
{
	//tph - uses the 12MHz internal clock divided by 4 = 3Mhz to produce (/75) a 40khz (25us) signal for Motor pwm
	hri_mclk_set_APBAMASK_TC0_bit(MCLK);
	hri_gclk_write_PCHCTRL_reg(GCLK, TC0_GCLK_ID, CONF_GCLK_TC0_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	timer_init(&TIMER_0, TC0, _tc_get_timer());
}

void ETHERNET_MAC_0_PORT_init(void)
{

	gpio_set_pin_function(PC11, PINMUX_PC11L_GMAC_GMDC);

	gpio_set_pin_function(PC12, PINMUX_PC12L_GMAC_GMDIO);

	gpio_set_pin_function(PA13, PINMUX_PA13L_GMAC_GRX0);

	gpio_set_pin_function(PA12, PINMUX_PA12L_GMAC_GRX1);

	gpio_set_pin_function(PC20, PINMUX_PC20L_GMAC_GRXDV);

	gpio_set_pin_function(PA15, PINMUX_PA15L_GMAC_GRXER);

	gpio_set_pin_function(PA18, PINMUX_PA18L_GMAC_GTX0);

	gpio_set_pin_function(PA19, PINMUX_PA19L_GMAC_GTX1);

	gpio_set_pin_function(PA14, PINMUX_PA14L_GMAC_GTXCK);

	gpio_set_pin_function(PA17, PINMUX_PA17L_GMAC_GTXEN);
}

void ETHERNET_MAC_0_CLOCK_init(void)
{
	hri_mclk_set_AHBMASK_GMAC_bit(MCLK);
	hri_mclk_set_APBCMASK_GMAC_bit(MCLK);
}

void ETHERNET_MAC_0_init(void)
{
	ETHERNET_MAC_0_CLOCK_init();
	mac_async_init(&ETHERNET_MAC_0, GMAC);
	ETHERNET_MAC_0_PORT_init();
}

void ETHERNET_MAC_0_example(void)
{
	mac_async_enable(&ETHERNET_MAC_0);
	mac_async_write(&ETHERNET_MAC_0, (uint8_t *)"Hello World!", 12);
}

void system_init(void)
{
	init_mcu();

	USART_0_init();

	USART_1_init();

	TIMER_0_init();
	ETHERNET_MAC_0_init();
}
