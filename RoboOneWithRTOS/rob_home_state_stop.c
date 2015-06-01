/* Stop state of the homing state machine for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 */

#include <string.h>
#include <rob_system.h>
#include <rob_wrappers.h>

#include <rob_home.h>
#include <rob_home_state_machine.h>
#include <rob_home_state_machine_events.h>

#include <rob_home_state_stop.h>
#include <rob_home_state_init.h>

#include <rob_motion.h> /* For stopNow() */

/*
 * MANIFEST CONSTANTS
 */
#define STATE_STOP_NAME "Stop" /* Not longer than STATE_NAME_STRING_LENGTH - 1 characters */

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

void transitionToHomeStop (HomeState *pState)
{
    /* Fill in default handlers and name first */
    defaultImplementation (pState);
    memcpy (&(pState->name[0]), STATE_STOP_NAME, strlen (STATE_STOP_NAME) + 1); /* +1 for terminator */
    rob_print_from_program_space (PSTR ("Hm: "));
    rob_print (&(pState->name[0]));

    /* No event handlers for this state, it is transitory */
    
    /* Do the entry actions */
    stopNow();

    /* Now switch state back to the start */
    transitionToHomeInit (pState);
}