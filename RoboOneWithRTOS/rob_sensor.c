/* Sensor - the sensor reading part of an application for the Pololu Orangutan X2
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

/* The number of ADC samples to average over */
#define NUM_ADC_SAMPLES 10

/* The number of ADCS to read when the read command comes in */
#define MAX_NUM_ADCS 6

/* The string length of one reading from an ADC (NOT including terminator), intended as "xx:yyyy ",
 * where xx is the sensor string and yyyy is the distance in cm. */
#define ADC_READING_STRING_LEN 7

/* The string length of the sensor string. */
#define SENSOR_STRING_LEN 2

/* The "nothing there" string */
#define NOTHING_THERE_STRING ":    "
#define NOTHING_THERE_STRING_LEN 5 /* Not including terminator */

/* - GLOBALS -------------------------------------------------------------------------- */

/* - STATIC VARIABLES ----------------------------------------------------------------- */

/* Map the character string to a given sensor.
 * They are arranged as follows around the robot:
 *               FF (1)
 *              ________
 *      FL (2) |        | FR (4)
 *             |        |
 *             |        |
 *             |        |
 *      BL (3) |________| BR (5)
 *               BB (6)
 *
 * Where FF is Front, FL Front Left, FR Front Right,
 * BL Back Left, BR Back Right and BB is back.
 */

static const char * channelToString[] = {"FF", "FL", "BL", "FR", "BR", "BB"};

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* Calibrate the ADCs, used in case the supply voltage has changed, which it might
   do quite frequently as we're on battery */
static void calibrateAdcs (void)
{
    set_millivolt_calibration (read_vcc_millivolts());
}

/* Read a given ADC */
static unsigned int readAdc (unsigned char channel)
{
    return analog_read_average_millivolts (channel, NUM_ADC_SAMPLES);
}

/* Determine if an object has been detected.  10 mV
 * is the minimum reading, 300 mV equates to a distance of
 * 40 cm, 256 millivolts is 50 cm, so use that. */
static bool objectDetected (unsigned int millivolts)
{
    bool detected = false;
    
    if (millivolts > 256)
    {
        detected = true;
    }
    
    return detected;
}

/* Convert an ADC reading into a distance in cm.
 * From the Sharp GP2Y0A41SK0F data sheet, the sensors can 
 * measure from 40 cm to 3.5 cm with a linear response, which
 * I calculate to be V = 12.8 * (1 / (distance in cm)).  This works
 * for voltages up to 3 volts (equating to a distance of 3.5 cm).
 * Below about 0.3 volts there is nothing there. */
static unsigned int voltageToDistance (unsigned int millivolts)
{
    unsigned int distance = 1280; /* Equivalent to 10 mV, which is about the minimum reading */
    
    if (millivolts > 10)
    {        
        distance = 12800 / (millivolts);
    }
    
    return distance;
}

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* The queue that the sensor reading task uses */
extern xQueueHandle xSensorCommandQueue;

/* The Sensor reading task */
void vTaskSensor (void *pvParameters)
{
    bool success;
    unsigned char x;
    char sendString [ADC_READING_STRING_LEN * MAX_NUM_ADCS + 1]; /* +1 for terminator */
    CodedCommand codedSensorCommand;
    portBASE_TYPE xStatus;

    while (1)
    {
        xStatus = xQueueReceive (xSensorCommandQueue, &codedSensorCommand, portMAX_DELAY);

        ASSERT_STRING (xStatus == pdPASS, "Failed to receive from sensor command queue.");

        success = false; /* Assume failure */

        /* Print out what command we're going to execute */
        rob_lcd_goto_xy (0, 1);
        rob_print_from_program_space (PSTR ("CMD: "));
        if (codedSensorCommand.buffer[CODED_COMMAND_INDEX_POS] != CODED_COMMAND_INDEX_UNUSED)
        {
            rob_print_character ('#');
            rob_print_unsigned_long (codedSensorCommand.buffer[CODED_COMMAND_INDEX_POS]);
            rob_print_character (' ');
        }
        rob_print_character (codedSensorCommand.buffer[CODED_COMMAND_ID_POS]);

        rob_lcd_goto_xy (0, 2);
		
        /* Now do it */
        switch (codedSensorCommand.buffer[CODED_COMMAND_ID_POS])
        {
            case '*': /* The only sensor command supported at the moment */
            {
                RobMemset (sendString, ' ', sizeof (sendString));
                calibrateAdcs();
                for (x = 0; x < MAX_NUM_ADCS; x++)
                {
                    unsigned int milliVolts;
                    
                    /* Write something like "FL:40  " (for ADC 2 of 6 (which is Front Left), object detected at 40 cm),
                       or "FL:    " if nothing is there. */
                    memcpy (&sendString[ADC_READING_STRING_LEN * x], channelToString[x], SENSOR_STRING_LEN);
                    memcpy (&sendString[(ADC_READING_STRING_LEN * x) + SENSOR_STRING_LEN], NOTHING_THERE_STRING, NOTHING_THERE_STRING_LEN);
                    
                    milliVolts = readAdc (x);
                    if (objectDetected (milliVolts))
                    {
                        itoa (voltageToDistance (milliVolts), &(sendString[(ADC_READING_STRING_LEN * x) + SENSOR_STRING_LEN + 1]), 10); /* +1 to leave the ':' there */
                    }                    
                    
                    /* Overwrite the null terminator that itoa() puts in with a space */
                    sendString[RobStrlen (sendString)] = ' ';
                }
                sendString[sizeof (sendString) - 1] = 0; /* Add terminator */
                sendSerialString (sendString, sizeof (sendString));
                rob_print (sendString);
                success = true;
            }
            break;
            default:
            {
                ASSERT_ALWAYS_PARAM (codedSensorCommand.buffer[CODED_COMMAND_ID_POS]);
            }
            break;
        }

        if (!success)
        {
            sendSerialString (ERROR_STRING, sizeof (ERROR_STRING));
        }
    }
}