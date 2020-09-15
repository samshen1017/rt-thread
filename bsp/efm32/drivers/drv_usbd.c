/* ********************************************************************************
 * 
 * Copyright (C) 2019 - 2020 Shanghai 3H Inc. All rights reserved.
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * File: drv_usbd.c
 * File Created: Wednesday, 25th March 2020 01:52:26
 * Author: Sam.Shen (samshen1017@sina.com)
 * 
 * Last Modified: Friday, 21st August 2020 05:44:05
 * Modified By: Sam.Shen (samshen1017@sina.com>)
 * 
 * Automatically generated; DO NOT EDIT.
 * 
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 * ********************************************************************************/

#include "drv_usbd.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <rthw.h>

#ifdef RT_USING_USB_DEVICE
#include "em_usb.h"
#include "em_usbtypes.h"
#include "em_usbhal.h"
#include "em_usbd.h"
#include "descriptors.h"

extern void cdcStateChangeEvent(USBD_State_TypeDef oldState, USBD_State_TypeDef newState);

// Set the callback functions (see src/cdc.c)
static const USBD_Callbacks_TypeDef callbacks = {
    .usbReset = NULL,
    .usbStateChange = cdcStateChangeEvent, // Called when the device changes state
    .setupCmd = cdcSetupCmd,               // Called on each setup request from the host
    .isSelfPowered = NULL,
    .sofInt = NULL,
};

// Set the initialization struct descriptors (see src/descriptors.c)
static const USBD_Init_TypeDef usbInitStruct = {
    .deviceDescriptor = &USBDESC_deviceDesc,
    .configDescriptor = USBDESC_configDesc,
    .stringDescriptors = USBDESC_strings,
    .numberOfStrings = sizeof(USBDESC_strings) / sizeof(void *),
    .callbacks = &callbacks,
    .bufferingMultiplier = USBDESC_bufferingMultiplier,
    .reserved = 0,
};

rt_size_t rt_cdc_tx(const rt_uint8_t *data, rt_size_t len)
{
    return cdcWrite((void *)data, len);
}

void rt_cdc_cb_Register(efm32_cdc_callback *cb)
{
    cdcCallbackRegister(cb);
}

int rt_hw_usbd_init(void)
{
    int result;
    result = USBD_Init(&usbInitStruct);
    if (result != USB_STATUS_OK)
    {
        rt_kprintf("USBD_Init failed.\n");
        return result;
    }
    USB->ROUTE = USB_ROUTE_PHYPEN; //默认USBD_Init打开了USB VBUS外部输出管脚(PF5)，改为关闭USB_VBUS管脚
    return result;
}

/**
 * 检查结尾是否为0x0d 0x0a(换行)
*/
static size_t line_length_check(const char *usbRxBuffer, size_t xferred)
{
    size_t length = xferred;
    for (int i = xferred; i > 0; i--)
    {
        if ((usbRxBuffer[i] == 0x0A) && (usbRxBuffer[i - 1] == 0x0D))
        {
            return length + 1;
        }
        else
        {
            length--;
        }
    }
    return length;
}

static void _rxcb(const void *usbRxBuffer, size_t xferred)
{
    size_t len = line_length_check((const char *)usbRxBuffer, xferred);
    rt_kprintf("r:[");
    for (int i = 0; i < len; i++)
    {
        rt_kprintf("%x ", ((char *)usbRxBuffer)[i]);
    }
    rt_kprintf("]\n");
}

static void _connected(void)
{
    rt_kprintf("connected\n");
}

static void _disconnect(void)
{
    rt_kprintf("disconnect\n");
}

static efm32_cdc_callback cb = {
    .connected = _connected,
    .disconnect = _disconnect,
    .receiveCallback = _rxcb,
};

static void usb_test(void)
{
    rt_cdc_cb_Register(&cb);
}
MSH_CMD_EXPORT(usb_test, usb test);

static void usb_write(int argc, void **argv)
{
    char w_buf[256] = {0};
    const char *w_str = argv[1];
    if (argc < 2)
    {
        rt_kprintf("Usage: usb_write [string]\n");
        return;
    }
    strncpy(w_buf, w_str, 256);
    rt_cdc_tx((rt_uint8_t *)w_buf, strlen(w_buf));
}
MSH_CMD_EXPORT(usb_write, usb write test);
#endif
