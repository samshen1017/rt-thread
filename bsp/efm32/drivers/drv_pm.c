/*
 * @Author: ShenQuan 
 * @Date: 2019-06-14 13:03:11 
 * @Last Modified by: ShenQuan
 * @Last Modified time: 2019-06-14 17:22:09
 */

#include "em_emu.h"
#include "emodes.h"
#include "board.h"
#include <rtthread.h>
#include <rtdevice.h>

#if defined(RT_USING_PM)

#ifdef RT_PM_DEBUG
#define pm_debug(format, args...) rt_kprintf(format, ##args)
#else
#define pm_debug(format, args...)
#endif

static bool run_lock = false;

/**
 * This function will enter sleep mode.
 *
 * @param pm pointer to power manage structure
 */
static void sleep(struct rt_pm *pm, uint8_t mode)
{
    switch (mode)
    {
    case PM_SLEEP_MODE_NONE:
        break;
    case PM_SLEEP_MODE_IDLE:
        em_EM0_Hfxo();
        break;
    case PM_SLEEP_MODE_LIGHT:
        em_EM1_Hfrco(cmuHFRCOBand_1MHz);
        break;
    case PM_SLEEP_MODE_DEEP:
        em_EM2_Lfxo();
        break;
    case PM_SLEEP_MODE_STANDBY:
        em_EM3();
        break;
    case PM_SLEEP_MODE_SHUTDOWN:
        em_EM4();
        break;
    default:
        pm_debug("unknown pm sleep mode.\n");
        RT_ASSERT(0);
    }
}

static void run(struct rt_pm *pm, uint8_t mode)
{
    if (run_lock == true)
    {
        return;
    }
    static uint8_t last_mode;
    if (mode == last_mode)
        return;
    last_mode = mode;

    switch (mode)
    {
    case PM_RUN_MODE_HIGH_SPEED:
        em_EM0_Hfxo();
        break;
    case PM_RUN_MODE_NORMAL_SPEED:
        em_EM1_Hfrco(cmuHFRCOBand_28MHz);
        break;
    case PM_RUN_MODE_MEDIUM_SPEED:
        em_EM1_Hfrco(cmuHFRCOBand_11MHz);
        break;
    case PM_RUN_MODE_LOW_SPEED:
        em_EM1_Hfrco(cmuHFRCOBand_1MHz);
        break;
    default:
        pm_debug("unknown pm run mode.\n");
        RT_ASSERT(0);
    }
    pm_debug("switch to  frequency = %d Hz\n", CMU_ClockFreqGet(cmuClock_HF));
}

static void emu_EM23init(void)
{

    /* Use default settings for EM23 */
    EMU_EM23Init_TypeDef em23Init = EMU_EM23INIT_DEFAULT;
    EMU_EM23Init(&em23Init);
}

//#include "drv_pin.h"
static void emu_EM4init(void)
{
    // rt_uint16_t wakeup_pin = get_PinNumber(GPIO_F_PORT, 2);
    // rt_pin_mode(wakeup_pin, PIN_MODE_INPUT_PULLUP);

    // GPIO_EM4EnablePinWakeup(GPIO_EM4WUEN_EM4WUEN_F2, 0);

    /* Use default settings for EM4 */
    EMU_EM4Init_TypeDef em4Init = EMU_EM4INIT_DEFAULT;
    //EMU_BUPDInit_TypeDef bupdInit = EMU_BUPDINIT_DEFAULT;
    //GPIO_EM4SetPinRetention(true);
    EMU_EM4Init(&em4Init);
}

#if defined(EFM32_USING_LETIMER0_COMPENSATION)

/******************** Timer 补偿 OS Tick ********************/
#include "drv_pmtimer.h"

/**
 * This function caculate the PM tick from OS tick
 *
 * @param tick OS tick
 *
 * @return the PM tick
 */
static rt_tick_t efm32gg_pm_tick_from_os_tick(rt_tick_t tick)
{
    rt_uint32_t freq = efm32gg_letimer_get_countfreq();
    return (freq * tick / RT_TICK_PER_SECOND);
}

/**
 * This function caculate the OS tick from PM tick
 *
 * @param tick PM tick
 *
 * @return the OS tick
 */
