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

#define COMMAND_LIST_SIZE 4
#define HELLO_STRING "RoboOne at your service.\r\n"
#define HELLO_TUNE ">g32>>c32"

         char *pCommandStringList[COMMAND_LIST_SIZE]; /* List of pointers to malloced strings containing received commands */
unsigned char nextCommandString = 0;
CodedCommand  codedCommandList[COMMAND_LIST_SIZE];  /* An array of coded command buffers */
unsigned char nextCodedCommand = 0;

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* One-time initialisation */
static void startStuff (void)
{
    unsigned char x;

	play_from_program_space (PSTR(HELLO_TUNE));  /* Play welcoming notes */
    lcd_init_printf();
    clear();

    commsInit();

    for (x = 0; x < COMMAND_LIST_SIZE; x++)
    {
        pCommandStringList[x] = PNULL;
    }

    /* Say hello */
    serial_send (USB_COMM, HELLO_STRING, strlen(HELLO_STRING));
}

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

int main()
{
    bool success = true;
    bool echo = false;

    startStuff();

	while (success)
	{
        /* Check for things going on on the USB interface (no interrupts there) */
        serial_check();

        success = receiveSerialCommand (&pCommandStringList[nextCommandString]);
        if (success && pCommandStringList[nextCommandString] != PNULL)
        {
            //printf ("Received: %s\n", pCommandStringList[nextCommandString]);
            if (!echo)
            {
                if (processCommand (pCommandStringList[nextCommandString], &codedCommandList[nextCommandString]))
                {
                    printf ("OK: #%d %c %d", codedCommandList[nextCommandString].buffer[CODED_COMMAND_INDEX_POS], codedCommandList[nextCommandString].buffer[CODED_COMMAND_ID_POS], (((unsigned int) codedCommandList[nextCommandString].buffer[CODED_COMMAND_VALUE_POS]) << 8) + codedCommandList[nextCommandString].buffer[CODED_COMMAND_VALUE_POS + 1]);
                    if (codedCommandList[nextCommandString].buffer[CODED_COMMAND_UNITS_POS] != 0)
                    {
                        printf (" %c", codedCommandList[nextCommandString].buffer[CODED_COMMAND_UNITS_POS]);
                    }
                    if (codedCommandList[nextCommandString].buffer[CODED_COMMAND_ID_POS] == 'E')
                    {
                        echo = true;
                    }
                }
                else
                {
                    printf ("Bad command");
                }
                printf ("\n");

                free (pCommandStringList[nextCommandString]);
                pCommandStringList[nextCommandString] = PNULL;
            }
            else
            {
                sendSerialString (&pCommandStringList[nextCommandString]); /* This handles the freeing of the entry by itself */
            }

            nextCommandString++;
            if (nextCommandString >= COMMAND_LIST_SIZE)
            {
                nextCommandString = 0;
            }

            ASSERT (pCommandStringList[nextCommandString] == PNULL, "Command List Full");
        }
	}

    endStuff();
}

