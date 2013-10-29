/* freeRTOSBoardDefs.h
 *
 */

#ifndef freeRTOSBoardDefs_h
#define freeRTOSBoardDefs_h

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/io.h>

/*-----------------------------------------------------------
 * MCU and application specific definitions.
 *
 * These definitions should be adjusted for your particular
 * application requirements.
 *
 *----------------------------------------------------------*/

    #define F_CPU 20000000L                                         // Set ORANGUTAN CPU frequency here

//	#define portUSE_TIMER0                                          // portUSE_TIMER0 to use 8 bit Timer0
//  #define portUSE_TIMER2											// portUSE_TIMER2 to use 8 bit RTC Timer2 on 1284p device
	#define portUSE_TIMER3											// portUSE_TIMER3 to use 16 bit Timer3 on 1284p device
    #define configTICK_RATE_HZ		( ( portTickType ) 100 )		// Use 500Hz for TIMER3. MINIMUM of 128Hz for TIMER2.
                                                                    // Use 1000Hz to get mSec timing using TIMER3.

	#define configCPU_CLOCK_HZ		( ( uint32_t ) F_CPU )			// This F_CPU variable set by Eclipse environment
    #define configTOTAL_HEAP_SIZE	( (size_t )  12000  )			// used for heap_1.c and heap2.c, and heap_4.c only (heap_3.c uses malloc() and free())

//	#define portW5200						// or we assume W5100 Ethernet

//	#define portHD44780_LCD					// define the use of the Freetronics HD44780 LCD (or other). Check include for (flexible) pin assignments.
//	#define portSD_CARD						// define the use of the SD Card for Goldilocks 1284p
//	#define portRTC_DEFINED					// RTC DS1307 implemented, therefore define.

	#define	portSERIAL_BUFFER_RX	64		// Define the size of the serial receive buffer.
	#define	portSERIAL_BUFFER_TX	255		// Define the size of the serial transmit buffer, only as long as the longest text.
	#define portSERIAL_BUFFER		portSERIAL_BUFFER_TX

//  #define portUSE_TIMER1_PWM				// Define which Timer to use as the PWM Timer (not the tick timer).
											// though it is better to use Pololu functions, as they support 8x multiplexed servos.

// I2C pins
#define I2C_PORT			PORTC
#define I2C_PORT_DIR		DDRC
#define I2C_PORT_STATUS		PINC
#define I2C_BIT_SDA			_BV(PC1)
#define I2C_BIT_SCL			_BV(PC0)

// SPI pins
#define SPI_PORT			PORTB
#define SPI_PORT_DIR		DDRB
#define SPI_PORT_PIN		PINB
#define SPI_BIT_SCK			_BV(PB7)
#define SPI_BIT_MISO		_BV(PB6)
#define SPI_BIT_MOSI		_BV(PB5)
#define SPI_BIT_SS			_BV(PB4)

#define SPI_BIT_SS_WIZNET	_BV(PB4)	// added for Wiznet 5100/5200 support with SS on PB4 (Pin 10)

//#define SPI_PORT_SS_SD		PORTB
//#define SPI_PORT_DIR_SS_SD	DDRB
//#define SPI_PORT_PIN_SS_SD	PINB
//#define SPI_BIT_SS_SD		_BV(PB0)	// added for support of integrated SD card on PB0 (Virtual Pin 14)
//#define SPI_PORT_SS_SD		PORTD
//#define SPI_PORT_DIR_SS_SD	DDRD
//#define SPI_PORT_PIN_SS_SD	PIND
//#define SPI_BIT_SS_SD		_BV(PD4)	// added for SD Card support with SS on PD4 (Pin 4) for Standard Arduino SD cages.

// port D pins
#define IO_D0				0
#define IO_D1				1
#define IO_D2				2
#define IO_D3				3
#define IO_D4				4
#define IO_D5				5
#define IO_D6				6
#define IO_D7				7

// port B pins - TAKEN OUT FOR ORANGUTAN as they are already in OrangutanDigital.h
//#define IO_B0				14
//#define IO_B1				15
//#define IO_B2				8
//#define IO_B3				9
//#define IO_B4				10
//#define IO_B5				11
//#define IO_B6				12
//#define IO_B7				13

// port C pins
#define IO_C0				16
#define IO_C1				17
//#define IO_C2				// JTAG
//#define IO_C3				// JTAG
//#define IO_C4				// JTAG
//#define IO_C5				// JTAG
//#define IO_C6				// TOSC RTC
//#define IO_C7				// TOSC RTC

// port A pins - TAKEN OUT FOR ORANGUTAN as they are already in OrangutanDigital.h
//#define IO_A0				24
//#define IO_A1				25
//#define IO_A2				26
//#define IO_A3				27
//#define IO_A4				28
//#define IO_A5				29
//#define IO_A6				30
//#define IO_A7				31

#ifdef __cplusplus
}
#endif

#endif // freeRTOSBoardDefs_h
