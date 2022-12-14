Requirements:
The code used the TextLCD library (https://os.mbed.com/users/simon/code/TextLCD/docs/tip/TextLCD_8h_source.html)
Please follows the installation of this library for normal use of the LCD

Structure:
The code is split into two files:
helper_funcs.h: A short library with inline functions that facilitate specific coding operations, such as inserting into arrays, 
averaging arrays, comparing arrays etc... Each function is described in this file.
main.cpp: Main control code, that executes the powering of the fan and peripherals, measures and controls speed.

Usage:

Manual Control: Rotate the grey encoder in the Nucleo board to select the desired target speed. Clockwise rotation to increase target speed and Counterclockwise to decrease target speed.

ON/OFF Functionality: The Blue push button activates and deactivates the control code. The LCD will indicate when the system is turned off and the LED in the extension board will be red when the system is deactivated. When ON, the fan will be powered and the LCD will display the values of Speed (measured RPM), PID value (Duty cycle) and Target speed (RPM calculated from encoder counter). The indicator LED will be green when the system is enabled.

The following code controls a fan using two modes that automatically swap at specific duty cycle modes.

>=800 RPM:  (Minimum speed in the FAN specification), the system runs in a closed loop using PID control and displays the speed (measured RPM), PID value (Duty cycle) and Target speed.
<800 RPM: The system switches automatically to an open loop and the user can control the speed value directly by changing the PWM duty cycle.