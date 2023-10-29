#pragma once

#include <customs.h>
#include <LittleFS.h>
#include <service.h>


void portalBuild();
void portalActions();
void portalInit();
void portalTick();
void createUpdateList(String &list);
void formsHandler();
void clicksHandler(uint8_t &result);
void updatesHandler(uint8_t &result);

GyverPortal ui(&LittleFS);


void portalBuild() {
  //------------------------------------------//
	GP.BUILD_BEGIN(900);
	GP.ONLINE_CHECK(5000);
	GP.THEME(GP_LIGHT);
	String update = "";
	createUpdateList(update);
	GP.UPDATE(update);
	GP.ALERT("setsalt");
	GP.RELOAD("reload");
	GP.GRID_RESPONSIVE(770); // Отключение респонза при узком экране
	GP.PAGE_TITLE("stab_manager");
	if (!wifi_settings.staModeEn) {
		GP.TITLE("Менеджер плат(AP)");
	} else {
		GP.TITLE("Менеджер плат (STA)");
	}
	GP.NAV_TABS_LINKS("/,/brdcfg,/addcfg,/wificfg", "Главная,Настройки платы,Доп. настройки,Настройка подключения");
	if (ui.uri("/")) {
		GP.HR();
		GP.BLOCK_BEGIN(GP_THIN, "", "Основные данные и статистика");
			GP_data_build();
		GP.BLOCK_END();
	}

	if(ui.uri("/brdcfg")) {
		GP.BLOCK_BEGIN(GP_THIN, "", "Основные настройки");
		GP_target_build();
		if (board.size()) {
			GP_mainsets_build(board[activeBoard]);
		}
		GP.BLOCK_END();
	}

	if(ui.uri("/addcfg")) {
		GP.BLOCK_BEGIN(GP_THIN, "", "Дополнительные настройки");
		if (board.size()) {
			GP_addsets_build(board[activeBoard]);
		}
		GP.BLOCK_END();
	}
	
	if(ui.uri("/wificfg")) {
		GP.BLOCK_BEGIN(GP_THIN, "", "Сетевые настройки");
		GP.FORM_BEGIN("/wificfg");
		GP.GRID_BEGIN();
			GP.BLOCK_BEGIN(GP_DIV_RAW, "", "Настройка точки дступа");
				GP.TEXT("apSsid", "Login AP", wifi_settings.apSsid, "", 20);
				GP.PASS_EYE("apPass", "Password AP", wifi_settings.apPass, "", 20);
			GP.BLOCK_END();

			GP.BLOCK_BEGIN(GP_DIV_RAW);
				GP.SUBMIT_MINI("SUBMIT & RESTART");
			GP.BLOCK_END();
			
			GP.BLOCK_BEGIN(GP_DIV_RAW, "", "Подключение к WIFI");
				GP.TEXT("staSsid", "Login STA", wifi_settings.staSsid, "", 20);
				GP.PASS_EYE("staPass","Password STA", wifi_settings.staPass, "", 20);
			GP.BLOCK_END();
		GP.GRID_END();
		GP.BUTTON_LINK("/ota_update", "Обновление прошивки");
		GP.FORM_END();
		GP.BLOCK_END();
	}
	GP.BUILD_END();
}

void portalActions() {
	static uint8_t txSuccess = 0;
	formsHandler();	
	updatesHandler(txSuccess);
	clicksHandler(txSuccess);	
}

void portalInit() {
	// Пытаемся подключиться к роутеру
	if (wifi_settings.staModeEn) {
		WiFi.mode(WIFI_STA);
		WiFi.begin(wifi_settings.staSsid, wifi_settings.staPass);
		int attemptCount = 0;
		while (WiFi.status() != WL_CONNECTED) {
			LED_blink(100);
			if (++attemptCount == 10) {
				LED_switch(0);
				wifi_settings.staModeEn = 0;  //переключаемся на режим точки доступа
				memoryWIFI.updateNow();       //сохраняемся
				ESP.restart();                //перезапускаем есп
				return;
			}
			delay(1000);
		}
		Serial.println(WiFi.localIP());
	} 
	// Иначе создаем свою сеть
	else {
		WiFi.mode(WIFI_AP);
		WiFi.softAP(wifi_settings.apSsid, wifi_settings.apPass);
		delay(1000);
		Serial.println(WiFi.softAPIP());
	}
	ui.attachBuild(portalBuild);
	ui.attach(portalActions);
	if (WiFi.getMode() == WIFI_MODE_STA) {
		ui.start("stab_webserver");
	} else {
		ui.start();
	}
	
	ui.enableOTA("admin", "012343210");
}

void portalTick() {
	ui.tick();
}



void createUpdateList(String &list) {
	for (uint8_t i = 0; i < board.size(); i++) {
		list += String("b_data/")+i;
		list += ",";
		list += String("b_stat/")+i;
		list += ",";
	}
	list += "setsalt,";
	list += "reload";
}

