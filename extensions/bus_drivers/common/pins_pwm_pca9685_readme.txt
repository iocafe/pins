PCA9685
notes 22.8.2020/pekka

PCA9685 is 16-Channel (12-bit) PWM/servo driver with I2C interface

- i2c-controlled PWM driver with a built in clock. 
- 5V compliant, which means you can control it from a 3.3V microcontroller and still safely drive up to 6V outputs.
- 6 address select pins so you can wire up to 62 of these on a single i2c bus.
- Adjustable frequency PWM up to about 1.6 KHz.
- 12-bit resolution for each output - for servos, that means about 4us resolution at 60Hz update rate.
- Configurable push-pull or open-drain output.

