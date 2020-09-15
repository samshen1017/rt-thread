#include "sen_zsc31014.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <rtdbg.h>
#include "drv_pin.h"

#define ZSC31014_COE 16383

struct zsc31014
{
    rt_uint8_t chip_id;
    rt_uint8_t addr;
    rt_uint16_t pow_pinnum;
    rt_int32_t range;
    struct rt_i2c_bus_device *i2c_bus;
};

struct zsc31014_data
{
    rt_int32_t baro;
    rt_size_t size;
};

static rt_int32_t calc_pressure(rt_int32_t value, int r_max, int r_min)
{
    return ((r_max - r_min) * ((double)value / ZSC31014_COE));
}

static rt_err_t zsc31014_get_data(struct zsc31014_data *data, const struct zsc31014 *dev)
{
    /* Check for null pointer in the device structure*/
    RT_ASSERT(dev != RT_NULL);
    RT_ASSERT(dev != RT_NULL);

    rt_uint8_t tmp[4] = {0, 0, 0, 0}; //获取I2C的原始数据
    rt_int32_t value = 0;             //移位后的原始数据
    struct rt_i2c_bus_device *i2c_bus = dev->i2c_bus;
    struct rt_i2c_msg msgs[1];
    if (i2c_bus == RT_NULL)
    {
        return RT_ERROR;
    }
    msgs[0].addr = dev->addr;
    msgs[0].flags = RT_I2C_RD; /* Read from slave */
    msgs[0].buf = tmp;
    msgs[0].len = sizeof(tmp) / sizeof(rt_uint8_t);
    data->size = rt_i2c_transfer(i2c_bus, msgs, 1);
    if (data->size == 0)
    {
        rt_kprintf("zsc err.\n");
        return RT_EIO;
    }
    value = ((tmp[0] & 0x7F) << 8) | tmp[1];
    // rt_kprintf("%s:%d\n", dev->i2c_bus->parent.parent.name, value);
    data->baro = calc_pressure(value, dev->range, 0);
    return RT_EOK;
}

static rt_err_t zsc31014_set_range(rt_sensor_t sensor, rt_int32_t range)
{
    struct zsc31014 *_zsc31014_dev = sensor->parent.user_data;
    if (sensor->info.type == RT_SENSOR_CLASS_BARO)
    {
        _zsc31014_dev->range = range;
    }
    return RT_EOK;
}

static rt_err_t zsc31014_set_power(rt_sensor_t sensor, rt_uint8_t power)
{
    struct zsc31014 *_zsc31014_dev = sensor->parent.user_data;
    rt_err_t result = RT_EOK;
    if (power == RT_SENSOR_POWER_DOWN)
    {
        rt_pin_write(_zsc31014_dev->pow_pinnum, PIN_HIGH); // 拉高关闭
    }
    else if (power == RT_SENSOR_POWER_NORMAL)
    {
        rt_pin_write(_zsc31014_dev->pow_pinnum, PIN_LOW); // 拉低使能
    }
    else
    {
        result = -RT_ERROR;
    }
    return result;
}

static rt_size_t zsc31014_fetch_data(rt_sensor_t sensor, void *buf, rt_size_t len)
{
    struct zsc31014 *_zsc31014_dev = sensor->parent.user_data;
    struct rt_sensor_data *data = buf;
    rt_size_t cnt = 0;
    rt_err_t result;
    if (sensor->info.type == RT_SENSOR_CLASS_BARO)
    {
        for (int i = 0; i < len; i++)
        {
            struct zsc31014_data comp_data;
            result = zsc31014_get_data(&comp_data, _zsc31014_dev);
            if (result != RT_EOK)
            {
                data[i].type = RT_SENSOR_CLASS_NONE;
                data[i].data.baro = 0x8000000;
                data[i].timestamp = rt_sensor_get_ts();
            }
            else
            {
                data[i].type = RT_SENSOR_CLASS_BARO;
                data[i].data.baro = comp_data.baro;
                data[i].timestamp = rt_sensor_get_ts();
                cnt++;
            }
        }
    }
    return cnt;
}

static rt_err_t zsc31014_control(rt_sensor_t sensor, int cmd, void *args)
{
    rt_err_t result = RT_EOK;
    switch (cmd)
    {
    case RT_SENSOR_CTRL_GET_ID:
        LOG_D("GET_ID");
        break;
    case RT_SENSOR_CTRL_SET_RANGE:
        LOG_D("SET_RANGE");
        result = zsc31014_set_range(sensor, (rt_int32_t)args);
        break;
    case RT_SENSOR_CTRL_SET_POWER:
        LOG_D("SET_POWER");
        result = zsc31014_set_power(sensor, (rt_uint32_t)args & 0xff);
        break;
    default:
        return -RT_ERROR;
    }
    return result;
}

static struct rt_sensor_ops sensor_ops =
    {
        zsc31014_fetch_data,
        zsc31014_control,
};

int rt_hw_zsc31014_init(const char *name, struct rt_sensor_config *cfg, rt_uint16_t pow_pin)
{
    rt_int8_t result;
    rt_sensor_t sensor = RT_NULL;
    struct zsc31014 *_zsc31014_dev = RT_NULL;

    sensor = rt_calloc(1, sizeof(struct rt_sensor_device));
    if (sensor == RT_NULL)
    {
        LOG_E("rt_sensor_t rt_calloc failed.");
        return -RT_ERROR;
    }
    sensor->info.type = RT_SENSOR_CLASS_BARO;
    sensor->info.vendor = RT_SENSOR_VENDOR_UNKNOWN;
    sensor->info.model = "zsc31014_baro";
    sensor->info.unit = RT_SENSOR_UNIT_PA;
    sensor->info.intf_type = RT_SENSOR_INTF_I2C;
    sensor->info.range_max = 32000000;
    sensor->info.range_min = 0;
    sensor->info.period_min = 5;
    rt_memcpy(&sensor->config, cfg, sizeof(struct rt_sensor_config));
    sensor->ops = &sensor_ops;

    result = rt_hw_sensor_register(sensor, name, RT_DEVICE_FLAG_RDWR, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("ZSC31014 register err code: %d", result);
        rt_free(sensor);
        return -RT_ERROR;
    }
    LOG_I("ZSC31014 sensor init success.");

    _zsc31014_dev = rt_calloc(1, sizeof(struct zsc31014));
    if (_zsc31014_dev == RT_NULL)
    {
        LOG_E("ZSC31014 create failed.");
        return -RT_ERROR;
    }
    _zsc31014_dev->addr = (rt_uint32_t)(cfg->intf.user_data) & 0xff;
    _zsc31014_dev->i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(cfg->intf.dev_name);
    _zsc31014_dev->range = cfg->range;
    _zsc31014_dev->pow_pinnum = pow_pin;
    rt_pin_mode(_zsc31014_dev->pow_pinnum, PIN_MODE_OUTPUT);
    rt_pin_write(_zsc31014_dev->pow_pinnum, PIN_HIGH); //默认关闭
    sensor->parent.user_data = _zsc31014_dev;
    return RT_EOK;
}

void rt_hw_zsc31014_detach(rt_sensor_t dev)
{
    rt_err_t result;
    zsc31014_set_power(dev, RT_SENSOR_POWER_DOWN);
    result = rt_device_unregister((rt_device_t)dev);
    if (result != RT_EOK)
    {
        LOG_E("rt_device_unregister failed.");
    }
    rt_free(dev->parent.user_data);
    dev->parent.user_data = RT_NULL;
    rt_free(dev);
    dev = RT_NULL;
    LOG_D("rt_hw_zsc31014_detach.");
}
