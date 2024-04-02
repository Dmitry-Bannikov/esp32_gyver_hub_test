#pragma once
//библиотека общения с платой
//
/*методы библиотеки
*!!!ВАЖНО!!! Ключи и команды должны соответствовать таким же ключам и командам в плате
*
*===Коды ошибок методов===
 0: Ошибок нет
 1: Объект не инициализирован
 2: Ошибка передачи запроса/данных на плату
 3: Таймаут ожидания приема
 4: Стартовый код не соответствует запросу
*
*/



#include <Arduino.h>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <nlohmann/json.hpp>


#define MAX						0
#define AVG						1
#define MIN						2

#define HEADER_DATA			0x10
#define HEADER_STATS		0x20
#define HEADER_MSETS		0x30
#define HEADER_ASETS		0x40
#define HEADER_SWITCH		0x50

#define XFER_WRITE	0x04
#define XFER_READ	0x03

#define RX_BUF_SIZE							70
#define TX_BUF_SIZE							70

#define SW_OUTSIGN	0
#define SW_REBOOT	1
#define SW_REGDIS	2
#define SW_RSTST	3
#define SW_TRANSIT	4


struct data {	//20
	volatile float Uin = 0;
	volatile float Uout = 0;
	volatile float Current = 0;
	volatile float Power = 0;
	int Events = 0;
	uint8_t structSize;
	uint8_t* buffer = nullptr;
	data() {
        structSize = offsetof(struct data, structSize);
	    buffer = new uint8_t[structSize];
    }
	void unpackData() {
        memcpy((uint8_t*)&Uin, buffer, structSize);
    }
	void packData() {
        memcpy(buffer, (uint8_t*)&Uin, structSize);
    }
};

struct stats {	//56
	int FlashCtrl;					//контрольный код
	float Uin[3] = { 0, 0, 300 };	//max,avg,min
	float Uout[3] = { 0, 0, 300 };
	float Current[3] = { 0, 0, 0 };
	float Power[3] = { 0, 0, 0 };
	int WorkTimeMins = 0;
	int Events = 0;
	uint8_t structSize;
	uint8_t* buffer = nullptr;
	stats() {
        structSize = offsetof(struct stats, structSize) - sizeof(FlashCtrl);
	    buffer = new uint8_t[structSize];
    }
	void unpackData() {
        memcpy((uint8_t*)Uin, buffer, structSize);
    }
	void packData() {
        memcpy(buffer, (uint8_t*)Uin, structSize);
    }

};

struct mainsets {	//32
	int16_t  FlashCtrl;				//контрольный код

	int16_t Liter = 78;				//0 буква платы
    int16_t IgnoreSetsFlag = 1;		//2 игнорировать настройки с платы (0...1)
	int16_t EnableTransit = 0;		//4 транзит при перегрузке
	int16_t MinVolt = 170;			//6 мин напряжение
	int16_t MaxVolt = 250;			//8 макс напряжение
	int16_t Hysteresis = 5;			//10 точность/гистерезис (1...11)
	int16_t Target = 220;			//12 целевое напряжение (210...240)
	int16_t TuneInVolt = 0;			//14 подстройка входа (-6...6)
	int16_t TuneOutVolt = 0;		//16 подстройка выхода (-6...6)
	int16_t TransRatioIndx = 5;		//18 коэффициент трансворматора тока (0...6) смотри addSets
	int16_t MotorType = 1;			//20 тип мотора (1...4) (0 - служебный)
	int16_t EmergencyTON = 2000;	//22 время включения после аварии
	int16_t EmergencyTOFF = 500;	//24 время аварийного отключения
	int16_t MaxCurrent = 35;		//26 макс ток платы
	int16_t CurrClbrtValue = 0;		//28 знач/100 - величина калибровочного тока
	int16_t	CurrClbrtKoeff = 100;	//30 знач/100 - калибровочный коэффициент

	uint8_t structSize;
	uint8_t *buffer = nullptr;
	mainsets() {
        structSize = offsetof(struct mainsets, structSize) - sizeof(FlashCtrl); //вычисляем размер структуры
	    buffer = new uint8_t[structSize];	
    }
	void packData() {
        memcpy(buffer, (uint8_t*)&Liter, structSize);
    }
	void unpackData() {
        memcpy((uint8_t*)&Liter, buffer, structSize);
    }
};

