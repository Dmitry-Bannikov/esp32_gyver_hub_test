#include <Board.h>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include <esp_err.h>

#define isEvent(event)					(bitRead(mainData.Events, event))
#define json	nlohmann::json


const char* jsonDataNames[NUM_VALS] = {
		"Uin" , "Uout" , "I" , "P" , 
		"Uin_avg" , "Uout_avg" , "I_avg" , "P_avg" ,
		"Uin_max" , "Uout_max" , "I_max" , "P_max" ,
		"work_h"
};

const char* jsonSetsNames[SETS_VALS] = {
		"Transit", "Uout_minoff", "Uout_maxoff",
		"Accuracy", "Target", "Uin_tune", "Uout_tune",
		"T_5", "M_type" , "Time_on" , "Time_off" ,
		"Password", "SN_1" , "SN_2" ,
		"Rst_max", "Save", "Outsignal"
};


//==================Public=================//



int8_t Board::StartI2C() {
	i2c_config_t config = { };
    config.mode = I2C_MODE_MASTER;
    config.sda_io_num = 21;
    config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    config.scl_io_num = 22;
    config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    config.master.clk_speed = 100000; 
	config.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;
	if (i2c_param_config(I2C_NUM_0, &config) != ESP_OK) return 1;
    if (i2c_driver_install(I2C_NUM_0, config.mode, 100, 0, 0) != ESP_OK) return 2;
	i2c_set_timeout(I2C_NUM_0, 0xFFFF);
	return 0;
}

int8_t Board::StopI2C() {
    if (i2c_driver_delete(I2C_NUM_0)) return 1;
    gpio_reset_pin(GPIO_NUM_21);
    gpio_reset_pin(GPIO_NUM_22);
	gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL<<21) | (1ULL<<22);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    if (gpio_config(&io_conf)) return 2;
	return 0;
}


bool Board::attach(const uint8_t addr, const char Liter) {
	if (addr == 0 || addr > 127) return false;
	if (addr != _board_addr)
		_board_addr = addr;
	startFlag = true;
	mainSets.Liter = Liter;
	gEventsList[0] = " ";
	gEventsList[1] = "Блокировка мотора";
	gEventsList[2] = "Термостат";
	gEventsList[3] = "Доп. контакт";
	gEventsList[4] = "Нет сети";
	gEventsList[5] = "Низкое напряжение";
	gEventsList[6] = "Высокое напряжение";
	gEventsList[7] = "Крайнее мин. напряжение";
	gEventsList[8] = "Крайнее макс. напряжение";
	gEventsList[9] = "Режим транзит";
	gEventsList[10] = "Перегрузка";
	gEventsList[11] = "Внешний сигнал";
	gEventsList[12] = "Выход отключен";
	Bdata.getMinMax(true);
	return isOnline();
}

uint8_t Board::isBoard(uint8_t addr) {
	int16_t Liter = 0;
    const uint8_t vals_cnt = 1;
    uint8_t byte_cnt = vals_cnt*2;
    uint8_t txBuffer[4] = {HEADER_MSETS, XFER_READ, 0, byte_cnt};
    uint8_t* rxBuffer = new uint8_t[byte_cnt + 1];
	esp_err_t ret = i2c_master_write_read_device(0, addr, txBuffer, sizeof(txBuffer), rxBuffer, byte_cnt + 1, pdMS_TO_TICKS(100));
	if (rxBuffer[0] == HEADER_MSETS && ret == 0) {
		int16_t tempLiter = *(int16_t*)(rxBuffer+1);
		if (tempLiter > 64 && tempLiter < 79) Liter = tempLiter; 
		Serial.printf("Address: %d, Liter: %c \n", addr, (char)Liter);
	}
	delete(rxBuffer);
    return (uint8_t)Liter;
}

bool Board::isOnline() {
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_board_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t error = i2c_master_cmd_begin(0, cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);
	if (error) {
		return false;
	}
	return true;							
}

