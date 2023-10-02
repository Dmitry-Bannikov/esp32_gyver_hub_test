#include <Arduino.h>
#include <myportal.h>
#include <service.h>
#ifdef RELEASE
#include <esp_task_wdt.h>
#endif

void setup() {
	connectionInit();
	memoryInit();
	portalInit();
	#ifdef RELEASE
	esp_task_wdt_init(20, true);
	esp_task_wdt_add(NULL);
	#endif
}

void loop() {
	boardTick();
	portalTick();
	if (WiFi.status() == WL_CONNECTED && WiFi.getMode() == WIFI_STA) {
		LED_blink(100, 2000);
	} else if (WiFi.getMode() == WIFI_AP){
		LED_blink(1000);
	} else {
		LED_blink(200);
	}
	#ifdef RELEASE
	esp_task_wdt_reset();
	#endif
}
