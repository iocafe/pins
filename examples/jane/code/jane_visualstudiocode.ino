#include <Arduino.h>
#include <eosalx.h>
#include <pins.h>

/*
  jane_visualstudiocode.ino
  Example to build simple client app with Visual Studio Code + Platform IO + Arduino libraries. 
  The setup routine runs once when the device starts.
 */
void setup() 
{
    /* Set up serial port for trace output.
     */
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Jane starting (Arduino mode)...");

    /* Initialize eosal library and start the simple client.
     */
    osal_initialize(OSAL_INIT_DEFAULT);
    osal_main(0, 0);
}

/* The loop function is called repeatedly while the device runs.
 */
void loop() 
{
    /* Start the included application.
     */
    if (osal_loop(osal_application_context)) osal_reboot(0);
}
