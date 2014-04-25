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
 * Info?
 * Echo
 * A"xxx"
 * T"xxx"
 * !
 *
 * Commands can be single letters, we only look for a numeric character, Forwards and
 * Backwards can be in units of metres or metres per second, turns can be 90 degrees
 * or a deviation in degrees, Mark will set the zero point and Distance? reads
 * the distance travelled since the last mark.  Home is the "return to charger"
 * command and only works if the robot is in sight of the charger. Progress?
 * reports what the robot is currently doing.  Info? returns a standard set of
 * status information.  Echo is used purely for testing and causes every received
 * command to be echoed without action (until reset). A is followed immediately
 * by a quoted Alphanumeric string that will be shown on the LCD display. T is
 * like A but the contents of the string is a Tune string.  "!" just causes an
 * OK response.  If a command is prefixed by # and a number then the responses
 * from the controller are prefixed with the same tag (so that sequences of
 * commands can be sent and the responses matched up).
 *
 * The main responses sent for each command are:
 *
 * OK
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

#include <rob_system.h>
#include <rob_wrappers.h>
#include <rob_comms.h>
#include <rob_processing.h>
#include <rob_motion.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#define COMMS_RECEIVE_QUEUE_SIZE 8
#define COMMS_TRANSMIT_QUEUE_SIZE 8
#define HELLO_STRING "RoboOne started.\r\n"
#define HELLO_STRING_NO_TERMINATOR "RoboOne started."
#define HELLO_TUNE ">g32>>c32"

/* - OS STUFF ------------------------------------------------------------------------- */

/* Queue for received serial strings */
xQueueHandle xCommsReceiveQueue;

/* Queue for transmitting serial strings */
xQueueHandle xCommsTransmitQueue;

/* Queue for commands */
xQueueHandle xCommandQueue;

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* One-time initialisation */
static void startStuff (void)
{
	rob_wait_play_from_program_space (PSTR(HELLO_TUNE));  /* Play welcoming notes */

    /* Sort the display */
#if 0
    rob_lcd_init_printf();
#endif
    rob_clear();

    /* Sort the serial port */
    commsInit();

    /* Say hello */
    rob_print_from_program_space (PSTR(HELLO_STRING_NO_TERMINATOR));
    rob_serial_send_blocking_usb_comm (HELLO_STRING, RobStrlen(HELLO_STRING));
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
        xCommsTransmitQueue = xQueueCreate (COMMS_TRANSMIT_QUEUE_SIZE, sizeof (char *));
        ASSERT_STRING (xCommsTransmitQueue, "Could not create xCommsTransmitQueue");

        /* Create the tasks */
        xTaskCreate (vTaskMotion, (signed char * const) "MotionTask", 500, PNULL, 1, NULL);
        xTaskCreate (vTaskProcessing, (signed char * const) "ProcessingTask", 500, PNULL, 2, NULL); /* Higher than motion so that we can interrupt it */
        xTaskCreate (vTaskCommsTransmit, (signed char * const) "CommsTransmitTask", 500, PNULL, 3, NULL);
        xTaskCreate (vTaskCommsReceive, (signed char * const) "CommsReceiveTask", 500, PNULL, 4, NULL);

        /* Start the scheduler */
        vTaskStartScheduler();

        ASSERT_ALWAYS_STRING ("Should never get here!");
	}

    endStuff();
}

void vApplicationStackOverflowHook (xTaskHandle taskHandle, char * pName)
{
    rob_print_from_program_space (PSTR("Stack overflow in "));
    rob_print (pName);
    while (1);
}