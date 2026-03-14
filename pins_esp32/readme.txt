iocom_esp32 directory - The ESP32 build system workaround
14.3.2026/Pekka 

In ESP_IDF the directory name is component name: The ESP-IDF build system identifies components based on the name of 
the directory containing the CMakeLists.txt file. Here, a directory named iocom_esp32 is treated as the 
component iocom_esp32.

Since our code is placed in "code" folders for other build systems, the new directory  
is created to work around ESP-IDF directory naming convention. 
It holds the CmakeList.txt to compile files code directory and register IDF component.