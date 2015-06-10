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

#include <rob_motion.h> /* For stopNow() and turn() */

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

/* The tolerance within which two adjacent detectors
 * can be considered to be the same. */
#define SIMILARITY_TOLERANCE  10

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
static unsigned int gLeftCount;
static unsigned int gFrontCount;
static unsigned int gRightCount;
static unsigned int gBackCount;

 /* The queue that this state uses */
extern xQueueHandle xHomeEventQueue;

/*
 * STATIC FUNCTIONS
 */

/* Do a rough integration */
static HomeEvent doRoughIntegration (void)
{
    countIrDetector (INTEGRATION_PERIOD_ROUGH_ALIGNMENT_SECS * 100, &gFrontCount, &gRightCount, &gBackCount, &gLeftCount);
    
    return HOME_ROUGH_INTEGRATION_DONE_EVENT;
}

/* Given a list, of length numMembers, fill in a second list of the same length
 * as an index of the order of the items in the first list, largest item in position 0. */
static void getListOrder (unsigned int *pList, unsigned int numMembers, unsigned char *pOrderList)
{
    unsigned int x;
    unsigned int tmpInt;
    unsigned char tmpChar;
    unsigned int * pItem;
    unsigned int * pItemList;
    unsigned char * pOrder;

    ASSERT_PARAM (pList != NULL, 0);
    ASSERT_PARAM (pOrderList != NULL, 0);
    
    pOrder = pOrderList;
    for (x = 0; x < numMembers; x++)
    {
        *pOrder = x;
        pOrder++;
    }

    pItemList = RobMalloc (numMembers * sizeof (*pList));
    ASSERT_PARAM (pItemList != NULL, 0);
    memcpy (pItemList, pList, numMembers * sizeof (*pList));
    
    pItem = pItemList;
    pOrder = pOrderList;
    
    for (x = 0; x < (numMembers - 1); x++)
    {
        if (*(pItem + 1) > *pItem)
        {
            tmpInt = *pItem;
            *pItem = *(pItem + 1);
            *(pItem + 1) = tmpInt;
            
            tmpChar = *pOrder;
            *pOrder = *(pOrder + 1);
            *(pOrder + 1) = tmpChar;
            
            x = 0;
            pItem = pItemList;
            pOrder = pOrderList;
        }
        else
        {            
            pItem++;
            pOrder++;
        }
    }
}

/* Find the strongest of the four directions relative to our current position in degrees. */
static int findStrongest (unsigned int countFront, unsigned int countRight, unsigned int countBack, unsigned int countLeft)
{
    int angleArray [] = {0, 45, 90, 135, 180, -135, -90, -45};
    unsigned char x;
    unsigned int count[4];
    unsigned char order[4];
    
    count[0] = countFront;
    count[1] = countRight;
    count[2] = countBack;
    count[3] = countLeft;
    
    /* Get the order of the counts in order[], largest first */
    getListOrder (&(count[0]), sizeof (count) / sizeof (count[0]), &(order[0]));
    ASSERT_PARAM (order[0] < (sizeof (count) / sizeof (count[0])), order[0]);
    
    /* Find the angle from that */
    x = order[0] * 2;
    
    /* If the top two are adjacent and similar, go half way */
    if (((order[0]++ == order[1]) || (order[1]++ == order[0])) &&
        ((count[order[0]] - count[order[1]]) < SIMILARITY_TOLERANCE))
    {
        x++;
    }
    
    return angleArray[x];
}

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
    HomeEvent event = HOME_ROUGH_ALIGNMENT_FAILED_EVENT;
    portBASE_TYPE xStatus;

    ASSERT_PARAM (pState != PNULL, 0);

    /* Check the results */
    if ((gFrontCount >= gLeftCount) &&
        (gFrontCount >= gRightCount) &&
        (gFrontCount > THRESHOLD_ROUGH_ALIGNMENT))
    {
        event = HOME_ROUGH_ALIGNMENT_DONE_EVENT;
    }
    else
    {
        gRoughAlignmentCount++;
        if (gRoughAlignmentCount <= MAX_COUNT_ROUGH_ALIGNMENT)
        {
            int turnAngle;
            
            /* Determine the direction to turn */
            turnAngle = findStrongest (gFrontCount, gRightCount, gBackCount, gLeftCount);
            
            rob_print_from_program_space (PSTR ("RA: "));
            rob_print_unsigned_long (gFrontCount);
            rob_print_from_program_space (PSTR ("/"));
            rob_print_unsigned_long (gRightCount);
            rob_print_from_program_space (PSTR ("/"));
            rob_print_unsigned_long (gBackCount);
            rob_print_from_program_space (PSTR ("/"));
            rob_print_unsigned_long (gLeftCount);
            rob_print_from_program_space (PSTR (":"));
            rob_print_long (turnAngle);
            /* Turn */
            if (turn (-turnAngle))  /* negative 'cos we're reversing to the charger */
            {
                /* Do another rough integration */
                event = doRoughIntegration();
            }
        }
    }    

    xStatus = xQueueSend (xHomeEventQueue, &event, 0);
    ASSERT_PARAM (xStatus == pdPASS, (unsigned long) xStatus);
}

/*
 * PUBLIC FUNCTIONS
 */

void transitionToHomeRoughAlignment (HomeState *pState)
{
    HomeEvent event;
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
        event = HOME_ROUGH_ALIGNMENT_FAILED_EVENT;
    }
    else
    {   
#endif
        /* Do a rough integration */
        event = doRoughIntegration();
                
#if MAX_ENTRIES_ROUGH_ALIGNMENT > 0
    }
#endif

    xStatus = xQueueSend (xHomeEventQueue, &event, 0);        
    ASSERT_PARAM (xStatus == pdPASS, (unsigned long) xStatus);
 }