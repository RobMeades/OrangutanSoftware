/* Motion - the motion control part of an application for the Pololu Orangutan X2
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
#include <rob_processing.h>
#include <rob_comms.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <pololu/orangutan.h>

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* Convert the two byte value field into an SInt16 */
static int convertValueToInt (unsigned char *pValueArray)
{
    int value;

    value = (((unsigned int) *pValueArray) << 8) + *(pValueArray + 1);

    return value;
}

/* Stop dead */
static bool stopNow (void)
{
    rob_print_from_program_space (PSTR ("STOP."));
    x2_set_motor (JOINT_MOTOR, BRAKE_LOW, 0);

    return true;
}

/* Move forwards or backwards */
static bool setDistance (int distance)
{
    rob_print_long (distance);
    rob_print_from_program_space (PSTR (" CM."));
    x2_set_motor (JOINT_MOTOR, ACCEL_DRIVE, distance);

    return true;
}

/* Set speed (forward or reverse) */
static bool setSpeed (int speed)
{
    rob_print_long (speed);
    rob_print_from_program_space (PSTR (" CM/S."));
    x2_set_motor (JOINT_MOTOR, ACCEL_DRIVE, speed);

    return true;
}

/* Turn (left or right) */
static bool turn (int angle)
{
    rob_print_long (angle);
    rob_print_from_program_space (PSTR (" TURN."));

    return true;
}

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* The queue that the motion control task uses */
extern xQueueHandle xCommandQueue;

/* The Motion control task */
void vTaskMotion (void *pvParameters)
{
    bool success;
    CodedCommand codedCommand;
    portBASE_TYPE xStatus;

    while (1)
    {
        xStatus = xQueueReceive (xCommandQueue, &codedCommand, portMAX_DELAY);

        ASSERT_STRING (xStatus == pdPASS, "Failed to receive from command queue.");

        success = false; /* Assume failure */

        /* Print out what command we're going to execute */
        rob_lcd_goto_xy (0, 1);
        if (codedCommand.buffer[CODED_COMMAND_INDEX_POS] != CODED_COMMAND_INDEX_UNUSED)
        {
            rob_print_character ('#');
            rob_print_unsigned_long (codedCommand.buffer[CODED_COMMAND_INDEX_POS]);
            rob_print_character (' ');
        }
        rob_print_character (codedCommand.buffer[CODED_COMMAND_ID_POS]);
        rob_print_character (' ');
        rob_print_unsigned_long ((((unsigned int) codedCommand.buffer[CODED_COMMAND_VALUE_POS]) << 8) + codedCommand.buffer[CODED_COMMAND_VALUE_POS + 1]);
        if (codedCommand.buffer[CODED_COMMAND_UNITS_POS] != 0)
        {
            rob_print_character (' ');
            rob_print_character (codedCommand.buffer[CODED_COMMAND_UNITS_POS]);
        }
        rob_print_character (':');
        rob_print_character (' ');

        /* Now do it*/
        switch (codedCommand.buffer[CODED_COMMAND_ID_POS])
        {
            case 'F': /* Forwards */
            case 'B': /* Backwards */
            {
                int value = convertValueToInt (&codedCommand.buffer[CODED_COMMAND_VALUE_POS]);

                if (codedCommand.buffer[CODED_COMMAND_ID_POS] == 'B')
                {
                    value = -value;
                }

                switch (codedCommand.buffer[CODED_COMMAND_UNITS_POS])
                {
                    case 'D': /* Move forward based on distance */
                    {
                        success = setDistance (value);
                    }
                    break;
                    case 'S': /* Move forward based on speed */
                    {
                        success = setSpeed (value);
                    }
                    break;
                    default:
                    {
                        ASSERT_ALWAYS_PARAM (codedCommand.buffer[CODED_COMMAND_UNITS_POS]);
                    }
                    break;
                }
            }
            break;
            case 'R': /* Right */
            case 'L': /* Left */
            {
                int value = convertValueToInt (&codedCommand.buffer[CODED_COMMAND_VALUE_POS]);

                if (codedCommand.buffer[CODED_COMMAND_ID_POS] == 'L')
                {
                    value = -value;
                }

                success = turn (value);
            }
            break;
            case 'S': /* Stop */
            {
                success = stopNow();
            }
            break;
            case 'H': /* Home */
            {
                rob_print_from_program_space (PSTR ("NOT IMPLEMENTED."));
            }
            break;
            case 'M': /* Mark */
            {
                rob_print_from_program_space (PSTR ("NOT IMPLEMENTED."));
            }
            break;
            case 'D': /* Distance? */
            {
                rob_print_from_program_space (PSTR ("NOT IMPLEMENTED."));
            }
            break;
            case 'V': /* Velocity? */
            {
                rob_print_from_program_space (PSTR ("NOT IMPLEMENTED."));
            }
            break;
            case 'P': /* Progress? */
            {
                rob_print_from_program_space (PSTR ("NOT IMPLEMENTED."));
            }
            break;
            case 'I': /* Info? */
            {
                rob_print_from_program_space (PSTR ("NOT IMPLEMENTED."));
            }
            break;
            default:
            {
                ASSERT_ALWAYS_PARAM (codedCommand.buffer[CODED_COMMAND_ID_POS]);
            }
            break;
        }

        if (success)
        {
            sendSerialString (OK_STRING, sizeof (OK_STRING));
        }
        else
        {
            stopNow();
            sendSerialString (ERROR_STRING, sizeof (ERROR_STRING));
        }
    }
}