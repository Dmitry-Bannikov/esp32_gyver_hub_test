/*
C:\Users\Viktoriya\.platformio\packages\framework-arduinoespressif32\libraries\WebServer\src 
find WebServer.cpp at line 649 and comment log_e
*/

#include <esp_task_wdt.h>
#include <Arduino.h>
#include <service.h>


void setup() {
	System_Init();
	Board_Init();
	Web_Init();
	esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);
}

void loop() {
	Board_Tick();
	Web_Tick();
	esp_task_wdt_reset();
}