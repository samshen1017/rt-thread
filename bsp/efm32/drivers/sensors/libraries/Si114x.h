#ifndef SI114X_H
#define SI114X_H

#include <rtthread.h>
#include <rtdevice.h>

rt_int16_t Si114xWriteToRegister(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t reg, rt_uint8_t value);
rt_int16_t Si114xReadFromRegister(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t reg);
rt_int16_t Si114xBlockWrite(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t reg, rt_uint8_t length, rt_uint8_t *values);
rt_int16_t Si114xBlockRead(struct rt_i2c_bus_device *i2c_bus, rt_uint8_t reg, rt_uint8_t length, rt_uint8_t *values);

rt_int16_t Si114x_GetPartID(struct rt_i2c_bus_device *i2c_bus);
rt_err_t Si114x_Initialize(struct rt_i2c_bus_device *i2c_bus);

rt_int16_t Si114x_reset(struct rt_i2c_bus_device *i2c_bus);
rt_int16_t Si114x_read_distance(struct rt_i2c_bus_device *dev);
rt_int16_t Si114x_read_response(struct rt_i2c_bus_device *dev);
rt_err_t Si114x_ParamSet(struct rt_i2c_bus_device *dev, rt_uint8_t address, rt_uint8_t value);

void _mdelay(int m);
#endif