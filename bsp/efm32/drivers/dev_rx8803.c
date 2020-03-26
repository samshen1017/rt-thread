/*
 * File      : drv_rx8803.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2008 - 2012, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author          Notes
 * 2012-09-26     heyuanjie87     the first version
 * 2013-11-05     aozima          fix issues: the mon register is 0-11.
 */

#include <time.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "dev_rx8803.h"
#include "board.h"
#include "drv_pin.h"
#include "string.h"

#ifdef RT_USING_RX8803

#define RX8803_ADDR 0x32
#define RTC_EINT_PORT GPIO_D_PORT
#define RTC_EINT_PIN 9

#ifndef RT_USING_I2C
#error "This RTC device need i2c bus(please enable RT_USING_I2C in rtconfig.h)"
#endif

static struct i2c_slave _rtc;
static rt_int32_t rtc_eint;

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

static int DEC_H(int x)
{
    return (x / 256);
}

static int DEC_L(int x)
{
    return (x % 256);
}

static rt_size_t rx8803sa_read(rt_uint8_t reg,
                               rt_uint8_t *data,
                               rt_size_t size)
{
    rt_size_t ret = 0;
    struct rt_i2c_msg msgs[1];
    uint8_t i2c_write_data[1];
    if (_rtc.device.user_data == RT_NULL)
        return (0);
    msgs[0].addr = RX8803_ADDR;
    msgs[0].flags = RT_I2C_WRITEREAD;
    /* Select register to start reading from */
    i2c_write_data[0] = reg;
    msgs[0].buf = i2c_write_data;
    msgs[0].len = 1;
    /* Select length of data to be read */
    msgs[0].buf2 = data;
    msgs[0].len2 = size;
    ret = rt_i2c_transfer((struct rt_i2c_bus_device *)_rtc.device.user_data, msgs, 1);
    if (ret == 1)
    {
        return size;
    }
    else
        return 0;
}

static rt_size_t rx8803sa_write(rt_uint8_t reg,
                                rt_uint8_t *data,
                                rt_size_t size)
{
    rt_size_t ret = 0;
    struct rt_i2c_msg msgs[1];
    rt_uint8_t data_t[8] = {0};

    if (_rtc.device.user_data == RT_NULL)
        return (0);
    data_t[0] = reg;
    memcpy(&data_t[1], data, size);
    msgs[0].addr = RX8803_ADDR;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = data_t;
    msgs[0].len = size + 1;
    ret = rt_i2c_transfer((struct rt_i2c_bus_device *)_rtc.device.user_data, msgs, 1);
    if (ret == 1)
    {
        return size;
    }
    else
        return 0;
}
/**
 * The following is rt-thread device operating interface
 */
static rt_err_t rt_rtc_init(rt_device_t dev)
{
    return (RT_EOK);
}

static void rtc_eint_on(void)
{
    int rtc_eint;
    rtc_eint = get_PinNumber(RTC_EINT_PORT, RTC_EINT_PIN);
    rt_pin_irq_enable(rtc_eint, PIN_IRQ_ENABLE);
}

static void rtc_eint_off(void)
{
    int rtc_eint;
    rtc_eint = get_PinNumber(RTC_EINT_PORT, RTC_EINT_PIN);
    rt_pin_irq_enable(rtc_eint, PIN_IRQ_DISABLE);
}

static void eint(int argc, void *argv[])
{
    char *arg = argv[1];
    int x = atoi(arg);
    if (x != 0)
    {
        rtc_eint_on();
    }
    else
    {
        rtc_eint_off();
    }
}

static void RTC_handler(void *args)
{
    rt_uint32_t val = 1;
    val = rt_pin_read(rtc_eint);
    if (val == 0)
    {
        rt_kprintf("Trigger\r\n");
    }
}

