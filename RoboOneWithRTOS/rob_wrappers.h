/* Rob's system stuff - an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 *  Author: Rob Meades
 */

/* Can play with using the C library functions directly by commenting/uncommenting as necessary below */
/* If you add any one of the native functions, uncomment the lines to let stdlib.h and string.h in also */
#include <stdlib.h>
#include <string.h>

#if 0
    //#define RobPrintf printf
#    define RobPrintf _RobPrintf
     void _RobPrintf (char* fmt, ...);
#endif

//#define RobMalloc malloc
#define RobMalloc _RobMalloc
void * _RobMalloc (size_t size);

//#define RobFree free
#define RobFree _RobFree
void _RobFree (void * ptr);

#define RobStrlen strlen
//#define RobStrlen _RobStrlen
//size_t _RobStrlen (const char * ptr);

#define RobMemset memset
//#define RobMemset _RobMemset
//void * _RobMemset (void * ptr, int value, size_t size);

#define RobMemcpy memcpy
//#define RobMemcpy _RobMemcpy
//void * _RobMemcpy (void * destPtr, void const * sourcePtr, size_t size);

void rob_wait_play_from_program_space (const char * pSequence);
void rob_lcd_init_printf (void);
void rob_clear (void);
void rob_serial_send_usb_comm (char * pBuffer, unsigned char size);
void rob_serial_send_blocking_usb_comm (char * pBuffer, unsigned char size);
void rob_serial_check (void);
void rob_serial_set_baud_rate_usb_comm (unsigned long baud);
void rob_serial_receive_ring_usb_comm (char * pBuffer, unsigned char size);
unsigned char rob_serial_get_received_bytes_usb_comm (void);
void rob_wait_serial_send_buffer_empty_usb_comm (void);
void rob_print_character (char c);
void rob_print (const char * pStr);
void rob_print_from_program_space (const char * pStr);
void rob_print_long (long value);
void rob_print_unsigned_long (unsigned long value);
void rob_lcd_goto_xy (int col, int row);
unsigned int rob_read_vcc_millivolts (void);