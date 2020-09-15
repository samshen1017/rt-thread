#include <board.h>
#include <drv_pin.h>

/**
 * set your hardware default IO Status
*/
static void _standby(void)
{
    /* 让MAX3471进去节能状态 */
    rt_uint16_t max3471_de_pin = get_PinNumber(GPIO_E_PORT, 7);
    rt_pin_mode(max3471_de_pin, PIN_MODE_OUTPUT);
    rt_pin_write(max3471_de_pin, PIN_LOW);

    /* 关闭蓝牙电源 */
    rt_uint16_t ble_pwr_pin = get_PinNumber(GPIO_D_PORT, 7);
    rt_pin_mode(ble_pwr_pin, PIN_MODE_OUTPUT);
    rt_pin_write(ble_pwr_pin, PIN_LOW); //拉低关闭管脚

    /* 关闭LCD电源 */
    rt_uint16_t lcd_pwr_pin = get_PinNumber(GPIO_C_PORT, 8);
    rt_pin_mode(lcd_pwr_pin, PIN_MODE_OUTPUT);
    rt_pin_write(lcd_pwr_pin, PIN_LOW); //拉低关闭管脚
}

void rt_init_thread_entry(void *parameter)
{
/* Initialize Spiflash  */
#if defined(RT_USING_W25QXX)
    rt_hw_w25qxx_init();
#endif

/*  Initialize Bluetooth */
#if defined(RT_USING_BLUEMOD_SR)
    rt_hw_bluemodSR_init();
#endif

/* uffs mount */
#if defined(NAND_DFS_AUTO_MOUNT)
    uffs_auto_mount();
#endif

/* Initialize LS027B7DH */
#if defined(RT_USING_LS027B7DH)
    rt_hw_memlcd_init();
#endif

/* Initialize Ulog */
#if defined(RT_USING_ULOG)
    ulog_init();
    ulog_console_backend_init();
    ulog_flash_backend_init();
#endif

/* Initialize USB DEVICE */
#if defined(RT_USING_USB_DEVICE)
    rt_hw_usbd_init();
#endif

    _standby();
#if defined(RT_USING_PM)
    rt_pm_default_set(PM_SLEEP_MODE_DEEP);
    rt_pm_run_enter(PM_RUN_MODE_LOW_SPEED);
#endif
}

int rt_application_init()
{
    rt_thread_t init_thread;
    init_thread = rt_thread_create("init", rt_init_thread_entry, RT_NULL, 2048, 5, 20);
    rt_thread_startup(init_thread);
    return 0;
}

/***************************************************************************/ /**
 * @}
 ******************************************************************************/
