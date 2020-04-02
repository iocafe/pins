# iocafe pins library - interface to GPIO, ADC/DAC, PWM, etc. 

<b>DRAFTING/PLANNING STAGE, NOT FOR SERIOUS USE.</b>

Function of the "pins" is to:
- Define hardware/platform independent API to GPIO, analogs, PWM, etc.
- Provide stubs to forward "pins" library calls to hardware dependent IO library.
- HW spwcific pin configuration is written once in JSON file and is separated from application code. 
- Pin configuration information in JSON is converted to C, iocom signals mappings and documentation. 

https://github.com/iocafe/pins/blob/master/doc/190916-pins-library/190916-pins-library.pdf
