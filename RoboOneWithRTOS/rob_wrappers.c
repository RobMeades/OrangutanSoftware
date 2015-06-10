/* Rob's wrapers for standard functions and Pololu functions that may not be thread safe - an application for the Pololu Orangutan X2
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 10/14/2013 7:10:02 PM
 *  Author: Rob Meades
 */

/* So, OK, this is utter overkill but I don't know what to trust in the Pololu library yet so I want to be safe */

#include <pololu/orangutan.h>

#include <stdio.h>
#include <string.h>
#include <rob_system.h>
#include <rob_wrappers.h>

#include <FreeRTOS.h>
#include <task.h>

#define ASSERT_TUNE "!L16 V8 dc#"

#define LCD_COL_MAX 15
#define LCD_ROW_MAX 3

/* - GLOBALS -------------------------------------------------------------------------- */

/* Need this to manage wrapping on the LCD display as the native Pololu C functions don't do that */
static unsigned int gCursorPosCol = 0;
static unsigned int gCursorPosRow = 0;

/* - STATIC FUNCTIONS ----------------------------------------------------------------- */

/* Move the cursor on the LCD display on, wrapping as necessary */
void moveCursorOn (int numChars)
{
    gCursorPosCol++;
    if (gCursorPosCol > LCD_COL_MAX)
    {
        gCursorPosCol = 0;
        gCursorPosRow++;
        if (gCursorPosRow > LCD_ROW_MAX)
        {
            gCursorPosRow = 0;
        }
    }
    lcd_goto_xy (gCursorPosCol, gCursorPosRow);
}

/* - PUBLIC FUNCTIONS ----------------------------------------------------------------- */

/* Wrappers for standard library functions that might not be thread safe */
/* Don't call these directly, call their macros in rob_wrappers.h so that we
** can swap between std library and wrapped functions */

#if 0
/* if you're going to use this, don't forget to add back in rob_lcd_init_printf() in the initialisation code */
void _RobPrintf (char * pFmt, ...)
{
    va_list args;

    vTaskSuspendAll();
    {
        va_start (args, pFmt);
        vprintf (pFmt, args);
        va_end (args);
    }
    xTaskResumeAll();
}
#endif

void * _RobMalloc (size_t size)
{
    return pvPortMalloc (size);
}

void _RobFree (void * pPtr)
{
   vPortFree (pPtr);
}

size_t _RobStrlen (const char * pPtr)
{
    size_t len;

    vTaskSuspendAll();
    {
        len = strlen (pPtr);
    }
    xTaskResumeAll();

    return len;
}

void * _RobMemset (void * pPtr, int value, size_t size)
{
    void * pReturnPtr;

    vTaskSuspendAll();
    {
        pReturnPtr = memset (pPtr, value, size);
    }
    xTaskResumeAll();

    return pReturnPtr;
}

void * _RobMemcpy (void * destPtr, void const * sourcePtr, size_t size)
{
    void * pReturnPtr;

    vTaskSuspendAll();
    {
        pReturnPtr = memcpy(destPtr, sourcePtr, size);
    }
    xTaskResumeAll();

    return pReturnPtr;
}

/* Wrappers for Pololu functions that may not be thread safe or like being delayed/swapped-out */
void rob_wait_play (const char * pSequence)
{
    vTaskSuspendAll();
    {
        play (pSequence);
        while (is_playing())
        {
        }
    }
    xTaskResumeAll();
}

void rob_wait_play_from_program_space (const char * pSequence)
{
    vTaskSuspendAll();
    {
        play_from_program_space (pSequence);
        while (is_playing())
        {
        }
    }
    xTaskResumeAll();
}

void rob_lcd_init_printf (void)
{
    vTaskSuspendAll();
    {
        lcd_init_printf();
    }
    xTaskResumeAll();
}

void rob_clear (void)
{
    vTaskSuspendAll();
    {
        gCursorPosCol = 0;
        gCursorPosRow = 0;
        clear();
    }
    xTaskResumeAll();
}

