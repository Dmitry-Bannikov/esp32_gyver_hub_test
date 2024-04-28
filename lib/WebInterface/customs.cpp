#include "customs.h"

void GP_data_build() {
	GP.GRID_BEGIN();
	if (board.size() == 0) {
		GP.BLOCK_BEGIN(GP_DIV_RAW);
		GP.TITLE("Не обнаружено подключенных плат!");
		M_BOX(
			GP.BUTTON_MINI("setbtn_rstesp", "Перезапустить\n систему");
			GP.BUTTON_MINI("setbtn_scan", "Пересканировать\n платы");
		);
		GP.BLOCK_END();
	} else {
		for (uint8_t i = 0; i < board.size(); i++) {
			GP.BLOCK_BEGIN(GP_THIN, "100%", String("Плата ")+ board[i].getLiteral());
			GP.AREA(String("b_data/")+i, 7, "Загрузка \nданных...");
			GP.AREA(String("b_stat/")+i, 13, "Загрузка \nстатистики...");
			M_BOX(GP_AROUND,
				GP.BUTTON_MINI(String("setbtn_reset/")+i, "Сброс");
				GP.LED(String("b_led/")+i, board[i].isOnline());
			);
			GP.BLOCK_END();
		}
	}
	GP.GRID_END();
	if (board.size()) GP.BUTTON_MINI("setbtn_scan", "Пересканировать\n платы");
}

void GP_target_build() {
	GP.GRID_BEGIN();

		GP.BLOCK_BEGIN(GP_DIV_RAW);//выбор платы
			String list = "";
            for (uint8_t i = 0; i < board.size();i++) {
				list += String("Плата ");
				list += board[i].getLiteral();
				list += "(";
				list += board[i].getAddress();
				list += "),";
			}
			list.remove(list.length() - 1);
			if (board.size() > 0) {
				GP.SELECT("setbtn_sel_act", list, activeBoard, 0, 0, 1);
			}
			
        GP.BLOCK_END();

		GP.BLOCK_BEGIN(GP_DIV_RAW);
			if (board.size() > 0) {
				GP.BUTTON("setbtn_saveall", "Сохранить");
			} else {
				GP.TITLE("Не обнаружено подключенных плат!");
			}
			
			GP.RELOAD("reload");
		GP.BLOCK_END();

		GP.BLOCK_BEGIN(GP_DIV_RAW);//назначение букв
			for (uint8_t i = 0; i < board.size(); i++) {
				String label = String("Плата ") + board[i].getAddress();
				uint8_t select = 0;
				if (board[i].getLiteral() == 'A') select = 1;
				else if (board[i].getLiteral() == 'B') select = 2;
				else if (board[i].getLiteral() == 'C') select = 3;
				GP.BOX_BEGIN();
				GP.LABEL(label);
				GP.SELECT(String("setbtn_sel_lit/")+i, "Нет,A,B,C", select); 
				GP.BOX_END();
			}
			
		GP.BLOCK_END();

	GP.GRID_END();

}

void GP_mainsets_build(Board &brd) {
	String motTypes_list;
	String tcRatio_list;
	brd.getMotTypesList(motTypes_list, true);
	brd.getTcRatioList(tcRatio_list);

	//------------------------------------------------//
	String title = "Настройка платы ";
	title += (brd.mainSets.Liter > 0 ? String(brd.getLiteral()) : String(brd.getAddress()));
	GP.BLOCK_BEGIN(GP_DIV_RAW, "", title);
	M_BOX(
		GP.BUTTON_MINI("setbtn_read", "Прочитать из платы"); 
		GP.BUTTON_MINI("setbtn_write", "Записать в плату")
	);
	M_BOX(GP_EDGES, GP.LABEL("Транзит при перегрузке"); GP.CHECK("setfield_transit", brd.mainSets.EnableTransit););
	M_BOX(GP_EDGES, GP.LABEL("Точность/ гистерезис");  GP.NUMBER("setfield_prec", "", brd.mainSets.Hysteresis, "70px"););
	M_BOX(GP_EDGES, GP.LABEL("Подстройка входа");  GP.NUMBER("setfield_tunIn", "", brd.mainSets.TuneInVolt, "70px"););
	M_BOX(GP_EDGES, GP.LABEL("Подстройка выхода");  GP.NUMBER("setfield_tunOut", "", brd.mainSets.TuneOutVolt, "70px"););
	M_BOX(GP_EDGES, GP.LABEL("Целевое напряжение");  GP.NUMBER("setfield_targetV", "", brd.mainSets.Target, "70px"););
	M_BOX(GP_EDGES, GP.LABEL("Ток выхода макс"); GP.NUMBER("setfield_maxcurr", "", brd.mainSets.MaxCurrent, "70px"); );
	M_BOX(GP_EDGES, GP.LABEL("Коэффициент трансформатора");  GP.SELECT("setfield_tcratio_idx", tcRatio_list, brd.mainSets.TransRatioIndx););
	M_BOX(GP_EDGES, GP.LABEL("Тип мотора"); GP.SELECT("setfield_mottype", motTypes_list, brd.mainSets.MotorType-1););
	
	GP.GRID_BEGIN();
		GP.BUTTON_MINI("setbtn_rstesp", "Перезапустить ESP");
		GP.BUTTON_MINI("setbtn_rstbrd", "Перезагрузить плату");	
	GP.GRID_END();
	GP.BLOCK_END();
}

