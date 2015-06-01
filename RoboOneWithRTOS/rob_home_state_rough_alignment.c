/* Rough Alignment state of the homing state machine for the Pololu Orangutan X2
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

#include <rob_home_state_rough_alignment.h>
#include <rob_home_state_failed.h>
#include <rob_home_state_fine_alignment.h>
#include <rob_home_state_stop.h>

#include <rob_motion.h> /* For stopNow() */

#include <FreeRTOS.h>
#include <queue.h>

/*
 * MANIFEST CONSTANTS
 */
#define STATE_ROUGH_ALIGNMENT_NAME "RouA" /* Not longer than STATE_NAME_STRING_LENGTH - 1 characters */

/* The period over which we measure
 * the left and right IR sensors
 * for a rough integration */
#define INTEGRATION_PERIOD_ROUGH_ALIGNMENT_SECS 10

/* The difference in count between left and right IR sensors 
 * (over the integration period) that we would like to
 * achieve with rough alignment. */
#define THRESHOLD_ROUGH_ALIGNMENT  30

/* The maximum number of times we can go around
 * the rough alignment loop before declaring a failure */
#define MAX_COUNT_ROUGH_ALIGNMENT 3

/* The maximum number of times we can (re)enter
 * rough alignment state before declaring a failure,
 * set to zero for no limit. */
#define MAX_ENTRIES_ROUGH_ALIGNMENT 3

/*
 * TYPES
 */

/*
 * GLOBAL VARIABLES
 */
static unsigned int gRoughAlignmentCount;

 /* The queue that this state uses */
extern xQueueHandle xHomeEventQueue;

/*
 * STATIC FUNCTIONS: EVENT HANDLERS
 */

 /* The idea is this:
 * Count the state of all the IR detectors over the integration period.
 * If left < front > right and front > threshold (pretty much == we've done this before) then enter Fine Alignment State.
 * Else, determine which IR direction is strongest:
 * - do this to within 45 degrees, so if two IR detectors are equally strong (within a tolerance) go half way between.
 * Rotate by this amount.
 * Repeat.
 * If Rough Alignment cannot be achieved, enter Failed State.
 */

/*
 * Handle a rough alignment integration completing.
 * 
 * pState    pointer to the state structure.
 */
static void eventHomeRoughIntegrationDone (HomeState *pState)
{
    portBASE_TYPE xStatus;

    ASSERT_PARAM (pState != PNULL, 0);

    /* TODO: something with the results */
    
    gRoughAlignmentCount++;
    if (gRoughAlignmentCount > MAX_COUNT_ROUGH_ALIGNMENT)
    {
        HomeEvent event;
        event.type = HOME_ROUGH_ALIGNMENT_FAILED_EVENT;
        xStatus = xQueueSend (xHomeEventQueue, &event, 0);
        
        ASSERT_PARAM (xStatus == pdPASS, (unsigned long) xStatus);
    }
}

/*
 * PUBLIC FUNCTIONS
 */

void transitionToHomeRoughAlignment (HomeState *pState)
{
    portBASE_TYPE xStatus;
    
    /* Fill in default handlers and name first */
    defaultImplementation (pState);
    memcpy (&(pState->name[0]), STATE_ROUGH_ALIGNMENT_NAME, strlen (STATE_ROUGH_ALIGNMENT_NAME) + 1); /* +1 for terminator */
    rob_print_from_program_space (PSTR ("Hm: "));
    rob_print (&(pState->name[0]));

    /* Now hook in the event handlers for this state */
    pState->pEventHomeRoughIntegrationDone = eventHomeRoughIntegrationDone;
    pState->pEventHomeRoughAlignmentDone = transitionToHomeFineAlignment;
    pState->pEventHomeRoughAlignmentFailed = transitionToHomeFailed;
    pState->pEventHomeStop = transitionToHomeStop;
    
    /* Do any entry actions */
    stopNow(); /* Just in case we were moving before */
    gRoughAlignmentCount = 0;
    pState->countRoughAlignmentEntries++;

 #if MAX_ENTRIES_ROUGH_ALIGNMENT > 0
    if (pState->countRoughAlignmentEntries > MAX_ENTRIES_ROUGH_ALIGNMENT)
    {
        HomeEvent event;
        event.type = HOME_ROUGH_ALIGNMENT_FAILED_EVENT;
        xStatus = xQueueSend (xHomeEventQueue, &event, 0);
        
        ASSERT_PARAM (xStatus == pdPASS, (unsigned long) xStatus);
    }
    else
    {   
#endif
        /* TODO: start a rough integration */
#if MAX_ENTRIES_ROUGH_ALIGNMENT > 0
    }
#endif
 }