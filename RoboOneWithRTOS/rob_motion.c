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
#include <rob_motion.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <pololu/orangutan.h>

#define MAX_SPEED_O_UNITS 255
#define METRES_TO_TIME_FACTOR 7
#define CM_S_TO_O_UNITS_FACTOR 3
#define MOTOR1_TO_MOTOR2_OFFSET 5 /* Positive if MOTOR2 (right) is weaker than MOTOR1 (left) */

/* - GLOBALS -------------------------------------------------------------------------- */

static int gSetSpeedOUnits = MINIMUM_USEFUL_SPEED_O_UNITS;

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* Convert the two byte value field into an int */
static int convertValueToInt (unsigned char *pValueArray)
{
    int value;

    value = (((unsigned int) *pValueArray) << 8) + *(pValueArray + 1);

    return value;
}

/* Offset the motor speed of motor1 to balance things out */
static int motor1Speed (int speed)
{
    if (speed >= 0)
    {
        if (speed <= MAX_SPEED_O_UNITS - MOTOR1_TO_MOTOR2_OFFSET)
        {
            speed += MOTOR1_TO_MOTOR2_OFFSET;
        }
    }
    else
    {
        if (speed >= -MAX_SPEED_O_UNITS + MOTOR1_TO_MOTOR2_OFFSET)
        {
            speed -= MOTOR1_TO_MOTOR2_OFFSET;
        }
    }

    return speed;
}

/* Offset the motor speed of motor2 to balance things out */
static int motor2Speed (int speed)
{
    if (speed >= 0)
    {
        if (speed > MAX_SPEED_O_UNITS - MOTOR1_TO_MOTOR2_OFFSET)
        {
            speed -= MOTOR1_TO_MOTOR2_OFFSET;
        }
    }
    else
    {
        if (speed < -MAX_SPEED_O_UNITS + MOTOR1_TO_MOTOR2_OFFSET)
        {
            speed += MOTOR1_TO_MOTOR2_OFFSET;
        }
    }

    return speed;
}

/* Set speed (forward or backward) */
static bool setSpeed (unsigned int speedCmS)
{
    int speedOUnits;

    speedOUnits = speedCmS * CM_S_TO_O_UNITS_FACTOR;

    if (speedOUnits > MAX_SPEED_O_UNITS)
    {
        speedOUnits = MAX_SPEED_O_UNITS;
    }
    else
    {
        if (speedOUnits < -MAX_SPEED_O_UNITS)
        {
            speedOUnits = -MAX_SPEED_O_UNITS;
        }
    }

    gSetSpeedOUnits = speedOUnits;

    rob_print_from_program_space (PSTR ("Set "));
    rob_print_unsigned_long (speedOUnits / CM_S_TO_O_UNITS_FACTOR);
    rob_print_from_program_space (PSTR (" cm/s ("));
    rob_print_unsigned_long (speedOUnits);
    rob_print_from_program_space (PSTR (")"));

    return true;
}

/* Move forwards or backwards a given distance */
static bool moveDistance (unsigned int distanceM, bool isForwards)
{
    int speedOUnits = gSetSpeedOUnits;
    int time10ms;

    time10ms = (distanceM / speedOUnits) * 100 * METRES_TO_TIME_FACTOR;

    if (!isForwards)
    {
        speedOUnits = -speedOUnits;
    }

    rob_print_from_program_space (PSTR ("~"));
    rob_print_long (time10ms / 100);
    rob_print_from_program_space (PSTR (" s @"));
    rob_print_long (speedOUnits / CM_S_TO_O_UNITS_FACTOR);
    rob_print_from_program_space (PSTR (" cm/s"));
    rob_print_from_program_space (PSTR (" ("));
    rob_print_long (speedOUnits);
    rob_print_from_program_space (PSTR (" )"));

    /* TODO fix this (1's and 2's swapped) */
    x2_set_motor (MOTOR1, ACCEL_DRIVE, motor2Speed (speedOUnits));
    x2_set_motor (MOTOR2, ACCEL_DRIVE, motor1Speed (speedOUnits));

    vTaskDelay (time10ms / portTICK_RATE_MS);

    stopNow();

    return true;
}

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* Move forwards or backwards until told to stop, tweaking left
 * or right if required. */
