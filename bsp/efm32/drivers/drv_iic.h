/* ********************************************************************************
 * 
 * Copyright (C) 2019 - 2020 Shanghai 3H Inc. All rights reserved.
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * File: drv_iic.h
 * File Created: Thursday, 23rd July 2020 05:41:12
 * Author: Sam.Shen (samshen1017@sina.com)
 * 
 * Last Modified: Monday, 3rd August 2020 01:15:06
 * Modified By: Sam.Shen (samshen1017@sina.com>)
 * 
 * Automatically generated; DO NOT EDIT.
 * 
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 * ********************************************************************************/
#ifndef DRV_IIC_H
#define DRV_IIC_H

#include <rtthread.h>
#include <rtdevice.h>

#define IIC_TRANSFER_TIMEOUT 8000

void rt_hw_iic_init(void);

#endif //DRV_IIC_H
