/* Fine Alignment state of the homing state machine for the Pololu Orangutan X2
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

#include <rob_home_state_fine_alignment.h>
#include <rob_home_state_rough_alignment.h>
#include <rob_home_state_travel.h>
#include <rob_home_state_stop.h>

#include <rob_motion.h> /* For stopNow() */

#include <FreeRTOS.h>
#include <queue.h>

/*
 * MANIFEST CONSTANTS
 */
#define STATE_FINE_ALIGNMENT_NAME "FinA" /* Not longer than STATE_NAME_STRING_LENGTH - 1 characters */

/* The integration period over which we
 * measure the left and right IR sensors
 * for each fine integration */
#define INTEGRATION_PERIOD_FINE_ALIGNMENT_SECS 3

/* The turn angle for one pulse */
#define TURN_ANGLE_DEGREES 10

/* The difference in count between left and right IR sensors 
 * (over the integration period) that we would like to
*  achieve with fine alignment. */
#define THRESHOLD_FINE_ALIGNMENT  10

/* The maximum number of times we can go around
 * the fine alignment loop before declaring a failure */
#define MAX_COUNT_FINE_ALIGNMENT 15

/* The maximum number of times we can (re)enter
 * fine alignment state before declaring a failure,
 * set to zero for no limit. */
#define MAX_ENTRIES_FINE_ALIGNMENT 3

/*
 * TYPES
 */

/*
 * GLOBAL VARIABLES
 */
static unsigned int gFineAlignmentCount;
static unsigned int gLeftCount;
static unsigned int gRightCount;

/* The queue that this state uses */
extern xQueueHandle xHomeEventQueue;

/*
 * STATIC FUNCTIONS
 */

/* Do a fine integration */
static HomeEventType doFineIntegration (void)
{
    countIrDetector (INTEGRATION_PERIOD_FINE_ALIGNMENT_SECS * 100, NULL, &gRightCount, NULL, &gLeftCount);
    
    return HOME_FINE_INTEGRATION_DONE_EVENT;
}

/*
 * STATIC FUNCTIONS: EVENT HANDLERS
 */
/* The idea is this:
 * Count the state of all the IR detectors over the integration period.
 * If abs(left-right) < threshold then enter Travel state.
 * Otherwise, turn one pulse sign(left-right).
 * Repeat.
 * If fine alignment cannot be achieved, enter Rough Alignment State.
 */

/*
 * Handle a fine alignment integration completing.
 * 
 * pState    pointer to the state structure.
 */
static void eventHomeFineIntegrationDone (HomeState *pState)
{
    HomeEvent event;
    portBASE_TYPE xStatus;
    int leftMinusRight;

    ASSERT_PARAM (pState != PNULL, 0);

    event.type = HOME_FINE_ALIGNMENT_FAILED_EVENT;

    leftMinusRight = gLeftCount - gRightCount;
    
    if (abs (leftMinusRight) < THRESHOLD_FINE_ALIGNMENT)
    {
        event.type = HOME_FINE_ALIGNMENT_DONE_EVENT;
    }
    else
    {
        gFineAlignmentCount++;
        if (gFineAlignmentCount <= MAX_COUNT_FINE_ALIGNMENT)
        {
            int turnAngle = TURN_ANGLE_DEGREES;
            
            if (leftMinusRight < 0)
            {
                turnAngle = -turnAngle;                
            }
            
            /* Turn */
            if (turn (turnAngle))
            {
                /* Do another fine integration */
                event.type = doFineIntegration();
            }
            else
            {
                event.type = HOME_ROUGH_ALIGNMENT_FAILED_EVENT;
            }
        }
    }
    
    xStatus = xQueueSend (xHomeEventQueue, &event, 0);
    ASSERT_PARAM (xStatus == pdPASS, (unsigned long) xStatus);        
}

/*
 * PUBLIC FUNCTIONS
 */

void transitionToHomeFineAlignment (HomeState *pState)
{
    HomeEvent event;
    portBASE_TYPE xStatus;

    /* Fill in default handlers and name first */
    defaultImplementation (pState);
    memcpy (&(pState->name[0]), STATE_FINE_ALIGNMENT_NAME, strlen (STATE_FINE_ALIGNMENT_NAME) + 1); /* +1 for terminator */
    rob_print_from_program_space (PSTR ("Hm: "));
    rob_print (&(pState->name[0]));

    /* Now hook in the event handlers for this state */
    pState->pEventHomeFineIntegrationDone = eventHomeFineIntegrationDone;
    pState->pEventHomeFineAlignmentDone = transitionToHomeTravel;
    pState->pEventHomeFineAlignmentFailed = transitionToHomeRoughAlignment;
    pState->pEventHomeStop = transitionToHomeStop;
    
    /* Do the entry actions */
    stopNow(); /* Just in case we came here from travel state */
    gFineAlignmentCount = 0;
    pState->countFineAlignmentEntries++;

#if MAX_ENTRIES_FINE_ALIGNMENT > 0
    if (pState->countFineAlignmentEntries > MAX_ENTRIES_FINE_ALIGNMENT)
    {
        event.type = HOME_FINE_ALIGNMENT_FAILED_EVENT;
    }
    else
    {
#endif        
        /* Do a fine integration */
        event.type = doFineIntegration();

#if MAX_ENTRIES_FINE_ALIGNMENT > 0
    }
#endif

    xStatus = xQueueSend (xHomeEventQueue, &event, 0);        
    ASSERT_PARAM (xStatus == pdPASS, (unsigned long) xStatus);
}