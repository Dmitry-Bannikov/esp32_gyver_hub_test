#include "Arduino.h"
#include "service.h"
#include "common_data.h"
#include "netconnection.h"
#include "mqtthandler.h"
#include "webinterface.h"






void System_Init() {
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
}

void Web_Init() {
	WifiInit();
	MqttInit();
	portalInit();
	uint8_t mac[6];
	WiFi.macAddress(mac);
	memcpy(&Board_SN, mac, 4);
	Serial.printf("\nStab SN: %d \n", Board_SN);
	Serial.println(WiFi.macAddress());
}




void Board_Tick() {
	static uint32_t tmr = 0, scanTmr = 0;
	static uint8_t denyDataRequest = 0;
	uint8_t boardsAmnt = board.size();
	if (millis() - tmr > 1000 && !boardRequest) {
		for (uint8_t i = 0; i < board.size() && !denyDataRequest; i++) {
			t = ui.getSystemTime();
			board[i].tick(t.encode());
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
		}
		scanNewBoards();
		scanTmr = millis();
	}
}

void System_Tick() {

}

void Web_Tick() {
	wifi_tick();
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
				if (board[i].sendMainSets()) return;
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