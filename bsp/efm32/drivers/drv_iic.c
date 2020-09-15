/* ********************************************************************************
 * 
 * Copyright (C) 2019 - 2020 Shanghai 3H Inc. All rights reserved.
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * File: drv_iic.c
 * File Created: Thursday, 23rd July 2020 05:41:27
 * Author: Sam.Shen (samshen1017@sina.com)
 * 
 * Last Modified: Friday, 24th July 2020 04:05:41
 * Modified By: Sam.Shen (samshen1017@sina.com>)
 * 
 * Automatically generated; DO NOT EDIT.
 * 
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 * ********************************************************************************/

#include "drv_iic.h"
#include "em_cmu.h"
#include "em_device.h"
#include "em_i2c.h"
#include "em_gpio.h"
#include "rtdbg.h"
#include <rtthread.h>
#include <rtdevice.h>

#if defined(RT_USING_I2C)
#ifdef RT_I2C_DEBUG
#define iic_debug(format, args...) rt_kprintf(format, ##args)
#else
#define iic_debug(format, args...)
#endif

typedef struct
{
    struct rt_i2c_bus_device bus;
    /* Peripheral port */
    I2C_TypeDef *port;
    /* SCL */
    GPIO_Port_TypeDef sclPort;
    uint8_t sclPin;
    /* SDL */
    GPIO_Port_TypeDef sdaPort;
    uint8_t sdaPin;
    /* location */
    uint8_t portLocation;
    /* I2C reference clock */
    uint32_t i2cRefFreq;
    /* I2C max bus frequency to use */
    uint32_t i2cMaxFreq;
    /* Clock low/high ratio control */
    I2C_ClockHLR_TypeDef i2cClhr;
    /* Lock */
    struct rt_semaphore lock;
} efm32_i2c_bus;

#if defined(RT_USING_I2C0)
static efm32_i2c_bus i2c0_bus = {
    .port = I2C0,                               /* Use I2C instance 0 */
    .sclPort = AF_I2C0_SCL_PORT(I2C0_LOCATION), /* SCL port */
    .sclPin = AF_I2C0_SCL_PIN(I2C0_LOCATION),   /* SCL pin */
    .sdaPort = AF_I2C0_SDA_PORT(I2C0_LOCATION), /* SDA port */
    .sdaPin = AF_I2C0_SDA_PIN(I2C0_LOCATION),   /* SDA pin */
    .portLocation = I2C0_LOCATION,              /* Location */
    .i2cRefFreq = 0,                            /* Use currently configured reference clock */
    .i2cMaxFreq = I2C_FREQ_FAST_MAX,            /* Set to standard rate  */
    .i2cClhr = i2cClockHLRFast,                 /* Set to use 4:4 low/high duty cycle */
};
#endif

#if defined(RT_USING_I2C1)
static efm32_i2c_bus i2c1_bus = {
    .port = I2C1,                               /* Use I2C instance 0 */
    .sclPort = AF_I2C1_SCL_PORT(I2C1_LOCATION), /* SCL port */
    .sclPin = AF_I2C1_SCL_PIN(I2C1_LOCATION),   /* SCL pin */
    .sdaPort = AF_I2C1_SDA_PORT(I2C1_LOCATION), /* SDA port */
    .sdaPin = AF_I2C1_SDA_PIN(I2C1_LOCATION),   /* SDA pin */
    .portLocation = I2C1_LOCATION,              /* Location */
    .i2cRefFreq = 0,                            /* Use currently configured reference clock */
    .i2cMaxFreq = I2C_FREQ_STANDARD_MAX,        /* Set to standard rate  */
    .i2cClhr = i2cClockHLRStandard,             /* Set to use 4:4 low/high duty cycle */
};
#endif

