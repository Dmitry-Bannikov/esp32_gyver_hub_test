#pragma once


#include <Arduino.h>
#include <EEmanager.h>
#include <WiFiUdp.h>
#include <WiFi.h>



#define AP_DEFAULT_SSID     "Stab_AP_FREE"			// Стандартное имя точки доступа ESP (До 20-ти символов)
#define AP_DEFAULT_PASS     ""						// Стандартный пароль точки доступа ESP (До 20-ти символов)
#define STA_DEFAULT_SSID    "honda"					// Стандартное имя точки доступа роутера (До 20-ти символов)
#define STA_DEFAULT_PASS    "orteamoscow"			// Стандартный пароль точки доступа роутера (До 20-ти символов)
#define STA_CONNECT_EN      1						// 1/0 - вкл./выкл. подключение к роутеру 

#define MEMORY_KEY          126                     //ключ памяти (от 0 до 255), если изменить, то настройки сбросятся

struct wifisets {                                       // Структура со всеми настройками wifi
    bool staModeEn = STA_CONNECT_EN;                // Подключаться роутеру по умолчанию?
    char apSsid[21] = AP_DEFAULT_SSID;              // Имя сети для AP режима по умолчанию
    char apPass[21] = AP_DEFAULT_PASS;              // Пароль сети для AP режима по умолчанию
    char staSsid[21] = STA_DEFAULT_SSID;            // Имя сети для STA режима по умолчанию
    char staPass[21] = STA_DEFAULT_PASS;            // Пароль сети для STA режима по умолчанию
};

struct global_vars {                                    //сюда добавлять переменные, которые нужны во многих частях программы
    bool mqttState = true;                         	//статус соединения по мктт
    char webInterfaceDNS[20] = "stab_brd";			
};


void WifiInit();
void wifi_tick();
void wifi_updateCFG();



extern struct wifisets wifi_settings;
extern struct global_vars globalData;