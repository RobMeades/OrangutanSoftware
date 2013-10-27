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

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#define SERIAL_BAUD_RATE 28800
#define NUM_BYTES_FROM_RECEIVE_RING(nEWpOS, oLDpOS) ((nEWpOS) >= (oLDpOS) ?  ((nEWpOS) - (oLDpOS)) : (sizeof (uartReceiveBuffer) - (oLDpOS) + (nEWpOS)))
#define COMMAND_TERMINATOR '\r'

unsigned char uartSendBuffer[128];
         char uartReceiveBuffer[128]; /* not more than 256, must be bigger than the longest command */
unsigned char uartReceiveBufferPos = 0;
unsigned char uartNextCommandStartPos = 0;

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* The queue that the comms task uses */
extern xQueueHandle xCommsReceiveQueue;

/* The Comms IO task */
void vTaskCommsIO (void *pvParameters)
{
    CommandString * pCommandString;
    portBASE_TYPE xStatus;

    while (1)
    {
endStuff();
while (1);
        /* Check for things going on on the USB interface (no interrupts there) */
        serial_check();

        /* Look for a command and add it to the queue if there is one */
        pCommandString = receiveSerialCommand();

        if (pCommandString != PNULL)
        {
            printf ("Received: %s\n", pCommandString);
            xStatus = xQueueSend (xCommsReceiveQueue, &pCommandString, 0);
            ASSERT_STRING (xStatus != pdPASS, "Failed to send to comms queue.");
        }

        /* Let other tasks run */
        taskYIELD();
    }
}

/* Initialisation */
void commsInit (void)
{
    serial_set_baud_rate (USB_COMM, SERIAL_BAUD_RATE);

    memset (uartReceiveBuffer, 0, sizeof (uartReceiveBuffer));
    serial_receive_ring (USB_COMM, uartReceiveBuffer, sizeof (uartReceiveBuffer));
}

/* Look for a command in the receive buffer. */
/* Returns a pointer to a malloc'ed command string if a command (terminated by COMMAND_TERMINATOR)
 * is found in the buffer, otherwise PNULL.  uartReceiveBufferPos and, if a command is found,
 * uartNexCommandStartPost are moved on by this function.
 */
CommandString * receiveSerialCommand (void)
{
    unsigned char newBufferPos;
    unsigned char numRxBytes;
    unsigned char x;
    CommandString * pCommandString = PNULL;
    bool success = true;

    newBufferPos = serial_get_received_bytes (USB_COMM);
    numRxBytes = NUM_BYTES_FROM_RECEIVE_RING (newBufferPos, uartReceiveBufferPos);

    for (x = 0; success && x < numRxBytes; x++)
    {
        if (uartReceiveBuffer[(uartReceiveBufferPos + x) % sizeof (uartReceiveBuffer)] == COMMAND_TERMINATOR)
        {
            unsigned char commandStringLen;

            commandStringLen = NUM_BYTES_FROM_RECEIVE_RING ((uartReceiveBufferPos + x) % sizeof (uartReceiveBuffer), uartNextCommandStartPos) + 1; /* +1 'cos uartNextCommandStartPos was already at character 1 */
            pCommandString = malloc (commandStringLen);
            ASSERT_PARAM (pCommandString, commandStringLen);
            if (pCommandString)
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
                        *(pCommandString + writePos) = uartReceiveBuffer[uartNextCommandStartPos];
                        writePos++;
                    }

                    uartNextCommandStartPos++;
                    if (uartNextCommandStartPos >= sizeof (uartReceiveBuffer))
                    {
                        uartNextCommandStartPos = 0;
                    }
                }
                *(pCommandString + writePos - 1) = 0; /* Make the terminator a null */
            }
            else
            {
                success = false;
            }
        }
    }

    uartReceiveBufferPos = newBufferPos;

    return pCommandString;
}

/* Send a string over the USB_COMM port. */
/* Takes as an argument a pointer to a pointer to a malloc'ed buffer containing the null-terminated string
 * to be sent ('\r' added).  Once the string has been sent the buffer will be freed and the pointer set
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
        unsigned char bytesToSend;

        bytesToSend = strlen (*pSendString) + 1; /* +1 becasue strlen() doesn't include the terminator and we need to send it */
        pMallocToFree = pSendString; /* Store the pointer to a pointer passed in into our local static variable */
        (*pSendString)[bytesToSend - 1] = '\r'; /* Replace the null terminator with a terminator that makes sense to a PC comms handler*/
        serial_send (USB_COMM, *pSendString, bytesToSend);
    }
}