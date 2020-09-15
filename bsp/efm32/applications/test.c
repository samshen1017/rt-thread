#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include <stdio.h>
#include <string.h>
#include "dev_sram.h"
//#include "dev_zsc31014.h"
#include "drv_pm.h"
#include "drv_pin.h"

#ifdef RT_USING_ZSC31014
#include "drivers/sensors/sen_zsc31014.h"
#include "sensor.h"

static void zsc31014_init(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.dev_name = "i2c0";
    cfg.intf.user_data = (void *)ZSC31014_ADDR_DEFAULT;
    cfg.irq_pin.pin = RT_PIN_NONE;
    cfg.range = 1000000;
    rt_uint16_t pow_pin = get_PinNumber(GPIO_B_PORT, 0);
    rt_hw_zsc31014_init("zsc31014", &cfg, pow_pin);
    return;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(zsc31014_init, zsc31014 register to device);

static void zsc31014_read(void)
{
    rt_device_t dev = RT_NULL;
    struct rt_sensor_data data;
    rt_size_t res, i;

    /* 查找系统中的传感器设备 */
    dev = rt_device_find("baro_zsc");
    if (dev == RT_NULL)
    {
        rt_kprintf("Can't find device.\n");
        return;
    }

    /* 以轮询模式打开传感器设备 */
    if (rt_device_open(dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
    {
        rt_kprintf("open device failed!");
        return;
    }

    for (i = 0; i < 5; i++)
    {
        /* 从传感器读取一个数据 */
        res = rt_device_read(dev, 0, &data, 1);
        if (res != 1)
        {
            rt_kprintf("read data failed!size is %d", res);
        }
        else
        {
            //sensor_show_data(i, (rt_sensor_t)dev, &data);
            rt_kprintf("num:%3d, press:%5d pa, timestamp:%5d\n", i, data.data.baro, data.timestamp);
        }
    }

    /* 关闭传感器设备 */
    rt_device_close(dev);
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(zsc31014_read, zsc31014 read);
#endif

static rt_thread_t led_thread = RT_NULL;
static void rt_led_thread_entry(void *parameter)
{
    rt_uint16_t led_pin = get_PinNumber(GPIO_A_PORT, 11);
    rt_pin_mode(led_pin, PIN_MODE_OUTPUT);

    while (1)
    {
        rt_pin_write(led_pin, PIN_HIGH);
        rt_thread_mdelay(1000);
        rt_pin_write(led_pin, PIN_LOW);
        rt_thread_mdelay(1000);
    }
}

static void led(int argc, void **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: led [ON OFF(1 or 0)]\n");
        return;
    }

    if (led_thread == RT_NULL)
    {
        led_thread = rt_thread_create("led", rt_led_thread_entry, RT_NULL, 256, 24, 20);
    }
    rt_bool_t en = strtol(argv[1], NULL, 0);
    if (en)
    {
        rt_kprintf("led start!\n");
        rt_thread_startup(led_thread);
    }
    else
    {
        rt_kprintf("led stop!\n");
        rt_thread_suspend(led_thread);
    }
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(led, led test);

/* ***************************************************
 * *************************************************** */

/* 定时器的控制块 */
static rt_thread_t th_test;
static rt_uint32_t cnt;

static void Tim0Run(void *parameter)
{
    while (1)
    {
        time_t now = time(RT_NULL);
        rt_kprintf("cnt:%d rtc:%u\n", cnt, now);
        cnt++;
        rt_thread_mdelay(1000);
    }
}

int timer_sample(void)
{
    /* 初始化线程 */
    th_test = rt_thread_create("Tim0Th", Tim0Run, RT_NULL, 1024, 24, 20);
    rt_thread_startup(th_test);
    return 0;
}

/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(timer_sample, timer sample);

/* ***************************************************
 * *************************************************** */