bool move (int speedOUnits, int tweakLeft, int tweakRight)
{
    rob_print_long (speedOUnits / CM_S_TO_O_UNITS_FACTOR);
    rob_print_from_program_space (PSTR (" cm/s"));
    rob_print_from_program_space (PSTR (" ("));
    rob_print_long (speedOUnits);
    rob_print_from_program_space (PSTR (" )"));
    rob_print_long (tweakLeft);
    rob_print_from_program_space (PSTR ("L "));
    rob_print_long (tweakRight);
    rob_print_from_program_space (PSTR ("R"));

    x2_set_motor (MOTOR1, ACCEL_DRIVE, motor2Speed (speedOUnits + tweakLeft));
    x2_set_motor (MOTOR2, ACCEL_DRIVE, motor1Speed (speedOUnits + tweakRight));

    return true;
}

/* Stop dead */
bool stopNow (void)
{
    rob_print_from_program_space (PSTR ("STOP."));
    x2_set_motor (MOTOR1, BRAKE_LOW, 0);
    x2_set_motor (MOTOR2, BRAKE_LOW, 0);

    return true;
}

/* Turn (left or right) */
bool turn (int degrees)
{
    rob_print_long (degrees);
    rob_print_from_program_space (PSTR (" TURN."));

    return true;
}

/* The queue that the motion control task uses */
extern xQueueHandle xMotionCommandQueue;

/* The Motion control task */
void vTaskMotion (void *pvParameters)
{
    bool success;
    bool isForwards;
    CodedCommand codedMotionCommand;
    portBASE_TYPE xStatus;

    while (1)
    {
        xStatus = xQueueReceive (xMotionCommandQueue, &codedMotionCommand, portMAX_DELAY);

        ASSERT_STRING (xStatus == pdPASS, "Failed to receive from motion command queue.");

        success = false; /* Assume failure */

        /* Print out what command we're going to execute */
        rob_lcd_goto_xy (0, 1);
        rob_print_from_program_space (PSTR ("CMD: "));
        if (codedMotionCommand.buffer[CODED_COMMAND_INDEX_POS] != CODED_COMMAND_INDEX_UNUSED)
        {
            rob_print_character ('#');
            rob_print_unsigned_long (codedMotionCommand.buffer[CODED_COMMAND_INDEX_POS]);
            rob_print_character (' ');
        }
        rob_print_character (codedMotionCommand.buffer[CODED_COMMAND_ID_POS]);
        rob_print_character (' ');
        rob_print_unsigned_long ((((unsigned int) codedMotionCommand.buffer[CODED_COMMAND_VALUE_POS]) << 8) + codedMotionCommand.buffer[CODED_COMMAND_VALUE_POS + 1]);
        if (codedMotionCommand.buffer[CODED_COMMAND_UNITS_POS] != 0)
        {
            rob_print_character (' ');
            rob_print_character (codedMotionCommand.buffer[CODED_COMMAND_UNITS_POS]);
        }

        rob_lcd_goto_xy (0, 2);
		
        /* Now do it */
        switch (codedMotionCommand.buffer[CODED_COMMAND_ID_POS])
        {
            case 'F': /* Forwards */
            case 'B': /* Backwards */
            {
                int value = convertValueToInt (&codedMotionCommand.buffer[CODED_COMMAND_VALUE_POS]);

                isForwards = true;
                if (codedMotionCommand.buffer[CODED_COMMAND_ID_POS] == 'B')
                {
                    isForwards = false;
                }

                switch (codedMotionCommand.buffer[CODED_COMMAND_UNITS_POS])
                {
                    case 'D': /* Move forward based on distance */
                    {
                        success = moveDistance (value, isForwards);
                    }
                    break;
                    case 'S': /* Set the speed */
                    {
                        success = setSpeed (value);
                    }
                    break;
                    default:
                    {
                        ASSERT_ALWAYS_PARAM (codedMotionCommand.buffer[CODED_COMMAND_UNITS_POS]);
                    }
                    break;
                }
            }
            break;
            case 'R': /* Right */
            case 'L': /* Left */
            {
                int value = convertValueToInt (&codedMotionCommand.buffer[CODED_COMMAND_VALUE_POS]);

                if (codedMotionCommand.buffer[CODED_COMMAND_ID_POS] == 'L')
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
                rob_print_from_program_space (PSTR ("TODO."));
            }
            break;
            case 'I': /* Info? */
            {
                rob_print_from_program_space (PSTR ("TODO."));
            }
            break;
            default:
            {
                ASSERT_ALWAYS_PARAM (codedMotionCommand.buffer[CODED_COMMAND_ID_POS]);
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