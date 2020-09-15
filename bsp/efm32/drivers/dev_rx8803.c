
/* ********************************************************************************
 * 
 * Copyright (C) 2019 - 2020 Shanghai 3H Inc. All rights reserved.
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * File: dev_rx8803.c
 * File Created: Monday, 17th August 2020 03:55:17
 * Author: Sam.Shen (samshen1017@sina.com)
 * 
 * Last Modified: Monday, 14th September 2020 04:50:30
 * Modified By: Sam.Shen (samshen1017@sina.com>)
 * 
 * Automatically generated; DO NOT EDIT.
 * 
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 * ********************************************************************************/

#include "dev_rx8803.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <time.h>
#include "drv_pin.h"

#ifdef RT_USING_RX8803

/* register definition */
#define REG_Secnods 0x00
#define REG_Minutes 0x01
#define REG_Hours 0x02
#define REG_Weekdays 0x03
#define REG_Days 0x04
#define REG_Months 0x05
#define REG_Years 0x06
#define REG_RAM 0x07
#define REG_AlarmW_Min 0x08
#define REG_AlarmW_Hour 0x09
#define REG_AlarmW_Weekday 0x0A
#define REG_TimerCounter0 0x0B
#define REG_TimerCounter1 0x0C

#define REG_Extension 0x0D
#define EXTEN_TEST (1 << 7)
#define EXTEN_WADA (1 << 6)
#define EXTEN_USEL (1 << 5)
#define EXTEN_TE (1 << 4)
#define EXTEN_FSEL1 (1 << 3)
#define EXTEN_FSEL0 (1 << 2)
#define EXTEN_TSEL1 (1 << 1)
#define EXTEN_TSEL0 (1 << 0)

#define REG_Flag 0x0E
#define FLAG_UF (1 << 5)
#define FLAG_TF (1 << 4)
#define FLAG_AF (1 << 3)
#define FLAG_VLF (1 << 1)
#define FLAG_VDET (1 << 0)

#define REG_Control 0x0F
#define CONTR_CSEL1 (1 << 7)
#define CONTR_CSEL0 (1 << 6)
#define CONTR_UIE (1 << 5)
#define CONTR_TIE (1 << 4)
#define CONTR_AIE (1 << 3)
#define CONTR_RESET (1 << 0)

/* IIC address */
/* 7bit, 0110010 */
#define RX8803_ADDR 0x32

/* Flag Reg */

#ifndef RT_USING_I2C
#error "This RTC device need i2c bus(please enable RT_USING_I2C in rtconfig.h)"
#endif

#define RX8803_DEBUG
#ifdef RX8803_DEBUG
#define rx8803_debug(format, args...) rt_kprintf(format, ##args)
#else
#define rx8803_debug(format, args...)
#endif

typedef struct
{
    struct rt_device device;
    rt_uint16_t int_pin;
    void (*callback)(void *args);
} rx8803;

static rx8803 _rx8803 = {.callback = RT_NULL};

/* convert DEC to BCD */
static rt_uint8_t DECtoBCD(rt_uint8_t x)
{
    return (((x / 10) << 4) + (x % 10));
}

/* convert BCD to DEC */
static rt_uint8_t BCDtoDEC(rt_uint8_t x)
{
    return (((x >> 4) * 10) + (x & 0x0F));
}

static rt_size_t _rx8803_read(rt_uint8_t reg, rt_uint8_t *data, rt_size_t size)
{
    RT_ASSERT(data != RT_NULL);
    rx8803_debug("_rx8803_read\n");
    rt_size_t ret = 0;
    struct rt_i2c_msg msgs[1];
    rt_uint8_t i2c_write_data[1];

    msgs[0].addr = RX8803_ADDR;
    msgs[0].flags = RT_I2C_WRITEREAD;

    /* Select register to start reading from */
    i2c_write_data[0] = reg;
    msgs[0].buf = i2c_write_data;
    msgs[0].len = 1;

    /* Select length of data to be read */
    msgs[0].buf2 = data;
    msgs[0].len2 = size;
    ret = rt_i2c_transfer((struct rt_i2c_bus_device *)_rx8803.device.user_data, msgs, 1);
    if (ret == 1)
    {
        return size;
    }
    return 0;
}

