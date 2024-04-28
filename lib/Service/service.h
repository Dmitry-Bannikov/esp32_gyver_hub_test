#pragma once



//#include "common_data.h"
#include "netconnection.h"
#include "mqtthandler.h"
#include "webinterface.h"

void System_Init();
void Board_Init();
void Web_Init();


void Board_Tick();
void System_Tick();
void Web_Tick();

void scanNewBoards();
void BoardRequest(uint8_t &request);