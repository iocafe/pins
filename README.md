# iocafe pins library - interface to GPIO, ADC/DAC, PWM, etc. 

<b>DRAFTING/PLANNING STAGE, NOT FOR SERIOUS USE.</b>

Function of the "pins" is to:
- Define hardware/platform independent API to GPIO, analogs, PWM, etc.
- Provide stubs to forward "pins" library calls to hardware dependent IO library.
- HW specific pin configuration is written once in JSON file and is separated from application code. 
- Pin configuration information in JSON is converted to C, iocom signals mappings and documentation. 

Pins library encapsulates:
- Digital and anlog inputs and outputs.
- Input pin state change interrupts
- Timers and timer interrupts
- PWM
- UART
- SPI
- Device state displays (TFT)
- Cameras (linear and pixel matrix)

Supported HW platforms/frameworks
- ESP32 is currently being tested/developed

https://github.com/iocafe/pins/blob/master/doc/190916-pins-library/190916-pins-library.pdf
