#ifndef DRV_USBD_H
#define DRV_USBD_H

#include <board.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "cdc_echo.h"

rt_size_t rt_cdc_tx(const rt_uint8_t *data, rt_size_t len);
void rt_cdc_cb_Register(efm32_cdc_callback *cb);

int rt_hw_usbd_init(void);

#endif // DRV_USBD_H