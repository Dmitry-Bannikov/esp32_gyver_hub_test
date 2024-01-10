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
void clicksHandler();
void updatesHandler();




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
	GP.GRID_RESPONSIVE(850); // Отключение респонза при узком экране
	GP.PAGE_TITLE("stab_manager");
	if (!wifi_settings.staModeEn) {
		GP.TITLE("Менеджер плат(AP)");
	} else {
		GP.TITLE("Менеджер плат (STA)");
	}
	GP.NAV_TABS_LINKS("/,/brdcfg,/wificfg", "Главная,Настройки платы,Настройка подключения");
	if (ui.uri("/")) {
		GP.HR();
		GP.LABEL("Соединение по MQTT");
		GP.LED("mqttConnected_led", mqttConnected);
		GP.BLOCK_BEGIN(GP_DIV_RAW, "", "Основные данные и статистика");
			GP_data_build();
		GP.BLOCK_END();
	}

	if(ui.uri("/brdcfg")) {
		GP.BLOCK_BEGIN(GP_THIN, "", "Основные настройки");
		GP_target_build();
		if (board.size()) {
			GP_mainsets_build(board[activeBoard]);
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
	formsHandler();	
	clicksHandler();
	updatesHandler();	
}

void portalInit() {
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
		list += String("b_led/")+i;
		list += ",";
	}
	list += "setsalt,reload,";
	list += "outsignal,mqttConnected_led";
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
		LED_blink(1);
		memoryWIFI.updateNow();
		delay(1000);
		ESP.restart();
	}

}

void clicksHandler() {
	
	if (ui.clickSub("brdLit")) {
		for (uint8_t i = 0; i < board.size();i++) {
			uint8_t num = 0;
			if (ui.clickInt(String("brdLit/")+i, num)) {
				if (num != 0) board[i].setLiteral(64+num);
				else board[i].setLiteral('N');
			}
		}
	}

	if (ui.clickUp("rst_btn")) 		ESP.restart();			//esp restart
	if (ui.clickUp("scan_btn")) 	scanNewBoards();			//scan boards
	if (ui.clickUp("saveall_btn")) 	boardRequest = 3;			//save to all boards
	if (ui.clickUp("read_btn") ) 	boardRequest = 10 + activeBoard;	//read settings from active board
	if (ui.clickUp("save_btn"))  	boardRequest = 20 + activeBoard; 	//save settings to active board
	if (ui.clickUp("mset_reboot")) 	boardRequest = 30 + activeBoard;	//reboot active board
	if (ui.clickUp("r_stat/0"))		boardRequest = 40;			//reset statistic
	if (ui.clickUp("r_stat/1")) 	boardRequest = 41;
	if (ui.clickUp("r_stat/2")) 	boardRequest = 42;
	if (ui.clickBool("outsignal", board[activeBoard].addSets.Switches[SW_OUTSIGN])) 	boardRequest = 4;		//200V out
	
	if (ui.clickInt("b_sel", activeBoard)) board[activeBoard].getMainSets();
	ui.clickBool("mset_transit", board[activeBoard].mainSets.EnableTransit);
	ui.clickInt("mset_targetV", board[activeBoard].mainSets.Target);
	ui.clickInt("mset_prec", board[activeBoard].mainSets.Hysteresis);
	ui.clickInt("mset_tunIn", board[activeBoard].mainSets.TuneInVolt);
	ui.clickInt("mset_tunOut", board[activeBoard].mainSets.TuneOutVolt);
	ui.clickInt("mset_tcratio_idx", board[activeBoard].mainSets.TransRatioIndx);
	uint8_t motType_local = 0;
	if (ui.clickInt("mset_mottype", motType_local)) board[activeBoard].mainSets.MotorType = motType_local+1;
	ui.clickInt("mset_maxcurr", board[activeBoard].mainSets.MaxCurrent);					
	ui.clickInt("mset_maxV", board[activeBoard].mainSets.MaxVolt);
	ui.clickInt("mset_minV", board[activeBoard].mainSets.MinVolt);
	ui.clickInt("mset_toff", board[activeBoard].mainSets.EmergencyTOFF);
	ui.clickInt("mset_ton", board[activeBoard].mainSets.EmergencyTON);
	
	if (ui.click("aset_motKoefs")) {
		String motKoefs_list = ui.getString();
		board[activeBoard].setMotKoefsList(motKoefs_list);
	}
 }

void updatesHandler() {
	if (!ui.update()) return;

	ui.updateBool("outsignal", (bool)(board[activeBoard].addSets.Switches[SW_OUTSIGN]));
	ui.updateBool("mqttConnected_led", mqttConnected);
	if (ui.update("setsalt")) {
		if (requestResult == 1)
		{
			ui.answer("Выполнено!");
		}
		requestResult = 0;
	}
	if (ui.update("reload") && webRefresh) {
		webRefresh = false;
		ui.answer(1);
	}
	for (uint8_t i = 0; i < board.size(); i++) {
		ui.updateString(String("b_data/") + i, board[i].mainData.Str);
		ui.updateString(String("b_stat/") + i, board[i].mainStats.Str);
		ui.updateBool(String("b_led/") + i, board[i].isOnline());
	}

	
}