static I2C_TransferReturn_TypeDef i2c_Transfer(efm32_i2c_bus *i2c_bus, I2C_TransferSeq_TypeDef *seq)
{
    I2C_TransferReturn_TypeDef ret;
    uint32_t timeout = IIC_TRANSFER_TIMEOUT;
    /* Do a polled transfer */
    ret = I2C_TransferInit(i2c_bus->port, seq);
    while (ret == i2cTransferInProgress && timeout--)
    {
        ret = I2C_Transfer(i2c_bus->port);
    }
    return ret;
}

static rt_size_t i2c_read(efm32_i2c_bus *i2c_bus, rt_off_t pos, void *buffer, rt_size_t size)
{
    rt_err_t err_code = RT_EOK;
    rt_size_t read_size = 0;
    I2C_TransferSeq_TypeDef seq;
    I2C_TransferReturn_TypeDef ret;
    if (size == 0)
    {
        return 0;
    }
    rt_sem_take(&(i2c_bus->lock), RT_WAITING_FOREVER);
    seq.addr = (rt_uint16_t)(pos << 1) | 0x1; //可能有问题
    seq.flags = I2C_FLAG_READ;
    seq.buf[0].data = (rt_uint8_t *)buffer;
    seq.buf[0].len = size;
    ret = i2c_Transfer(i2c_bus, &seq);
    if (ret != i2cTransferDone)
    {
        iic_debug("read err %d\n", ret);
        if (ret == i2cTransferInProgress)
        {
            err_code = RT_ETIMEOUT;
        }
        else
        {
            err_code = RT_ERROR;
        }
    }
    else
    {
        read_size = size;
        iic_debug("read size %d\n", read_size);
    }
    rt_sem_release(&(i2c_bus->lock));
    /* set error code */
    rt_set_errno(err_code);
    return read_size;
}

static rt_size_t i2c_write(efm32_i2c_bus *i2c_bus, rt_off_t pos, const void *buffer, rt_size_t size)
{
    rt_err_t err_code = RT_EOK;
    rt_size_t write_size = 0;
    I2C_TransferSeq_TypeDef seq;
    I2C_TransferReturn_TypeDef ret;
    if (size == 0)
    {
        return 0;
    }
    rt_sem_take(&(i2c_bus->lock), RT_WAITING_FOREVER);
    seq.addr = (rt_uint16_t)(pos << 1);
    seq.flags = I2C_FLAG_WRITE;
    seq.buf[0].data = (rt_uint8_t *)buffer;
    seq.buf[0].len = size;
    ret = i2c_Transfer(i2c_bus, &seq);
    if (ret != i2cTransferDone)
    {
        iic_debug("write err %d\n", ret);
        if (ret == i2cTransferInProgress)
        {
            err_code = RT_ETIMEOUT;
        }
        else
        {
            err_code = RT_ERROR;
        }
    }
    else
    {
        write_size = size;
        iic_debug("write size %d\n", write_size);
    }
    rt_sem_release(&(i2c_bus->lock));
    /* set error code */
    rt_set_errno(err_code);
    return write_size;
}

static rt_size_t i2c_writeread(efm32_i2c_bus *i2c_bus, rt_off_t pos, const void *txbuf, rt_size_t tx_size, const void *rxbuf, rt_size_t rx_size)
{
    rt_err_t err_code = RT_EOK;
    rt_size_t size = 0;
    I2C_TransferSeq_TypeDef seq;
    I2C_TransferReturn_TypeDef ret;
    if ((tx_size == 0) || (rx_size == 0))
    {
        return 0;
    }
    rt_sem_take(&(i2c_bus->lock), RT_WAITING_FOREVER);
    seq.addr = (rt_uint16_t)(pos << 1) | 0x1;
    seq.flags = I2C_FLAG_WRITE_READ;
    seq.buf[0].data = (rt_uint8_t *)txbuf;
    seq.buf[0].len = tx_size;
    seq.buf[1].data = (rt_uint8_t *)rxbuf;
    seq.buf[1].len = rx_size;
    ret = i2c_Transfer(i2c_bus, &seq);
    if (ret != i2cTransferDone)
    {
        iic_debug("WriteRead err %d\n", ret);
        if (ret == i2cTransferInProgress)
        {
            err_code = RT_ETIMEOUT;
        }
        else
        {
            err_code = RT_ERROR;
        }
    }
    else
    {
        size = rx_size;
        iic_debug("w_size %d, r_size %d\n", tx_size, rx_size);
    }
    rt_sem_release(&(i2c_bus->lock));
    /* set error code */
    rt_set_errno(err_code);
    return size;
}

