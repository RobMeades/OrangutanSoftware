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