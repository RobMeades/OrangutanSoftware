/* Comms - comms part of an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 * Author: Rob Meades
 */

#define CODED_COMMAND_SIZE 5 /* The number of characters in a coded-up command */

#define CODED_COMMAND_INDEX_POS     0
#define CODED_COMMAND_ID_POS        CODED_COMMAND_INDEX_POS + 1
#define CODED_COMMAND_VALUE_POS     CODED_COMMAND_ID_POS + 1
#define CODED_COMMAND_VALUE_SIZE    2
#define CODED_COMMAND_UNITS_POS     CODED_COMMAND_VALUE_POS + CODED_COMMAND_VALUE_SIZE

#define CODED_COMMAND_INDEX_UNUSED 255 /* A unique value for a coded command without an index */

/* A buffer containing a single coded command */
typedef struct CodedCommandTag
{
    unsigned char buffer[CODED_COMMAND_SIZE];
} CodedCommand;

void vTaskProcessing (void *pvParameters);

bool processCommand (char * pCommandString, CodedCommand *pCodedCommand);