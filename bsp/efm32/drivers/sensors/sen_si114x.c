#include "sen_si114x.h"
#include <rtdbg.h>
#include "drv_pin.h"
#include "Si114_defs.h"
#include "Si114x.h"

typedef struct
{
    rt_uint16_t chip_id;
    rt_uint8_t addr;
    rt_uint16_t int_pinNum;
    rt_int32_t range;
    struct rt_i2c_bus_device *i2c_bus;
} Si114x;

typedef struct
{
    rt_int32_t distance;
    rt_size_t size;
} Si114x_data;

static rt_err_t _set_power(rt_sensor_t sensor, rt_uint8_t power)
{
    rt_err_t result = RT_EOK;
    if (power == RT_SENSOR_POWER_NORMAL)
    {
        /* Init Si114x */
        rt_kprintf("Init Si114x.\n");
        Si114x *_si114x_dev = sensor->parent.user_data;
        result = Si114x_Initialize(_si114x_dev->i2c_bus);
    }
    return result;
}

static rt_err_t _select_mode(rt_sensor_t sensor, rt_uint32_t mode)
{
    if (mode == RT_SENSOR_MODE_INT)
    {
        rt_kprintf("set interrupt.\n");
    }
    return RT_EOK;
}

static rt_size_t si114x_fetch_data(rt_sensor_t sensor, void *buf, rt_size_t len)
{
    Si114x *_si114x_dev = sensor->parent.user_data;
    struct rt_sensor_data *data = buf;
    rt_size_t cnt = 0;
    if (sensor->info.type == RT_SENSOR_CLASS_PROXIMITY)
    {
        for (int i = 0; i < len; i++)
        {
            rt_int16_t distance = Si114x_read_distance(_si114x_dev->i2c_bus);
            data[i].type = RT_SENSOR_CLASS_PROXIMITY;
            data[i].data.proximity = distance;
            data[i].timestamp = rt_sensor_get_ts();
            cnt++;
        }
    }
    return cnt;
}

static rt_err_t si114x_control(rt_sensor_t sensor, int cmd, void *args)
{
    rt_err_t result = RT_EOK;
    switch (cmd)
    {
    case RT_SENSOR_CTRL_GET_ID:
    {
        Si114x *_si114x_dev = sensor->parent.user_data;
        rt_int16_t id = Si114x_GetPartID(_si114x_dev->i2c_bus);
        _si114x_dev->chip_id = id;
        *(rt_int16_t *)args = id;
    }
    case RT_SENSOR_CTRL_SET_RANGE:
        rt_kprintf("SET_RANGE\n");
        break;
    case RT_SENSOR_CTRL_SET_ODR:
        rt_kprintf("SET_ODR\n");
        break;
    case RT_SENSOR_CTRL_SET_MODE:
    {
        result = _select_mode(sensor, (rt_uint32_t)args & 0xffff);
        break;
    }
    case RT_SENSOR_CTRL_SET_POWER:
    {
        result = _set_power(sensor, (rt_uint32_t)args & 0xff);
        break;
    }
    case RT_SENSOR_CTRL_SELF_TEST:
        rt_kprintf("SELF_TEST\n");
        break;
    default:
        return -RT_ERROR;
    }
    return result;
}

static struct rt_sensor_ops sensor_ops =
    {
        si114x_fetch_data,
        si114x_control,
};