bool Board::isAnswer() {
	if (_disconnected) return false;
	return true;
}

float Board::readDataRaw(const uint8_t start_addr, uint8_t vals_cnt) {
	vals_cnt = constrain(vals_cnt, 1, mainData.structSize/4 - start_addr);
	mainData.packData();
    float result = NAN;
    uint8_t byte_cnt = vals_cnt*4;
    uint8_t txBuffer[4] = {HEADER_DATA, XFER_READ, start_addr, byte_cnt};
    uint8_t* rxBuffer = new uint8_t[byte_cnt + 1];
	esp_err_t ret = i2c_master_write_read_device(0, _board_addr, txBuffer, sizeof(txBuffer), rxBuffer, byte_cnt + 1, pdMS_TO_TICKS(100));
	if (rxBuffer[0] == HEADER_DATA) {
        result = *(float*)(rxBuffer+1);
		memcpy(mainData.buffer + start_addr*4, rxBuffer + 1, byte_cnt);
		mainData.unpackData();
	}
	delete(rxBuffer);
	return result;
}

float Board::readStatsRaw(const uint8_t start_addr, uint8_t vals_cnt) {
    vals_cnt = constrain(vals_cnt, 1, mainStats.structSize/4 - start_addr);
	mainStats.packData();
    float result = NAN;
    uint8_t byte_cnt = vals_cnt*4;
    uint8_t txBuffer[4] = {HEADER_STATS, XFER_READ, start_addr, byte_cnt};
    uint8_t* rxBuffer = new uint8_t[byte_cnt + 1];
	esp_err_t ret = i2c_master_write_read_device(0, _board_addr, txBuffer, sizeof(txBuffer), rxBuffer, byte_cnt + 1, pdMS_TO_TICKS(100));
	if (rxBuffer[0] == HEADER_STATS) {
        result = *(float*)(rxBuffer+1);
		memcpy(mainStats.buffer + start_addr*4, rxBuffer + 1, byte_cnt);
		mainStats.unpackData();
	}
	delete(rxBuffer);
	return result;
}

int16_t Board::readMainSets(const uint8_t start_addr, uint8_t vals_cnt) {
	vals_cnt = constrain(vals_cnt, 1, mainSets.structSize/2 - start_addr);
    int16_t result = INT16_MIN;
	mainSets.packData();
    uint8_t byte_cnt = vals_cnt*2;
    uint8_t txBuffer[4] = {HEADER_MSETS, XFER_READ, start_addr, byte_cnt};
    uint8_t* rxBuffer = new uint8_t[byte_cnt + 1];
	esp_err_t ret = i2c_master_write_read_device(0, _board_addr, txBuffer, sizeof(txBuffer), rxBuffer, byte_cnt + 1, pdMS_TO_TICKS(100));
	if (rxBuffer[0] == HEADER_MSETS) {
        result = *(int16_t*)(rxBuffer+1);
		memcpy(mainSets.buffer + start_addr*2, rxBuffer + 1, byte_cnt);
		mainSets.unpackData();
	}
	delete(rxBuffer);
	return result;
}

int16_t Board::readAddSets(const uint8_t start_addr, uint8_t vals_cnt) {
	vals_cnt = constrain(vals_cnt, 1, addSets.structSize/2 - start_addr);
    int16_t result = INT16_MIN;
	addSets.packData();
    uint8_t byte_cnt = vals_cnt*2;
    uint8_t txBuffer[4] = {HEADER_ASETS, XFER_READ, start_addr, byte_cnt};
    uint8_t* rxBuffer = new uint8_t[byte_cnt + 1];
	esp_err_t ret = i2c_master_write_read_device(0, _board_addr, txBuffer, sizeof(txBuffer), rxBuffer, byte_cnt + 1, pdMS_TO_TICKS(100));
	if (rxBuffer[0] == HEADER_ASETS) {
        result = *(int16_t*)(rxBuffer+1);
		memcpy(addSets.buffer + start_addr*2, rxBuffer + 1, byte_cnt);
		addSets.unpackData();
	}
	delete(rxBuffer);
	return result;
}

