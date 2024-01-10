/*
C:\Users\Viktoriya\.platformio\packages\framework-arduinoespressif32\libraries\WebServer\src 
find WebServer.cpp at line 649 and comment log_e
*/


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
}

void loop() {
	
	boardTick();
	portalTick();
	WiFi_tick();
	Mqtt_tick();
	
}

//сделать поддержку пирометра?
//добавить почту пароль мак
//добавить время алярмов
//при внешнем сигнале минута 190, минута 240
//изучить еспшку с симкой