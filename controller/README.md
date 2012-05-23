
Scalexercise
============

This is a base Arduino sketch for controlling up to 6 Scalextric SSD (digital) throttle controls.

These are essentially a 5K variable (throttle) + 8.2K (lane-change) and 18K (brake) resistor, 
so are easily replicated with a pair of digital pots.

12 digital pots are controlled via 2x shift registers.

The code takes input from the serial console or an assigned input pin, and converts the button push intervals
to a throttle value. This is designed so that a 'button push' can be replaced with a reed switch on a rotating wheel.
