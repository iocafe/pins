Wiring alarm clock
pekka 18.12.2020

remember to enable SPI from "sudo raspi-config"

Pi pin numbers are board pin numbers, not GPIO numbers.

# Pins st7735s

TFT pin	    Pi pin  Name	Description
1	                VCC	    +3.3V
2	                GND	    Ground 0V
3	        24      CS	    [SPI] Chip Select
4	        36      RESET	Reset (reconnect the display) (Pi pin can be changed)
5	        22      A0	    DC, Data/Command (Pi pin can be changed)
6	        19      SDA	    [SPI] MOSI, Master Output Slave Input
7	        23      SCK	    [SPI] CLK, Serial Clock
8	        16      LED	    Backlight (Pi pin can be changed) *** CHANGED, WAS 18!!!

------

# Audio 
Simple way to configure the Pi GPIO pins for PWM audio Add the line below to your /boot/config.txt will reconfigure the pins at boot without any external software or services:

dtoverlay=audremap,pins_18_19
dtoverlay=pwm-2chan,pin=18,func=2,pin2=13,func2=4

CHECK ARE THESE REALLY BOARD PIN NUMBERS OR GPIO NUMBERS ????

Pi pin  Name	        Description
18      AUDIO_LEFT      PWM out for left audio channel (If 18 is gpio, board is 12)
13      AUDIO_RIGHT     PWM out for right audio channel (If 13 is gpio, board is 33)


# Switches/buttons and LIGHT

Pi pin  Name	        Description
10      INSP_BUTTON     Inspirational life advice button
7       LEFT_BUTTON     Configuration buttons
15      RIGHT_BUTTON
29      UP_BUTTON
31      DOWN_BUTTON
32      MID_BUTTON      Same as set button?  *** CHANGED, WAS 10 
35      ON_OFF_BUTTON   Turn alarm on/off *** CHANGED, WAS 18 
37      NIGHT_LIGHT     Night light  *** CHANGED, WAS 8 

