#include "common_data.h"


GyverPortal ui;
GPtime t;
std::vector<Board> board;					//объекты плат
std::vector<device> Devices;

uint8_t activeBoard = 0;
bool mqttConnected = false;
bool webRefresh = true;  
uint8_t boardRequest = 0; //запрос на плату
uint8_t requestResult = 0;
uint32_t Board_SN = 0;

void LED_blink(uint16_t period_on, uint16_t period_off) {
  	static uint64_t tick = 0;
	static bool led_state = false;
	if (period_on <= 1) {
		digitalWrite(LED_BUILTIN, !period_on);
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


int findDeviceIndxFromID(uint32_t id) {
    uint8_t count = Devices.size();
    for (uint8_t i = 0; i < count; i++) {
        if (Devices[i].ID == id) {
            return i;
        }
    }
    return -1;
}


void UpdateDevice(const char* name, const char* owner, const char* type, const char* page, uint32_t serial_n, uint32_t id, int is_active, bool reg) {
	//name, email, type, page, SN, ID, isActive, Reg
    uint8_t count = Devices.size();
    int isExists = -1;
    
    isExists = findDeviceIndxFromID(id);

    if (isExists != -1) {
        Devices[isExists].setParameters(name, owner, type, page, serial_n, id, is_active, reg);
    } else {
        Devices.emplace_back();
        uint8_t dev_num = Devices.size();
        if (dev_num > 0) {
            dev_num = dev_num - 1;
            strcpy(Devices[dev_num].Name, name);
            strcpy(Devices[dev_num].OwnerEmail, owner);
            strcpy(Devices[dev_num].Type, type);
            strcpy(Devices[dev_num].Page, page);
            Devices[dev_num].SN = serial_n;
            Devices[dev_num].ID = id;
            Devices[dev_num].IsActive = is_active;
            Devices[dev_num].Reg = reg;
        }
    }
}

void DeleteDevice(uint32_t id) {
    int isExists = -1;
    isExists = findDeviceIndxFromID(id);
    if (isExists != -1) Devices.erase(Devices.begin() + isExists);
}