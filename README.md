# OrangutanSoftware
The SW in this repository is the Orangutan portion of the SW for RoboOne, described in full here:

http://www.meades.org/robots/robots.html

The Orangutan (X2) is a board from Pololu including an ATmega1284P AVR microcontroller and motor drivers.  The SW is written in C and can be built inside AtmelStudio 6 using WinAVR compilation tools and Pololu's own supporting C libraries.  The SW is built with FreeRTOS and runs a small number of tasks communicating via a single message queue.  It accepts command strings over the Orangutan board's USB interface and interprets these to result in either movement of the motors or reading of various IP ports to which various IR sensors are attached.

The Orangutan is slave to the Pi Software elsewhere in this repository.

Rob Meades
