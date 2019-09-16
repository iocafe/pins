# pins
iocafe pins - interface to GPIO, ADC/DAC, PWM, etc. 

COMPLETELY ON DRAFTING/PLANNING STAGE, NOT USEFUL.

Function of the "pins" is to:
- Provide an application with hardware independent access to GPIO, analogs, PWM, timers, etc.
- Provide subs to call forward hardware dependent IO library, like pigpio on Raspberry PI, HAL on STM32, IO simulation on PC and so forth.
- Define hardware pin configuration in such way that it needs to be written only once, and is then available from C code, memory maps and in documentation. 

Some ideas about implementation
- Hardware speficic IO headers are not included in application (except when there is need to bypass "pins").
- Hardware addressess, etc. need to be mapped to application at some point. This could be simplyy writeen as one C file. But we write it as JSON and then generate the C file automatically by Python script. This enable using the PIN information to automate documentation, IOCOM memory mapping, etc. 