static rt_size_t _rx8803_write(rt_uint8_t reg, rt_uint8_t *data, rt_size_t size)
{
    RT_ASSERT(data != RT_NULL);
    rx8803_debug("_rx8803_write\n");
    rt_size_t ret = 0;
    struct rt_i2c_msg msgs[1];
    rt_uint8_t data_t[8] = {0};

    data_t[0] = reg;
    rt_memcpy(&data_t[1], data, size);
    msgs[0].addr = RX8803_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = data_t;
    msgs[0].len = size + 1;
    ret = rt_i2c_transfer((struct rt_i2c_bus_device *)_rx8803.device.user_data, msgs, 1);
    if (ret == 1)
    {
        return size;
    }
    return 0;
}

static rt_err_t _rx8803_init(void)
{
    rx8803_debug("_rx8803_init\n");
    rt_size_t size;
    rt_uint8_t ctrl[1] = {0x01};      //init Control Register
    rt_uint8_t flag[1] = {0x00};      //clear state
    rt_uint8_t extension[1] = {0x00}; //init Extension

    size = _rx8803_write(REG_Control, ctrl, 1);
    if (size != 1)
    {
        rx8803_debug("set REG_Control failed.\n");
        return -RT_ERROR;
    }

    size = _rx8803_write(REG_Extension, extension, 1);
    if (size != 1)
    {
        rx8803_debug("set REG_Extension failed.\n");
        return -RT_ERROR;
    }

    size = _rx8803_write(REG_Flag, flag, 1);
    if (size != 1)
    {
        rx8803_debug("set REG_Flag failed.\n");
        return -RT_ERROR;
    }
    return RT_EOK;
}
#ifdef RT_USING_FINSH
FINSH_FUNCTION_EXPORT(_rx8803_init, _rx8803_init);
#endif
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(_rx8803_init, _rx8803_init);
#endif

static rt_err_t _rtc_init(rt_device_t dev)
{
    RT_ASSERT(dev != RT_NULL);
    rx8803_debug("_rtc_init\n");
    dev->user_data = rt_device_find(RX8803_USING_I2C_NAME);
    if (dev->user_data == RT_NULL)
    {
        rt_kprintf("not find %s\n", RX8803_USING_I2C_NAME);
        return -RT_ERROR;
    }
    return _rx8803_init();
}

static rt_err_t rt_rtc_control(rt_device_t dev, int cmd, void *args)
{
    rx8803_debug("rt_rtc_control\n");
    int ret = 0;
    rt_uint8_t data[7];
    struct tm t, *p_tm;
    switch (cmd)
    {
    case RT_DEVICE_CTRL_RTC_GET_TIME:
    {
        ret = _rx8803_read(REG_Secnods, data, 7);
        if (ret != 7)
        {
            return -RT_ERROR;
        }
        t.tm_sec = BCDtoDEC(data[0] & 0x7F);
        t.tm_min = BCDtoDEC(data[1]);
        t.tm_hour = BCDtoDEC(data[2]);
        t.tm_wday = BCDtoDEC(data[3]);
        t.tm_mday = BCDtoDEC(data[4]);
        t.tm_mon = BCDtoDEC(data[5] & 0x1F) - 1; /* tm_mon: 0-11 */
        t.tm_year = 100 + BCDtoDEC(data[6]);     /* 2000 - 2099 */
        *(time_t *)args = mktime(&t);
        break;
    }
    case RT_DEVICE_CTRL_RTC_SET_TIME:
    {
        p_tm = localtime_r((time_t *)args, &t);
        if (p_tm->tm_year < 100)
        {
            return -RT_ERROR;
        }
        data[0] = DECtoBCD(p_tm->tm_sec);
        data[1] = DECtoBCD(p_tm->tm_min);
        data[2] = DECtoBCD(p_tm->tm_hour);
        data[3] = DECtoBCD(p_tm->tm_wday);
        data[4] = DECtoBCD(p_tm->tm_mday);
        data[5] = DECtoBCD((p_tm->tm_mon + 1));  /* data[mon]: 1-12 */
        data[6] = DECtoBCD(p_tm->tm_year - 100); /* 2000 - 2099 */
        ret = _rx8803_write(REG_Secnods, data, 7);
        if (ret != 7)
        {
            return -RT_ERROR;
        }
        break;
    }
    default:
        rt_kprintf("unknow cmd.\n");
        return -RT_ERROR;
    }
    return RT_EOK;
}

