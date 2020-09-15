
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
 * Last Modified: Wednesday, 29th April 2020 01:18:59
 * Modified By: Sam.Shen (samshen1017@sina.com>)
 * 
 * Automatically generated; DO NOT EDIT.
 * 
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 * ********************************************************************************/

#ifndef SEN_ZSC31014_H
#define SEN_ZSC31014_H

#include "sensor.h"

#define ZSC31014_ADDR_DEFAULT (0x28)

int rt_hw_zsc31014_init(const char *name, struct rt_sensor_config *cfg, rt_uint16_t pow_pin);

void rt_hw_zsc31014_detach(rt_sensor_t dev);
#endif // SEN_ZSC31014_H
