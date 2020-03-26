
/* ********************************************************************************
 * 
 * Copyright (C) 2019 - 2020 Shanghai 3H Inc. All rights reserved.
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * File: sensor_zsc31014.h
 * File Created: Monday, 23rd March 2020 02:45:31
 * Author: Sam.Shen (samshen1017@sina.com)
 * 
 * Last Modified: Tuesday, 24th March 2020 02:28:59
 * Modified By: Sam.Shen (samshen1017@sina.com>)
 * 
 * Automatically generated; DO NOT EDIT.
 * 
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 * ********************************************************************************/

#ifndef SENSOR_ZSC31014_H
#define SENSOR_ZSC31014_H

#include "sensor.h"

#define ZSC31014_ADDR_DEFAULT (0x28)

int rt_hw_zsc31014_init(const char *name, struct rt_sensor_config *cfg);

#endif // SENSOR_ZSC31014_H