uint8_t Board::readSwitches(const uint8_t start_addr, uint8_t vals_cnt) {
	vals_cnt = constrain(vals_cnt, 1, sizeof(addSets.Switches) - start_addr);
	uint8_t result = 255;
    uint8_t byte_cnt = vals_cnt*1;
    uint8_t txBuffer[4] = {HEADER_SWITCH, XFER_READ, start_addr, byte_cnt};
    uint8_t* rxBuffer = new uint8_t[byte_cnt + 1];
	esp_err_t ret = i2c_master_write_read_device(0, _board_addr, txBuffer, sizeof(txBuffer), rxBuffer, byte_cnt + 1, pdMS_TO_TICKS(100));
	if (rxBuffer[0] == HEADER_SWITCH) {
		result = *(uint8_t*)(rxBuffer+1);
		memcpy(addSets.Switches + start_addr*1, rxBuffer + 1, byte_cnt);
	}
	delete(rxBuffer);
	return result;
}

uint8_t Board::getData() {
	if (!startFlag) return 1;
	static uint8_t disconn = 0;
	static uint32_t last_update = 0;
	float result = readDataRaw();
	if (millis() - last_update > 5000) {
		result = readStatsRaw();
		last_update = millis();
	}
	if (result == NAN) {
		(disconn < 10) ? (disconn++) : (disconn = 10, _disconnected = 1);
	} else {
		disconn = 0;
		_disconnected = 0;
	}
	Bdata.settings[0] = mainSets.EnableTransit; Bdata.settings[1] = mainSets.MinVolt; Bdata.settings[2] = mainSets.MaxVolt; 
	Bdata.settings[3] = mainSets.Hysteresis; Bdata.settings[4] = mainSets.Target; Bdata.settings[5] = mainSets.TuneInVolt; 
	Bdata.settings[6] = mainSets.TuneOutVolt; Bdata.settings[7] = addSets.tcRatioList[mainSets.TransRatioIndx]; Bdata.settings[8] = mainSets.MotorType; 
	Bdata.settings[9] = mainSets.EmergencyTON; Bdata.settings[10] = mainSets.EmergencyTOFF; Bdata.settings[11] = addSets.password; 
	Bdata.settings[12] = addSets.SerialNumber[0]; Bdata.settings[13] = addSets.SerialNumber[1]; Bdata.settings[14] = 0; 
	Bdata.settings[15] = 0; Bdata.settings[16] = addSets.Switches[SW_OUTSIGN];

	Bdata.online[0] = mainData.Uin; Bdata.online[1] = mainData.Uout; Bdata.online[2] = mainData.Current; 
	Bdata.online[3] = mainData.Power/1000.0; Bdata.online[4] = mainStats.Uin[1]; Bdata.online[5] = mainStats.Uout[1]; 
	Bdata.online[6] = mainStats.Current[1]; Bdata.online[7] = mainStats.Power[1]/1000.0; Bdata.online[8] = mainStats.Uin[0]; 
	Bdata.online[9] = mainStats.Uout[0]; Bdata.online[10] = mainStats.Current[0]; Bdata.online[11] = mainStats.Power[0]/1000.0; 
	Bdata.online[12] = mainStats.WorkTimeMins/60; 
	return _disconnected;
}

uint8_t Board::sendMainSets(const uint8_t start_addr, uint8_t vals_cnt, int16_t value) {
	validate();
	vals_cnt = constrain(vals_cnt, 1, mainSets.structSize/2 - start_addr);
	mainSets.packData();
    uint8_t byte_cnt = vals_cnt*2;
    uint8_t* txBuffer = new uint8_t[byte_cnt + 4];
    txBuffer[0] = HEADER_MSETS; txBuffer[1] = XFER_WRITE; txBuffer[2] = start_addr; txBuffer[3] = byte_cnt;
    if (vals_cnt == 1 && value != INT16_MIN) {
        memcpy(txBuffer + 4, &value, 2);
    } else {
        memcpy(txBuffer + 4, mainSets.buffer + start_addr*2, byte_cnt);
    }
	esp_err_t ret = i2c_master_write_to_device(0, _board_addr, txBuffer, byte_cnt + 4, pdMS_TO_TICKS(100));
	delete(txBuffer);
	return ret;
}

