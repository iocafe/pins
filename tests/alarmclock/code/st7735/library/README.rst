Python ST7735
=============

`Build Status <https://travis-ci.com/pimoroni/st7735-python>`__
`Coverage
Status <https://coveralls.io/github/pimoroni/st7735-python?branch=master>`__
`PyPi Package <https://pypi.python.org/pypi/st7735>`__ `Python
Versions <https://pypi.python.org/pypi/st7735>`__

Python library to control an ST7735 TFT LCD display. Allows simple
drawing on the display without installing a kernel module.

Designed specifically to work with a ST7735 based 160x80 pixel TFT SPI
display. (Specifically the 0.96" SPI LCD from Pimoroni).

Make sure you have the following dependencies:

::

   sudo apt-get update
   sudo apt-get install python-rpi.gpio python-spidev python-pip python-imaging python-numpy

Install this library by running:

::

   sudo pip install st7735

See example of usage in the examples folder.

Licensing & History
===================

This library is a modification of a modification of code originally
written by Tony DiCola for Adafruit Industries, and modified to work
with the ST7735 by Clement Skau.

It has been modified by Pimoroni to include support for their 160x80 SPI
LCD breakout, and hopefully also generalised enough so that it will
support other ST7735-powered displays.

Modifications include:
----------------------

-  PIL/Pillow has been removed from the underlying display driver to
   separate concerns- you should create your own PIL image and display
   it using ``display(image)``
-  ``width``, ``height``, ``rotation``, ``invert``, ``offset_left`` and
   ``offset_top`` parameters can be passed into ``__init__`` for
   alternate displays
-  ``Adafruit_GPIO`` has been replaced with ``RPi.GPIO`` and ``spidev``
   to closely align with our other software (IE: Raspberry Pi only)
-  Test fixtures have been added to keep this library stable

Pimoroni invests time and resources forking and modifying this open
source code, please support Pimoroni and open-source software by
purchasing products from us, too!

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing products
from Adafruit!

Modified from ‘Modified from ’Adafruit Python ILI9341’ written by Tony
DiCola for Adafruit Industries.’ written by Clement Skau.

MIT license, all text above must be included in any redistribution
