# Modified E-Paper (Repaper.org Gratis)

Modified by Brody Kenrick to support partial screen updates (allowing for use on Arduino Uno with smaller SRAM).

The memory consumption of the buffer for the 2.7" display can be reduced down to 33 bytes (a single line of screen) from 5808 bytes.

This is a trade-off of display memory for processing cost and delay in display -- although not massively so (considering e-paper is slow update it is a good trade-off).


# gratis


## Sketches

These are example programs that will compile and run on the following platforms:

0. Arduino Uno

The following should still work but have not been tested.

1. TI LaunchPad with M430G2553 using the [Energia](https://github.com/energia) IDE
2. Arduino Leonardo using the [Arduino](http://arduino.cc) IDE


======

Gratis is a Repaper.org repository, initiated by E Ink and PDI for the purpose of making sure ePaper can go everywhere.
