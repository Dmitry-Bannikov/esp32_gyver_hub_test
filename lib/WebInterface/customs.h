#pragma once

#include "common_data.h"
#include "GyverPortal.h"

void GP_data_build();
void GP_target_build();
void GP_mainsets_build(Board &brd);
void GP_addsets_build(Board &brd);
void GP_SUBMIT_MINI_LINK(const String &text, const String &link, PGM_P st = GP_GREEN, const String &cls = "");
void GP_CreateDevicesList();
void GP_DeviceInfo(device & Device, int num);