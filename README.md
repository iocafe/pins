# iocafe pins library - interface to GPIO, ADC/DAC, PWM, etc. 

<b>DRAFTING/PLANNING STAGE, NOT FOR SERIOUS USE.</b>

Function of the "pins" is to:
- Provide an application with hardware independent access to GPIO, analogs, PWM, etc.
- Provide stubs to forward "pins" library calls to hardware dependent IO library.
- Specify hardware pin configuration so that it is written once, and is then available from C, iocom memory maps and in documentation. 