void rob_serial_send_usb_comm (char * pBuffer, unsigned char size)
{
    vTaskSuspendAll();
    {
        serial_send (USB_COMM, pBuffer, size);
    }
    xTaskResumeAll();
}

void rob_serial_send_blocking_usb_comm (char * pBuffer, unsigned char size)
{
    vTaskSuspendAll();
    {
        serial_send_blocking (USB_COMM, pBuffer, size);
    }
    xTaskResumeAll();
}

void rob_serial_check (void)
{
    vTaskSuspendAll();
    {
        serial_check();
    }
    xTaskResumeAll();
}

void rob_serial_set_baud_rate_usb_comm (unsigned long baud)
{
    vTaskSuspendAll();
    {
        serial_set_baud_rate (USB_COMM, baud);
    }
    xTaskResumeAll();
}

void rob_serial_receive_ring_usb_comm (char * pBuffer, unsigned char size)
{
    vTaskSuspendAll();
    {
        serial_receive_ring (USB_COMM, pBuffer, size);
    }
    xTaskResumeAll();
}

unsigned char rob_serial_get_received_bytes_usb_comm (void)
{
    unsigned char numBytes;

    vTaskSuspendAll();
    {
        numBytes = serial_get_received_bytes (USB_COMM);
    }
    xTaskResumeAll();

    return numBytes;
}

void rob_wait_serial_send_buffer_empty_usb_comm ()
{
    vTaskSuspendAll();
    {
        while (!serial_send_buffer_empty (USB_COMM))
        {
        }
    }
    xTaskResumeAll();
}

void rob_print_character_no_suspend (char c)
{
    print_character (c);
    moveCursorOn (1);    
}

void rob_print_character (char c)
{
    vTaskSuspendAll();
    {
        print_character (c);
        moveCursorOn (1);
    }
    xTaskResumeAll();
}

void rob_print (const char * pStr)
{
    vTaskSuspendAll();
    {
        while (*pStr != 0)
        {
            rob_print_character_no_suspend (*pStr);
            pStr++;
        }
        
    }
    xTaskResumeAll();
}

void rob_print_from_program_space (const char * pStr)
{
    vTaskSuspendAll();
    {
	    char c;
	    while ((c = pgm_read_byte (pStr)) != 0)
	    {
    	    rob_print_character_no_suspend (c);
    	    pStr++;
	    }
    }
    xTaskResumeAll();
}

/* This function use to just call the Pololu print_long() function
 * but that doesn't cope with line wrap */
void rob_print_long (long value)
{
    vTaskSuspendAll();
    {
        if (value < 0)
        {
            value = -value;
            rob_print_character_no_suspend ('-'); // print the minus sign
        }
        rob_print_unsigned_long ((unsigned long) value);
    }
    xTaskResumeAll();
}

/* This function use to just call the Pololu print_unsigned_long() function
 * but that doesn't cope with line wrap */
void rob_print_unsigned_long (unsigned long value)
{
    vTaskSuspendAll();
    {
        unsigned char str[10];
        unsigned char i = 10;

        unsigned char digit;

        do
        {
            digit = value;
            value /= 10;
            digit -= value * 10;
            str[--i] = '0' + (unsigned char)digit;
        }
        while (value != 0);

        for(; i < 10; i++)
        {
            rob_print_character_no_suspend (str[i]);            
        }
    }
    xTaskResumeAll();
}

void rob_lcd_goto_xy (int col, int row)
{
    vTaskSuspendAll();
    {
        lcd_goto_xy (col, row);
        gCursorPosCol = col;
        gCursorPosRow = row;
    }
    xTaskResumeAll();
}

unsigned int rob_read_vcc_millivolts()
{
    unsigned int supplyVolts;

    vTaskSuspendAll();
    {
        supplyVolts = read_vcc_millivolts();
    }
    xTaskResumeAll();

    return supplyVolts;
}




