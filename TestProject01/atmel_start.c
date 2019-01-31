#include <atmel_start.h>

/**
 * Initializes MCU, drivers and middleware in the project
 **/
void atmel_start_init(void)
{

	system_init();  //initializes mac
	stdio_redirect_init();

	ethernet_phys_init();
	//ethernet_phy_reset(&MACIF_PHY_desc);
	//wait 0.5s
	//delay_ms(500);
	//ethernet_phys_init();  //reinitialize phy
	
}
