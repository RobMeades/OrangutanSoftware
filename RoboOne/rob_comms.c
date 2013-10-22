/* Comms - comms part of an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 * Author: Rob Meades
 */

#include <pololu/orangutan.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <rob_system.h>
#include <rob_comms.h>

#define SERIAL_BAUD_RATE 128000
#define NUM_BYTES_FROM_RECEIVE_RING(nEWpOS, oLDpOS) ((nEWpOS) >= (oLDpOS) ?  ((nEWpOS) - (oLDpOS)) : (sizeof (uartReceiveBuffer) - (oLDpOS) + (nEWpOS)))
#define COMMAND_TERMINATOR '\r'

unsigned char uartSendBuffer[32];
         char uartReceiveBuffer[32]; /* not more than 256, must be bigger than the longest command */
unsigned char uartReceiveBufferPos = 0;
unsigned char uartNextCommandStartPos = 0;

typedef enum CommandEncodeStateTag
{
    COMMAND_ENCODE_STATE_NULL = 0,
    COMMAND_ENCODE_STATE_GET_INDEX,
    COMMAND_ENCODE_STATE_GET_ID,
    COMMAND_ENCODE_STATE_GET_VALUE_MANTISSA,
    COMMAND_ENCODE_STATE_GET_VALUE_FRACTIONAL,
    COMMAND_ENCODE_STATE_GET_UNITS,
    COMMAND_ENCODE_STATE_FINISHED
} CommandEncodeState;

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* Initialisation */
void commsInit (void)
{
    serial_set_baud_rate (USB_COMM, SERIAL_BAUD_RATE);

    memset (uartReceiveBuffer, 0, sizeof (uartReceiveBuffer));
    serial_receive_ring (USB_COMM, uartReceiveBuffer, sizeof (uartReceiveBuffer));    
}

/* Look for a command in the receive buffer. */
/* Takes as an argument a pointer to a pointer where the address of a malloc'ed command string will be stored if
 * a command (terminated by COMMAND_TERMINATOR) is found in the buffer.  The pointer should be passed in as PNULL
 * so that it can be tested on return.  uartReceiveBufferPos and, if a command is found, uartNexCommandStartPost
 * are moved on by this function.
 * Return: success unless a command has been found and there has also been a malloc() failure, in which case false.
 */
bool receiveSerialCommand (char **pCommandStringStore)
{
    unsigned char newBufferPos;
    unsigned char numRxBytes;
    unsigned char x;
    bool success = true;

    ASSERT (*pCommandStringStore == PNULL, "Rx command store in use.");

    newBufferPos = serial_get_received_bytes(USB_COMM);
    numRxBytes = NUM_BYTES_FROM_RECEIVE_RING (newBufferPos, uartReceiveBufferPos);

    for (x = 0; success && x < numRxBytes; x++)
    {
        if (uartReceiveBuffer[(uartReceiveBufferPos + x) % sizeof (uartReceiveBuffer)] == COMMAND_TERMINATOR)
        {
            unsigned char commandStringLen;

            commandStringLen = NUM_BYTES_FROM_RECEIVE_RING ((uartReceiveBufferPos + x) % sizeof (uartReceiveBuffer), uartNextCommandStartPos) + 1; /* +1 'cos uartNextCommandStartPos was already at character 1 */
            *pCommandStringStore = malloc (commandStringLen);
            if (*pCommandStringStore)
            {
                unsigned char y;
                unsigned char writePos = 0;
                for (y = 0; y < commandStringLen; y++)
                {
                    if (uartReceiveBuffer[uartNextCommandStartPos] == '\b') /* handle backspace */
                    {
                        if (writePos > 0)
                        {
                            writePos--;
                        }                        
                    }
                    else
                    {
                        *(*pCommandStringStore + writePos) = uartReceiveBuffer[uartNextCommandStartPos];
                        writePos++;
                    }
                    
                    uartNextCommandStartPos++;
                    if (uartNextCommandStartPos >= sizeof (uartReceiveBuffer))
                    {
                        uartNextCommandStartPos = 0;
                    }
                }
                *(*pCommandStringStore + writePos - 1) = 0; /* Make the terminator a null */
            }
            else
            {
                success = false;
            }
        }
    }

    uartReceiveBufferPos = newBufferPos;

    return success;
}

/* Send a string over the USB_COMM port. */
/* Takes as an argument a pointer to a pointer to a malloc'ed buffer containing the null-terminated string
 * to be sent.  Once the string has been sent the buffer will be freed and the pointer set
 * to PNULL the next time this function is called.  If you want the buffer freed sooner
 * just call this function again with PNULL as the argument.
 */
void sendSerialString (char **pSendString)
{
    static char ** pMallocToFree = PNULL; /* Storage for the pointer to a pointer so that we can free it in future */

    /* If we've been called before, make sure any previous sends have completed */
    while (!serial_send_buffer_empty(USB_COMM))
    {
    }

    /* If we've got a stored pointer to a pointer from a previous call... */
    if (pMallocToFree != PNULL)
    {
        free (*pMallocToFree); /* Free the pointed-to pointer */
        *pMallocToFree = PNULL; /* Set the pointed-to pointer to PNULL as well */
        pMallocToFree = PNULL;  /* Then set the local variable to PNULL */
    }

    /* Do the sending thang, if we've been given something to send */
    if (pSendString != PNULL)
    {
        pMallocToFree = pSendString; /* Store the pointer to a pointer passed in into our local static variable */
        serial_send (USB_COMM, *pSendString, strlen (*pSendString));
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
 *
 * Format of coded command is:
 * index (1 byte 0-255),
 * ID (1 byte from FBRLSHMDVP),
 * value (2 bytes) and it is actually converted to cm or cm/s here, with the higher nibbles to the left,
 * units (1 byte from S for speed D for distance) */

/* Return: success if the command was parseable */
bool processCommand (char * pCommandString, CodedCommand *pCodedCommand)
{
    bool success = true;
    unsigned char x;
    unsigned char commandStringSize;
    CommandEncodeState commandEncodeState = COMMAND_ENCODE_STATE_NULL;
    unsigned char mantissa = 0;
    unsigned char fractional = 0;
    unsigned char multiplier = 10;

    commandStringSize = strlen (pCommandString);
    memset (&(pCodedCommand->buffer), 0, sizeof (pCodedCommand->buffer));
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
                if ((commandEncodeState == COMMAND_ENCODE_STATE_NULL || commandEncodeState == COMMAND_ENCODE_STATE_GET_ID) &&
                    (pCodedCommand->buffer[CODED_COMMAND_ID_POS] == 0))
                {
                    pCodedCommand->buffer[CODED_COMMAND_ID_POS] = y & ~ 0x20; /* AND with inverse of 0x20 to convert to upper case */
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
            case 0:
                commandEncodeState = COMMAND_ENCODE_STATE_FINISHED; /* Stop if we hit a null */
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

        /* None of Stop, Halt, Mark, Distance, Velocity or Progress can have values or units */
        if (id == 'S' || id == 'H' || id == 'M' || id == 'D' || id == 'V' || id == 'P')
        {
            if (value > 0 || units > 0)
            {
                success = false;
            }
        }
    }

    return success;
}