void GP_addsets_build(Board &brd) {
	String motKoefs_list;
	String tcRatio_list;
	brd.getMotTypesList(motKoefs_list, false);
	brd.getTcRatioList(tcRatio_list);

	String title = "Настройка платы ";
	title += (brd.mainSets.Liter > 0 ? String(brd.getLiteral()) : String(brd.getAddress()));
	GP.BLOCK_BEGIN(GP_DIV_RAW, "", title);
	M_BOX(GP_EDGES, GP.LABEL("Коэф. моторов, %"); GP.TEXT("setfield_motKoefs", "Через ,", motKoefs_list, "170px"); );
	M_BOX(GP_EDGES, GP.LABEL("Максимальное напряжение");  GP.NUMBER("setfield_maxV", "", brd.mainSets.MaxVolt, "70px"););
	M_BOX(GP_EDGES, GP.LABEL("Минимальное напряжение");  GP.NUMBER("setfield_minV", "", brd.mainSets.MinVolt, "70px"););
	M_BOX(GP_EDGES, GP.LABEL("Время отключения, мс");  GP.NUMBER("setfield_toff", "", brd.mainSets.EmergencyTOFF, "70px"););
	M_BOX(GP_EDGES, GP.LABEL("Время включения, мс");  GP.NUMBER("setfield_ton", "", brd.mainSets.EmergencyTON, "70px"););
	M_BOX(GP_EDGES, 
		GP.LABEL("Калибровочный ток:"); GP.NUMBER_F("setfield_CurrClbrValue", "", brd.CurrClbrtValue, 2, "70px");
		GP.LABEL("Коэффициент", "", GP_GRAY); GP.NUMBER_F("setfield_CurrClbrKoeff", "", brd.CurrClbrtKoeff, 2, "70px");
		GP.BUTTON_MINI("setbtn_curr_tx", "Передать");
	);
	M_BOX(GP_EDGES, GP.LABEL("Внеш. сигнал"); GP.CHECK("setfield_outsignal", (bool)(brd.addSets.Switches[SW_OUTSIGN])););
	GP.BLOCK_END();
}

void GP_SUBMIT_MINI_LINK(const String &text, const String &link, PGM_P st, const String &cls) {
    *_GPP += F("<input type='submit' onclick='openNewTab()' value='");
    *_GPP += text;
    if (st != GP_GREEN) {
        *_GPP += F("' style='background:");
        *_GPP += FPSTR(st);
    }
    if (cls.length()) {
        *_GPP += F("' class='");
        *_GPP += cls;
    }
    *_GPP += F("' >\n");

    *_GPP += F("<script>");
    *_GPP += F("function openNewTab() {");
    *_GPP += F("window.open('");
    *_GPP += link;
    *_GPP += F("', '_blank');");
    *_GPP += F("return false;}");
    *_GPP += F("</script>");
    GP.send();
}

void GP_CreateDevicesList() {
	
    //UpdateDevice("stab_brd", "example@mail.com", "stabilizer", "/dashboard", Board_SN, 1235, 21, true);
	for (uint8_t i = 0; i < Devices.size(); i++) {
		GP_DeviceInfo(Devices[i], i);
	}
	
}

void GP_DeviceInfo(device & Device, int num) {
    if (strcmp(Device.Status, "Registred") != 0) return;
	GP.GRID_BEGIN();
		GP.BLOCK_BEGIN(GP_DIV_RAW);
			GP.TEXT("dev_name/" + String(num), "Name", Device.Name, "", 20);
		GP.BLOCK_END();

		GP.BLOCK_BEGIN(GP_DIV_RAW);
			if (Device.IsActive) {
				GP.LABEL("В работе", "", "#000");
				GP.LED("", true);
                //GP.NUMBER("", "Hours", Device.IsActive, "50px");
			} else {
				GP.LABEL("Отключен", "", "#000");
				GP.LED("", false);
                GP.NUMBER("", "Hours", Device.IsActive, "50px", true);
			}
		GP.BLOCK_END();

		GP.BLOCK_BEGIN(GP_DIV_RAW);
		GP.BOX_BEGIN();
			GP.BUTTON_MINI("edit_btn/" + String(num), "Изменить");
			GP.BUTTON_MINI("delete_btn/" + String(num), "Удалить", "", GP_RED);
			GP.BUTTON_MINI_LINK(Device.Page, "Открыть");
		GP.BOX_END();
		GP.BLOCK_END();
	GP.GRID_END();
	GP.HR();
}

void GP_AddEditDevice(int dev) {
	if (dev == -1) {
		GP.GRID_BEGIN();
		GP.FORM_BEGIN("/add_device");
			GP.BLOCK_BEGIN(GP_DIV_RAW);
				GP.TEXT("newName", "ИМЯ", "", "", 20);
			GP.BLOCK_END();
			GP.BLOCK_BEGIN(GP_DIV_RAW);
				GP.TEXT("newType", "ТИП", "", "", 20);
			GP.BLOCK_END();
			GP.BLOCK_BEGIN(GP_DIV_RAW);
				GP.SUBMIT_MINI("Добавить");
			GP.BLOCK_END();
		GP.FORM_END();
		GP.GRID_END();
	} else {
		GP.GRID_BEGIN();
		GP.FORM_BEGIN("/add_device");
			GP.BLOCK_BEGIN(GP_DIV_RAW);
				GP.TEXT("newName", "ИМЯ", Devices[dev].Name, "", 20);
			GP.BLOCK_END();
			GP.BLOCK_BEGIN(GP_DIV_RAW);
				GP.TEXT("newType", "ТИП", Devices[dev].Type, "", 20);
			GP.BLOCK_END();
			GP.BLOCK_BEGIN(GP_DIV_RAW);
				GP.BOX_BEGIN();
					GP.SUBMIT_MINI("Изменить");
					GP.FORM_END();
					GP.FORM_SUBMIT("/cancel", "X", "", GP_RED);
				GP.BOX_END();
			GP.BLOCK_END();
		GP.GRID_END();
	}
	
}































//========================================


