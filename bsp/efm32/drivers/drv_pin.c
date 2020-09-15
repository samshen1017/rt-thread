/*
 * @Author: ShenQuan 
 * @Date: 2019-05-13 16:41:19 
 * @Last Modified by: ShenQuan
 * @Last Modified time: 2019-05-14 15:15:07
 */

#include "drv_pin.h"
#include "board.h"
#include <hdl_interrupt.h>

#ifdef RT_USING_PIN
#ifdef RT_PIN_DEBUG
#define pin_debug(format, args...) rt_kprintf(format, ##args)
#else
#define pin_debug(format, args...)
#endif

#define PIN_COUNT 16

rt_uint16_t get_PinNumber(GPIO_PIN_PORT port, rt_uint8_t pin)
{
    switch (port)
    {
    case GPIO_A_PORT:
    case GPIO_B_PORT:
    case GPIO_C_PORT:
    case GPIO_D_PORT:
    case GPIO_E_PORT:
    case GPIO_F_PORT:
    {
        return (port * PIN_COUNT) + pin;
    }
    default:
        return 0xFF;
    }
}

static GPIO_Port_TypeDef get_gpio_port(rt_uint16_t pinNumber)
{
    switch (pinNumber / PIN_COUNT)
    {
    case GPIO_A_PORT:
        return gpioPortA;
    case GPIO_B_PORT:
        return gpioPortB;
    case GPIO_C_PORT:
        return gpioPortC;
    case GPIO_D_PORT:
        return gpioPortD;
    case GPIO_E_PORT:
        return gpioPortE;
    case GPIO_F_PORT:
        return gpioPortF;
    default:
        return 0xFF;
    }
}

static rt_uint8_t get_gpio_pin(rt_uint16_t pinNumber)
{
    return (pinNumber % PIN_COUNT);
}

static void efm32_pin_write(rt_device_t dev, rt_base_t pinNumber, rt_base_t value)
{
    rt_uint8_t pin = get_gpio_pin(pinNumber);
    GPIO_Port_TypeDef port = get_gpio_port(pinNumber);
    switch (value)
    {
    case PIN_HIGH:
        GPIO_PinOutSet(port, pin);
        break;
    case PIN_LOW:
        GPIO_PinOutClear(port, pin);
        break;
    }
    pin_debug("pin write: %x --> %d", pinNumber, value);
}

static int efm32_pin_read(rt_device_t dev, rt_base_t pinNumber)
{
    rt_base_t ret = 0;
    rt_uint8_t pin = get_gpio_pin(pinNumber);
    GPIO_Port_TypeDef port = get_gpio_port(pinNumber);
    ret = GPIO_PinInGet(port, pin);
    pin_debug("pin read: %x --> %d", pinNumber, ret);
    return ret;
}

static void efm32_pin_mode(rt_device_t dev, rt_base_t pinNumber, rt_base_t mode)
{
    rt_uint8_t pin = get_gpio_pin(pinNumber);
    GPIO_Port_TypeDef port = get_gpio_port(pinNumber);
    CMU_ClockEnable(cmuClock_GPIO, true);
    switch (mode)
    {
    case PIN_MODE_OUTPUT:
        GPIO_PinModeSet(port, pin, gpioModePushPull, 0);
        break;
    case PIN_MODE_INPUT:
        GPIO_PinModeSet(port, pin, gpioModeInput, 0);
        break;
    case PIN_MODE_INPUT_PULLUP:
        GPIO_PinModeSet(port, pin, gpioModeInputPull, 1);
        break;
    case PIN_MODE_INPUT_PULLDOWN:
        GPIO_PinModeSet(port, pin, gpioModeInputPull, 0);
        break;
    case PIN_MODE_OUTPUT_OD:
        GPIO_PinModeSet(port, pin, gpioModeWiredAnd, 0);
        break;
    }
    pin_debug("efm32_pin_mode pindata:%d mode:%d\n", pinNumber, mode);
}

