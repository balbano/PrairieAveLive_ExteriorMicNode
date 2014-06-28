Prairie Avenue Live - Exterior Mote
===================================

The exterior mic node reads the volume data from each of the exterior
mics and transmits it to the microcontroller running the LED panel.

Terminology
-----------

- Mote: Arduino Fio and XBee collects sound levels from mic and transmits to controller.
  - There are multiple interior motes with one mic each and one exterior mote with multiple mics.
- Mic: the electret mics used by the motes.
- Controller: Teensy 3.1 and XBee that receive the sound data from the motes and control the LEDs.
- Node: the origin point (on the LED array) of the visualization for a particular mic.
  
Parts
-----

- Arduino Fio: https://www.sparkfun.com/products/10116
- Electret mics with auto gain: http://www.adafruit.com/products/1713
- XBee Series 2: http://www.adafruit.com/products/968

Repos
-----

 - [PrairieAveLive_Controller](https://github.com/balbano/PrairieAveLive_Controller)
 - [PrairieAveLive_InteriorMote](https://github.com/balbano/PrairieAveLive_InteriorMote)
 - [PrairieAveLive_ExteriorMote](https://github.com/balbano/PrairieAveLive_ExteriorMote)