static rt_size_t master_xfer(struct rt_i2c_bus_device *bus, struct rt_i2c_msg msgs[], rt_uint32_t num)
{
    rt_size_t ret = (0);
    rt_uint32_t index = 0;
    efm32_i2c_bus *i2c = RT_NULL;
    struct rt_i2c_msg *msg = RT_NULL;
    RT_ASSERT(bus != RT_NULL);
    i2c = (efm32_i2c_bus *)bus;
    for (int i = 0; i < num; i++)
    {
        msg = &msgs[i];
        if (msg->flags & RT_I2C_RD)
        {
            ret = i2c_read(i2c, msg->addr, msg->buf, msg->len);
        }
        else if (msg->flags == RT_I2C_WR)
        {
            ret = i2c_write(i2c, msg->addr, msg->buf, msg->len);
        }
        else if (msg->flags & RT_I2C_WRITEREAD)
        {
            ret = i2c_writeread(i2c, msg->addr, msg->buf, msg->len, msg->buf2, msg->len2);
        }
        if (ret > 0)
        {
            index++;
        }
        else
        {
            return index;
        }
    }
    return index;
}

static rt_size_t slave_xfer(struct rt_i2c_bus_device *bus, struct rt_i2c_msg msgs[], rt_uint32_t num)
{
    return 0;
}

static rt_err_t i2c_bus_control(struct rt_i2c_bus_device *bus, rt_uint32_t cmd, rt_uint32_t arg)
{
    return RT_EOK;
}

static const struct rt_i2c_bus_device_ops ops =
    {
        master_xfer,
        slave_xfer,
        i2c_bus_control,
};