static rt_tick_t efm32gg_os_tick_from_pm_tick(rt_uint32_t tick)
{
    static rt_uint32_t os_tick_remain = 0;
    rt_uint32_t ret, freq;

    freq = efm32gg_letimer_get_countfreq();
    ret = (tick * RT_TICK_PER_SECOND + os_tick_remain) / freq;

    os_tick_remain += (tick * RT_TICK_PER_SECOND);
    os_tick_remain %= freq;

    return ret;
}

static void _timer_start(struct rt_pm *pm, rt_uint32_t timeout)
{
    RT_ASSERT(pm != RT_NULL);
    RT_ASSERT(timeout > 0);

    if (timeout != RT_TICK_MAX)
    {
        /* Convert OS Tick to letimer timeout value */
        timeout = efm32gg_pm_tick_from_os_tick(timeout);
        /* MAX 0xFFFF */
        if (timeout > efm32gg_letimer_tick_max())
        {
            timeout = efm32gg_letimer_tick_max();
        }

        /* Enter PM_TIMER_MODE */
        efm32gg_letimer_start(timeout);
    }
}

static void _timer_stop(struct rt_pm *pm)
{
    RT_ASSERT(pm != RT_NULL);

    /* Reset letimer status */
    efm32gg_letimer_stop();
}

static rt_tick_t _timer_get_tick(struct rt_pm *pm)
{
    rt_uint32_t timer_tick;

    RT_ASSERT(pm != RT_NULL);

    timer_tick = efm32gg_letimer_get_tick();

    return efm32gg_os_tick_from_pm_tick(timer_tick);
}

static const struct rt_pm_ops _ops =
    {
        sleep,
        run,
        _timer_start,
        _timer_stop,
        _timer_get_tick,
};
#else

static const struct rt_pm_ops _ops =
    {
        sleep,
        run,
        RT_NULL,
        RT_NULL,
        RT_NULL,
};
#endif

/**
 * This function initialize the power manager
 */
int rt_hw_pm_init(void)
{
    emu_EM23init();
    emu_EM4init();

    rt_uint8_t timer_mask = 0;

#if defined(EFM32_USING_LETIMER0_COMPENSATION)
    efm32gg_hw_letimer_init();

    /* initialize timer mask */
    timer_mask = 1UL << PM_SLEEP_MODE_DEEP;
    //timer_mask = 1UL << PM_SLEEP_MODE_STANDBY;
#endif

    /* initialize system pm module */
    rt_system_pm_init(&_ops, timer_mask, RT_NULL);

    return 0;
}
//INIT_BOARD_EXPORT(rt_hw_pm_init);

void rt_pm_run_lock(bool islock)
{
    run_lock = islock;
}

static void pm_lock(void)
{
    if (run_lock)
    {
        rt_kprintf("PM locked.\n");
    }
    else
    {
        rt_kprintf("PM unlock.\n");
    }
}
FINSH_FUNCTION_EXPORT(pm_lock, Is pm locked);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(pm_lock, Is pm locked);
#endif

static void em0(void)
{
    rt_kprintf("enter EM0.\n");
    rt_pm_request(PM_SLEEP_MODE_NONE);
    rt_pm_run_enter(PM_RUN_MODE_HIGH_SPEED);
}
FINSH_FUNCTION_EXPORT(em0, enter em0 mode);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(em0, enter em0 mode);
#endif

static void em2(void)
{
    rt_kprintf("enter EM2.\n");
    rt_pm_release(PM_SLEEP_MODE_NONE);
    rt_pm_run_enter(PM_RUN_MODE_LOW_SPEED);
}
FINSH_FUNCTION_EXPORT(em2, enter em2 mode);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(em2, enter em2 mode);
#endif

#include <drv_pin.h>
static void em4(void)
{
    rt_kprintf("enter EM4.\n");
    rt_uint16_t wakeup_pin = get_PinNumber(GPIO_F_PORT, 2);
    rt_pin_mode(wakeup_pin, PIN_MODE_INPUT_PULLUP);
    GPIO_EM4EnablePinWakeup(GPIO_EM4WUEN_EM4WUEN_F2, 0);

    rt_pm_release(PM_SLEEP_MODE_DEEP);
    rt_pm_request(PM_SLEEP_MODE_SHUTDOWN);
}
FINSH_FUNCTION_EXPORT(em4, enter em4 mode);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(em4, enter em4 mode);
#endif

#endif
