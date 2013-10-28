/* freeRTOS Block Q example - an application for the Pololu Orangutan X2
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
#include <stdbool.h>

#define ASSERT_TUNE "!L16 V8 dc#"


int main()
{
    lcd_init_printf();
    clear();
	play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.

	while(1)
	{
		clear();

        vStartBlockingQueueTasks(3);

        vTaskStartScheduler();
        return 0;
	}
}

void vApplicationStackOverflowHook (xTaskHandle taskHandle, char * name)
{
    printf ("Stack in %s overflowed", name);
    while (1);
}

/* Stuff to do when exiting */
void endStuff (void)
{
    printf ("Exit %d RAM free.", get_free_ram());
}

/* An assert handler for debugging */
/* The parameters are the filename or function name, line number and a null-terminated string to be printed
 * stating something useful about the condition.
 * This function doesn't actually return. */
bool assertFunc (const char * place, int line, const char * pText, int param1)
{
    if (pText)
    {
        printf ("%s#%d:%s %d\n", place, line, pText, param1);    
    }
    else
    {
        printf ("%s#%d: %d\n", place, line, param1);        
    }
    play_from_program_space (PSTR(ASSERT_TUNE));  // Play some warning notes.
    while (is_playing())
    {
    }

    endStuff ();
    while (1);

    return false;
}