#pragma once

#include <TimeTicker.h>
#include <Board.h>

struct device {
	char Name[32] = "";                     //имя устройства
	char Type[20] = "";                     //тип устройства (стаб/мультиметр и тд)
	char OwnerEmail[32] = "";               // е-майл пользователя
	char Page[40] = "";                     //веб адрес устройства (присваивается сервером)
	char Status[10] = "";
	uint32_t SN = 0;                        //серийник устройства
	int IsActive = 0;                  //активно ли сейчас устройство (передает ли данные)

	void setParameters(const char* name, const char* type, const char* owner, const char* page, const char* status, uint32_t sn, int is_act) {
		strcpy(Name, name);
		strcpy(Type, type);
		strcpy(OwnerEmail, owner);
        strcpy(Page, page);
		strcpy(Status, status);
		SN = sn;
		IsActive = is_act;
	}
};

#define MAX_BOARDS	3


void LED_blink(uint16_t period_on, uint16_t period_off = 0);

void Devices_Init();
void Devices_Tick();
void Devices_Save();

int findDeviceIndxFromSN(uint32_t sn);
void CreateDevices(const char * json);
void UpdateDevice(const String &name, const String &type, const String &owner = "", const String &page = "", const String &status = "", uint32_t serial_n = 0, int is_active = 0);
void DeleteDevice(uint32_t sn);
void DeleteDevice(uint8_t indx);


extern std::vector<Board> board;					//объекты плат
extern std::vector<device> Devices;
extern uint8_t activeBoard;
extern bool mqttConnected;
extern bool webRefresh;  
extern uint8_t boardRequest; //запрос на плату
extern uint8_t requestResult;
extern uint32_t Board_SN;
extern char OwnerEmail[32];