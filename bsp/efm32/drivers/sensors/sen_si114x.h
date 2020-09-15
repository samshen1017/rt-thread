/* ********************************************************************************
 * 
 * Copyright (C) 2019 - 2020 Shanghai 3H Inc. All rights reserved.
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * File: sen_si114x.h
 * File Created: Wednesday, 22nd July 2020 04:02:31
 * Author: Sam.Shen (samshen1017@sina.com)
 * 
 * Last Modified: Friday, 31st July 2020 05:55:38
 * Modified By: Sam.Shen (samshen1017@sina.com>)
 * 
 * Automatically generated; DO NOT EDIT.
 * 
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 * ********************************************************************************/

#ifndef SEN_SI114X_H
#define SEN_SI114X_H

#include "sensor.h"

#define SI114X_INT_PIN_PORT GPIO_D_PORT
#define SI114X_INT_PIN_NUM 6

int rt_hw_si114x_init(const char *name, struct rt_sensor_config *cfg);

void rt_hw_si114x_detach(rt_sensor_t dev);
#endif // SEN_SI114X_H
