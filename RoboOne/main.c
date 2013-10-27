/* RoboOne - an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 *  Author: Rob Meades
 */

/* Command set:
 *
 * Forwards x.xx m or m/s
 * Backwards x.xx m or m/s
 * Right xx
 * Left xx
 * Stop
 * Home
 * Mark
 * Distance?
 * Velocity?
 * Progress?
 * Echo
 *
 * Commands can be single letters, we only look for a numeric character, Forwards and
 * Backwards can be in units of metres or metres per second, turns can be 90 degrees
 * or a deviation in degrees, Mark will set the zero point and Distance? reads
 * the distance travelled since the last mark.  Home is the "return to charger"
 * command and only works if the robot is in sight of the charger. Progress?
 * reports what the robot is currently doing.  Echo is used purely for testing and
 * causes every received command to be echoed without action (until reset). If a
 * command is prefixed by # and a number then the responses from the controller are
 * prefixed with the same tag (so that sequences of commands can be sent and the
 * responses matched up).
 *
 * The main responses sent for each command are:
 *
 * Done
 * Error (potentially followed by more informative text)
 *
 * The response to the Distance? query is
 *
 * x.xx metres
 *
 * The response to the Velocity? query is
 *
 * Forwards/Backwards x.xx m/s
 *
 * The response to the Progress? query is to return the command in progress, e.g.
 *
 * Home
 */

#include <pololu/orangutan.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <rob_system.h>
#include <rob_comms.h>
#include <rob_processing.h>
#include <rob_motion.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#define COMMS_RECEIVE_QUEUE_SIZE 8
#define COMMAND_QUEUE_SIZE 8
#define HELLO_STRING "RoboOne at your service.\r\n"
#define HELLO_TUNE ">g32>>c32"

/* - OS STUFF ------------------------------------------------------------------------- */

/* Queue for received serial strings */
xQueueHandle xCommsReceiveQueue;

/* Queue for commands */
xQueueHandle xCommandQueue;

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* One-time initialisation */
static void startStuff (void)
{
	play_from_program_space (PSTR(HELLO_TUNE));  /* Play welcoming notes */

    /* Sort the display */
    lcd_init_printf();
    clear();

    /* Sort the serial port */
    commsInit();

    /* Say hello */
    serial_send (USB_COMM, HELLO_STRING, strlen(HELLO_STRING));
}

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

int main()
{
    startStuff();

	while (1)
	{
        /* Create the queues */
        xCommsReceiveQueue = xQueueCreate (COMMS_RECEIVE_QUEUE_SIZE, sizeof (CommandString *));
        ASSERT_STRING (xCommsReceiveQueue, "Could not create xCommsReceiveQueue");
        xCommandQueue = xQueueCreate (COMMAND_QUEUE_SIZE, sizeof (CodedCommand));
        ASSERT_STRING (xCommandQueue, "Could not create xCommandQueue");

        /* Create the tasks */
        xTaskCreate (vTaskProcessing, (signed char * const) "ProcessingTask", 250, PNULL, 1, NULL);
        xTaskCreate (vTaskMotion, (signed char * const) "MotionTask", 250, PNULL, 2, NULL);
        xTaskCreate (vTaskCommsIO, (signed char * const) "CommsIOTask", 250, PNULL, 3, NULL);

        /* Start the scheduler */
        vTaskStartScheduler();

        ASSERT_ALWAYS_STRING ("Should never get here!");
	}

    endStuff();
}

void vApplicationStackOverflowHook (xTaskHandle taskHandle, char * name)
{
    printf ("Stack in %s overflowed", name);
    while (1);
}