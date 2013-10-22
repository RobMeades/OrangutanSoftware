/* freeRTOS - an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/22/2013 8:11:21 PM
 *  Author: Rob Meades
 */

#include <pololu/orangutan.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <BlockQ.h>

#include <stdio.h>

int main()
{
    lcd_init_printf();
    clear();
	play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.

	while(1)
	{
		// Print battery voltage (in mV) on the LCD.
		clear();
		print_long(read_battery_millivolts_x2());

        vStartBlockingQueueTasks(10);

        vTaskStartScheduler();
        return 0;
	}
}

void vApplicationStackOverflowHook (xTaskHandle taskHandle, char * name)
{
    printf ("Stack in %s overflowed", name);
    while (1);
}
