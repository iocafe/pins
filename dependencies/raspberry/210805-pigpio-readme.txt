*** Compile and install 
Create temporary directory /tmp/pigpio
Copy  *-pigpio-X.X.X.zip into /tmp/pigpio.
cd /tmp/pigpio
unzip /tmp/pigpio.zip
cd pigpio-master
Create temporary directory /tmp/pigpio/pigpio-master/tmp (mkdir tmp): 
cd tmp
make
sudo make install

*** root priviliges required
The pigpio C library and the pigpio C library requires root privileges. 

Pigpio daemon could be used to circumvent this, by running daemon as root and 
using it from application through pipe as normal user. I have not done this.
