#include "Arduino.h"
#include "service.h"
#include "common_data.h"
#include "netconnection.h"
#include "mqtthandler.h"
#include "webinterface.h"






void System_Init() {
	uint8_t mac[6];
	WiFi.macAddress(mac);
	memcpy(&Board_SN, mac, 4);
	Board_SN += *(uint16_t*)(mac+4);
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(21, INPUT);
	pinMode(22, INPUT);
	Serial.begin(115200);
	Serial.printf("\nProram Started! \rI2C pins state: %d | %d\n", digitalRead(21), digitalRead(22));
}

void Board_Init() {
	Board::StartI2C();
	board.reserve(MAX_BOARDS);
	delay(10);
	scanNewBoards();
	for (uint8_t i = 0; i < board.size(); i++) {
		board[i].readAll();
		board[i].CurrClbrtKoeff = (float)board[i].mainSets.CurrClbrtKoeff/100.0;
	}
}

void Web_Init() {
	WiFi_Init();
	portalInit();
	MqttInit();
	Serial.printf("\nStab SN: %d \n", Board_SN);
}




void Board_Tick() {
	static uint32_t tmr = 0, scanTmr = 0;
	static uint8_t denyDataRequest = 0;
	uint8_t boardsAmnt = board.size();
	if (millis() - tmr > 1000 && !boardRequest) {
		for (uint8_t i = 0; i < board.size() && !denyDataRequest; i++) {
			board[i].tick("");
		}
		denyDataRequest > 0 ? denyDataRequest-- : (denyDataRequest = 0);
		tmr = millis();
	} else if (boardRequest && board.size()){
		denyDataRequest = 3;
		BoardRequest(boardRequest);
	}

	if (millis() - scanTmr > 60000) {
		for (uint8_t i = 0; i < board.size() && !denyDataRequest; i++) {
			board[i].readAll();
			board[i].CurrClbrtKoeff = board[i].mainSets.CurrClbrtKoeff/100.0;
		}
		scanNewBoards();
		scanTmr = millis();
	}
}

void System_Tick() {
	//MemoryTick();
}

void Web_Tick() {
	WiFi_Tick();
	Mqtt_tick();
	portalTick();
}

void scanNewBoards() {
	static uint8_t old_amount = 0; 
	Board::scanBoards(board, MAX_BOARDS);
	if (old_amount != board.size()) {
		webRefresh = true;
		old_amount = board.size();
	}
}

void BoardRequest(uint8_t &request) {
	if (!request) return;
	if (!board[activeBoard].isAnswer()) return;
	//uint8_t requestResult = 0;
	if (request < 10) {
		if (request == 2) scanNewBoards();
		if (request == 3) {
			for (uint8_t i = 0; i < board.size(); i++) {
				delay(1);
				if (board[i].sendMainSets(0, 1, board[i].mainSets.Liter)) return;
			}
			requestResult = 1;
		} else if (request == 4) {
			uint8_t result = board[activeBoard].addSets.Switches[SW_OUTSIGN];
			if(board[activeBoard].sendSwitches()) {
				board[activeBoard].addSets.Switches[SW_OUTSIGN] = !result;
			}
		}
		request = 0;
		
	} else {
		uint8_t command = request / 10;
		uint8_t target = request % 10;
		if (!board[target].isOnline()) return;
		if (command == 1) {
			if (board[target].readAll()) {
				requestResult = 1;
				request = 0;
			}
		}
		else if (command == 2) {
			delay(100);
			if(!board[target].sendMainSets() && !board[target].sendAddSets()) {
				requestResult = 1;
				request = 0;
			}
		}
		else if (command == 3) {
			if(!board[target].sendSwitches(SW_REBOOT, 1, 1)) {
				delay(250);
				requestResult = 1;
				request = 0;
			}
		}
		else if (command == 4) {
			board[target].sendSwitches(SW_RSTST, 1, 1);
			request = 0;
		}
	}
	
	if (requestResult) webRefresh = true;
}


//===========================================