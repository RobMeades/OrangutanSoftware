/* Processing - generic processsing part of an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 * Author: Rob Meades
 */

#include <string.h>
#include <rob_system.h>
#include <rob_comms.h>
#include <rob_processing.h>
#include <rob_wrappers.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

typedef enum CommandEncodeStateTag
{
    COMMAND_ENCODE_STATE_NULL = 0,
    COMMAND_ENCODE_STATE_GET_INDEX,
    COMMAND_ENCODE_STATE_GET_ID,
    COMMAND_ENCODE_STATE_GET_VALUE_MANTISSA,
    COMMAND_ENCODE_STATE_GET_VALUE_FRACTIONAL,
    COMMAND_ENCODE_STATE_GET_UNITS,
    COMMAND_ENCODE_STATE_GET_OPENING_QUOTE,
    COMMAND_ENCODE_STATE_GET_STRING,
    COMMAND_ENCODE_STATE_FINISHED
} CommandEncodeState;

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* The queues that the processing task uses */
extern xQueueHandle xCommsReceiveQueue;
extern xQueueHandle xCommandQueue;
extern xQueueHandle xCommsTransmitQueue;

/* The processing task */
void vTaskProcessing (void *pvParameters)
{
    CommandString * pCommandString;
    CodedCommand codedCommand;
    portBASE_TYPE xStatus;
    bool echo = false;

    while (1)
    {
        xStatus = xQueueReceive (xCommsReceiveQueue, &pCommandString, portMAX_DELAY);

        ASSERT_STRING (xStatus == pdPASS, "Failed to receive from comms receive queue.");

        if (!echo)
        {
            if (processCommand (pCommandString, &codedCommand))
            {
                if (codedCommand.buffer[CODED_COMMAND_ID_POS] != 'E' &&
                    codedCommand.buffer[CODED_COMMAND_ID_POS] != 'A' &&
                    codedCommand.buffer[CODED_COMMAND_ID_POS] != 'T' &&
                    codedCommand.buffer[CODED_COMMAND_ID_POS] != '!')
                {
                    /* Send the command off to the command queue */
                    /* For some very weird reason the xQueueSend function here never, ever,
                    ** returns errQUEUE_FULL, even if the queue really is full.  To combat
                    ** this, check if there are already too many messages waiting before
                    ** sending */
                    if (uxQueueMessagesWaiting (xCommandQueue) < COMMAND_QUEUE_SIZE)
                    {
                        xStatus = xQueueSend (xCommandQueue, &codedCommand, 0);
                    }
                    else
                    {
                        xStatus  = errQUEUE_FULL;
                    }

                    if (xStatus != pdPASS)
                    {
                        sendSerialString (BUSY_STRING, sizeof (BUSY_STRING));
                    }
                }
                else
                {
                    /* E, A and T are dealt with locally */
                    if (codedCommand.buffer[CODED_COMMAND_ID_POS] == 'E')
                    {
                        echo = true;
                        sendSerialString (OK_STRING, sizeof (OK_STRING));
                    }
                    else if (codedCommand.buffer[CODED_COMMAND_ID_POS] == '!')
                    {
                        sendSerialString (OK_STRING, sizeof (OK_STRING));
                    }
                    else if (codedCommand.buffer[CODED_COMMAND_ID_POS] == 'A')
                    {
                        rob_lcd_goto_xy (0, 1);
                        rob_print ((const char *) &(pCommandString[codedCommand.buffer[CODED_COMMAND_VALUE_POS]]));
                        sendSerialString (OK_STRING, sizeof (OK_STRING));
                    }
                    else if (codedCommand.buffer[CODED_COMMAND_ID_POS] == 'T')
                    {
                        rob_wait_play ((const char *) &(pCommandString[codedCommand.buffer[CODED_COMMAND_VALUE_POS]]));
                        sendSerialString (OK_STRING, sizeof (OK_STRING));
                    }
                    else
                    {
                        ASSERT_ALWAYS_STRING ("Only IDs E, A and T are handled locally.");
                    }
                }
            }
            else
            {
                rob_lcd_goto_xy (0, 1);
                rob_print_from_program_space (PSTR ("Bad command"));
                sendSerialString (ERROR_STRING, sizeof (ERROR_STRING));
            }

            RobFree (pCommandString); /* Free up the memory that held the received command string */
        }
        else
        {
            xQueueSend (xCommsTransmitQueue, &pCommandString, 0); /* Forward to the transmit queue */
        }
    }
}

