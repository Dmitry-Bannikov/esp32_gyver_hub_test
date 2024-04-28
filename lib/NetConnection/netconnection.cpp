#include "netconnection.h"
#include <common_data.h>
#include <EEManager.h>

wifisets wifi_settings;
global_vars globalData;

EEManager memoryWiFi(wifi_settings, 20000);

void WiFi_Init() {
	EEPROM.begin(2048);
	memoryWiFi.begin(0, MEMORY_KEY);
	delay(100);
	WiFi_Connect();
}

void WiFi_Connect() {
	if (wifi_settings.staModeEn) {
		if (strcmp(wifi_settings.staSsid, "") && WiFi.status() != WL_CONNECTED) {
			WiFi.mode(WIFI_STA);
			WiFi.begin(wifi_settings.staSsid, wifi_settings.staPass);
			uint32_t tmr = millis();
			while (WiFi.status() != WL_CONNECTED) {
				LED_blink(250);
				if (millis() - tmr > 30000) break;
			}
		} else {
			wifi_settings.staModeEn = 0;
		} 
	} 
	if (!wifi_settings.staModeEn) {
		if (WiFi.getMode() != WIFI_AP) {
			WiFi.mode(WIFI_AP);
			WiFi.softAP(wifi_settings.apSsid, wifi_settings.apPass);
			delay(1000);
		}
		if (strcmp(wifi_settings.staSsid, "")) {
			wifi_settings.staModeEn = 1;
		}
	}
	if (WiFi.status() == WL_CONNECTED && WiFi.getMode() == WIFI_STA) {
		Serial.print("\nLocal IP: ");
		Serial.println(WiFi.localIP());
	} else {
		Serial.print("\nSoft AP: ");
		Serial.println(WiFi.softAPIP());
	}
	
}

void WiFi_Tick() {
	static uint32_t tmr = 0;
	if (WiFi.status() == WL_CONNECTED && WiFi.getMode() == WIFI_STA) {
		LED_blink(100, 2000);
	} else if (WiFi.getMode() == WIFI_AP){
		LED_blink(1000);
	}
	if (millis() - tmr > 60000) {
		WiFi_Connect();
		tmr = millis();
	}
	memoryWiFi.tick();
}

void WiFi_Save() {
	memoryWiFi.updateNow();
}












//=================================