uint8_t Board::sendAddSets(const uint8_t start_addr, uint8_t vals_cnt, int16_t value) {
	validate();
	vals_cnt = constrain(vals_cnt, 1, addSets.structSize/2 - start_addr);
	addSets.packData();
    uint8_t byte_cnt = vals_cnt*2;
    uint8_t* txBuffer = new uint8_t[byte_cnt + 4];
    txBuffer[0] = HEADER_ASETS; txBuffer[1] = XFER_WRITE; txBuffer[2] = start_addr; txBuffer[3] = byte_cnt;
    if (vals_cnt == 1 && value != INT16_MIN) {
        memcpy(txBuffer + 4, &value, 2);
    } else {
        memcpy(txBuffer + 4, addSets.buffer + start_addr*2, byte_cnt);
    }
	esp_err_t ret = i2c_master_write_to_device(0, _board_addr, txBuffer, byte_cnt + 4, pdMS_TO_TICKS(100));
	delete(txBuffer);
	return ret;
}

uint8_t Board::sendSwitches(const uint8_t start_addr, uint8_t vals_cnt, uint8_t value) {
	vals_cnt = constrain(vals_cnt, 1, sizeof(addSets.Switches) - start_addr);
    uint8_t byte_cnt = vals_cnt*1;
    uint8_t* txBuffer = new uint8_t[byte_cnt + 4];
    txBuffer[0] = HEADER_SWITCH; txBuffer[1] = XFER_WRITE; txBuffer[2] = start_addr; txBuffer[3] = byte_cnt;
	if (vals_cnt == 1 && value != 255) {
		memcpy(txBuffer + 4, &value, 1);
	} else {
		memcpy(txBuffer + 4, addSets.Switches + start_addr*1, byte_cnt);
	}
	esp_err_t ret = i2c_master_write_to_device(0, _board_addr, txBuffer, byte_cnt + 4, pdMS_TO_TICKS(100));
	delete(txBuffer);
	return ret;
}

bool Board::readAll() {
	if (
		readMainSets() != INT16_MIN &&
		readAddSets() != INT16_MIN  &&
		readSwitches() != 255
	) return true;
	return false;
}

bool Board::writeAll() {
	if (
		sendMainSets() == 0 &&
		sendAddSets() == 0  &&
		sendSwitches() == 0
	) return true;
	return false;
}

void Board::getDataStr(String & result) {
	float full_pwr = mainData.Power/1000.0;
	String s = "";
	char data[500];
	sprintf(data, 
	" Данные "
	"\nU вход  | %d В"
	"\nU выход | %d В"
	"\nТок     | %.1f A"
	"\nP полная| %.1f кВА"
	,
	(int)mainData.Uin, (int)mainData.Uout,
	mainData.Current, full_pwr
	);
	s += String(data);
	s += F("\nСобытия | ");
	s += errorsToStr(mainData.Events, EVENTS_FULL);
	result = s;
}