int rt_hw_si114x_init(const char *name, struct rt_sensor_config *cfg)
{
    rt_int8_t result;
    rt_sensor_t sensor = RT_NULL;
    Si114x *_si114x_dev = RT_NULL;

    sensor = rt_calloc(1, sizeof(struct rt_sensor_device));
    if (sensor == RT_NULL)
    {
        LOG_E("rt_sensor_t rt_calloc failed.");
        return -RT_ERROR;
    }
    sensor->info.type = RT_SENSOR_CLASS_PROXIMITY;
    sensor->info.vendor = RT_SENSOR_VENDOR_UNKNOWN;
    sensor->info.model = "si114x_prox";
    sensor->info.unit = RT_SENSOR_UNIT_CM;
    sensor->info.range_max = 65535;
    sensor->info.range_min = 0;
    sensor->info.period_min = 5;
    rt_memcpy(&sensor->config, cfg, sizeof(struct rt_sensor_config));
    sensor->ops = &sensor_ops;

    result = rt_hw_sensor_register(sensor, name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("Si114x register err code: %d", result);
        rt_free(sensor);
        return -RT_ERROR;
    }
    LOG_I("Si114x sensor init success");

    _si114x_dev = rt_calloc(1, sizeof(Si114x));
    if (_si114x_dev == RT_NULL)
    {
        LOG_E("Si114x create failed.");
        return -RT_ERROR;
    }
    _si114x_dev->addr = (rt_uint32_t)(cfg->intf.user_data) & 0xff;
    _si114x_dev->i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->intf.dev_name);
    _si114x_dev->range = cfg->range;
    _si114x_dev->int_pinNum = cfg->irq_pin.pin;
    sensor->parent.user_data = _si114x_dev;
    return RT_EOK;
}

void rt_hw_si114x_detach(rt_sensor_t dev)
{
    rt_err_t result;
    rt_int16_t response;
    Si114x *_si114x_dev = dev->parent.user_data;
    do
    {
        response = Si114x_reset(_si114x_dev->i2c_bus);
        _mdelay(2);

    } while (response != 0x00);

    result = rt_pin_detach_irq(_si114x_dev->int_pinNum);
    if (result != RT_EOK)
    {
        rt_kprintf("rt_pin_detach_irq failed.\n");
    }

    rt_pin_mode(_si114x_dev->int_pinNum, PIN_MODE_OUTPUT);
    rt_pin_write(_si114x_dev->int_pinNum, PIN_HIGH); //拉低关闭管脚

    result = rt_device_unregister((rt_device_t)dev);
    if (result != RT_EOK)
    {
        LOG_E("rt_device_unregister failed.");
    }
    rt_free(dev->parent.user_data);
    dev->parent.user_data = RT_NULL;
    rt_free(dev);
    dev = RT_NULL;
    LOG_D("rt_hw_si114x_detach.");
}

static void si114x_init(void)
{
    rt_kprintf("si114x_test.\n");
    struct rt_sensor_config cfg;
    cfg.intf.dev_name = "i2c1";
    cfg.intf.user_data = (void *)SI1141_ADDR;
    cfg.irq_pin.pin = get_PinNumber(SI114X_INT_PIN_PORT, SI114X_INT_PIN_NUM);
    cfg.irq_pin.mode = PIN_MODE_INPUT_PULLUP;
    cfg.mode = RT_SENSOR_MODE_INT;
    rt_hw_si114x_init("si114x", &cfg);
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(si114x_init, si114x init);

static void si114x_test(void)
{
    rt_device_t dev = RT_NULL;
    struct rt_sensor_data data;
    rt_size_t res, i;

    /* 查找系统中的传感器设备 */
    dev = rt_device_find("pr_si114");
    if (dev == RT_NULL)
    {
        rt_kprintf("Can't find device.\n");
        return;
    }

    /* 以轮询模式打开传感器设备 */
    if (rt_device_open(dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
    {
        rt_kprintf("open device failed!\n");
        return;
    }

    for (i = 0; i < 1000; i++)
    {
        /* 从传感器读取一个数据 */
        res = rt_device_read(dev, 0, &data, 1);
        if (res != 1)
        {
            rt_kprintf("read data failed!size is %d\n", res);
        }
        rt_kprintf("distance:%d \n", data.data.proximity);
    }

    /* 关闭传感器设备 */
    rt_device_close(dev);
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(si114x_test, si114x test);
