/* Failed state of the homing state machine for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 */

#include <string.h>
#include <rob_system.h>
#include <rob_wrappers.h>

#include <rob_home_state_machine.h>
#include <rob_home_state_failed.h>
#include <rob_home_state_init.h>

#include <rob_motion.h> /* For stopNow() */

/*
 * MANIFEST CONSTANTS
 */
#define STATE_FAILED_NAME "Fail" /* Not longer than STATE_NAME_STRING_LENGTH - 1 characters */

/*
 * TYPES
 */

/*
 * STATIC FUNCTIONS: EVENT HANDLERS
 */

/*
 * STATIC FUNCTIONS: THE STATE BODY
 */

/*
 * PUBLIC FUNCTIONS
 */

void transitionToHomeFailed (HomeState *pState)
{
    /* Fill in default handlers and name first */
    defaultImplementation (pState);
    memcpy (&(pState->name[0]), STATE_FAILED_NAME, strlen (STATE_FAILED_NAME) + 1); /* +1 for terminator */
    rob_print_from_program_space (PSTR ("Hm: "));
    rob_print (&(pState->name[0]));

    /* No event handlers for this state, it is transitory */
    
    /* Do any entry actions */
    stopNow(); /* Just in case we were moving before */
    
    /* Now switch state back to the start */
    transitionToHomeInit (pState);
}