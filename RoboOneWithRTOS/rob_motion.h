/* Motion - motion control part of an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 * Author: Rob Meades
 */

void vTaskMotion (void *pvParameters);

/* Slower than this and it won't go */
#define MINIMUM_USEFUL_SPEED_O_UNITS 60

bool move (int speedOUnits, int tweakLeft, int tweakRight);
bool stopNow (void);
bool turn (int degrees);