static void i2c_hw_init(efm32_i2c_bus *bus)
{
    RT_ASSERT(bus != RT_NULL);
    iic_debug("i2c_hw_init.\n");

    int i;
    CMU_Clock_TypeDef i2cClock;
    I2C_Init_TypeDef i2cInit;

#if defined(_CMU_HFPERCLKEN0_MASK)
    CMU_ClockEnable(cmuClock_HFPER, true);
#endif

    /* Select I2C peripheral clock */
    if (false)
    {
#if defined(I2C0)
    }
    else if (bus->port == I2C0)
    {
        i2cClock = cmuClock_I2C0;
#endif
#if defined(I2C1)
    }
    else if (bus->port == I2C1)
    {
        i2cClock = cmuClock_I2C1;
#endif
#if defined(I2C2)
    }
    else if (bus->port == I2C2)
    {
        i2cClock = cmuClock_I2C2;
#endif
    }
    else
    {
        /* I2C clock is not defined */
        rt_kprintf("I2C clock is not defined.\n");
        return;
    }
    CMU_ClockEnable(i2cClock, true);

    /* Output value must be set to 1 to not drive lines low. Set
     SCL first, to ensure it is high before changing SDA. */
    GPIO_PinModeSet(bus->sclPort, bus->sclPin, gpioModeWiredAndPullUp, 1);
    GPIO_PinModeSet(bus->sdaPort, bus->sdaPin, gpioModeWiredAndPullUp, 1);

    /* In some situations, after a reset during an I2C transfer, the slave
     device may be left in an unknown state. Send 9 clock pulses to
     set slave in a defined state. */
    for (i = 0; i < 9; i++)
    {
        GPIO_PinOutSet(bus->sclPort, bus->sclPin);
        GPIO_PinOutClear(bus->sclPort, bus->sclPin);
    }

    /* Enable pins and set location */
#if defined(_I2C_ROUTEPEN_MASK)
    bus->port->ROUTEPEN = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
    bus->port->ROUTELOC0 = (bus->portLocationSda << _I2C_ROUTELOC0_SDALOC_SHIFT) | (bus->portLocationScl << _I2C_ROUTELOC0_SCLLOC_SHIFT);
#elif defined(_I2C_ROUTE_MASK)
    bus->port->ROUTE = I2C_ROUTE_SDAPEN | I2C_ROUTE_SCLPEN | (bus->portLocation << _I2C_ROUTE_LOCATION_SHIFT);
#else
#if defined(I2C0)
    if (bus->port == I2C0)
    {
        GPIO->I2CROUTE[0].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;
        GPIO->I2CROUTE[0].SCLROUTE = (bus->sclPin << _GPIO_I2C_SCLROUTE_PIN_SHIFT) | (bus->sclPort << _GPIO_I2C_SCLROUTE_PORT_SHIFT);
        GPIO->I2CROUTE[0].SDAROUTE = (bus->sdaPin << _GPIO_I2C_SDAROUTE_PIN_SHIFT) | (bus->sdaPort << _GPIO_I2C_SDAROUTE_PORT_SHIFT);
    }
#endif
#if defined(I2C1)
    if (bus->port == I2C1)
    {
        GPIO->I2CROUTE[1].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;
        GPIO->I2CROUTE[1].SCLROUTE = (bus->sclPin << _GPIO_I2C_SCLROUTE_PIN_SHIFT) | (bus->sclPort << _GPIO_I2C_SCLROUTE_PORT_SHIFT);
        GPIO->I2CROUTE[1].SDAROUTE = (bus->sdaPin << _GPIO_I2C_SDAROUTE_PIN_SHIFT) | (bus->sdaPort << _GPIO_I2C_SDAROUTE_PORT_SHIFT);
    }
#endif
#if defined(I2C2)
    if (bus->port == I2C2)
    {
        GPIO->I2CROUTE[2].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;
        GPIO->I2CROUTE[2].SCLROUTE = (bus->sclPin << _GPIO_I2C_SCLROUTE_PIN_SHIFT) | (bus->sclPort << _GPIO_I2C_SCLROUTE_PORT_SHIFT);
        GPIO->I2CROUTE[2].SDAROUTE = (bus->sdaPin << _GPIO_I2C_SDAROUTE_PIN_SHIFT) | (bus->sdaPort << _GPIO_I2C_SDAROUTE_PORT_SHIFT);
    }
#endif
#endif

    /* Set emlib init parameters */
    i2cInit.enable = true;
    i2cInit.master = true; /* master mode only */
    i2cInit.freq = bus->i2cMaxFreq;
    i2cInit.refFreq = bus->i2cRefFreq;
    i2cInit.clhr = bus->i2cClhr;

    I2C_Init(bus->port, &i2cInit);
}

void rt_hw_iic_init(void)
{
#if defined(RT_USING_I2C0)
    i2c0_bus.bus.ops = &ops;
    rt_sem_init(&i2c0_bus.lock, "iic0", 1, RT_IPC_FLAG_FIFO);
    i2c_hw_init(&i2c0_bus);
    rt_i2c_bus_device_register(&i2c0_bus.bus, RT_I2C0_NAME);
#endif

#if defined(RT_USING_I2C1)
    i2c1_bus.bus.ops = &ops;
    rt_sem_init(&i2c1_bus.lock, "iic1", 1, RT_IPC_FLAG_FIFO);
    i2c_hw_init(&i2c1_bus);
    rt_i2c_bus_device_register(&i2c1_bus.bus, RT_I2C1_NAME);
#endif
}

#endif
