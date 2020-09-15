#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"

/* 用于测试SysTick，和OSTick */

static void getRTTick(void)
{
    rt_kprintf("SysTick:%u\n", rt_tick_get());
}
FINSH_FUNCTION_EXPORT(getRTTick, get systick);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(getRTTick, get systick);
#endif

void getCoreClock(void)
{
    rt_kprintf("CoreClock:%uHz\n", SystemCoreClockGet());
}
FINSH_FUNCTION_EXPORT(getCoreClock, get CMU frequency);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(getCoreClock, get CMU frequency);
#endif

static void getSysTick(void)
{
    rt_kprintf("SysTick->VAL:%u, SysTick->LOAD:%u\n", SysTick->VAL, SysTick->LOAD);
}
FINSH_FUNCTION_EXPORT(getSysTick, get SysTick);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(getSysTick, get SysTick);
#endif
