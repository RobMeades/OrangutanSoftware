/* Rob's system level funcs - an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 *  Author: Rob Meades
 */

#include <pololu/orangutan.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <rob_system.h>

#define ASSERT_TUNE "!L16 V8 dc#"

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* Stuff to do when exiting */
void endStuff (void)
{
    printf ("Exit %d RAM free.", get_free_ram());
}

/* An assert handler for debugging */
/* The parameters are the filename or function name, line number and a null-terminated string to be printed
 * stating something useful about the condition.
 * This function doesn't actually return. */
bool assertFunc (const char * place, int line, const char * text)
{
    printf ("%s#%d:%s\n", place, line, text);
    play_from_program_space (PSTR(ASSERT_TUNE));  // Play some warning notes.
    while (is_playing())
    {
    }

    endStuff ();
    while (1);

    return false;
}