static void rtc_irq(void *args)
{
    _rx8803.callback(args);
}

rt_err_t rtc_timer_Start(int interval_sec)
{
    rt_kprintf("rtc_timer_Start\n");
    RT_ASSERT(_rx8803.callback != RT_NULL);
    rt_size_t size;

    /* Set Timer Counter0 register */
    rt_uint8_t cnt0 = interval_sec & 0xFF;
    size = _rx8803_write(REG_TimerCounter0, &cnt0, 1);
    if (size != 1)
    {
        rt_kprintf("_rx8803_write failed.\n");
        return RT_EIO;
    }

    /* Set Timer Counter1 register */
    rt_uint8_t cnt1 = (interval_sec & 0xFF00) >> 8;
    size = _rx8803_write(REG_TimerCounter1, &cnt1, 1);
    if (size != 1)
    {
        rt_kprintf("_rx8803_write failed.\n");
        return RT_EIO;
    }

    /* Set Extension Register*/
    /* TSEL1=1,TSEL0=0 设置秒为最小单位， TE为使能 */
    rt_uint8_t extension = EXTEN_TE | EXTEN_TSEL1;
    size = _rx8803_write(REG_Extension, &extension, 1);
    if (size != 1)
    {
        rt_kprintf("_rx8803_write failed.\n");
        return RT_EIO;
    }

    /* Set Control Register */
    /* TIE=1 设置Timer产生外部中断 */
    rt_uint8_t ctrl = CONTR_TIE;
    size = _rx8803_write(REG_Control, &ctrl, 1);
    if (size != 1)
    {
        rt_kprintf("_rx8803_write failed.\n");
        return RT_EIO;
    }

    /* Set interrupt pin */
    _rx8803.int_pin = get_PinNumber(GPIO_B_PORT, 10);
    rt_pin_mode(_rx8803.int_pin, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(_rx8803.int_pin, PIN_IRQ_MODE_FALLING, rtc_irq, RT_NULL);
    rt_pin_irq_enable(_rx8803.int_pin, PIN_IRQ_ENABLE);
    return RT_EOK;
}

void rtc_timer_Stop(void)
{
    rt_kprintf("rtc_timer_Stop\n");
    RT_ASSERT(_rx8803.callback != RT_NULL);
    rt_size_t size;

    /* Set Extension Register*/
    rt_uint8_t extension = 0x0;
    size = _rx8803_write(REG_Extension, &extension, 1);
    if (size != 1)
    {
        rt_kprintf("_rx8803_write failed.\n");
    }
}

void rtc_timer_SetTimerCallback(void (*cb)(void *args))
{
    _rx8803.callback = cb;
    rt_kprintf("rtc_timer_SetTimerCallback\n");
}

void rt_hw_rx8803_init(void)
{
    rx8803_debug("rt_hw_rx8803_init\n");

    /* Register RTC device */
    _rx8803.device.type = RT_Device_Class_RTC;
    _rx8803.device.init = _rtc_init;
    _rx8803.device.control = rt_rtc_control;

    rt_device_register((rt_device_t)&_rx8803, RX8803_RTC_NAME, RT_DEVICE_FLAG_RDWR);
}

/**
 * Used for test
 */

static void setdate(int argc, void *argv[])
{
    if (argc < 2)
    {
        return;
    }
    char *arg = argv[1];
    int min = atoi(arg);
    rt_kprintf("min:%d\n", min);
    set_date(2019, 8, 23);
    set_time(14, min, 22);
}
#ifdef RT_USING_FINSH
FINSH_FUNCTION_EXPORT(setdate, set date);
#endif
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(setdate, set date);
#endif

static void rtc_read(int argc, void **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: rtc_read [REG ADDR(0x__)]\n");
        return;
    }

    rt_device_t rtc_dev = rt_device_find(RX8803_USING_I2C_NAME);
    if (rtc_dev == RT_NULL)
    {
        rt_kprintf("not find %s\n", RX8803_USING_I2C_NAME);
        return;
    }

    rt_uint8_t reg = strtol(argv[1], NULL, 16);

    struct rt_i2c_msg msgs[1];
    rt_uint8_t writebuf[1];
    rt_uint8_t readbuf[1];

    msgs[0].addr = RX8803_ADDR;
    msgs[0].flags = RT_I2C_WRITEREAD;

    /* Select register to start reading from */
    writebuf[0] = reg;
    msgs[0].buf = writebuf;
    msgs[0].len = 1;

    /* Select length of data to be read */
    msgs[0].buf2 = readbuf;
    msgs[0].len2 = 1;
    rt_size_t ret = rt_i2c_transfer((struct rt_i2c_bus_device *)rtc_dev, msgs, 1);
    if (ret != 1)
    {
        rt_kprintf("rt_i2c_transfer failed.\n");
        return;
    }
    rt_kprintf("val:0x%x\n", readbuf[0]);
}
#ifdef RT_USING_FINSH
FINSH_FUNCTION_EXPORT(rtc_read, rtc read reg);
#endif
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(rtc_read, rtc read reg);
#endif