struct addsets {
	int16_t password = 1234;
	int16_t motKoefsList[5] = {0,30,60,90,120};			//(0 - служебный)коэффициент мощности мотора в % от motorDefPwr
	int16_t motorMaxCurrentList[4] = {3000, 4000, 5000, 6000};
	int16_t tcRatioList[7] = {25,40,50,60,80,100,150};		//список коэффициентов трансов
	int32_t SerialNumber[2] = {123456789, 123456};
	
	uint8_t structSize;
	uint8_t *buffer = nullptr;
	uint8_t Switches[8] = {0,0,0,0,0,0,0,0};
	
	addsets() {
		structSize = offsetof(struct addsets, structSize); //вычисляем размер структуры
		buffer = new uint8_t[structSize];//выделяем место под буфер
	}
	void packData() {
		memcpy(buffer, (uint8_t*)&password, structSize);
	}
	void unpackData() {
		memcpy((uint8_t*)&password, buffer, structSize);
	}
};


// =======================================================================================//
#define NUM_VALS 13
#define SETS_VALS 17

class Board
{
private:
	enum EventsFormat {
		EVENTS_FULL,
		EVENTS_SHORT
	};
	uint8_t _board_addr = 0;
	static const int _poll = 200;
	bool startFlag = false;
	uint32_t _dataUpdatePrd = 1000UL;		//период обновления значений
	uint32_t _statisUpdatePrd = 60000UL;	//период обновления статистики
	bool _active = false;
	uint8_t _disconnected = 0;
	String actTime;
	std::map<int, std::string> gEventsList;
	void 	validate();
	String errorsToStr(const int32_t errors, EventsFormat f);
	String getWorkTime(const uint32_t mins);
	
	
public:
	Board() {attach(0, 'N');}
	Board(const uint8_t addr) {attach(addr, 'N');};
	static int8_t StartI2C();		
	static int8_t StopI2C();
	bool 		attach(const uint8_t addr, const char Liter);								//подключить плату (указать адрес)
	static uint8_t isBoard(const uint8_t addr);
	static uint8_t scanBoards(std::vector<Board>&brd, const uint8_t max);
	bool 		isOnline();												//проверить, онлайн ли плата
	bool 		isAnswer();
	uint8_t 	getAddress() {return _board_addr;};						//получить адрес платы		

	uint8_t 	getData();		
	//=============================Получение данных с платы=========================================//										
	float 		readDataRaw(const uint8_t start_addr = 0, uint8_t vals_cnt = 5);		//получить данные с платы
	float 		readStatsRaw(const uint8_t start_addr = 0, uint8_t vals_cnt = 14);		//получить статистику с платы
	int16_t 	readMainSets(const uint8_t start_addr = 0, uint8_t vals_cnt = 16);		//получить настройки с платы
	int16_t 	readAddSets(const uint8_t start_addr = 0, uint8_t vals_cnt = 21);		//получить настройки с платы
	uint8_t 	readSwitches(const uint8_t start_addr = 0, uint8_t vals_cnt = 8);
	//=============================Отправка данных на плату========================================//	
	uint8_t 	sendMainSets(const uint8_t start_addr = 0, uint8_t vals_cnt = 14, int16_t value = INT16_MIN);	
	uint8_t 	sendAddSets( const uint8_t start_addr = 0, uint8_t vals_cnt = 21, int16_t value = INT16_MIN);
	uint8_t 	sendSwitches(const uint8_t start_addr = 0, uint8_t vals_cnt = 8, uint8_t value = 255);
	
	bool 		readAll();
	bool 		writeAll();
	void 		getDataStr(String & result);
	void 		getStatisStr(String & result);
	void 		getJsonData(std::string & result, uint8_t mode);
	uint8_t 	setJsonData(std::string input);
	void 		getMotTypesList(String &result, bool mode);
	void 		setMotKoefsList(String &str);
	void	 	getTcRatioList(String &result);
	uint8_t 	getNextActiveAlarm(std::string& result, uint32_t alarms);
	void 		setLiteral(char lit);
	char 		getLiteral();
	float 		getData(std::string request);
	void 		tick(const String time);
	void 		detach();
	~Board();
	
	data mainData;
	stats mainStats;
	mainsets mainSets;
	addsets addSets;
	struct board_data_t {
		std::string dataJson;
		std::string settingsJson;
		int32_t settings[SETS_VALS];
		float online[NUM_VALS];
		float min[NUM_VALS];
		float max[NUM_VALS];
		void getMinMax(bool set_zero = false);
	} Bdata;
	float CurrClbrtKoeff = 1.0;
	float CurrClbrtValue = 0;
};



