carol directory maps IO device application "jane" to IO pins of "carol" hardware.

ESP32 Test connection

Inputs
DIP_SWITCH_3 - GPIO 34: Blue 8 dip switch block, switch pin 3 grounds the input. Dip switch is also declared as pull-up, but this doesn't seem to do anything on ESP32.
DIP_SWITCH_4 - GPIO 35: Blue 8 dip switch block, switch pin 4 grounds the input.

TOUCH_SENSOR - GPIO 4: touch the sensor T0 (GPIO4)
POTENTIOMETER: GRIP 25: 10k potentiometer wired to give voltage from 0 to 3.3V, ADC channel 8

SERVO - GPIO 32: Servo PWM control. PWM channel 0 (set as bank) is used with 50 Hz frequency and 12 bits precision, and initialized at middle (2048)
DIMMER_LED - GPIO 33: LED dimmer PWM control. PWM channel 1 (set as bank) is used with 5000 Hz frequency and 12 bits precision, and initialized at dark (0)

min and max can be used to set range of values. These do not effect anything to driver, but allow the hardware configuration .json to pass these to application.
So if same application is compiled for different hardwares, it can adopt to HW capabilities and settings.
