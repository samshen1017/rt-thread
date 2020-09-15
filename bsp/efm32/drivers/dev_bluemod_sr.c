#include "dev_bluemod_sr.h"
#include "drv_pin.h"
#include "drv_serial.h"
#include "string.h"
#include <at.h>

#if defined(RT_USING_BLUEMOD_SR)

#ifdef BLUEMOD_SR_DEBUG
#define ble_debug(format, args...) rt_kprintf(format, ##args)
#else
#define ble_debug(format, args...)
#endif

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

typedef struct
{
    rt_uint16_t rst_pin;
    rt_uint16_t pwr_pin;
    rt_uint16_t rts_pin;
    rt_uint16_t cts_pin;
    rt_uint16_t iur_out_pin;
    rt_uint16_t iur_in_pin;
    void (*connect_cb)(void);
    void (*disconnect_cb)(void);
    void (*receive_cb)(const char *data, rt_size_t size);
} BlueMod_SR;

static BlueMod_SR ble_obj;

#define BLUEMOD_VERSION_SIZE 20

#define BLUEMOD_POWER_ON 1
#define BLUEMOD_POWER_OFF 0

// PA 7     IUR_OUT
#define IUROUT_LOW() rt_pin_write(ble_obj.iur_out_pin, PIN_LOW);
#define IUROUT_HIGH() rt_pin_write(ble_obj.iur_out_pin, PIN_HIGH);

// PA 8     CTS
#define CTS_LOW() rt_pin_write(ble_obj.cts_pin, PIN_LOW);
#define CTS_HIGH() rt_pin_write(ble_obj.cts_pin, PIN_HIGH);

// PA 10    IUR_IN/IUC_IN
#define IURIN_LOW() rt_pin_write(ble_obj.iur_in_pin, PIN_LOW);
#define IURIN_HIGH() rt_pin_write(ble_obj.iur_in_pin, PIN_HIGH);

// PA 9     RTS/IUC_OUT
#define RTS_LOW() rt_pin_write(ble_obj.rts_pin, PIN_LOW);
#define RTS_HIGH() rt_pin_write(ble_obj.rts_pin, PIN_HIGH);

void ble_urc_connected(struct at_client *client, const char *data, rt_size_t size)
{
    ble_obj.connect_cb();
}

void ble_urc_disconnect(struct at_client *client, const char *data, rt_size_t size)
{
    ble_obj.disconnect_cb();
}

void ble_urc_transfer(struct at_client *client, const char *data, rt_size_t size)
{
    ble_obj.receive_cb(data, size);
}

void bluemodSR_SetConnectCallback(void (*cb)(void))
{
    ble_obj.connect_cb = cb;
}

void bluemodSR_SetDisconnectCallback(void (*cb)(void))
{
    ble_obj.disconnect_cb = cb;
}

void bluemodSR_SetReceiveCallback(void (*cb)(const char *data, rt_size_t size))
{
    ble_obj.receive_cb = cb;
}

static const char p_head[] = {0xFE, 0x01, 0x00};

static struct at_urc ble_urc_table[] = {
    {"{connected}", "\r\n", ble_urc_connected},
    {"{disconnected}", "\r\n", ble_urc_disconnect},
    {p_head, "\r\n", ble_urc_transfer},
};

// static void CTS_handler(void *args)
// {
//     rt_uint32_t val;
//     val = rt_pin_read(ble_obj.cts_pin);
//     if (val)
//     {
//         rt_kprintf("H、n");
//     }
//     else
//     {
//         rt_kprintf("L\n");
//     }
// }

static void IURIN_handler(void *args)
{
    rt_uint32_t val;
    val = rt_pin_read(ble_obj.iur_in_pin);
    if (val)
    {
        RTS_HIGH(); //由ble模块拉高, 系统进入低功耗运行
        RELEASE_PM;
    }
    else
    {
        REQUEST_PM;
        RTS_LOW(); //由ble模块拉低，开始接收数据, 系统进入高功耗
    }
}

static void bluemodSR_PowerOn(BlueMod_SR *instance, rt_uint8_t onoff)
{
    switch (onoff)
    {
    case 1:
        //POWER ON
        rt_pin_write(instance->rst_pin, PIN_HIGH); //默认高电平
        rt_pin_write(instance->pwr_pin, PIN_HIGH); //高使能
        break;
    case 0:
        //POWER OFF
        rt_pin_write(instance->pwr_pin, PIN_LOW);
        rt_pin_write(instance->rst_pin, PIN_LOW);
        break;
    }
}

