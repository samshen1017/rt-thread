/*
 * File      : drv_pmtimer.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2008 - 2012, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author          Notes
 * 2012-08-21     heyuanjie87     the first version
 */

#ifndef  __DRV_PMTIMER_H__
#define  __DRV_PMTIMER_H__

#include <rtthread.h>

void efm32gg_hw_letimer_init(void);
rt_uint32_t efm32gg_letimer_get_tick(void);
rt_uint32_t efm32gg_letimer_tick_max(void);
rt_uint32_t efm32gg_letimer_get_countfreq(void);
rt_err_t efm32gg_letimer_start(rt_uint32_t load);
void efm32gg_letimer_stop(void);

#endif /* __DRV_PMTIMER_H__ */