void formsHandler() {
	if (ui.form("/wificfg")) { // Если есть сабмит формы - копируем все в переменные
			
		ui.copyStr("apSsid", wifi_settings.apSsid);
		ui.copyStr("apPass", wifi_settings.apPass);
		ui.copyStr("staSsid", wifi_settings.staSsid);
		ui.copyStr("staPass", wifi_settings.staPass);
		String s = wifi_settings.staSsid;
		if (s.length() > 1) {
		wifi_settings.staModeEn = 1;
		} else {
		wifi_settings.staModeEn = 0;
		}
		LED_switch(1);
		memoryWIFI.updateNow();
		delay(1000);
		ESP.restart();
	}

}

void clicksHandler(uint8_t &result) {
	if (ui.clickUp("rset_btn") ) {	//кнопка прочитать настройки
		if (!board[activeBoard].readSettings()) {
			result = 1;
			webRefresh = true;
		}
	}
	if (ui.clickUp("rboard_btn") ) {	//кнопка прочитать настройки
		if (!board[activeBoard].getMainSets() && !board[activeBoard].getAddSets()) {
			result = 1;
			webRefresh = true;
		}
	}
	if (ui.clickUp("wset_btn")) {	//кнопка записать настройки
		if (!board[activeBoard].sendMainSets() && !board[activeBoard].sendAddSets()) {
			result = 2;
			webRefresh = true;
		}
	}
	if (ui.clickUp("mset_disreg")) {	//кнопка переключить регуляцию
		if (!board[activeBoard].toggleRegulation()) {
			result = 4;
			webRefresh = true;
		}
	}
	if (ui.clickUp("mset_reboot")) {	//кнопка перезагрузить плату
		if (!board[activeBoard].reboot()) {
			result = 5;
			webRefresh = true;
		}
	}
	if (ui.clickUp("rst_btn")) {
		ESP.restart();
	}
	if (ui.clickUp("scan_btn")) {
		scanNewBoards();
	}
	if (ui.clickUp("svlit_btn")) {
		board[activeBoard].saveSettings();
		result = 3;
		webRefresh = true;
	}

	for (uint8_t i = 0; i < board.size();i++) {
		uint8_t num = 0;
		if (ui.clickInt(String("brdLit/")+i, num)) {
			if (num != 0) board[i].setLiteral(64+num);
			else board[i].setLiteral('N');
		}
	}

	ui.clickInt("b_sel", activeBoard);
	ui.clickBool("mset_ignor", board[activeBoard].mainSets.ignoreSetsFlag);
	ui.clickInt("mset_trgtV_idx", board[activeBoard].mainSets.targetVoltIndx);
	ui.clickInt("mset_prec", board[activeBoard].mainSets.precision);
	ui.clickInt("mset_tunIn", board[activeBoard].mainSets.tuneInVolt);
	ui.clickInt("mset_tunOut", board[activeBoard].mainSets.tuneOutVolt);
	ui.clickInt("mset_tcratio_idx", board[activeBoard].mainSets.transRatioIndx);
	ui.clickInt("mset_mottype", board[activeBoard].mainSets.motorType);
	ui.clickInt("mset_disptype", board[activeBoard].mainSets.displayType);
	ui.clickInt("mset_maxcurr_idx", board[activeBoard].mainSets.maxCurrentIndx);
						
	ui.clickInt("aset_maxV", board[activeBoard].addSets.maxVolt);
	ui.clickInt("aset_minV", board[activeBoard].addSets.minVolt);
	ui.clickInt("aset_toff", board[activeBoard].addSets.emergencyTOFF);
	ui.clickInt("aset_ton", board[activeBoard].addSets.emergencyTON);
	//ui.clickStr
}

void updatesHandler(uint8_t &result) {
	if (!ui.update()) return;
	if (ui.update("reload") && webRefresh) {
		webRefresh = false;
		ui.answer(1);
	}
	if (ui.update("setsalt")) {
		switch (result)
		{
		case 1:
			ui.answer("Настройки прочитаны из памяти!");
			break;
		case 2:
			ui.answer("Настройки переданы на плату!");
			break;
		case 3:
			ui.answer("Настройки сохранены в память!");
			break;
		case 4:
			ui.answer("Переключена регулировка!");
			break;
		case 5:
			ui.answer("Плата перезагружена!");
			break;
		default:
			break;
		}
		result = 0;
	}
	for (uint8_t i = 0; i < board.size(); i++) {
		board[i].getDataStr();
		board[i].getStatisStr();
		ui.updateString(String("b_data/") + i, board[i].mainData.Str);
		ui.updateString(String("b_stat/") + i, board[i].mainStats.Str);
	}
	
}
