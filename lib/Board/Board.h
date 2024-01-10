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

#define MAX						0
#define AVG						1
#define MIN						2

#define I2C_DATA_START						0x30
#define I2C_MAINSETS_START					0x35
#define I2C_ADDSETS_START					0x40
#define I2C_STAT_START						0x45
#define I2C_SWITCHES_START					0x46

#define I2C_REQUEST_MAINSETS				0x21
#define I2C_REQUEST_ADDSETS					0x22
#define I2C_REQUEST_DATA					0x23
#define I2C_REQUEST_STAT					0x24
#define I2C_REQUEST_SWITCHES				0x25

#define RX_BUF_SIZE							100
#define TX_BUF_SIZE							100

#define SW_OUTSIGN	0
#define SW_REBOOT	1
#define SW_REGDIS	2
#define SW_RSTST	3
#define SW_TRANSIT	4


struct data {
	int16_t 	Uin;
	int16_t 	Uout;
	float 		Current;
	float 		Power;
	float 		cosfi;
	uint32_t 	events;
	uint8_t 	structSize;
	String 		Str;
	uint8_t* 	buffer = nullptr;
	data() {
		structSize = offsetof(struct data, structSize);
		buffer = new uint8_t[structSize];
		cosfi = 1.0;
		Uin = 0;
		Uout = 0;
		Current = 0;
		Power = 0;
		events = 0;
	}
	void packData() {
		memcpy(buffer, (uint8_t*)&Uin, structSize);
	}
	void unpackData() {
		memcpy((uint8_t*) &Uin, buffer, structSize);
	}
};

struct stats {
	uint32_t WorkTimeMins = 0;
	uint32_t Events = 0;
	int16_t Uin[3] 		= {0,0,300};	//max,avg,min
	int16_t Uout[3] 	= {0,0,300};
	float 	Current[3] 	= {0,0,0};
	float	Power[3] 	= {0,0,0};

	uint8_t 	structSize;
	String  	Str;
	uint8_t* buffer = nullptr;
	stats() {
		structSize = offsetof(struct stats, structSize);
		buffer = new uint8_t[structSize];
	}
	void packData() {
		memcpy(buffer, (uint8_t*)&WorkTimeMins, structSize);
	}
	void unpackData() {
		memcpy((uint8_t*) &WorkTimeMins, buffer, structSize);
	}
};

//----------------------BOARD MAIN SETS----------------------//
struct mainsets {
	uint8_t IgnoreSetsFlag = 0;		//игнорировать настройки с платы (0...1)
	uint8_t EnableTransit = 0;	//транзит при перегрузке
	uint8_t Hysteresis = 3;			//точность/гистерезис (1...6)
	uint8_t Target = 222;			//целевое напряжение (210...240) 
	uint8_t MotorType = 2;			//тип мотора (1...4)
	uint8_t TransRatioIndx = 3;		//коэффициент трансворматора тока (0...6) смотри addSets
	uint8_t	MaxCurrent = 30;
	int8_t TuneInVolt = 0;			//подстройка входа (-6...6)
	int8_t TuneOutVolt = 0;		    //подстройка выхода (-6...6)
	char	Liter = 'N';
	int16_t MinVolt = 170;			//мин напряжение
	int16_t MaxVolt = 250;			//макс напряжение
	int16_t EmergencyTOFF = 500;	//время аварийного отключения
	int16_t EmergencyTON = 2000;	//время включения после аварии
	
	uint8_t structSize;
	uint8_t *buffer = nullptr;
	mainsets() {
		structSize = offsetof(struct mainsets, structSize); //вычисляем размер структуры
		buffer = new uint8_t[structSize];					//выделяем место под буфер
	}
	void packData() {
		memcpy(buffer, (uint8_t*) &IgnoreSetsFlag, structSize);
	}
	void unpackData() {
		memcpy((uint8_t*) &IgnoreSetsFlag, buffer, structSize);
	}
};

struct addsets {
	int16_t password = 1234;
	int16_t motKoefsList[5] = {};			//коэффициент мощности мотора в % от motorDefPwr
	int16_t motorMaxCurrentList[4] = {}; //список макс токов на каждый из типов моторов
	int16_t tcRatioList[6] = {};		//список коэффициентов трансов
	int32_t SerialNumber[2] = {};
	uint8_t structSize;
	uint8_t Switches[8] = {0,0,0,0,0,0,0,0};
	uint8_t *buffer = nullptr;
	addsets() {
		structSize = offsetof(struct addsets, structSize); //вычисляем размер структуры
		buffer = new uint8_t[structSize];//выделяем место под буфер
		packData();
	}
	void packData() {
		memcpy(buffer, (uint8_t*)&password, structSize);
	}
	void unpackData() {
		memcpy((uint8_t*)&password, buffer, structSize);
	}
};




// =======================================================================================//


class Board
{
private:
	enum EventsFormat {
		EVENTS_FULL,
		EVENTS_SHORT
	};
	String PROGMEM gEventsList[32] = {
	"Нет",
	"Блок мотора", 
	"Тревога-1",
	"Тревога-2",
	"<80 В",
	"Недо-напряжение",
	"Перенапряжение",
	"Мин. напряжение",
	"Мак. напряжение",
	"Транзит",
	"Перегрузка",
	"Внеш. сигнал"
	};
	uint8_t _txbuffer[TX_BUF_SIZE];
	uint8_t _rxbuffer[RX_BUF_SIZE];
	uint8_t _board_addr = 0;
	static const int _poll = 200;
	bool startFlag = false;
	uint32_t _dataUpdatePrd = 1000UL;		//период обновления значений
	uint32_t _statisUpdatePrd = 60000UL;	//период обновления статистики
	bool _active = false;
	uint8_t _disconnected = 0;
	void 	validate();
	String errorsToStr(const int32_t errors, EventsFormat f);
	String getWorkTime(const uint32_t mins);
	
public:
	Board() {attach(0);}
	Board(const uint8_t addr) {attach(addr);};
	static int8_t StartI2C();		
	static int8_t StopI2C();
	bool 		attach(const uint8_t addr);								//подключить плату (указать адрес)
	static uint8_t isBoard(const uint8_t addr);
	static uint8_t scanBoards(std::vector<Board>&brd, const uint8_t max);
	bool 		isOnline();												//проверить, онлайн ли плата
	bool 		isAnswer();
	uint8_t 	getAddress() {return _board_addr;};						//получить адрес платы		

	uint8_t 	getData();												//получить данные с платы
	uint8_t 	getDataRaw();
	uint8_t 	getMainSets();											//получить настройки с платы
	uint8_t 	sendMainSets(uint8_t attempts = 0);						//отправить настройки
	uint8_t 	sendCommand(uint8_t command, uint8_t value);
	uint8_t 	sendCommand(uint8_t* command);
	uint8_t 	sendCommand();
	void 		getDataStr();
	void 		getStatisStr();
	void 		createJsonData(String& result, uint8_t mode);
	void 		getMotTypesList(String &result, bool mode);
	void 		setMotKoefsList(String &str);
	void	 	getTcRatioList(String &result);
	void 		setLiteral(char lit);
	char 		getLiteral();
	void 		tick();
	void 		detach();
	~Board();

	data mainData;
	stats mainStats;
	mainsets mainSets;
	addsets addSets;
	

};



