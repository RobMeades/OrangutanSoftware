/* Travel state of the homing state machine for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 */

#include <string.h>
#include <rob_system.h>
#include <rob_wrappers.h>
#include <rob_motion.h>

#include <rob_home.h>
#include <rob_home_state_machine.h>
#include <rob_home_state_machine_events.h>

#include <rob_home_state_fine_alignment.h>
#include <rob_home_state_travel.h>
#include <rob_home_state_stop.h>

#include <FreeRTOS.h>
#include <queue.h>

/*
 * MANIFEST CONSTANTS
 */
#define STATE_TRAVEL_NAME "Trvl" /* Not longer than STATE_NAME_STRING_LENGTH - 1 characters */

/* The maximum we allow the left or right motors to be tweaked by */
#define MAX_TWEAK    20

/* The speed at which we attempt to travel home */
#define HOME_SPEED   MINIMUM_USEFUL_SPEED_O_UNITS + MAX_TWEAK

/* The integration period over which we
 * measure the left and right IR sensors
 * during travel */
#define INTEGRATION_PERIOD_TRAVEL_SECS 3

/* The difference in count between left and right IR sensors 
 * (over the integration period) that we would like to
*  achieve when travelling. */
#define THRESHOLD_TRAVEL  10

/* The maximum number of times we can re-align during
 * travel before declaring a failure */
#define MAX_COUNT_TRAVEL_ALIGNMENT 50

/* The maximum number of times we can (re)enter
 * travel state before declaring a failure,
 * set to zero for no limit. */
#define MAX_ENTRIES_TRAVEL 3

/*
 * TYPES
 */

/*
 * GLOBAL VARIABLES
 */
static unsigned int gTravelAlignmentCount;
static int gSpeedLeft;
static int gSpeedRight;

 /* The queue that this state uses */
extern xQueueHandle xHomeEventQueue;

/*
 * STATIC FUNCTIONS: EVENT HANDLERS
 */

 /*
 * The idea is this:
 * Set left and right motion to speedLeft and speedRight.
 * Count the state of all IR detectors over the integration period.
 * If left > right then speedLeft++, speedRight--.
 * Otherwise if right > left then speedRight++, speedLeft--.
 * If abs(left-right) > threshold then drop back to Fine Alignment State.
 *
 * Note that we have no idea if we've made it to the charger or not, this
 * has to come over the USB interface from the Pi as only it can detect
 * 12V.
 */

/*
 * Handle a travel alignment integration completing.
 * 
 * pState    pointer to the state structure.
 */
static void eventHomeTravelIntegrationDone (HomeState *pState)
{
    portBASE_TYPE xStatus;

    ASSERT_PARAM (pState != PNULL, 0);

    /* TODO: something with the results */
    
    gTravelAlignmentCount++;
    if (gTravelAlignmentCount > MAX_COUNT_TRAVEL_ALIGNMENT)
    {
        HomeEvent event;
        event.type = HOME_TRAVEL_ALIGNMENT_FAILED_EVENT;
        xStatus = xQueueSend (xHomeEventQueue, &event, 0);

        ASSERT_PARAM (xStatus == pdPASS, (unsigned long) xStatus);
    }
}

/*
 * PUBLIC FUNCTIONS
 */

void transitionToHomeTravel (HomeState *pState)
{
    portBASE_TYPE xStatus;

    /* Fill in default handlers and name first */
    defaultImplementation (pState);
    memcpy (&(pState->name[0]), STATE_TRAVEL_NAME, strlen (STATE_TRAVEL_NAME) + 1); /* +1 for terminator */
    rob_print_from_program_space (PSTR ("Hm: "));
    rob_print (&(pState->name[0]));

    /* Now hook in the event handlers for this state */
    pState->pEventHomeTravelIntegrationDone = eventHomeTravelIntegrationDone;
    pState->pEventHomeTravelAlignmentFailed = transitionToHomeFineAlignment;
    pState->pEventHomeStop = transitionToHomeStop;

    /* Do any entry actions */
    gSpeedLeft = HOME_SPEED;
    gSpeedRight = HOME_SPEED;
    gTravelAlignmentCount = 0;
    pState->countTravelEntries++;

#if MAX_ENTRIES_TRAVEL > 0
    if (pState->countTravelEntries > MAX_ENTRIES_TRAVEL)
    {
        HomeEvent event;
        event.type = HOME_TRAVEL_ALIGNMENT_FAILED_EVENT;
        xStatus = xQueueSend (xHomeEventQueue, &event, 0);

        ASSERT_PARAM (xStatus == pdPASS, (unsigned long) xStatus);
    }
    else
    {
#endif
        /* TODO: start a travel integration */
#if MAX_ENTRIES_TRAVEL > 0
    }
#endif    
}