static void rtc_write(int argc, void **argv)
{
    if (argc < 3)
    {
        rt_kprintf("Usage: rtc_read [REG ADDR(0x__)] [VALUE(0x__)]\n");
        return;
    }

    rt_device_t rtc_dev = rt_device_find(RX8803_USING_I2C_NAME);
    if (rtc_dev == RT_NULL)
    {
        rt_kprintf("not find %s\n", RX8803_USING_I2C_NAME);
        return;
    }

    rt_uint8_t reg = strtol(argv[1], NULL, 16);
    rt_uint8_t value = strtol(argv[2], NULL, 16);

    struct rt_i2c_msg msgs[1];
    rt_uint8_t writebuf[2];

    msgs[0].addr = RX8803_ADDR;
    msgs[0].flags = RT_I2C_WR;

    /* Select register to start reading from */
    writebuf[0] = reg;
    writebuf[1] = value;
    msgs[0].buf = writebuf;
    msgs[0].len = 2;

    rt_size_t ret = rt_i2c_transfer((struct rt_i2c_bus_device *)rtc_dev, msgs, 1);
    if (ret != 1)
    {
        rt_kprintf("rt_i2c_transfer failed.\n");
        return;
    }
    rt_kprintf("written\n");
}
#ifdef RT_USING_FINSH
FINSH_FUNCTION_EXPORT(rtc_write, rtc write reg);
#endif
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(rtc_write, rtc write reg);
#endif

static void cb(void *args)
{
    rt_kprintf("-\n");
}

static void rtc_tim_str()
{
    time(RT_NULL);
    rtc_timer_SetTimerCallback(cb);
    rtc_timer_Start(1);
}
#ifdef RT_USING_FINSH
FINSH_FUNCTION_EXPORT(rtc_tim_str, rtc timer start);
#endif
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(rtc_tim_str, rtc timer start);
#endif

static void rtc_tim_stp()
{
    rtc_timer_Stop();
}
#ifdef RT_USING_FINSH
FINSH_FUNCTION_EXPORT(rtc_tim_stp, rtc timer stop);
#endif
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(rtc_tim_stp, rtc timer stop);
#endif

#endif // RT_USING_RX8803