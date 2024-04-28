#include "netconnection.h"
#include <common_data.h>

wifisets wifi_settings;
global_vars globalData;
EEManager memoryWIFI(wifi_settings, 20000);



void WifiInit() {
    EEPROM.begin(512);
	memoryWIFI.begin(0, MEMORY_KEY);

	if (wifi_settings.staModeEn) {
		WiFi.mode(WIFI_STA);
		WiFi.begin(wifi_settings.staSsid, wifi_settings.staPass);
		uint32_t tmr = millis();
		while (WiFi.status() != WL_CONNECTED) {
			LED_blink(250);
			if (millis() - tmr > 30000) break;
		}
		if (WiFi.status() != WL_CONNECTED) {
			LED_blink(0);
			wifi_settings.staModeEn = 0; // переключаемся на режим точки доступа
			wifi_updateCFG();		 // сохраняемся
			ESP.restart();				 // перезапускаем есп
			return;
		}
        Serial.print("\nLocal IP: ");
		Serial.println(WiFi.localIP());
	} else {
		WiFi.mode(WIFI_AP);
		WiFi.softAP(wifi_settings.apSsid, wifi_settings.apPass);
		delay(100);
        Serial.print("\nSoft AP: ");
		Serial.println(WiFi.softAPIP());
	}
}

void wifi_tick() {
	memoryWIFI.tick();
	if (WiFi.status() == WL_CONNECTED && WiFi.getMode() == WIFI_STA) {
		LED_blink(100, 2000);
	} else if (WiFi.getMode() == WIFI_AP){
		LED_blink(1000);
	}
}

void wifi_updateCFG() {
	memoryWIFI.updateNow();
}
