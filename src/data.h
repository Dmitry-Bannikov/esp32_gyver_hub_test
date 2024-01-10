#pragma once
#include <GyverPortal.h>
#include <TimeTicker.h>
#include <EEManager.h>
#include <Board.h>
#include <Display.h>



GyverPortal ui;
//GPtime time(ui.getSystemTime());
#define RELEASE

#define AP_DEFAULT_SSID     "Stab_AP_FREE"			// Стандартное имя точки доступа ESP (До 20-ти символов)
#define AP_DEFAULT_PASS     ""						// Стандартный пароль точки доступа ESP (До 20-ти символов)
#ifdef RELEASE
#define STA_DEFAULT_SSID    "honda"					// Стандартное имя точки доступа роутера (До 20-ти символов)
#define STA_DEFAULT_PASS    "orteamoscow"			// Стандартный пароль точки доступа роутера (До 20-ти символов)
#define STA_CONNECT_EN      1						// 1/0 - вкл./выкл. подключение к роутеру 
#else
#define STA_DEFAULT_SSID    "" 
#define STA_DEFAULT_PASS    "" 
#define STA_CONNECT_EN      0 
#endif
#define MEMORY_KEY          126                     //ключ памяти (от 0 до 255), если изменить, то настройки сбросятся

struct wifisets {                                   // Структура со всеми настройками wifi
    bool staModeEn = STA_CONNECT_EN;                // Подключаться роутеру по умолчанию?
    char apSsid[21] = AP_DEFAULT_SSID;              // Имя сети для AP режима по умолчанию
    char apPass[21] = AP_DEFAULT_PASS;              // Пароль сети для AP режима по умолчанию
    char staSsid[21] = STA_DEFAULT_SSID;            // Имя сети для STA режима по умолчанию
    char staPass[21] = STA_DEFAULT_PASS;            // Пароль сети для STA режима по умолчанию
} wifi_settings;

#define MAX_BOARDS	3
std::vector<Board> board;					//объекты плат
EEManager memoryWIFI(wifi_settings, 20000);

uint8_t activeBoard = 0;
bool mqttConnected = false;
bool webRefresh = true;  
uint8_t boardRequest = 0; //запрос на плату
uint8_t requestResult = 0;
bool isSerial = true;
#define unitBytes(byteH, byteL)				(byteH << 8 | byteL )
#define getByteLow(value)						( value & 0xFF )
#define getByteHigh(value)						( (value >> 8) & 0xFF )

uint8_t global_add_pointer = 0;

template<typename T>
T reverseBytes(T& value) {
    uint8_t* front = reinterpret_cast<uint8_t*>(&value);
    uint8_t* back = front + sizeof(T) - 1;
    while (front < back) {
        std::swap(*front, *back);
        ++front;
        --back;
    }
    return value;
}

template<typename T>
void Buffer_addNewValue(T value, uint8_t* buffer, size_t bufferSize, uint8_t reset) {
    uint8_t size = sizeof(T);
    if (reset || global_add_pointer + size >= bufferSize - 1) global_add_pointer = 0;
    reverseBytes(value);
    std::memcpy(buffer + 4 + global_add_pointer, &value, size);
    global_add_pointer += size;
}