/* Process a null-terminated command string into a coded string */
/* Command strings:
 *
 * [#x] F[orwards] x.xx[[ ]m]/[[ ]m/s]
 * [#x] B[ackwards] x.xx[[ ]m]/[[ ]m/s]
 * [#x] R[ight] [xx]
 * [#x] L[eft] [xx]
 * [#x] S[top]
 * [#x] H[ome]
 * [#x] M[ark]
 * [#x] D[istance?]
 * [#x] V[elocity?]
 * [#x] P[rogress?]
 * [#x] I[nfo?]
 * [#x] E[cho]
 * [#x] A"[]"
 * [#x] T"[]"
 * [#x] !
 *
 * Format of coded command is:
 * - index (1 byte 0-255),
 * - ID (1 byte from FBRLSHMDVPIA),
 * - value (2 bytes) and it is actually converted to cm or cm/s here, with the higher nibbles to the left,
 *   in the case of A/T this is the position of the start of the printable string,
 * - units (1 byte from S for speed D for distance), in the case of A/T this contains the string length. */

/* Return: success if the command was parseable */
bool processCommand (char * pCommandString, CodedCommand *pCodedCommand)
{
    bool success = true;
    unsigned char x;
    unsigned char commandStringSize;
    CommandEncodeState commandEncodeState = COMMAND_ENCODE_STATE_NULL;
    unsigned char posOpeningQuote = 0;
    unsigned char posClosingQuote = 0;
    unsigned char mantissa = 0;
    unsigned char fractional = 0;
    unsigned char multiplier = 10;

    commandStringSize = RobStrlen (pCommandString);
    RobMemset (&(pCodedCommand->buffer), 0, sizeof (pCodedCommand->buffer));
    pCodedCommand->buffer[CODED_COMMAND_INDEX_POS] = CODED_COMMAND_INDEX_UNUSED;

    for (x = 0; success && x < commandStringSize; x++)
    {
        unsigned char y;

        y = pCommandString[x];
        switch (y)
        {
            case '#':
                if (commandEncodeState == COMMAND_ENCODE_STATE_NULL)
                {
                    commandEncodeState = COMMAND_ENCODE_STATE_GET_INDEX;
                }
                else if (commandEncodeState == COMMAND_ENCODE_STATE_GET_STRING)
                {
                    /* Do nothing */
                }
                else
                {
                    success = false; /* # is not a valid character elsewhere */
                }
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (commandEncodeState == COMMAND_ENCODE_STATE_GET_INDEX || commandEncodeState == COMMAND_ENCODE_STATE_GET_VALUE_MANTISSA)
                {
                    mantissa = 10 * mantissa + (y - '0');
                    if (mantissa >= 255)
                    {
                        success = false; /* too big */
                    }
                }
                else if (commandEncodeState == COMMAND_ENCODE_STATE_GET_VALUE_FRACTIONAL)
                {
                    fractional = multiplier * (y - '0') + fractional;
                    multiplier = multiplier / 10;
                    if (fractional > 99)
                    {
                        success = false; /* too big */
                    }
                }
                else if (commandEncodeState == COMMAND_ENCODE_STATE_GET_STRING)
                {
                    /* Do nothing */
                }
                else
                {
                    success = false; /* numbers are not valid characters elsewhere */
                }
                break;
            case '.':
                if (commandEncodeState == COMMAND_ENCODE_STATE_GET_VALUE_MANTISSA)
                {
                    commandEncodeState = COMMAND_ENCODE_STATE_GET_VALUE_FRACTIONAL;
                }
                else if (commandEncodeState == COMMAND_ENCODE_STATE_GET_STRING)
                {
                    /* Do nothing */
                }
                else
                {
                    success = false; /* full stops are not valid characters elsewhere */
                }
                break;
            case 'f':
            case 'F':
            case 'b':
            case 'B':
            case 'r':
            case 'R':
            case 'l':
            case 'L':
                if ((commandEncodeState == COMMAND_ENCODE_STATE_NULL || commandEncodeState == COMMAND_ENCODE_STATE_GET_ID) &&
                    (pCodedCommand->buffer[CODED_COMMAND_ID_POS] == 0))
                {
                    pCodedCommand->buffer[CODED_COMMAND_ID_POS] = y & ~ 0x20; /* AND with inverse of 0x20 to convert to upper case */
                    commandEncodeState = COMMAND_ENCODE_STATE_GET_ID; /* Ensure that we are at "Get Id" here as otherwise it's confusing */
                }
                break;
            case 'a':
            case 'A':
            case 't':
            case 'T':
                if ((commandEncodeState == COMMAND_ENCODE_STATE_NULL || commandEncodeState == COMMAND_ENCODE_STATE_GET_ID) &&
                    (pCodedCommand->buffer[CODED_COMMAND_ID_POS] == 0))
                {
                    pCodedCommand->buffer[CODED_COMMAND_ID_POS] = y & ~ 0x20; /* AND with inverse of 0x20 to convert to upper case */
                    commandEncodeState = COMMAND_ENCODE_STATE_GET_OPENING_QUOTE;
                }
                break;
            case 's':
            case 'S':
                if (commandEncodeState == COMMAND_ENCODE_STATE_GET_UNITS)
                {
                    pCodedCommand->buffer[CODED_COMMAND_UNITS_POS] = UNITS_ARE_SPEED; /* If there's an 'S' then this is m/s so units are distance */
                }
                /* Deliberate fall-through - S is also a valid ID */
            case 'm':
            case 'M':
                if (commandEncodeState == COMMAND_ENCODE_STATE_GET_UNITS && pCodedCommand->buffer[CODED_COMMAND_UNITS_POS] != UNITS_ARE_SPEED) /* Prevent the fall-through overwriting the 'D' */
                {
                    pCodedCommand->buffer[CODED_COMMAND_UNITS_POS] = UNITS_ARE_DISTANCE; /* If there's an 'M' then this is metres unless it later becomes m/s */
                }
                /* Deliberate fall-through - M is also a valid ID */
            case 'h':
            case 'H':
            case 'd':
            case 'D':
            case 'v':
            case 'V':
            case 'p':
            case 'P':
            case 'I':
            case 'i':
            case 'E':
            case 'e':
                if ((commandEncodeState == COMMAND_ENCODE_STATE_NULL || commandEncodeState == COMMAND_ENCODE_STATE_GET_ID) &&
                    (pCodedCommand->buffer[CODED_COMMAND_ID_POS] == 0))
                {
                    pCodedCommand->buffer[CODED_COMMAND_ID_POS] = y & ~ 0x20; /* AND with inverse of 0x20 to convert to upper case */
                    commandEncodeState = COMMAND_ENCODE_STATE_FINISHED;
                }
                break;
            case '!':
                if ((commandEncodeState == COMMAND_ENCODE_STATE_NULL || commandEncodeState == COMMAND_ENCODE_STATE_GET_ID) &&
                    (pCodedCommand->buffer[CODED_COMMAND_ID_POS] == 0))
                {
                    pCodedCommand->buffer[CODED_COMMAND_ID_POS] = y;
                    commandEncodeState = COMMAND_ENCODE_STATE_FINISHED;
                }
                break;
            case ' ': /* space just moves the state on */
                if (commandEncodeState == COMMAND_ENCODE_STATE_GET_INDEX)
                {
                    if (mantissa > 99)
                    {
                        success = false;
                    }
                    else
                    {
                        pCodedCommand->buffer[CODED_COMMAND_INDEX_POS] = mantissa;
                    }
                    mantissa = 0;
                    commandEncodeState = COMMAND_ENCODE_STATE_GET_ID;
                }
                else if (commandEncodeState == COMMAND_ENCODE_STATE_GET_ID)
                {
                    commandEncodeState = COMMAND_ENCODE_STATE_GET_VALUE_MANTISSA;
                }
                else if (commandEncodeState == COMMAND_ENCODE_STATE_GET_VALUE_MANTISSA || commandEncodeState == COMMAND_ENCODE_STATE_GET_VALUE_FRACTIONAL)
                {
                    commandEncodeState = COMMAND_ENCODE_STATE_GET_UNITS;
                }
                else
                {
                    /* do nothing */
                }
                break;
            case '"':
                if (commandEncodeState == COMMAND_ENCODE_STATE_GET_OPENING_QUOTE)
                {
                    commandEncodeState = COMMAND_ENCODE_STATE_GET_STRING;
                    posOpeningQuote = x;
                }
                else
                {
                    if (commandEncodeState == COMMAND_ENCODE_STATE_GET_STRING)
                    {
                        commandEncodeState = COMMAND_ENCODE_STATE_FINISHED;
                        posClosingQuote = x;
                        pCommandString[x] = 0; /* Overwrite it with null so that we have a printable string */
                    }
                    else
                    {
                        success = false; /* " is not a valid character elsewhere */
                    }
                }
                break;
            case 0:
                commandEncodeState = COMMAND_ENCODE_STATE_FINISHED; /* Stop if we hit a null, or an LF */
                break;
            default:
                /* Do nothing - all other characters can just be discarded */
                break;
        }
    }

    /* Have to have said _something_ */
    if (pCodedCommand->buffer[CODED_COMMAND_ID_POS] == 0)
    {
        success = false;
    }

    /* Now figure out if what we've got makes sense and sort out the value */
    if (success)
    {
        unsigned char id = pCodedCommand->buffer[CODED_COMMAND_ID_POS];
        unsigned char units = pCodedCommand->buffer[CODED_COMMAND_UNITS_POS];
        unsigned int value;

        value = (unsigned int) mantissa * 100 + fractional;

        if (id == 'R' || id == 'L')
        {
            value /= 100; /* values in degrees must have the * 100 multiplier (that was converting metres into centimetres) removed */
            if (value > MAX_TURN_DEGREES)
            {
                success = false;
            }
            else
            {
                /* Default turn is DEFAULT_TURN_DEGREES if not specified */
                if (value == 0)
                {
                    value = DEFAULT_TURN_DEGREES;
                }
            }

            pCodedCommand->buffer[CODED_COMMAND_VALUE_POS + 1] = (unsigned char) value;
        }

        /* Forwards or backwards have to have units */
        if (id == 'F' || id == 'B')
        {
            if (units == 0)
            {
                success = false;
            }
            else
            {
                if (units == UNITS_ARE_DISTANCE)
                {
                    if (value > MAX_DISTANCE_CM)
                    {
                        success = false;
                    }
                }
                else
                {
                    if (value > MAX_SPEED_CM_SEC)
                    {
                        success = false;
                    }
                }

                if (success)
                {
                    /* Write the value in a specific way so that we can remove it in the same order */
                    pCodedCommand->buffer[CODED_COMMAND_VALUE_POS]     = (unsigned char) (value >> 8);
                    pCodedCommand->buffer[CODED_COMMAND_VALUE_POS + 1] = (unsigned char) value;
                }
            }
        }

        if (id == 'A' || id == 'T')
        {
            /* For an Alphanumeric/Tune string, value contains the position of the start of the body of the string and units the length of it */
            pCodedCommand->buffer[CODED_COMMAND_VALUE_POS] = 0;
            pCodedCommand->buffer[CODED_COMMAND_VALUE_POS + 1] = 0;
            pCodedCommand->buffer[CODED_COMMAND_UNITS_POS] = 0;

            if (posOpeningQuote > 0 && posClosingQuote > posOpeningQuote + 1) /* +1 to ensure there are some contents */
            {
                pCodedCommand->buffer[CODED_COMMAND_VALUE_POS] = posOpeningQuote + 1;
                pCodedCommand->buffer[CODED_COMMAND_UNITS_POS] = posClosingQuote - posOpeningQuote - 1;
            }
            else
            {
                success = false;
            }
        }

        /* None of Stop, Halt, Mark, Distance, Velocity, Progress, Info or "!" can have values or units */
        if (id == 'S' || id == 'H' || id == 'M' || id == 'D' || id == 'V' || id == 'P' || id == 'I' || id == '!')
        {
            if (value > 0 || units > 0)
            {
                success = false;
            }
        }
    }

    return success;
}
