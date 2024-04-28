#pragma once

#include <GyverPortal.h>
#include <TimeTicker.h>
#include <Board.h>

struct device {
	char Name[32] = "";                     //имя устройства
	char OwnerEmail[32] = "";               // е-майл пользователя
	char Type[20] = "";                     //тип устройства (стаб/мультиметр и тд)
	char Page[40] = "";                     //веб адрес устройства (присваивается сервером)
	uint32_t SN = 0;                        //серийник устройства
	uint32_t ID = 0;                        //уникальный номер (присваивается сервером)
	uint32_t IsActive = 0;                  //активнг ли сейчас устройство (передает ли данные)
	bool Reg = false;
	void setParameters(const char* name, const char* owner, const char* type, const char* page, uint32_t sn, uint32_t id, uint32_t is_act, bool reg) {
		strcpy(Name, name);
		strcpy(OwnerEmail, owner);
		strcpy(Type, type);
        strcpy(Page, page);
		SN = sn;
		ID = id;
		IsActive = is_act;
        Reg = reg;
	}
};

extern std::vector<device> Devices;

#define MAX_BOARDS	3
extern GyverPortal ui;
extern GPtime t;
extern std::vector<Board> board;					//объекты плат

extern uint8_t activeBoard;
extern bool mqttConnected;
extern bool webRefresh;  
extern uint8_t boardRequest; //запрос на плату
extern uint8_t requestResult;
extern uint32_t Board_SN;

void LED_blink(uint16_t period_on, uint16_t period_off = 0);
int findDeviceIndxFromID(uint32_t id);
void createDevicesList(const char * json);
void UpdateDevice(const char* name, const char* owner, const char* type, const char* page, uint32_t serial_n, uint32_t id, int is_active, bool reg);
void DeleteDevice(uint32_t id);
