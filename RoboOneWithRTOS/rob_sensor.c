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

/* The string length of one reading from an ADC (NOT including terminator), intended as "x:yyyy ",
 * where x is the channel number (single ASCII digit) and yyyy is the reading in millivolts. */
#define ADC_READING_STRING_LEN 7

/* - GLOBALS -------------------------------------------------------------------------- */

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
                    /* Write something like "2:4998 " (for ADC 2 of 6, voltage reading 4998 mV)*/
                    itoa (x + 1, &(sendString[ADC_READING_STRING_LEN * x]), 10);
                    sendString[(ADC_READING_STRING_LEN * x) + 1] = ':';
                    itoa (readAdc (x), &(sendString[(ADC_READING_STRING_LEN * x) + 2]), 10);
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