rt_err_t rt_hw_bluemodSR_init(void)
{
    rt_err_t err_code;
    ble_obj.pwr_pin = get_PinNumber(GPIO_D_PORT, 7); //电源
    rt_pin_mode(ble_obj.pwr_pin, PIN_MODE_OUTPUT);
    rt_pin_write(ble_obj.rst_pin, PIN_LOW); //拉低关闭管脚

    ble_obj.rst_pin = get_PinNumber(GPIO_D_PORT, 3); //reset
    rt_pin_mode(ble_obj.rst_pin, PIN_MODE_OUTPUT);
    rt_pin_write(ble_obj.rst_pin, PIN_LOW); //拉低关闭管脚

    ble_obj.iur_out_pin = get_PinNumber(GPIO_E_PORT, 3); //IUR OUT
    rt_pin_mode(ble_obj.iur_out_pin, PIN_MODE_OUTPUT);
    rt_pin_write(ble_obj.iur_out_pin, PIN_LOW); //拉低关闭管脚

    ble_obj.iur_in_pin = get_PinNumber(GPIO_E_PORT, 0); //IUR IN
    rt_pin_mode(ble_obj.iur_in_pin, PIN_MODE_OUTPUT);
    rt_pin_write(ble_obj.iur_in_pin, PIN_LOW); //拉低关闭管脚

    ble_obj.rts_pin = get_PinNumber(GPIO_E_PORT, 2); //RTS
    rt_pin_mode(ble_obj.rts_pin, PIN_MODE_OUTPUT);
    rt_pin_write(ble_obj.rts_pin, PIN_LOW); //拉低关闭管脚

    ble_obj.cts_pin = get_PinNumber(GPIO_E_PORT, 1); //CTS
    rt_pin_mode(ble_obj.cts_pin, PIN_MODE_OUTPUT);
    rt_pin_write(ble_obj.cts_pin, PIN_LOW); //拉低关闭管脚

    err_code = at_client_init(BLUEMOD_SR_USING_SERIAL_NAME, 512);
    if (err_code != RT_EOK)
    {
        ble_debug("at_client_init failed. %d\n", err_code);
        return err_code;
    }
    /* urc数据列表初始化 */
    err_code = at_set_urc_table(ble_urc_table, sizeof(ble_urc_table) / sizeof(ble_urc_table[0]));
    if (err_code != RT_EOK)
    {
        ble_debug("at_set_urc_table failed. %d\n", err_code);
        return err_code;
    }

    /* SET SERIAL */
    rt_hw_serial_enable("usart2", RT_FALSE);
    return err_code;
}

// rt_err_t bluemodSR_Open(void)
// {
//     rt_err_t err_code = RT_EOK;
//     REQUEST_PM;
//     bluemodSR_PowerOn(&ble_obj, BLUEMOD_POWER_ON);
//     rt_hw_us_delay(5000);
//     at_response_t resp = RT_NULL;
//     resp = at_create_resp(256, 0, 1000);
//     err_code = at_exec_cmd(resp, "AT&F");
//     if (err_code != 0)
//     {
//         rt_kprintf("%d, AT&F\n", err_code);
//     }
//     err_code = at_exec_cmd(resp, "ATE0");
//     if (err_code != 0)
//     {
//         rt_kprintf("%d, ATE0\n", err_code);
//     }
//     err_code = at_exec_cmd(resp, "AT+UICP=1");
//     if (err_code != 0)
//     {
//         rt_kprintf("%d, AT+UICP=1\n", err_code);
//     }
//     RTS_LOW();
//     IUROUT_LOW();
//     rt_thread_mdelay(100);
//     at_delete_resp(resp);
//     RELEASE_PM;
//     return err_code;
// }

// void bluemodSR_Close(void)
// {
//     bluemodSR_PowerOn(&ble_obj, BLUEMOD_POWER_OFF);
// }

#include "drv_serial.h"
void bluemodSR_Open(void)
{
    REQUEST_PM;
    /* 打开电源 */
    bluemodSR_PowerOn(&ble_obj, BLUEMOD_POWER_ON);

    // rt_hw_us_delay(1000);
    rt_thread_mdelay(1000);
    /* SET SERIAL */
    rt_hw_serial_enable("usart2", RT_TRUE);

    /* IO CONFIG */
    ble_obj.iur_out_pin = get_PinNumber(GPIO_E_PORT, 3); //IUR OUT
    rt_pin_mode(ble_obj.iur_out_pin, PIN_MODE_OUTPUT);
    IUROUT_HIGH();

    ble_obj.rts_pin = get_PinNumber(GPIO_E_PORT, 2); //RTS
    rt_pin_mode(ble_obj.rts_pin, PIN_MODE_OUTPUT);
    RTS_HIGH();

    ble_obj.cts_pin = get_PinNumber(GPIO_E_PORT, 1); //CTS
    rt_pin_mode(ble_obj.cts_pin, PIN_MODE_INPUT_PULLUP);

    ble_obj.iur_in_pin = get_PinNumber(GPIO_E_PORT, 0); //IUR IN
    rt_pin_mode(ble_obj.iur_in_pin, PIN_MODE_INPUT_PULLUP);

    rt_pin_attach_irq(ble_obj.iur_in_pin, PIN_IRQ_MODE_RISING_FALLING, IURIN_handler, RT_NULL);
    rt_pin_irq_enable(ble_obj.iur_in_pin, PIN_IRQ_ENABLE);

    RELEASE_PM;
}

