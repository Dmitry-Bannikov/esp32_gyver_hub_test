#include "common_data.h"
#include "netconnection.h"
#include "EEManager.h"

std::vector<Board> board;					//объекты плат
std::vector<device> Devices;



EEManager memoryDevices(Devices, 5000);


uint8_t activeBoard = 0;
bool mqttConnected = false;
bool webRefresh = true;  
uint8_t boardRequest = 0; //запрос на плату
uint8_t requestResult = 0;
uint32_t Board_SN = 0;
char OwnerEmail[32] = "";


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

void Devices_Init() {
    memoryDevices.begin(100, MEMORY_KEY);
}

void Devices_Tick() {
    memoryDevices.tick();
}

void Devices_Save() {
    memoryDevices.updateNow();
}

int findDeviceIndxFromSN(uint32_t sn) {
    uint8_t count = Devices.size();
    for (uint8_t i = 0; i < count; i++) {
        if (Devices[i].SN == sn) {
            return i;
        }
    }
    return -1;
}

void UpdateDevice(const String &name, const String &type, const String &owner, const String &page, const String &status, uint32_t serial_n, int is_active) {
	//имя, тип, емайл владельца, страница веб-интерфейса, статус устройства, серийник усройства, время работы
    uint8_t count = Devices.size();
    int isExists = -1;
    isExists = findDeviceIndxFromSN(serial_n); //ищем устройство по его серийнику

    if (isExists != -1) {   //если устройство с таким серийником есть
        Devices[isExists].setParameters(
            name.c_str(), type.c_str(), 
            owner == "" ? Devices[isExists].OwnerEmail : owner.c_str(), 
            page == "" ? Devices[isExists].Page : page.c_str(),
            status == "" ? Devices[isExists].Status : status.c_str(),
            serial_n, is_active
        );
    } else {                //иначе добавляем в конец списка
        Devices.emplace_back();
        int8_t num = Devices.size() - 1;
        if (num >= 0) {
            Devices[isExists].setParameters(
                name.c_str(), type.c_str(), 
                owner.c_str(), page.c_str(),
                status.c_str(), serial_n, is_active
            );
        }
    }
}

void DeleteDevice(uint32_t sn) {
    int isExists = -1;
    isExists = findDeviceIndxFromSN(sn);
    if (isExists != -1) Devices.erase(Devices.begin() + isExists);
}

void DeleteDevice(uint8_t indx) {
    if (indx >= Devices.size()) return;
    Devices.erase(Devices.begin() + indx);
}

void CreateDevices(const char * json) {
    
}