static rt_err_t rt_rx8803_eint_init(void)
{
    rtc_eint = get_PinNumber(RTC_EINT_PORT, RTC_EINT_PIN);
    rt_pin_mode(rtc_eint, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(rtc_eint, PIN_IRQ_MODE_FALLING, RTC_handler, RT_NULL);
    rt_pin_irq_enable(rtc_eint, PIN_IRQ_ENABLE);
    return RT_EOK;
}

static rt_err_t rt_rx8803_timetask_start(void)
{
    rt_uint8_t data[2];
    rt_err_t ret = -RT_ERROR;
    data[0] = 18;
    ret = rx8803sa_write(0x0D, data, 1);
    if (ret != 1)
    {
        return -RT_ERROR;
    }
    return RT_EOK;
}

static rt_err_t rt_rx8803_timetask_stop(void)
{
    rt_uint8_t data[2];
    rt_err_t ret = -RT_ERROR;
    data[0] = 2;
    ret = rx8803sa_write(0x0D, data, 1);
    if (ret != 1)
    {
        return -RT_ERROR;
    }
    return RT_EOK;
}

static rt_err_t rt_rx8803_timetask_set(rt_time_t time)
{
    rt_uint8_t data[2];
    rt_err_t ret = -RT_ERROR;
    if (time > 4095)
    {
        rt_kprintf("value must be equal to or less than 4095\r\n");
        return -RT_ERROR;
    }
    data[0] = (rt_uint8_t)DEC_L(time);
    // rt_kprintf("x_L: %d\r\n", (int)data[0]);
    data[1] = (rt_uint8_t)DEC_H(time);
    // rt_kprintf("x_H: %d\r\n", (int)data[1]);
    ret = rx8803sa_write(0x0B, data, 2);
    if (ret != 2)
    {
        return -RT_ERROR;
    }
    return ret;
}

static rt_err_t rt_rx8803_init(void)
{
    rt_uint8_t data[2];
    rt_err_t ret = -RT_ERROR;

    if (_rtc.device.user_data == RT_NULL)
        _rtc.device.user_data = rt_device_find(RX8803_USING_I2C_NAME);

    /* repeated mode,enable timer interrupt */

    if (_rtc.device.user_data == RT_NULL)
        return (0);

    data[0] = 2;
    ret = rx8803sa_write(0x0D, data, 1);
    if (ret != 1)
    {
        return -RT_ERROR;
    }

    data[0] = 46;
    ret = rx8803sa_write(0x0E, data, 1);
    if (ret != 1)
    {
        return -RT_ERROR;
    }

    data[0] = 80;
    ret = rx8803sa_write(0x0F, data, 1);
    if (ret != 1)
    {
        return -RT_ERROR;
    }
    //_exit:
    rt_rx8803_eint_init();
    return RT_EOK;
}

static rt_err_t rt_rtc_open(rt_device_t dev, rt_uint16_t oflag)
{
    rt_err_t ret = RT_EOK;

    struct i2c_slave *slave;

    RT_ASSERT(dev != RT_NULL);

    if (!dev->user_data)
    {
        slave = (struct i2c_slave *)dev;

        dev->user_data = rt_device_find(slave->busname);
        if (dev->user_data == RT_NULL)
            ret = -RT_ERROR;

        if (RT_EOK == rt_rx8803_init())
        {
            rt_kprintf("8803 set OK\r\n");
        }
        else
        {
            rt_kprintf("8803 set  ERROR\r\n");
        }
    }

    return (ret);
}

static rt_err_t rt_rtc_close(rt_device_t dev)
{
    return (RT_EOK);
}

static rt_err_t rt_rtc_control(rt_device_t dev, int cmd, void *args)
{
    int ret = 0;
    int *time = 0;
    rt_uint8_t data[7];
    struct tm t, *p_tm;
    switch (cmd)
    {
    case RT_DEVICE_CTRL_RTC_GET_TIME:
    {
        ret = rx8803sa_read(0, data, 7);
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
        ret = rx8803sa_write(0, data, 7);
        if (ret != 7)
        {
            return -RT_ERROR;
        }
        break;
    }
    // case RT_DEVICE_CTRL_RTC_SET_TIMER:
    // {
    //     time = (int *)args;
    //     memset(data,0,sizeof data);
    //     data[0] = DEC_L(*time);
    //     data[1] = DEC_H(*time);
    //     rt_kprintf("time = %d\r\n",*time);
    //     ret = rx8803sa_write(0x0B, data, 2);
    //     if (ret != 2)
    //     {
    //         return -RT_ERROR;
    //     }
    //     break;
    // }
    // case RT_DEVICE_CTRL_RTC_GET_TIMER:
    // {
    //     ret = rx8803sa_read(0x0B, data, 2);
    //     if (ret != 2)
    //     {
    //        return -RT_ERROR;
    //     }
    //     *(int *)args = data[1] * 256 + data[0];
    //     break;
    // }
    default:
    {
        rt_kprintf("unknow cmd.\n");
        return -RT_ERROR;
    }
    }
    return RT_EOK;
}

void rt_hw_rx8803_init(void)
{
    rt_device_t device;
    _rtc.busname = (char *)RX8803_USING_I2C_NAME;
    // _rtc.busname = "i2c0";

    device = (rt_device_t)&_rtc;

    /* Register RTC device */
    device->type = RT_Device_Class_RTC;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;
    device->init = rt_rtc_init;
    device->open = rt_rtc_open;
    device->close = rt_rtc_close;
    device->read = RT_NULL;
    device->write = RT_NULL;
    device->control = rt_rtc_control;
    device->user_data = RT_NULL;

    rt_device_register(device, RX8803_RTC_NAME, RT_DEVICE_FLAG_RDWR);
}

/**
 * Used for test
 */

static void rtcread(void)
{
    int i;
    rt_uint8_t data[10] = {0};
    rt_memset(data, 0, 7);
    _rtc.device.user_data = rt_device_find(_rtc.busname);
    i = rx8803sa_read(0, data, 7);

    for (i = 0; i < 7; i++)
        rt_kprintf("<%x>", data[i]);
    rt_kprintf("\n");
}

static void rtcwrite(void)
{
    rt_uint8_t data[7] = {DECtoBCD(55), DECtoBCD(59), DECtoBCD(23), DECtoBCD(1), DECtoBCD(31), DECtoBCD(12), DECtoBCD(19)};
    _rtc.device.user_data = rt_device_find(_rtc.busname);
    rx8803sa_write(0, data, 7);
}

static void setdate(int argc, void *argv[])
{
    if (argc < 2)
    {
        return;
    }
    char *arg = argv[1];
    int year = atoi(arg);
    rt_kprintf("year:%d\n", year);
    set_date(year, 10, 30);
    set_time(9, 56, 10);
}

static void setreg(int argc, void *argv[])
{
    int reg;
    rt_uint8_t data[2];
    reg = atoi(argv[1]);
    data[0] = (rt_uint8_t)atoi(argv[2]);
    rx8803sa_write((rt_uint8_t)reg, data, 1);
}

static void readreg(int argc, void *argv[])
{
    char *arg = argv[1];
    rt_uint8_t data[1];
    int x = atoi(arg);
    rx8803sa_read((rt_uint8_t)x, data, 1);
    rt_kprintf("reg:%d = %d\r\n", x, data[0]);
}

static void settimetask(int argc, void *argv[])
{
    char *arg = argv[1];
    int x = atoi(arg);
    rt_rx8803_timetask_set(x);
}

static void timetaskstart(int argc, void *argv[])
{
    rt_rx8803_timetask_start();
}

static void timetaskstop(int argc, void *argv[])
{
    rt_rx8803_timetask_stop();
}

#ifdef RT_USING_FINSH
FINSH_FUNCTION_EXPORT(setreg, set reg);
FINSH_FUNCTION_EXPORT(readreg, read alarm);
FINSH_FUNCTION_EXPORT(setdate, set date);
FINSH_FUNCTION_EXPORT(rtcread, read rtc);
FINSH_FUNCTION_EXPORT(rtcwrite, write rtc);
FINSH_FUNCTION_EXPORT(settimetask, set timetask);
FINSH_FUNCTION_EXPORT(timetaskstart, timetask start);
FINSH_FUNCTION_EXPORT(timetaskstop, timetask stop);
// FINSH_FUNCTION_EXPORT(set_alarm, set alarm);
#endif /* RT_USING_FINSH */
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(setreg, set reg);
MSH_CMD_EXPORT(readreg, read reg);
MSH_CMD_EXPORT(setdate, set date);
MSH_CMD_EXPORT(rtcread, read rtc);
MSH_CMD_EXPORT(rtcwrite, write rtc);
MSH_CMD_EXPORT(eint, set rtc_eint);
MSH_CMD_EXPORT(settimetask, set timetask);
MSH_CMD_EXPORT(timetaskstart, timetask start);
MSH_CMD_EXPORT(timetaskstop, timetask stop);
// MSH_CMD_EXPORT(set_alarm, set alarm);
#endif
// #endif /* DRV_FUNCTION_EXPORT */

#endif /* RT_USING_RX8803 */
