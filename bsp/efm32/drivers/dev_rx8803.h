/* ********************************************************************************
 * 
 * Copyright (C) 2019 - 2020 Shanghai 3H Inc. All rights reserved.
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * File: dev_rx8803.h
 * File Created: Monday, 17th August 2020 03:18:49
 * Author: Sam.Shen (samshen1017@sina.com)
 * 
 * Last Modified: Monday, 14th September 2020 03:32:06
 * Modified By: Sam.Shen (samshen1017@sina.com>)
 * 
 * Automatically generated; DO NOT EDIT.
 * 
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 * ********************************************************************************/

#ifndef DEV_RX8803_H
#define DEV_RX8803_H

#include <rtthread.h>

rt_err_t rtc_timer_Start(int interval_sec);
void rtc_timer_Stop(void);
void rtc_timer_SetTimerCallback(void (*cb)(void *args));
void rt_hw_rx8803_init(void);

#endif /* DRV_RX8803_H */
