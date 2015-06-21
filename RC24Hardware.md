# Introduction #

Details of some of the hardware items that have been built for rc24.

# Details #

A number of hardware items have been built for the rc24 system.

  * Transmitter.
  * 6 channel receiver.
  * 8 channel receiver.

## Transmitter ##

TBD

## 6 channel receiver ##

TBD

## 8 channel receiver ##

This receiver was designed by Graham and has 8 servo connectors with the two outer ones having an extra pin each to allow for further interfacing options.  On one end the additional pin gives access to the SPI MISO pin to allow the rx to act as a slave, and at the other end the top two pins (rc servo signal and the extra one) give access to the DIO14/SIF\_CLK/IP\_CLK and DIO15/SIF\_D/IP\_DO lines to allow the rx to communicate using I2C.

Additional components on the pcb provide an external reset circuit, an LED driven by a DIO, a switch connected to another DIO and a potential divider connected to the pcb input power line and one ADC channel.