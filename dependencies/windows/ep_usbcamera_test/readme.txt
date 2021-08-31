ep_usbcamera library test program
notes 29.4.2020/pekka

This is main program to test USB camera library. It uses opencv, which is not included here.
To run this you need some work to install and set up the opencv, and modify paths of opencv include 
files and libraries.

I placed opencv in c:\coderoot\opencv directory simply copied files listed below to I run 64 bit build:
- opencv_world430.lib and opencv_world430d.lib to C:\coderoot\lib\win64_vs2019
- opencv_world430.dll and opencv_world430d.dll to C:\coderoot\bin\win64