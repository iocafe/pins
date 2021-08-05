*** Compile and install 
Create temporary directory /tmp/raspicam
Copy  *-raspicam-X.X.X.zip into /tmp/raspicam.
cd /tmp/raspicam
unzip /tmp/raspicam.zip
Create temporary directory /tmp/raspicam/raspicam-X.X.X/tmp
cd /tmp/raspicam/raspicam-X.X.X/tmp
cmake ..
make
sudo make install

*** Add raspicam library to path (Ubuntu and Raspberry)
Create a new file in /etc/ld.so.conf.d/ called raspicam.conf

Edit the file and add a line per directory of shared libraries (*.so files), the file needs to look something like:

    /usr/local/lib

Reload the list of system-wide library paths:
    
    sudo ldconfig


*** Using QT debugger
It may be necessary to add following to run environment (Projects). 

LD_LIBRARY_PATH=/usr/local/lib:/usr/lib

It may be useful to DISABLE SIGSTOP from tools, options, debugger, GDB tab "Show message box when receiving signal" 