void Board::getStatisStr(String & result) {
	float maxPwr = mainStats.Power[MAX]/1000.0;
	float avgPwr = mainStats.Power[AVG]/1000.0;
	String s = "";
	s += F(" Cтатистика ");
	s += F("\nt работы:");
	s += getWorkTime(mainStats.WorkTimeMins);
/*
U вход   |	220	220	220
U выход  | 
Вых. Ток |	
Мощность |
События  | A01, A03, A04
*/
	char statis[500];
	sprintf(statis,
	"\n_____| max avg min"
	"\nUin  | %d %d %d "
	"\nUout | %d %d %d "
	"\nI    | %.1f %.1f "
	"\nP    | %.1f %.1f ", 
	(int)mainStats.Uin[MAX], (int)mainStats.Uin[AVG], (int)mainStats.Uin[MIN],
	(int)mainStats.Uout[MAX], (int)mainStats.Uout[AVG], (int)mainStats.Uout[MIN],
	mainStats.Current[MAX], mainStats.Current[AVG],
	maxPwr, avgPwr
	);
	s += String(statis);
	s += F("\nСобытия: ");
	s += errorsToStr(mainStats.Events, EVENTS_SHORT);
	result = s;
}


float Board::getData(std::string request) {
	json jdata = json::parse(Bdata.dataJson);
	json jsets = json::parse(Bdata.settingsJson);
	for (auto it = jdata.begin(); it != jdata.end(); ++it) {
		if (it.key() == request) return it.value();
	}
	for (auto it = jsets.begin(); it != jsets.end(); ++it) {
		if (it.key() == request) return it.value();
	}
	return nan("");
}

void Board::getJsonData(std::string & result, uint8_t mode) {
	std::stringstream ss;
	ss << "{\"Fase\":\"" << getLiteral() << "\",";
	if (mode < 3) {
		bool first = true;
		for (uint8_t i = 0; i < NUM_VALS; i++) {
			if (!first) ss << ",";
			if (mode==0) {
				ss << "\"" << jsonDataNames[i] << "\":\"" << round(Bdata.online[i]*10)/10 << "\"";
			} else if (mode==1) {
				ss << "\"" << jsonDataNames[i] << "\":\"" << round(Bdata.min[i]*10)/10 << "\"";
			} else {
				ss << "\"" << jsonDataNames[i] << "\":\"" << round(Bdata.max[i]*10)/10 << "\"";
			}
			first = false;
		}
		ss << "}";
		//Bdata.dataJson = ss.str();
	} else if (mode == 4) {
		bool first = true;
		for (uint8_t i = 0; i < SETS_VALS; i++) {
			if (!first) ss << ",";
			ss << "\"" << jsonSetsNames[i] << "\":\"" << std::fixed << Bdata.settings[i] << "\"";
			first = false;
		}
		ss << "}";
		//Bdata.settingsJson = ss.str();
	}
	
	result = ss.str();
}

uint8_t Board::setJsonData(std::string input) {
	json j = json::parse(input);

	for (const auto& pair : j.items()) {
        for (int i = 0; i < SETS_VALS; ++i) {
            if (pair.key() == jsonSetsNames[i]) {
				Bdata.settings[i] = std::stof(pair.value().get<std::string>());
            }
        }
    }
	int16_t tcRatio = 0;
	int isNeedSave = 0;
	int isNeedRstMax = 0;
	int isNeedOutsign = 0;
	mainSets.EnableTransit = Bdata.settings[0]; mainSets.MinVolt = Bdata.settings[1]; mainSets.MaxVolt = Bdata.settings[2]; 
	mainSets.Hysteresis = Bdata.settings[3]; mainSets.Target = Bdata.settings[4]; mainSets.TuneInVolt = Bdata.settings[5]; 
	mainSets.TuneOutVolt = Bdata.settings[6]; addSets.tcRatioList[mainSets.TransRatioIndx] = Bdata.settings[7]; mainSets.MotorType = Bdata.settings[8]; 
	mainSets.EmergencyTON = Bdata.settings[9]; mainSets.EmergencyTOFF = Bdata.settings[10]; addSets.password = Bdata.settings[11]; 
	addSets.SerialNumber[0] = Bdata.settings[12]; addSets.SerialNumber[1] = Bdata.settings[13]; 
	isNeedRstMax = Bdata.settings[14]; isNeedSave = Bdata.settings[15]; addSets.Switches[SW_OUTSIGN] = Bdata.settings[16];
	for (uint8_t i = 0; i < sizeof(addSets.tcRatioList) / sizeof(addSets.tcRatioList[0]); i++) {
		if (tcRatio == addSets.tcRatioList[i]) {
			mainSets.TransRatioIndx = i;
			break; 
		}
	}
	validate();
    sendSwitches(SW_RSTST, 1, isNeedRstMax);
	sendSwitches(SW_OUTSIGN, 1, isNeedOutsign);
	if (isNeedSave) sendMainSets();
	return 0;
}

