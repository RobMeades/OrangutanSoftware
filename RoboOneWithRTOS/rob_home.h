/* Home - homing part of an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 * Author: Rob Meades
 */

/*
 * MESSAGE TYPES
 */

/* The types of events that can occur */
typedef enum HomeEventTypeTag
{
    HOME_START_EVENT,
    HOME_ROUGH_INTEGRATION_START_EVENT,
    HOME_ROUGH_INTEGRATION_DONE_EVENT,
    HOME_ROUGH_ALIGNMENT_DONE_EVENT,
    HOME_ROUGH_ALIGNMENT_FAILED_EVENT,
    HOME_FINE_INTEGRATION_START_EVENT,
    HOME_FINE_INTEGRATION_DONE_EVENT,
    HOME_FINE_ALIGNMENT_DONE_EVENT,
    HOME_FINE_ALIGNMENT_FAILED_EVENT,
    HOME_TRAVEL_INTEGRATION_START_EVENT,
    HOME_TRAVEL_INTEGRATION_DONE_EVENT,
    HOME_TRAVEL_ALIGNMENT_FAILED_EVENT,
    HOME_STOP_EVENT,
    MAX_NUM_HOME_EVENTS
} HomeEventType;    
  
 /* A buffer containing a state machine event */
typedef struct HomeEventTag
{
    HomeEventType type;
} HomeEvent;

void vTaskHome (void *pvParameters);