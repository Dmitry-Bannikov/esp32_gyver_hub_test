/*
C:\Users\Viktoriya\.platformio\packages\framework-arduinoespressif32\libraries\WebServer\src 
find WebServer.cpp at line 649 and comment log_e
*/

#include <esp_task_wdt.h>
#include <Arduino.h>
#include <myportal.h>
#include <service.h>


void setup() {
	connectionInit();
	WiFi_Init();
	MqttInit();
	memoryInit();
	portalInit();
	Serial.println(WiFi.macAddress());
	esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);
}

void loop() {
	boardTick();
	portalTick();
	WiFi_tick();
	Mqtt_tick();
	esp_task_wdt_reset();
}