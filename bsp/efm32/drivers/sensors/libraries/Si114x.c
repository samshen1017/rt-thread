#include "Si114x.h"
#include "Si114_defs.h"

#if defined(RT_USING_PM)
#define REQUEST_PM                               \
    do                                           \
    {                                            \
        rt_pm_request(PM_SLEEP_MODE_NONE);       \
        rt_pm_run_enter(PM_RUN_MODE_HIGH_SPEED); \
    } while (0)

#define RELEASE_PM                              \
    do                                          \
    {                                           \
        rt_pm_release(PM_SLEEP_MODE_NONE);      \
        rt_pm_run_enter(PM_RUN_MODE_LOW_SPEED); \
    } while (0)
#else
#define REQUEST_PM
#define RELEASE_PM
#endif

void _mdelay(int m)
{
    REQUEST_PM;
    rt_thread_mdelay(m);
    RELEASE_PM;
}

static rt_size_t Si1141_Read_Register(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t addr, rt_uint8_t reg, rt_uint8_t *data)
{
    RT_ASSERT(i2c_bus != RT_NULL);
    struct rt_i2c_msg msgs[1];
    rt_uint8_t i2c_write_data[1];
    rt_uint8_t i2c_read_data[1];
    msgs[0].addr = addr;
    msgs[0].flags = RT_I2C_WRITEREAD;
    i2c_write_data[0] = reg;
    msgs[0].buf = i2c_write_data;
    msgs[0].len = 1;
    msgs[0].buf2 = i2c_read_data;
    msgs[0].len2 = 1;
    rt_size_t ret = rt_i2c_transfer(i2c_bus, msgs, 1);
    *data = i2c_read_data[0];
    return ret;
}

static rt_size_t Si1141_Write_Register(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t addr, rt_uint8_t reg, rt_uint8_t data)
{
    RT_ASSERT(i2c_bus != RT_NULL);
    struct rt_i2c_msg msgs[1];
    rt_uint8_t i2c_write_data[2];
    msgs[0].addr = addr;
    msgs[0].flags = RT_I2C_WR;
    /* Select register and data to write */
    i2c_write_data[0] = reg;
    i2c_write_data[1] = data;
    msgs[0].buf = i2c_write_data;
    msgs[0].len = 2;
    rt_size_t ret = rt_i2c_transfer(i2c_bus, msgs, 1);
    return ret;
}

static rt_size_t Si1141_Write_Block_Register(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t addr, rt_uint8_t reg, rt_uint8_t length, rt_uint8_t const *data)
{
    RT_ASSERT(i2c_bus != RT_NULL);
    struct rt_i2c_msg msgs[1];
    rt_uint8_t i2c_write_data[10];
    msgs[0].addr = addr;
    msgs[0].flags = RT_I2C_WR;
    i2c_write_data[0] = reg;
    for (int i = 0; i < length; i++)
    {
        i2c_write_data[i + 1] = data[i];
    }
    msgs[0].buf = i2c_write_data;
    msgs[0].len = 1 + length;
    rt_size_t ret = rt_i2c_transfer(i2c_bus, msgs, 1);
    return ret;
}

static rt_size_t Si1141_Read_Block_Register(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t addr, rt_uint8_t reg, rt_uint8_t length, rt_uint8_t *data)
{
    RT_ASSERT(i2c_bus != RT_NULL);
    struct rt_i2c_msg msgs[1];
    rt_uint8_t i2c_write_data[1];
    msgs[0].addr = addr;
    msgs[0].flags = RT_I2C_WRITEREAD;
    /* Select register to start reading from */
    i2c_write_data[0] = reg;
    msgs[0].buf = i2c_write_data;
    msgs[0].len = 1;
    /* Select length of data to be read */
    msgs[0].buf2 = data;
    msgs[0].len2 = length;
    rt_size_t ret = rt_i2c_transfer(i2c_bus, msgs, 1);
    return ret;
}

rt_int16_t Si114xWriteToRegister(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t reg, rt_uint8_t value)
{
    rt_size_t ret = Si1141_Write_Register(i2c_bus, SI1141_ADDR, reg, value);
    if (ret != 1)
    {
        rt_kprintf("Si1141_Write_Register failed.\n");
        return -1;
    }
    return ret;
}

rt_int16_t Si114xReadFromRegister(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t reg)
{
    rt_uint8_t data = 0;
    rt_size_t ret = Si1141_Read_Register(i2c_bus, SI1141_ADDR, reg, &data);
    if (ret != 1)
    {
        rt_kprintf("Si1141_Read_Register failed.\n");
        return -1;
    }
    return data;
}

rt_int16_t Si114xBlockWrite(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t reg, rt_uint8_t length, rt_uint8_t *values)
{
    rt_size_t ret = Si1141_Write_Block_Register(i2c_bus, SI1141_ADDR, reg, length, values);
    if (ret != 1)
    {
        rt_kprintf("Si1141_Write_Block_Register failed.\n");
        return -1;
    }
    return length;
}

