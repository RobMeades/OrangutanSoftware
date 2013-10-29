/* Rob's system stuff - an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 *  Author: Rob Meades
 */

#ifdef WIN32
#include <rob_win32stubs.h>
#else
#include <stdbool.h>
#include <avr/pgmspace.h>
#endif

#define MAX_TURN_DEGREES           180 /* Not greater than 255 */
#define MAX_DISTANCE_CM            10000 /* Not greater than 10000 */
#define MAX_SPEED_CM_SEC           100 /* Not greater than 10000 */
#define UNITS_ARE_DISTANCE         'D'
#define UNITS_ARE_SPEED            'S'
#define DEFAULT_TURN_DEGREES       90

#define PNULL (void *) NULL
#define ASSERT_ALWAYS_STRING(sTRING) ((assertFunc (__FUNCTION__, __LINE__, PSTR(sTRING), 0)))
#define ASSERT_ALWAYS_PARAM(pARAM1) ((assertFunc (__FUNCTION__, __LINE__, PNULL, (pARAM1))))
#define ASSERT_STRING(cONDITION,sTRING) ((cONDITION) ? true : (assertFunc (__FUNCTION__, __LINE__, PSTR(sTRING), 0)))
#define ASSERT_PARAM(cONDITION,pARAM1) ((cONDITION) ? true : (assertFunc (__FUNCTION__, __LINE__, PNULL, (pARAM1))))

bool assertFunc (const char * place, int line, const char * pText, int param1);
void endStuff (void);