rt_err_t efm32_pin_attach_irq(struct rt_device *device, rt_int32_t pinNumber, rt_uint32_t mode, void (*hdr)(void *args), void *args)
{
    rt_uint8_t pin = get_gpio_pin(pinNumber);
    GPIO_Port_TypeDef port = get_gpio_port(pinNumber);
    /* Configure interrupt pin */
    switch (mode)
    {
    case PIN_IRQ_MODE_RISING:
        GPIO_PinModeSet(port, pin, gpioModeInputPullFilter, 0);
        GPIO_IntConfig(port, pin, true, false, true);
        break;
    case PIN_IRQ_MODE_FALLING:
        GPIO_PinModeSet(port, pin, gpioModeInputPullFilter, 1);
        GPIO_IntConfig(port, pin, false, true, true);
        break;
    case PIN_IRQ_MODE_RISING_FALLING:
        GPIO_PinModeSet(port, pin, gpioModeInputPullFilter, 1);
        GPIO_IntConfig(port, pin, true, true, true);
        break;
    case PIN_IRQ_MODE_HIGH_LEVEL:
        rt_kprintf("warning: PIN_IRQ_MODE_HIGH_LEVEL unsupported, default using PIN_IRQ_MODE_RISING.\n");
        GPIO_PinModeSet(port, pin, gpioModeInputPullFilter, 0);
        GPIO_IntConfig(port, pin, true, false, true);
        break;
    case PIN_IRQ_MODE_LOW_LEVEL:
        rt_kprintf("warning: PIN_IRQ_MODE_LOW_LEVEL unsupported, default using PIN_IRQ_MODE_FALLING.\n");
        GPIO_PinModeSet(port, pin, gpioModeInputPullFilter, 1);
        GPIO_IntConfig(port, pin, false, true, true);
        break;
    }
    efm32_irq_hook_init_t hook;
    hook.type = efm32_irq_type_gpio;
    hook.unit = pin;
    hook.cbFunc = hdr;
    hook.userPtr = args;
    efm32_irq_hook_register(&hook);
    return RT_EOK;
}

rt_err_t efm32_pin_detach_irq(struct rt_device *device, rt_int32_t pinNumber)
{
    rt_uint8_t pin = get_gpio_pin(pinNumber);
    GPIO_Port_TypeDef port = get_gpio_port(pinNumber);
    GPIO_IntConfig(port, pin, false, false, false);
    efm32_irq_hook_init_t hook;
    hook.type = efm32_irq_type_gpio;
    hook.unit = pin;
    hook.cbFunc = RT_NULL;
    hook.userPtr = RT_NULL;
    efm32_irq_hook_register(&hook);
    return RT_EOK;
}

rt_err_t efm32_pin_irq_enable(struct rt_device *device, rt_base_t pinNumber, rt_uint32_t enabled)
{
    rt_uint8_t pin = get_gpio_pin(pinNumber);
    if (enabled == PIN_IRQ_ENABLE)
    {
        if ((rt_uint8_t)pin % 2)
        {
            NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
            NVIC_SetPriority(GPIO_ODD_IRQn, EFM32_IRQ_PRI_DEFAULT);
            NVIC_EnableIRQ(GPIO_ODD_IRQn);
        }
        else
        {
            NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
            NVIC_SetPriority(GPIO_EVEN_IRQn, EFM32_IRQ_PRI_DEFAULT);
            NVIC_EnableIRQ(GPIO_EVEN_IRQn);
        }
    }
    else
    {
        if ((rt_uint8_t)pin % 2)
        {
            NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
            NVIC_DisableIRQ(GPIO_ODD_IRQn);
        }
        else
        {
            NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
            NVIC_DisableIRQ(GPIO_EVEN_IRQn);
        }
    }
    return RT_EOK;
}

const static struct rt_pin_ops _efm32_pin_ops =
    {
        efm32_pin_mode,
        efm32_pin_write,
        efm32_pin_read,
        efm32_pin_attach_irq,
        efm32_pin_detach_irq,
        efm32_pin_irq_enable,
};

int rt_hw_pin_init(void)
{
    int result;
    result = rt_device_pin_register("pin", &_efm32_pin_ops, RT_NULL);
    pin_debug("rt_hw_pin_init: %d\n", result);
    return result;
}
INIT_BOARD_EXPORT(rt_hw_pin_init);

static void pin_ctl(int argc, char **argv)
{
    if (argc < 4)
    {
        rt_kprintf("Usage: io_test [gpio group (a ... f)] [gpio number] [0/1]\n");
    }
    else
    {
        char port = (char)argv[1][0];
        int pin = strtol(argv[2], NULL, 0);
        int pin_state = strtol(argv[3], NULL, 0);
        rt_uint16_t pin_number = get_PinNumber((GPIO_PIN_PORT)(port - 'a'), pin);
        rt_kprintf("PinNum=%u, State:%d\n", pin_number, pin_state);
        rt_pin_mode(pin_number, PIN_MODE_OUTPUT);
        rt_pin_write(pin_number, pin_state);
    }
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(pin_ctl, gpio ctrl);

#endif
