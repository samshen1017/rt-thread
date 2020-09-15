/*
 * @Author: ShenQuan 
 * @Date: 2019-05-23 14:22:22 
 * @Last Modified by:   ShenQuan 
 * @Last Modified time: 2019-05-23 14:22:22 
 */

#ifndef __DEV_LS027B7DH__
#define __DEV_LS027B7DH__
#include <rtthread.h>
#include <rtdevice.h>

void rt_hw_memlcd_clearScreen(void);
void rt_hw_memlcd_fillMemory(void *buf);
int rt_hw_memlcd_init(void);

#endif //__DEV_MEMLCD__i