void Board::tick(const String time) {
	actTime = time;
	getData();
}

void Board::detach() {
	if (!startFlag) return;
	_board_addr = 0;
	startFlag = false;
}

char Board::getLiteral() {
	return mainSets.Liter;
}

void Board::setLiteral(char lit) {
	mainSets.Liter = lit;
}

void Board::getMotTypesList(String &result, bool mode) {
	result = "";
	if (mode == true) {
		for (uint8_t i = 1; i < sizeof(addSets.motKoefsList) / sizeof(addSets.motKoefsList[0]); i++) {
			result += String(i) + "(";
			result += String(addSets.motKoefsList[i]);
			result += "),";
		}
		if (result.length()) {
			result.remove(result.length() - 1);
		}
	} else {
		for (uint8_t i = 1; i < sizeof(addSets.motKoefsList) / sizeof(addSets.motKoefsList[0]); i++){
			result += String(addSets.motKoefsList[i]);
			result += String(",");
		}
		if (result.length()) {
			result.remove(result.length() - 1);
		}
	}
	
}

void Board::setMotKoefsList(String &str) {
	char strArr[20];
	int array[5];
	str.toCharArray(strArr, sizeof(str));
	sscanf(strArr, "%d,%d,%d,%d", 
	&array[1],&array[2],&array[3],&array[4]);
	Serial.printf("\n%d %d %d %d", array[1],array[2],array[3],array[4]);
	addSets.motKoefsList[1] = constrain(array[1], 0, 300);
	addSets.motKoefsList[2] = constrain(array[2], 0, 300);
	addSets.motKoefsList[3] = constrain(array[3], 0, 300);
	addSets.motKoefsList[4] = constrain(array[4], 0, 300);
}

void Board::getTcRatioList(String &result) {
	result = "";
	for (uint8_t i = 0; i < sizeof(addSets.tcRatioList)/sizeof(addSets.tcRatioList[0]); i++) {
		result += String(addSets.tcRatioList[i]);
		result += String(",");
	}
	if (result.length()) {
		result.remove(result.length() - 1);
	}
}

uint8_t Board::getNextActiveAlarm(std::string& result, uint32_t alarms) {
	static uint8_t i = 0;
	if (!alarms) return 0;
	uint32_t errors = alarms;
	std::string tempResult = "";
	while (i <= 32) {
		if (errors & (1 << i)) {
			result = gEventsList[i];
			return i++;
		}
		i < 32 ? i++ : (i = 0);
	}
	return 0;
}

//========Private=======//


void Board::validate() {
	mainSets.EnableTransit = constrain(mainSets.EnableTransit, 0, 1);
	mainSets.IgnoreSetsFlag = constrain(mainSets.IgnoreSetsFlag, 0, 1);
	mainSets.MotorType = constrain(mainSets.MotorType, 1, 4);
	mainSets.Hysteresis = constrain(mainSets.Hysteresis, 1, 10);
	mainSets.Target = constrain(mainSets.Target, 210, 240);
	mainSets.TransRatioIndx = constrain(mainSets.TransRatioIndx, 0, sizeof(addSets.tcRatioList)/sizeof(addSets.tcRatioList[0]) - 1);
	mainSets.MaxCurrent = constrain(mainSets.MaxCurrent, 1, 200);
	mainSets.TuneInVolt = constrain(mainSets.TuneInVolt, -6, 6);
	mainSets.TuneOutVolt = constrain(mainSets.TuneOutVolt, -6, 6);
	mainSets.EmergencyTOFF = constrain(mainSets.EmergencyTOFF, 500, 5000);
	mainSets.EmergencyTON = constrain(mainSets.EmergencyTON, 500, 5000);
	mainSets.MinVolt = constrain(mainSets.MinVolt, 160, mainSets.Target);
	mainSets.MaxVolt = constrain(mainSets.MaxVolt, mainSets.Target, 260);
	addSets.SerialNumber[0] = constrain(addSets.SerialNumber[0], 0, 999999999);
	addSets.SerialNumber[1] = constrain(addSets.SerialNumber[1], 0, 999999);
}

