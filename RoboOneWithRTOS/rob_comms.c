/* Comms - comms part of an application for the Pololu Orangutan X2
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
#include <rob_comms.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#define SERIAL_BAUD_RATE 9600
#define NUM_BYTES_FROM_RECEIVE_RING(nEWpOS, oLDpOS) ((nEWpOS) >= (oLDpOS) ?  ((nEWpOS) - (oLDpOS)) : (sizeof (uartReceiveBuffer) - (oLDpOS) + (nEWpOS)))
#define COMMAND_TERMINATOR '\r'

unsigned char uartSendBuffer[128];
         char uartReceiveBuffer[128]; /* not more than 256, must be bigger than the longest command */
unsigned char uartReceiveBufferPos = 0;
unsigned char uartNextCommandStartPos = 0;

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* The queue that the comms receive task uses */
extern xQueueHandle xCommsReceiveQueue;

/* The Comms receive task */
void vTaskCommsReceive (void *pvParameters)
{
    CommandString * pCommandString;
    portBASE_TYPE xStatus;

    while (1)
    {
        /* Check for things going on on the USB interface (no interrupts there) */
        rob_serial_check();

        /* Look for a command and add it to the queue if there is one */
        pCommandString = receiveSerialCommand();

        if (pCommandString != PNULL)
        {
            rob_clear();
            rob_print_from_program_space (PSTR("Received: "));
            rob_print (pCommandString);
            //RobPrintf ("Received: %s\n", pCommandString);
            xStatus = xQueueSend (xCommsReceiveQueue, &pCommandString, 0);
            ASSERT_STRING (xStatus == pdPASS, "Failed to send to receive queue.");
        }

        /* Let other tasks run */
        vTaskDelay (10 / portTICK_RATE_MS);
    }
}

/* The queue that the comms transmit task uses */
extern xQueueHandle xCommsTransmitQueue;

/* The Comms transmit task */
void vTaskCommsTransmit (void *pvParameters)
{
    char * pSendString;
    portBASE_TYPE xStatus;

    while (1)
    {
        xStatus = xQueueReceive (xCommsTransmitQueue, &pSendString, portMAX_DELAY);

        ASSERT_STRING (xStatus == pdPASS, "Failed to receive from serial transmit queue.");
        
        /* Do the sending thang, if we've been given something to send */
        if (pSendString != PNULL)
        {
            unsigned char bytesToSend;

            bytesToSend = RobStrlen (pSendString) + 1; /* +1 because strlen() doesn't include the terminator and we need to send it */
            *(pSendString + bytesToSend - 1) = '\r'; /* Replace the null terminator with a terminator that makes sense to a PC comms handler*/
            rob_serial_send_blocking_usb_comm (pSendString, bytesToSend); // TODO: REALLY shouldn't block here

            /* Free the memory */
            RobFree (pSendString);
        }        
    }    
}

/* Initialisation */
void commsInit (void)
{
    rob_serial_set_baud_rate_usb_comm (SERIAL_BAUD_RATE);
    rob_serial_receive_ring_usb_comm (uartReceiveBuffer, sizeof(uartReceiveBuffer));
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

    newBufferPos = rob_serial_get_received_bytes_usb_comm ();
    numRxBytes = NUM_BYTES_FROM_RECEIVE_RING (newBufferPos, uartReceiveBufferPos);

    for (x = 0; success && x < numRxBytes; x++)
    {
        if (uartReceiveBuffer[(uartReceiveBufferPos + x) % sizeof (uartReceiveBuffer)] == COMMAND_TERMINATOR)
        {
            unsigned char commandStringLen;

            commandStringLen = NUM_BYTES_FROM_RECEIVE_RING ((uartReceiveBufferPos + x) % sizeof (uartReceiveBuffer), uartNextCommandStartPos) + 1; /* +1 'cos uartNextCommandStartPos was already at character 1 */
            pCommandString = RobMalloc (commandStringLen);
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