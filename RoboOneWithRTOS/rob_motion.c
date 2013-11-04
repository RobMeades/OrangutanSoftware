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

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* The queue that the motion control task uses */
extern xQueueHandle xCommandQueue;

/* The Motion control task */
void vTaskMotion (void *pvParameters)
{
    CodedCommand codedCommand;
    portBASE_TYPE xStatus;

    while (1)
    {
        xStatus = xQueueReceive (xCommandQueue, &codedCommand, portMAX_DELAY);

        ASSERT_STRING (xStatus == pdPASS, "Failed to receive from command queue.");

        rob_lcd_goto_xy (0, 1);
        rob_print_from_program_space (PSTR("CMD: "));
        if (codedCommand.buffer[CODED_COMMAND_INDEX_POS] != CODED_COMMAND_INDEX_UNUSED)
        {
            rob_print_character ('#');
            rob_print_unsigned_long (codedCommand.buffer[CODED_COMMAND_INDEX_POS]);            
        }
        rob_print_character (' ');
        rob_print_character (codedCommand.buffer[CODED_COMMAND_ID_POS]);
        rob_print_character (' ');
        rob_print_unsigned_long ((((unsigned int) codedCommand.buffer[CODED_COMMAND_VALUE_POS]) << 8) + codedCommand.buffer[CODED_COMMAND_VALUE_POS + 1]);
        if (codedCommand.buffer[CODED_COMMAND_UNITS_POS] != 0)
        {
            rob_print_character (' ');
            rob_print_character (codedCommand.buffer[CODED_COMMAND_UNITS_POS]);
        }
    }
}