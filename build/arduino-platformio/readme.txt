copy-pins-for-platformio.py 8.1.2020/pekka

Copies pins library files needed for PlatformIO Arduino build
into /coderoot/lib/arduino-platformio/pins directory. 
To make this look like Arduino library all .c and .cpp
files are copied to target root folder, and all header
files info subfolders.
