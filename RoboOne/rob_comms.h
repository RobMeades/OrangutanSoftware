/* Comms - comms part of an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 * Author: Rob Meades
 */

typedef char CommandString;

void vTaskCommsIO (void *pvParameters);

void commsInit (void);

CommandString * receiveSerialCommand (void);

void sendSerialString (char **pSendString);