#include <atmel_start.h>

/**
 * Initializes MCU, drivers and middleware in the project
 **/
void atmel_start_init(void)
{
	system_init();
	stdio_redirect_init(); //tph - needs to be before ethernet_phy_init()
	ethernet_phys_init();

}
