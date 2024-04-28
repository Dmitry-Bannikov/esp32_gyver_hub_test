#pragma once
#include <GyverPortal.h>

#include "Arduino.h"

void portalInit();
void portalTick();
void portalBuild();
void portalActions();
void createUpdateList(String &list);
void formsHandler();
void clicksHandler();
void updatesHandler();
void AddEditDevice_handler(int &device);
void DeleteEditDevice_handler();
