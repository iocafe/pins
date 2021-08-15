# iocafe pins library - interface to GPIO, ADC/DAC, PWM, etc. 

15.8.2021 status: The pins library is in raw prototype status which can be used for ESP32 with iocom. 

Function of the "pins" is to:
- Define hardware/platform independent API to GPIO, analogs, PWM, etc.
- Provide stubs to forward "pins" library calls to hardware dependent IO library.
- HW specific pin configuration is written once in JSON file and is separated from application code. 
- Pin configuration information in JSON is converted to C, iocom signals mappings and documentation. 

Pins library encapsulates:
- Digital and analog inputs and outputs.
- Input pin state change interrupts.
- Timers and timer interrupts.
- PWM
- UART
- SPI
- Device state displays (TFT).
- Cameras (linear and pixel matrix).

Supported HW platforms/frameworks:
- ESP32 is currently being tested/developed.

https://github.com/iocafe/pins/blob/master/doc/190916-pins-library/190916-pins-library.pdf

Note about folder names:
- code: Base library code, this has about same functionality for all platforms
- extensions: Extension code, support varies by platform
- common: Common code for all platforms
- simulation: Simulation code for Windows, Linux, etc. without actual IO.
- duino: Common arduino API code, for Arduino, STM32duino and Teensyduino (not for ESP32)
- arduino: Arduino specific code
- esp32: ESP32 specific code
