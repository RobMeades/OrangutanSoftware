/* Home - the homing part of an application for the Pololu Orangutan X2
 * This task is different to the other tasks in that it doesn't expect a command string
 * from the serial interface or send a command string back to the serial interface.  It 
 * instead uses messaging private to the Home task and sub-task.
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 * Author: Rob Meades
 */

#include <rob_system.h>
#include <rob_wrappers.h>
#include <rob_comms.h>
#include <rob_home_state_machine.h>
#include <rob_home_state_init.h>
#include <rob_home_state_machine_events.h>
#include <rob_home.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <pololu/orangutan.h>

/* - GLOBALS -------------------------------------------------------------------------- */

static HomeContext gHomeContext;

/* - STATIC VARIABLES ----------------------------------------------------------------- */

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* The queue that the homing task uses */
extern xQueueHandle xHomeEventQueue;

/* The Homing task */
void vTaskHome (void *pvParameters)
{
    bool success;
    HomeEvent event;
    portBASE_TYPE xStatus;

    memset (&gHomeContext, 0, sizeof (gHomeContext));
    transitionToHomeInit (&(gHomeContext.state));
    
    while (1)
    {
        xStatus = xQueueReceive (xHomeEventQueue, &event, portMAX_DELAY);

        ASSERT_STRING (xStatus == pdPASS, "Failed to receive from home event queue.");

        success = false; /* Assume failure */

        /* Now call the indicated event */
        switch (event.type)
        {
            case HOME_START_EVENT:
            {
                eventHomeStartOrangutan (&gHomeContext);
                success = true;
            }
            break;
            case HOME_ROUGH_INTEGRATION_DONE_EVENT:
            {
                eventHomeRoughIntegrationDoneOrangutan (&gHomeContext);
                success = true;
            }
            break;
            case HOME_ROUGH_ALIGNMENT_DONE_EVENT:
            {
                eventHomeRoughAlignmentDoneOrangutan (&gHomeContext);
                success = true;
            }
            break;
            case HOME_ROUGH_ALIGNMENT_FAILED_EVENT:
            {
                eventHomeRoughAlignmentFailedOrangutan (&gHomeContext);
                success = true;
            }
            break;
            case HOME_FINE_INTEGRATION_DONE_EVENT:
            {
                eventHomeFineIntegrationDoneOrangutan (&gHomeContext);
                success = true;
            }
            break;
            case HOME_FINE_ALIGNMENT_DONE_EVENT:
            {
                eventHomeFineAlignmentDoneOrangutan (&gHomeContext);
                success = true;
            }
            break;
            case HOME_FINE_ALIGNMENT_FAILED_EVENT:
            {
                eventHomeFineAlignmentFailedOrangutan (&gHomeContext);
                success = true;
            }
            break;
            case HOME_TRAVEL_INTEGRATION_DONE_EVENT:
            {
                eventHomeTravelIntegrationDoneOrangutan (&gHomeContext);
                success = true;
            }
            break;
            case HOME_TRAVEL_ALIGNMENT_FAILED_EVENT:
            {
                eventHomeTravelAlignmentFailedOrangutan (&gHomeContext);
                success = true;
            }
            break;
            case HOME_STOP_EVENT:
            {
                eventHomeStopOrangutan (&gHomeContext);
                success = true;
            }
            break;
            default:
            {
                ASSERT_ALWAYS_PARAM (event.type);   
            }
            break;
        }

        if (!success)
        {
            sendSerialString (ERROR_STRING, sizeof (ERROR_STRING));
        }
    }
}