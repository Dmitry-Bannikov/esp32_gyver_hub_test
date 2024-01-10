#include <Board.h>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include <esp_err.h>


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


bool Board::attach(const uint8_t addr) {
	if (addr == 0 || addr > 127) return false;
	if (addr != _board_addr)
		_board_addr = addr;
	startFlag = true;
	return isOnline();
}

uint8_t Board::isBoard(uint8_t addr) {
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t error = i2c_master_cmd_begin(0, cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);
	if (error) return 0;
	uint8_t txbuf[sizeof(_txbuffer)] = {0x20, 0};
	uint8_t rxbuf[sizeof(_rxbuffer)] = {0};
	rxbuf[2] = 78;
	esp_err_t ret = i2c_master_write_read_device(0, addr, txbuf, sizeof(txbuf), rxbuf, sizeof(rxbuf), pdMS_TO_TICKS(20));
	if (ret != ESP_OK) return 0;
	if (rxbuf[0] == 0x20 && rxbuf[1] == 0xF0) return rxbuf[2];
	return 0;
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

uint8_t Board::getDataRaw() {
	if (!startFlag) return 1;
	memset(_txbuffer, 0, sizeof(_txbuffer));
	*_txbuffer = I2C_REQUEST_DATA;
	memset(_rxbuffer, 0, sizeof(_rxbuffer));
	esp_err_t ret = i2c_master_write_read_device(0, _board_addr, _txbuffer, sizeof(_txbuffer), _rxbuffer, sizeof(_rxbuffer), pdMS_TO_TICKS(100));
	if (ret != ESP_OK) return 3;
	if (*_rxbuffer != I2C_DATA_START) return 4;
	memcpy(mainData.buffer, _rxbuffer + 1, mainData.structSize);
	memcpy(mainStats.buffer, _rxbuffer + 1 + mainData.structSize, mainStats.structSize);
	mainData.unpackData();
	mainStats.unpackData();
	return 0;
}

uint8_t Board::getMainSets() {
	if (!startFlag) return 1;
	memset(_txbuffer, 0, sizeof(_txbuffer));
	*_txbuffer = I2C_REQUEST_MAINSETS;
	memset(_rxbuffer, 0, sizeof(_rxbuffer));
	esp_err_t ret = i2c_master_write_read_device(0, _board_addr, _txbuffer, sizeof(_txbuffer), _rxbuffer, sizeof(_rxbuffer), pdMS_TO_TICKS(100));
	if (ret != ESP_OK) return 3;
	if (*_rxbuffer != I2C_MAINSETS_START) return 4;
	memcpy(mainSets.buffer, _rxbuffer + 1, mainSets.structSize);
	memcpy(addSets.buffer, _rxbuffer + 1 + mainSets.structSize, addSets.structSize);
	mainSets.unpackData();
	addSets.unpackData();
	return 0;
}

uint8_t Board::getData() {
	if (!startFlag) return 1;
	static uint8_t disconn = 0;
	static uint32_t last_update = 0;
	uint8_t error = getDataRaw();
	getDataStr();
	getStatisStr();
	if (error) {
		(disconn < 5) ? (disconn++) : (disconn = 5, _disconnected = 1);
	} else {
		disconn = 0;
		_disconnected = 0;
	}
	return error;
}

uint8_t Board::sendMainSets(uint8_t attempts) {
	if (!startFlag) return 1;

	validate();
	memset(_rxbuffer, 0, sizeof(_txbuffer));
	*_txbuffer = I2C_MAINSETS_START;
	mainSets.packData();
	addSets.packData();
	memcpy(_txbuffer+1, mainSets.buffer, mainSets.structSize);
	memcpy(_txbuffer+1 + mainSets.structSize, addSets.buffer, addSets.structSize);
  	esp_err_t ret = i2c_master_write_to_device(0, _board_addr, _txbuffer, sizeof(_txbuffer), pdMS_TO_TICKS(100));
	if (ret != ESP_OK) return 2;
	return 0;
}

uint8_t Board::sendCommand(uint8_t command, uint8_t value) {
	if (!startFlag) return 1;
	addSets.Switches[command] = value;
	memset(_rxbuffer, 0, sizeof(_txbuffer));
	*_txbuffer = I2C_SWITCHES_START;
	memcpy(_txbuffer+1, addSets.Switches, sizeof(addSets.Switches));
	esp_err_t ret = i2c_master_write_to_device(0, _board_addr, _txbuffer, sizeof(_txbuffer), pdMS_TO_TICKS(100));
	if (ret != ESP_OK) return 2;
	return 0;
}

uint8_t Board::sendCommand(uint8_t* command) {
	if (!startFlag) return 1;
	memset(_rxbuffer, 0, sizeof(_txbuffer));
	*_txbuffer = I2C_SWITCHES_START;
	memcpy(_txbuffer+1, command, 8);
	esp_err_t ret = i2c_master_write_to_device(0, _board_addr, _txbuffer, sizeof(_txbuffer), pdMS_TO_TICKS(100));
	if (ret != ESP_OK) return 2;
	return 0;
}

uint8_t Board::sendCommand() {
	if (!startFlag) return 1;
	memset(_rxbuffer, 0, sizeof(_txbuffer));
	*_txbuffer = I2C_SWITCHES_START;
	memcpy(_txbuffer+1, addSets.Switches, sizeof(addSets.Switches));
	esp_err_t ret = i2c_master_write_to_device(0, _board_addr, _txbuffer, sizeof(_txbuffer), pdMS_TO_TICKS(100));
	if (ret != ESP_OK) return 2;
	return 0;
}

void Board::getDataStr() {
	float full_pwr = mainData.Power/mainData.cosfi/1000.0;
	String s = "";
	s += F(" Данные ");

	s += F("\nU вход   : ");
	s += String(mainData.Uin);
	s += F("V");

	s += F("\nU выход  : ");
	s += String(mainData.Uout);
	s += F("V");

	s += F("\nI выход  : ");
	s += String(mainData.Current, 1);
	s += F("A");

	s += F("\nP полн.  : ");
	s += String(full_pwr, 1);
	s += F("kVA");

	//s += F("\nP актив. : ");
	//s += String(mainData.Power/1000.0, 1);
	s += F("\nСобытия  : ");
	s += errorsToStr(mainData.events, EVENTS_FULL);
	mainData.Str = s;
}

void Board::getStatisStr() {
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
	char statis[100];
	sprintf(statis,
	"\n_____| max avg min"
	"\nUin  | %d %d %d "
	"\nUout | %d %d %d "
	"\nI    | %1.f %1.f "
	"\nP    | %1.f %1.f ", 
	mainStats.Uin[MAX], mainStats.Uin[AVG], mainStats.Uin[MIN],
	mainStats.Uout[MAX], mainStats.Uout[AVG], mainStats.Uout[MIN],
	mainStats.Current[MAX], mainStats.Current[AVG],
	maxPwr, avgPwr
	);
	s += String(statis);
	s += F("\nСобытия: ");
	s += errorsToStr(mainStats.Events, EVENTS_SHORT);
	mainData.Str = s;
}


void Board::createJsonData(String& result, uint8_t mode) {
	
	char json[300];
	char fase = mainSets.Liter;
	if (mode == 0) {
		float Pwr = mainData.Power/1000.0;
		float maxPwr = mainStats.Power[MAX]/1000.0;
		float avgPwr = mainStats.Power[AVG]/1000.0;
		sprintf(json, 
					"{"
					"\"Mode\":\"Data\","
					"\"Fase\":\"%c\",\"Uin\":\"%d\",\"Uout\":\"%d\",\"I\":\"%.1f\",\"P\":\"%.1f\","
					"\"Uin_avg\":\"%d\",\"Uout_avg\":\"%d\",\"I_avg\":\"%.1f\",\"P_avg\":\"%.1f\","
					"\"Uin_max\":\"%d\",\"Uout_max\":\"%d\",\"I_max\":\"%.1f\",\"P_max\":\"%.1f\","
					"\"work_h\":\"%d\""
					"}\0", 
				fase, mainData.Uin, mainData.Uout, mainData.Current, Pwr,
				mainStats.Uin[AVG],mainStats.Uout[AVG],mainStats.Current[AVG],avgPwr,
				mainStats.Uin[MAX],mainStats.Uout[MAX],mainStats.Current[MAX],maxPwr,
				mainStats.WorkTimeMins/60
		);
	} else if (mode == 1) {
		int trRatio = addSets.tcRatioList[mainSets.TransRatioIndx];
		sprintf(json, 
					"{"
					"\"Mode\":\"Sets\","
					"\"FASE\":\"%c\",\"Uout_minoff\":\"%d\",\"Uout_maxoff\":\"%d\",\"Accuracy\":\"%d\",\"Uout_target\":\"%d\","
					"\"Uin_tune\":\"%d\",\"Uout_tune\":\"%d\",\"t_5\":\"%d\",\"SN_1\":\"%d\",\"SN_2\":\"%d\","
					"\"M_type\":\"%d\",\"Time_on\":\"%.1f\",\"Time_off\":\"%.1f\",\"Rst_max\":\"%d\",\"Save\":\"%d\","
					"\"Transit\":\"%d\",\"Password\":\"%d\",\"Outsignal\":\"%d\""
					"}\0", 
				fase, mainSets.MaxVolt, mainSets.MinVolt, mainSets.Hysteresis, mainSets.Target, mainSets.TuneInVolt,
				mainSets.TuneOutVolt, trRatio, addSets.SerialNumber[0], addSets.SerialNumber[1], mainSets.MotorType,
				mainSets.EmergencyTON/1000.0, mainSets.EmergencyTOFF/1000.0, 
				addSets.Switches[SW_RSTST], 0, addSets.Switches[SW_TRANSIT], addSets.password, addSets.Switches[SW_OUTSIGN]
		);
	}
	result = String(json);
}



void Board::tick() {
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
	int16_t array[5];
	str.toCharArray(strArr, sizeof(strArr));
	sscanf(strArr, "%d,%d,%d,%d", 
	array[1],array[2],array[3],array[4]);
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


//========Private=======//


void Board::validate() {
	mainSets.IgnoreSetsFlag = constrain(mainSets.IgnoreSetsFlag, 0, 1);
	mainSets.MotorType = constrain(mainSets.MotorType, 0, 3);
	mainSets.Hysteresis = constrain(mainSets.Hysteresis, 1, 10);
	mainSets.Target = constrain(mainSets.Target, 210, 240);
	mainSets.TransRatioIndx = constrain(mainSets.TransRatioIndx, 0, sizeof(addSets.tcRatioList)/sizeof(addSets.tcRatioList[0]) - 1);
	mainSets.MaxCurrent = constrain(mainSets.MaxCurrent, 1, 30);
	mainSets.TuneInVolt = constrain(mainSets.TuneInVolt, -6, 6);
	mainSets.TuneOutVolt = constrain(mainSets.TuneOutVolt, -6, 6);
	
	mainSets.EmergencyTOFF = constrain(mainSets.EmergencyTOFF, 500, 5000);
	mainSets.EmergencyTON = constrain(mainSets.EmergencyTON, 500, 5000);
	mainSets.MinVolt = constrain(mainSets.MinVolt, 160, mainSets.Target);
	mainSets.MaxVolt = constrain(mainSets.MaxVolt, mainSets.Target, 260);
	mainSets.EnableTransit = constrain(mainSets.EnableTransit, 0, 1);
	addSets.SerialNumber[0] = constrain(addSets.SerialNumber[0], 0, 999999999);
	addSets.SerialNumber[1] = constrain(addSets.SerialNumber[1], 0, 999999);
}

String Board::errorsToStr(const int32_t errors, EventsFormat f) {
	String s = "";
	if (errors <= 1) {
		s = "Нет";
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
		static uint8_t i = 0;
		while (i <= 32) {
			if (errors & (1<<i)) {
				s = gEventsList[i];
				i++;
				return s;
			}
			i < 32 ? i++ : (i = 0);
		}
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
	StopI2C();
	pinMode(21|22, INPUT);
	Serial.print("\nI2C pins state: ");
	Serial.print(digitalRead(21));
	Serial.println(digitalRead(22));
	StartI2C();
	uint32_t tmrStart = millis();
	for (uint8_t addr = 1; addr < 128; addr++) {				//проходимся по возможным адресам
		uint8_t ret = Board::isBoard(addr);
		if (ret) {				//если на этом адресе есть плата
			Serial.println((char)ret);
			bool reserved = false;	
			for (uint8_t i = 0; i < brd.size(); i++) {				//проходимся по уже существующим платам
				if (brd[i].getAddress() == addr) {						//если эта плата уже имеет этот адрес
					reserved = true;									//то отмечаем как зарезервировано
				}
			}
			if (!reserved) {
				brd.emplace_back(addr);					//если не зарезервировано, то создаем новую плату с этим адресом
				brd[brd.size() - 1].setLiteral((char)ret);	
				//return 1;//test
			}
		}
		if (millis() - tmrStart > 2500) return 0; //если сканирование заняло более 5 секунд - отменяем.
	}

	auto compareByLiteral = [](Board& board1, Board& board2) {
        return board1.getLiteral() < board2.getLiteral();
    };

    // Сортируем вектор
    std::sort(brd.begin(), brd.end(), compareByLiteral);




	return brd.size();
}



