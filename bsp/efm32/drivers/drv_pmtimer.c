/*
 * File      : drv_pmtimer.c
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
 * 2020-09-09     ShenQuan        modify to rtt 4.0
 */

#include "drv_pmtimer.h"
#include "board.h"
#include <em_letimer.h>

#if defined(EFM32_USING_LETIMER0_COMPENSATION)

static rt_uint32_t _count_freq = 0;
static rt_uint16_t _reload = 0;

/**
 * This function initialize the letimer
 */
void efm32gg_hw_letimer_init(void)
{
    LETIMER_Init_TypeDef init = LETIMER_INIT_DEFAULT;
    init.comp0Top = true;

    /* LETIMER0's count clock is 32768/1 */
    CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_8);
    CMU_ClockEnable(cmuClock_LETIMER0, true);

    _count_freq = 32768 / cmuClkDiv_8;

    /* Enable underflow interrupt */
    LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);
    LETIMER_IntEnable(LETIMER0, LETIMER_IF_UF);

    /* Enable LETIMER0 interrupt vector in NVIC */
    NVIC_ClearPendingIRQ(LETIMER0_IRQn);
    NVIC_SetPriority(LETIMER0_IRQn, EFM32_IRQ_PRI_DEFAULT);
    NVIC_EnableIRQ(LETIMER0_IRQn);

    LETIMER_Init(LETIMER0, &init);
}

/**
 * This function get current count value of LETIMER
 *
 * @return the count vlaue
 */
rt_uint32_t efm32gg_letimer_get_tick(void)
{
    rt_uint16_t ret;

    ret = LETIMER_CounterGet(LETIMER0);
    if ((ret == 0) && ((LETIMER0->IF & LETIMER_IF_UF) == 0))
        ret = _reload;

    return (ret);
}

/**
 * This function get the max value that LETIMER can count
 *
 * @return the max count
 */
rt_uint32_t efm32gg_letimer_tick_max(void)
{
    /* Max count of LETIMER0 */
    return (0xFFFF);
}

/**
 * This function start LETIMER with reload value
 *
 * @param reload The value that LETIMER count down from
 *
 * @return RT_EOK 
 */
rt_err_t efm32gg_letimer_start(rt_uint32_t reload)
{
    LETIMER0->CMD |= LETIMER_CMD_CLEAR;
    /* Wait for sync */
    while ((LETIMER0->SYNCBUSY) & 0x3F)
        ;
    _reload = reload;
    LETIMER_RepeatSet(LETIMER0, 0, 1);
    LETIMER_CompareSet(LETIMER0, 0, reload);
    LETIMER_Enable(LETIMER0, RT_TRUE);
    // rt_kprintf("reload:%d\n", reload);
    return (RT_EOK);
}

/**
 * This function stop LETIMER
 */
void efm32gg_letimer_stop(void)
{
    LETIMER_Enable(LETIMER0, RT_FALSE);
}

/**
 * This function get the count clock of LETIMER
 *
 * @return the count clock frequency in Hz
 */
rt_uint32_t efm32gg_letimer_get_countfreq(void)
{
    return (_count_freq);
}

#endif /* RT_USING_PM */