void bluemodSR_Close(void)
{
    rt_err_t result;
    REQUEST_PM;
    /* SET SERIAL */
    rt_hw_serial_enable("usart2", RT_FALSE);

    result = rt_pin_detach_irq(ble_obj.iur_in_pin);
    if (result != RT_EOK)
    {
        rt_kprintf("rt_pin_detach_irq failed.\n");
    }

    //rt_pin_irq_enable(ble_obj.iur_in_pin, PIN_IRQ_DISABLE);
    // if (result != RT_EOK)
    // {
    //     rt_kprintf("rt_pin_irq_enable failed.\n");
    // }

    /* IO CONFIG */
    ble_obj.iur_out_pin = get_PinNumber(GPIO_E_PORT, 3); //IUR OUT
    rt_pin_mode(ble_obj.iur_out_pin, PIN_MODE_OUTPUT);
    rt_pin_write(ble_obj.iur_out_pin, PIN_LOW); //拉低关闭管脚

    ble_obj.iur_in_pin = get_PinNumber(GPIO_E_PORT, 0); //IUR IN
    rt_pin_mode(ble_obj.iur_in_pin, PIN_MODE_OUTPUT);
    rt_pin_write(ble_obj.iur_in_pin, PIN_LOW); //拉低关闭管脚

    ble_obj.rts_pin = get_PinNumber(GPIO_E_PORT, 2); //RTS
    rt_pin_mode(ble_obj.rts_pin, PIN_MODE_OUTPUT);
    rt_pin_write(ble_obj.rts_pin, PIN_LOW); //拉低关闭管脚

    ble_obj.cts_pin = get_PinNumber(GPIO_E_PORT, 1); //CTS
    rt_pin_mode(ble_obj.cts_pin, PIN_MODE_OUTPUT);
    rt_pin_write(ble_obj.cts_pin, PIN_LOW); //拉低关闭管脚

    /* 关闭电源 */
    bluemodSR_PowerOn(&ble_obj, BLUEMOD_POWER_OFF);
    RELEASE_PM;
}

rt_err_t bluemodSR_GetVersion(char *buffer)
{
    rt_err_t err_code;
    at_response_t resp = RT_NULL;
    REQUEST_PM;
    resp = at_create_resp(256, 0, 1000);
    IUROUT_LOW();
    while (rt_pin_read(ble_obj.cts_pin))
        ;
    rt_kprintf("ATI3\n");
    err_code = at_exec_cmd(resp, "ATI3");
    if (err_code != 0)
    {
        rt_kprintf("%d, ATI3\n", err_code);
    }
    rt_thread_mdelay(100);
    IUROUT_HIGH();
    while (rt_pin_read(ble_obj.cts_pin) == 0)
        ;
    const char *str = at_resp_get_line(resp, 1);
    rt_strncpy(buffer, str, BLUEMOD_VERSION_SIZE);
    at_delete_resp(resp);
    RELEASE_PM;
    return err_code;
}

rt_err_t bluemodSR_SetBCName(const char *bname)
{
    rt_err_t err_code;
    at_response_t resp = RT_NULL;
    REQUEST_PM;
    resp = at_create_resp(256, 0, 1000);
    IUROUT_LOW();
    while (rt_pin_read(ble_obj.cts_pin))
        ;
    rt_kprintf("AT+BNAME\n");
    err_code = at_exec_cmd(resp, "AT+BNAME=%s", bname);
    if (err_code != 0)
    {
        rt_kprintf("%d, AT+BNAME\n", err_code);
    }
    rt_thread_mdelay(100);
    IUROUT_HIGH();
    while (rt_pin_read(ble_obj.cts_pin) == 0)
        ;
    at_delete_resp(resp);
    RELEASE_PM;
    return err_code;
}

rt_err_t bluemodSR_Sleep(void)
{
    rt_err_t err_code;
    at_response_t resp = RT_NULL;
    REQUEST_PM;
    resp = at_create_resp(256, 0, 1000);
    IUROUT_LOW();
    while (rt_pin_read(ble_obj.cts_pin))
        ;
    rt_kprintf("BLE Sleep.\n");
    err_code = at_exec_cmd(resp, "AT+SLEEP");
    if (err_code != 0)
    {
        rt_kprintf("%d, AT+SLEEP\n", err_code);
    }
    //rt_hw_us_delay(110000);
    rt_thread_mdelay(100);
    IUROUT_HIGH();
    rt_kprintf("SLEEP SET FINISH.\n");
    at_delete_resp(resp);
    RELEASE_PM;
    return err_code;
}

rt_size_t bluemodSR_Write(const char *data, rt_size_t len)
{
    rt_size_t size;
    REQUEST_PM;
    IUROUT_LOW();
    while (rt_pin_read(ble_obj.cts_pin))
        ;
    size = at_client_send(data, len);
    //rt_hw_us_delay(110000);
    rt_thread_mdelay(100);
    IUROUT_HIGH();
    while (rt_pin_read(ble_obj.cts_pin) == 0)
    {
    }
    RELEASE_PM;
    return size;
}

static void ble_open(void)
{
    bluemodSR_Open();
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(ble_open, BlueTooth Open);

static void ble_close(void)
{
    bluemodSR_Close();
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(ble_close, BlueTooth Close);

static void ble_bname(void)
{
    bluemodSR_SetBCName("0600654321");
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(ble_bname, BlueTooth set BName);

#endif