String Board::errorsToStr(const int32_t errors, EventsFormat f) {
	String s = "";
	if (errors <= 1) {
		s = "";
		return s;
	}
	if (f == EVENTS_SHORT) {
		for (uint8_t i = 0; i < 32; i++) {
			if (errors & (1<<i)) {
				if (i < 10) {
					s += "A0";
					s += String(i);
				} else {
					s += "A";
					s += String(i);
				}
				s += ", ";
			}  
		}
		if (s.length()) {
			s.remove(s.length() - 2);
		}
	} else {
		std::string resultStr = "";
		getNextActiveAlarm(resultStr, errors);
		s = String(resultStr.c_str());
	}
	return s;
}

String Board::getWorkTime(const uint32_t mins) {
	uint32_t raw_mins = mins;
	uint32_t days, hours, minutes;
	days = raw_mins / (24 * 60);
	raw_mins %= (24 * 60);
	hours = raw_mins / 60;
    raw_mins %= 60;
	minutes = raw_mins;
	String s = "";
	s += F(" ");
	s += String(days);
	s += F("d ");
	s += String(hours);
	s += F("h ");
	s += String(minutes);
	s += F("m ");
	return s;
}

Board::~Board(){}

uint8_t Board::scanBoards(std::vector<Board> &brd, const uint8_t max) {
	for (uint8_t i = 0; i < brd.size(); i++) {
		if (!brd[i].isOnline()) { 					//если плата под номером i не онлайн
			brd.erase(brd.begin() + i);				//удаляем 
			delay(1);
		}
	}
	if (brd.size() == max) return brd.size();
	//StartI2C();
	uint32_t tmrStart = millis();
	for (uint8_t addr = 1; addr < 128; addr++) {				//проходимся по возможным адресам
		tmrStart = millis();
		uint8_t ret = Board::isBoard(addr);
		if (ret) {				//если на этом адресе есть плата
			bool reserved = false;	
			for (uint8_t i = 0; i < brd.size(); i++) {				//проходимся по уже существующим платам
				if (brd[i].getAddress() == addr) {						//если эта плата уже имеет этот адрес
					reserved = true;									//то отмечаем как зарезервировано
				}
			}
			if (!reserved) {
				brd.emplace_back(addr);					//если не зарезервировано, то создаем новую плату с этим адресом
				brd[brd.size() - 1].setLiteral((char)ret);	
			}
		}
		if (millis() - tmrStart > 50) return 0; //если сканирование заняло более 5 секунд - отменяем.
	}

	auto compareByLiteral = [](Board& board1, Board& board2) {
        return board1.getLiteral() < board2.getLiteral();
    };

    // Сортируем вектор
    std::sort(brd.begin(), brd.end(), compareByLiteral);
	return brd.size();
}

void Board::board_data_t::getMinMax(bool set_zero) {
	static uint32_t tmr = 0;
	if (millis() -  tmr > 70000) {
		set_zero = true;
	}
	if (set_zero) {
		memset(min, 0x46, sizeof(min));
		memset(max, 0, sizeof(max));
		tmr = millis();
		return;
	}
	for (uint8_t i = 0; i < NUM_VALS; i++) {
		min[i] = (online[i] < min[i] ? online[i] : min[i]);
		max[i] = (online[i] > max[i] ? online[i] : max[i]);
	}
} 

