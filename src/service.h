#pragma once

#include <Arduino.h>
#include <data.h>
#include <mqtthandle.h>
#include <WiFiUdp.h>
#include <NTPClient.h>



void connectionInit();
void memoryInit();
void LED_blink(uint16_t period_on, uint16_t period_off);
void scanNewBoards();
void boardTick();
void WiFi_Init();
void WiFi_tick();
void BoardRequest(uint8_t &request);



void LED_blink(uint16_t period_on, uint16_t period_off = 0) {
  	static uint64_t tick = 0;
	static bool led_state = false;
	if (!period_on) {
		digitalWrite(LED_BUILTIN, HIGH);
		return;
	} 
	if (period_on == 1) {
		digitalWrite(LED_BUILTIN, LOW);
		return;
	}
	if (!period_off) {
		if (millis() - tick > period_on) {
			digitalWrite(LED_BUILTIN, (led_state = !led_state));
			tick = millis();
		}
	} else {
		if (millis() - tick > (led_state ? period_on : period_off)) {
			digitalWrite(LED_BUILTIN, (led_state = !led_state));
			tick = millis();
		}
	}
}

void connectionInit() {
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(21, INPUT);
	pinMode(22, INPUT);
	Serial.begin(115200);
	Serial.print("\nI2C pins state: ");
	Serial.print(digitalRead(21));
	Serial.println(digitalRead(22));
	Board::StartI2C();
	board.reserve(MAX_BOARDS);
	delay(10);
	scanNewBoards();
}

void memoryInit() {
	LED_blink(1);
	EEPROM.begin(512);
	memoryWIFI.begin(0, 127);
	LED_blink(0);
	for (uint8_t i = 0; i < board.size(); i++) {	
		//board[i].getDataRaw(); 
		delay(250);
	}
}

void boardTick() {
	static uint32_t tmr = 0;
	static uint32_t tmrGetSets = 0;
	static uint8_t denyDataRequest = 0;
	//sendDwinData();
	uint8_t boardsAmnt = board.size();
	if (millis() - tmr > 400 && !boardRequest) {
		
		
		uint32_t tmrDisconn = millis();
		for (uint8_t i = 0; i < board.size() && !denyDataRequest; i++) {
			board[i].tick();
		}
		denyDataRequest > 0 ? denyDataRequest-- : (denyDataRequest = 0);
		tmr = millis();
	} else if (boardRequest){
		denyDataRequest = 3;
		BoardRequest(boardRequest);
	}

	if (millis() - tmrGetSets > 5000) {
		//time(ui.getSystemTime());
		//Serial.println(time.encode());
		Serial.println();
		for (uint8_t i = 0; i < board.size() && !denyDataRequest; i++) board[i].getMainSets();
	}
}

void scanNewBoards() {

	Serial.println("Start scanning...");
	static uint8_t old_amount = 0; 
	static uint8_t counter = 0;
	counter++;
	Board::scanBoards(board, MAX_BOARDS);
	if (old_amount != board.size()) {
		webRefresh = true;
		old_amount = board.size();
	} 
	Serial.printf("Complete. Found: %d \n", board.size());
}

void WiFi_Init() {
	if (wifi_settings.staModeEn)
	{
		WiFi.mode(WIFI_STA);
		WiFi.begin(wifi_settings.staSsid, wifi_settings.staPass);
		int attemptCount = 0;
		while (WiFi.status() != WL_CONNECTED)
		{
			LED_blink(100);
			if (++attemptCount == 10)
			{
				LED_blink(0);
				wifi_settings.staModeEn = 0; // переключаемся на режим точки доступа
				memoryWIFI.updateNow();		 // сохраняемся
				ESP.restart();				 // перезапускаем есп
				return;
			}
			delay(1000);
		}
		Serial.println(WiFi.localIP());
	}
	// Иначе создаем свою сеть
	else
	{
		WiFi.mode(WIFI_AP);
		WiFi.softAP(wifi_settings.apSsid, wifi_settings.apPass);
		delay(1000);
		Serial.println(WiFi.softAPIP());
	}
}

void WiFi_tick() {
	memoryWIFI.tick();
	if (WiFi.status() == WL_CONNECTED && WiFi.getMode() == WIFI_STA) {
		LED_blink(100, 2000);
	} else if (WiFi.getMode() == WIFI_AP){
		LED_blink(1000);
	}
}

void BoardRequest(uint8_t &request) {
	if (!request) return;
	if (!board[activeBoard].isAnswer()) return;

	if (request < 10) {
		if (request == 3) {
			for (uint8_t i = 0; i < board.size(); i++) {
				delay(1);
				if (board[i].sendMainSets()) return;
			}
			requestResult = 1;
		} else if (request == 4) {
			uint8_t result = board[activeBoard].addSets.Switches[SW_OUTSIGN];
			if(board[activeBoard].sendCommand()) {
				board[activeBoard].addSets.Switches[SW_OUTSIGN] = !result;
			}
		}
		
	} else {
		uint8_t command = request / 10;
		uint8_t target = request % 10;
		if (!board[target].isAnswer()) return;
		if (command == 1)
		{
			if (!board[target].getMainSets()) requestResult = 1;	
		}
		else if (command == 2)
		{
			if(!board[target].sendMainSets()) requestResult = 1;
		}
		else if (command == 3)
		{
			if(!board[target].sendCommand(SW_REBOOT, 1)) requestResult = 1;
			
		}
		else if (command == 4)
		{
			while (board[target].sendCommand(SW_RSTST, 1));
		}
	}
	
	if (requestResult) webRefresh = true;
	request = 0;
}





















//-------------------------------------------------------------//