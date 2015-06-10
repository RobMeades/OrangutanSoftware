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

/* How pins of the digital input connectors are connected */
#define IR_DETECTOR_ENABLE_PIN  IO_D0
#define IR_DETECTOR_FRONT_PIN   IO_D1
#define IR_DETECTOR_RIGHT_PIN   IO_D2
#define IR_DETECTOR_BACK_PIN    IO_D3
#define IR_DETECTOR_LEFT_PIN    IO_D4

/* - GLOBALS -------------------------------------------------------------------------- */

static HomeContext gHomeContext;

/* - STATIC VARIABLES ----------------------------------------------------------------- */

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* Setup the pins we need to the states we need */
static void setPins (void)
{
    set_digital_output (IR_DETECTOR_ENABLE_PIN, 1);
    set_digital_input (IR_DETECTOR_RIGHT_PIN, 0);
    set_digital_input (IR_DETECTOR_LEFT_PIN, 0);
    set_digital_input (IR_DETECTOR_FRONT_PIN, 0);
    set_digital_input (IR_DETECTOR_BACK_PIN, 0);    
}

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* The queue that the homing task uses */
extern xQueueHandle xHomeEventQueue;

/* Count the state of the IR detectors over an integration period */
void countIrDetector (int period10ms, unsigned int * pCountFront, unsigned int * pCountRight, unsigned int * pCountBack, unsigned int * pCountLeft)
{
    int x;
    
    if (pCountFront != NULL)
    {
        *pCountFront = 0;
    }
    if (pCountRight != NULL)
    {
        *pCountRight = 0;
    }
    if (pCountBack != NULL)
    {
        *pCountBack = 0;
    }
    if (pCountLeft != NULL)
    {
        *pCountLeft = 0 ;
    }
    for (x = 0; x < period10ms; x++)
    {
        if (pCountFront != NULL && !is_digital_input_high (IR_DETECTOR_FRONT_PIN))
        {
            (*pCountFront)++;
        }
        if (pCountRight != NULL && !is_digital_input_high (IR_DETECTOR_RIGHT_PIN))
        {            
            (*pCountRight)++;
        }
        if (pCountBack != NULL && !is_digital_input_high (IR_DETECTOR_BACK_PIN))
        {
            (*pCountBack)++;
        }
        if (pCountLeft != NULL && !is_digital_input_high (IR_DETECTOR_LEFT_PIN))
        {
            (*pCountLeft)++;
        }
        vTaskDelay (1 / portTICK_RATE_MS);
    }    
}

/* The Homing task */
void vTaskHome (void *pvParameters)
{
    bool success;
    HomeEvent event;
    portBASE_TYPE xStatus;

    setPins();
    memset (&gHomeContext, 0, sizeof (gHomeContext));
    transitionToHomeInit (&(gHomeContext.state));
    
    while (1)
    {
        xStatus = xQueueReceive (xHomeEventQueue, &event, portMAX_DELAY);

        ASSERT_STRING (xStatus == pdPASS, "Failed to receive from home event queue.");

        success = true; /* Assume success */

        /* Now call the indicated event */
        switch (event)
        {
            case HOME_START_EVENT:
            {
                sendSerialString (OK_STRING, sizeof (OK_STRING));
                eventHomeStartOrangutan (&gHomeContext);
            }
            break;
            case HOME_ROUGH_INTEGRATION_DONE_EVENT:
            {
                eventHomeRoughIntegrationDoneOrangutan (&gHomeContext);
            }
            break;
            case HOME_ROUGH_ALIGNMENT_DONE_EVENT:
            {
                eventHomeRoughAlignmentDoneOrangutan (&gHomeContext);
            }
            break;
            case HOME_ROUGH_ALIGNMENT_FAILED_EVENT:
            {
                eventHomeRoughAlignmentFailedOrangutan (&gHomeContext);
            }
            break;
            case HOME_FINE_INTEGRATION_DONE_EVENT:
            {
                eventHomeFineIntegrationDoneOrangutan (&gHomeContext);
            }
            break;
            case HOME_FINE_ALIGNMENT_DONE_EVENT:
            {
                eventHomeFineAlignmentDoneOrangutan (&gHomeContext);
            }
            break;
            case HOME_FINE_ALIGNMENT_FAILED_EVENT:
            {
                eventHomeFineAlignmentFailedOrangutan (&gHomeContext);
            }
            break;
            case HOME_TRAVEL_INTEGRATION_DONE_EVENT:
            {
                eventHomeTravelIntegrationDoneOrangutan (&gHomeContext);
            }
            break;
            case HOME_TRAVEL_ALIGNMENT_FAILED_EVENT:
            {
                eventHomeTravelAlignmentFailedOrangutan (&gHomeContext);
            }
            break;
            case HOME_STOP_EVENT:
            {
                eventHomeStopOrangutan (&gHomeContext);
            }
            break;
            default:
            {
                success = false;
                ASSERT_ALWAYS_PARAM (event);   
            }
            break;
        }

        if (!success)
        {
            sendSerialString (ERROR_STRING, sizeof (ERROR_STRING));
        }
    }
}