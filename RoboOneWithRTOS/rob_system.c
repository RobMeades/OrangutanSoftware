/* Rob's system level funcs - an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 *  Author: Rob Meades
 */

#include <rob_system.h>
#include <rob_wrappers.h>
#include <FreeRTOS.h>

#define ASSERT_TUNE "!L16 V8 dc#"

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* Stuff to do when exiting */
void endStuff (void)
{
    rob_lcd_goto_xy(0, 3);
    rob_print_from_program_space (PSTR("Free heap: "));
    rob_print_long (xPortGetFreeHeapSize());  /* Only works for heap_1, heap_2 and heap_4 */
}

/* An assert handler for debugging */
/* The parameters are the filename or function name, line number and a null-terminated string to be printed
 * stating something useful about the condition.
 * This function doesn't actually return. */
bool assertFunc (const char * pPlace, int line, const char * pText, int param1)
{
    rob_clear();
    if (pText)
    {
        rob_print (pPlace);
        rob_print_character ('#');
        rob_print_unsigned_long (line);
        rob_print_character (':');
        rob_print (pText);
        rob_print_character (' ');
        rob_print_unsigned_long (param1);

        // RobPrintf ("%s#%d:%s %d\n", place, line, pText, param1);
    }
    else
    {
        rob_print (pPlace);
        rob_print_character ('#');
        rob_print_unsigned_long (line);
        rob_print_character (':');
        rob_print_character (' ');
        rob_print_unsigned_long (param1);
        //RobPrintf ("%s#%d: %d\n", place, line, param1);
    }
    rob_wait_play_from_program_space (PSTR(ASSERT_TUNE));  // Play some warning notes.

    endStuff ();
    while (1);

    return false;
}