					jw, Mo 30. Nov 02:57:25 CET 2015

Motor driver for a 3D-scanner turntable
---------------------------------------

Stepper Motor: Neuftech 28BYJ 48
  The 28BYJ - 48 motor is a 4 - phase, 8 - beat motor, geared down by
  a factor of 68. One bipolar winding is on motor pins 1 & 3 and
  the other on motor pins 2 & 4. The step angle is 5.625/ 64 and the
  operating Frequency is 100pps. Current draw is 92mA.

Controller: Arduino Nano
  Programmer: USBtinyISP
  USB Serial: 9600 baud
  The controller starts in demo mode, when powered up, the turntable will move in a 
  predefined pattern back and forth.

Commands:
  s		stop. Ends demo mode and stops the motor.
  m (-)STEPS	move the motor STEPS forward (or backward)
  g (-)DEG	move the motor, so that the table turns DEG counterclockwise (or clockwise).
  d		enter demo mode.
  c		print current position in steps

