#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include <stdio.h>
#include <string.h>
#include "dev_sram.h"
//#include "dev_zsc31014.h"
#include "drv_pm.h"
#include "drv_pin.h"

/* ***************************************************
 * **************** Base Test Command ****************  
 * *************************************************** */
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

/* ***************************************************
 * *************************************************** */

#include <dfs_posix.h>
static void readwrite_sample(void)
{
    int fd, size;
    char s[] = "RT-Thread Programmer!", buffer[80];

    rt_kprintf("Write string %s to test.txt.\n", s);

    /* 以创建和读写模式打开 /text.txt 文件，如果该文件不存在则创建该文件 */
    fd = open("/text.txt", O_WRONLY | O_CREAT);
    if (fd >= 0)
    {
        write(fd, s, sizeof(s));
        close(fd);
        rt_kprintf("Write done.\n");
    }

    /* 以只读模式打开 /text.txt 文件 */
    fd = open("/text.txt", O_RDONLY);
    if (fd >= 0)
    {
        size = read(fd, buffer, sizeof(buffer));
        close(fd);
        rt_kprintf("Read from file test.txt : %s \n", buffer);
        if (size < 0)
            return;
    }
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(readwrite_sample, readwrite sample);

#include <stdio.h>
static void fs_test(void)
{
    FILE *f;
    f = fopen("/test.txt", "w");
    if (f == NULL)
    {
        rt_kprintf("fopen failed.\n");
        return;
    }
    char *str = "HelloWorld!\n";
    int length = strlen(str);
    int ret = fwrite(str, sizeof(char), length, f);
    if (ret != length)
    {
        rt_kprintf("fwrite failed.\n");
    }
    fflush(f);
    fclose(f);
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(fs_test, file system test);
#include "drivers/sensors/sensor_zsc31014.h"
static void sensor_test(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.dev_name = "i2c0";
    cfg.intf.user_data = (void *)ZSC31014_ADDR_DEFAULT;
    cfg.irq_pin.pin = RT_PIN_NONE;
    cfg.range = 1000000;

    rt_hw_zsc31014_init("zsc31014", &cfg);
    return 0;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(sensor_test, sensor test);

static void ble_test(void)
{
#ifdef RT_USING_PM
    rt_pm_request(PM_SLEEP_MODE_NONE);
#endif
    bluemod_sr_open();
    bluemod_sr_set_bname("0600FFFFFF");
#ifdef RT_USING_PM
    rt_pm_release(PM_SLEEP_MODE_NONE);
#endif
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(ble_test, ble test);

static void pin_ctl(int argc, char **argv)
{
    if (argc < 4)
    {
        rt_kprintf("Usage: io_test [gpio group (a ... f)] [gpio number] [0/1]\n");
    }
    else
    {
        char pin_port = (char)argv[1][0];
        int pin_number = strtol(argv[2], NULL, 0);
        int pin_state = strtol(argv[3], NULL, 0);
        rt_kprintf("_P%c%d : %d\n", pin_port - ('a' - 'A'), pin_number, pin_state);
        rt_uint16_t pin = get_PinNumber((GPIO_PIN_PORT)(pin_port - 'a' + 1), pin_number);
        rt_pin_mode(pin, PIN_MODE_OUTPUT);
        rt_pin_write(pin, pin_state);
    }
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(pin_ctl, gpio ctrl);

/* 指向信号量的指针 */
static rt_sem_t dynamic_sem = RT_NULL;

/* 定时器超时回调函数 */
static rt_err_t timeout_cb(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(dynamic_sem);
    return 0;
}

static int hwtimer_sample(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    rt_hwtimerval_t timeout_s;    /* 定时器超时值 */
    rt_device_t hw_dev = RT_NULL; /* 定时器设备句柄 */
    rt_hwtimer_mode_t mode;       /* 定时器模式 */

    /* 查找定时器设备 */
    hw_dev = rt_device_find(RT_LETIMER0_NAME);
    if (hw_dev == RT_NULL)
    {
        rt_kprintf("hwtimer sample run failed! can't find %s device!\n", RT_LETIMER0_NAME);
        return RT_ERROR;
    }

    /* 以读写方式打开设备 */
    ret = rt_device_open(hw_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK)
    {
        rt_kprintf("open %s device failed!\n", RT_LETIMER0_NAME);
        return ret;
    }

    /* 设置超时回调函数 */
    rt_device_set_rx_indicate(hw_dev, timeout_cb);

    /* 设置模式为周期性定时器 */
    mode = HWTIMER_MODE_PERIOD;
    ret = rt_device_control(hw_dev, HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK)
    {
        rt_kprintf("set mode failed! ret is :%d\n", ret);
        return ret;
    }

    /* 设置定时器超时值为5s并启动定时器 */
    timeout_s.sec = 1;  /* 秒 */
    timeout_s.usec = 0; /* 微秒 */

    if (rt_device_write(hw_dev, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
    {
        rt_kprintf("set timeout value failed\n");
        return RT_ERROR;
    }

    /* 读取定时器当前值 */
    rt_device_read(hw_dev, 0, &timeout_s, sizeof(timeout_s));
    rt_kprintf("Read: Sec = %d, Usec = %d\n", timeout_s.sec, timeout_s.usec);

    return ret;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(hwtimer_sample, hwtimer sample);

static char str[50] = {0};
static rt_thread_t t_thread;
static void test_entry(void *args)
{
    static rt_err_t result;
    static rt_uint8_t number = 0;
    while (1)
    {
        /* 永久方式等待信号量，获取到信号量，则执行 number 自加的操作 */
        result = rt_sem_take(dynamic_sem, RT_WAITING_FOREVER);
        if (result != RT_EOK)
        {
            rt_kprintf("t2 take a dynamic semaphore, failed.\n");
            rt_sem_delete(dynamic_sem);
            return;
        }
        else
        {
            number++;
            //rt_kprintf("t2 take a dynamic semaphore. number = %d\n", number);
            time_t m_now = time(RT_NULL);
            rt_uint8_t temp[4] = {0, 0, 0, 0};
            //zsc31014_read(temp, 4);
            sprintf(str, "_ %u %x %x %x %x\n", (rt_uint32_t)m_now, temp[0], temp[1], temp[2], temp[3]);
            if (number > 4)
            {
                rt_pm_request(PM_SLEEP_MODE_NONE);
                FILE *f;
                f = fopen("/test.txt", "ab");
                fwrite(str, 1, 50, f);
                fclose(f);
                number = 0;
                rt_pm_release(PM_SLEEP_MODE_NONE);
            }
        }
    }
}

void test_th(void)
{
    /* 创建一个动态信号量，初始值是 0 */
    dynamic_sem = rt_sem_create("dsem", 0, RT_IPC_FLAG_FIFO);
    if (dynamic_sem == RT_NULL)
    {
        rt_kprintf("create dynamic semaphore failed.\n");
        return -1;
    }
    else
    {
        rt_kprintf("create done. dynamic semaphore value = 0.\n");
    }

    t_thread = rt_thread_create("test", test_entry, RT_NULL, 4096, 24, 20);
    rt_thread_startup(t_thread);
    // rt_uint16_t btn_pin = get_PinNumber(GPIO_F_PORT, 2);
    // rt_pin_mode(btn_pin, PIN_MODE_INPUT_PULLUP);
    // rt_pin_attach_irq(btn_pin, PIN_IRQ_MODE_FALLING, PF2_irq, RT_NULL);
    // rt_pin_irq_enable(btn_pin, PIN_IRQ_ENABLE);
}
FINSH_FUNCTION_EXPORT(test_th, thread test);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(test_th, thread test);
#endif