rt_int16_t Si114xBlockRead(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t reg, rt_uint8_t length, rt_uint8_t *values)
{
    rt_size_t ret = Si1141_Read_Block_Register(i2c_bus, SI1141_ADDR, reg, length, values);
    if (ret != 1)
    {
        rt_kprintf("Si1141_Read_Block_Register failed.\n");
        return -1;
    }
    return length;
}

rt_int16_t Si114x_GetPartID(struct rt_i2c_bus_device *i2c_bus)
{
    rt_int16_t id = Si114xReadFromRegister(i2c_bus, 0x00);
    return id;
}

rt_err_t Si114x_Initialize(struct rt_i2c_bus_device *i2c_bus)
{
    rt_int16_t response;
    /* RESET */
    do
    {
        response = Si114x_reset(i2c_bus);
        _mdelay(2);

    } while (response != 0x00);

    /* write HW_KEY for proper operation */
    Si114xWriteToRegister(i2c_bus, SI114_REG_HW_KEY, SI114_HW_KEY_VAL0);

    /* Set PS1 threshold */
    rt_uint8_t ps1_th[2] = {0x10, 0x27};
    Si114xBlockWrite(i2c_bus, SI114_REG_PS1_TH, 2, (rt_uint8_t *)&ps1_th);

    /* turn on interrupts */
    Si114xWriteToRegister(i2c_bus, SI114_REG_IRQ_CFG, SI114_ICG_INTOE | SI114_ICG_INTMODE);

    /* Enable IRQ */
    Si114xWriteToRegister(i2c_bus, SI114_REG_IRQ_ENABLE, SI114_IE_PS1);

    /* 01: PS1_INT is set whenever the current PS1 measurement crosses the PS1_TH threshold. */
    Si114xWriteToRegister(i2c_bus, SI114_REG_IRQ_MODE1, SI114_IM1_PS1_CROSS_TH | SI114_IM1_PS1_EXCEED_TH);

    /* Rate set */
    Si114xWriteToRegister(i2c_bus, SI114_REG_MEAS_RATE, 0xDF);
    Si114xWriteToRegister(i2c_bus, SI114_REG_ALS_RATE, 0x08);
    Si114xWriteToRegister(i2c_bus, SI114_REG_PS_RATE, 0x08);

    /* Initialize LED Current */
    Si114xWriteToRegister(i2c_bus, SI114_REG_PS_LED21, 0x0007);

    /* Parameter 0x01 = 0x37 */
    response = Si114x_ParamSet(i2c_bus, SI114_PARAM_CH_LIST, SI114_ALS_IR_TASK + SI114_ALS_VIS_TASK + SI114_PS1_TASK + SI114_PS2_TASK + SI114_PS3_TASK);

    /* Set PS Auto */
    Si114xWriteToRegister(i2c_bus, SI114_REG_COMMAND, SI114_CMD_PS_ALS_AUTO);

    response = Si114x_read_response(i2c_bus);
    return RT_EOK;
}

rt_int16_t Si114x_reset(struct rt_i2c_bus_device *i2c_bus)
{
    rt_int16_t response;
    response = Si114xWriteToRegister(i2c_bus, SI114_REG_COMMAND, SI114_CMD_RESET);
    if (response != 1)
    {
        rt_kprintf("Si114x_reset failed.\n");
    }
    _mdelay(2);
    // Wait for command to finish
    response = Si114x_read_response(i2c_bus);
    return response;
}

rt_int16_t Si114x_read_distance(struct rt_i2c_bus_device *i2c_bus)
{
    rt_uint8_t buffer[2] = {0, 0};
    rt_uint16_t ret = Si114xBlockRead(i2c_bus, SI114_REG_PS1_DATA0, 2, buffer);
    if (ret == -1)
    {
        return 0;
    }
    // rt_kprintf("PS1:%d\n", (rt_uint16_t)((buffer[1] << 8) | buffer[0]));
    return (rt_uint16_t)((buffer[1] << 8) | buffer[0]);
}

rt_int16_t Si114x_read_response(struct rt_i2c_bus_device *i2c_bus)
{
    return Si114xReadFromRegister(i2c_bus, SI114_REG_RESPONSE);
}

rt_err_t Si114x_ParamSet(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t address, rt_uint8_t value)
{
    rt_uint8_t buffer[2];
    rt_err_t result = RT_EOK;
    rt_int16_t response;
    do
    {
        Si114xWriteToRegister(i2c_bus, SI114_REG_COMMAND, SI114_CMD_NOP);
        response = Si114xReadFromRegister(i2c_bus, SI114_REG_RESPONSE);
        _mdelay(2);
    } while (response != 0x00);
    buffer[0] = value;
    buffer[1] = SI114_CMD_PARAM_SET | (address & 0x1F);
    Si114xBlockWrite(i2c_bus, SI114_REG_PARAM_WR, 2, (rt_uint8_t *)buffer);
    response = Si114x_read_response(i2c_bus);
    if (response != 0x01)
    {
        _mdelay(25);
        response = Si114x_read_response(i2c_bus);
        if (response != 0x01)
        {
            result = RT_ERROR;
        }
    }
    return result;
}
