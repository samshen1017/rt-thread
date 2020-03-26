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
 * Last Modified: Wednesday, 25th March 2020 10:46:39
 * Modified By: Sam.Shen (samshen1017@sina.com>)
 * 
 * Automatically generated; DO NOT EDIT.
 * 
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 * ********************************************************************************/

#include "drv_usbd.h"
#include <rtdevice.h>

#ifdef RT_USING_USB_DEVICE
#include "em_usb.h"
#include "em_usbtypes.h"
#include "em_usbhal.h"
#include "em_usbd.h"

// USB specific includes
#include "cdc_echo.h"
#include "descriptors.h"

extern void cdcStateChangeEvent(USBD_State_TypeDef oldState, USBD_State_TypeDef newState);

static struct udcd _efm32_udc;

static struct ep_id _ep_pool[] =
    {
        {0x0, USB_EP_ATTR_CONTROL, USB_DIR_INOUT, 64, ID_ASSIGNED},
        {0x1, USB_EP_ATTR_BULK, USB_DIR_OUT, 64, ID_UNASSIGNED},
        {0x81, USB_EP_ATTR_BULK, USB_DIR_IN, 64, ID_UNASSIGNED},
        {0x82, USB_EP_ATTR_BULK, USB_DIR_IN, 64, ID_UNASSIGNED},
};

static rt_err_t _set_address(rt_uint8_t address)
{
    rt_kprintf("_set_address\n");
    USBD_SetUsbState(USBD_STATE_ADDRESSED);
    USBDHAL_SetAddr(address);
    return RT_EOK;
}

static rt_err_t _set_config(rt_uint8_t address)
{
    rt_kprintf("_set_config\n");
    return RT_EOK;
}

static rt_err_t _ep_set_stall(rt_uint8_t address)
{
    rt_kprintf("_ep_set_stall\n");
    USBD_StallEp(address);
    return RT_EOK;
}

static rt_err_t _ep_clear_stall(rt_uint8_t address)
{
    rt_kprintf("_ep_clear_stall\n");
    USBD_UnStallEp(address);
    return RT_EOK;
}

static rt_err_t _ep_enable(uep_t ep)
{
    rt_kprintf("_ep_enable\n");
    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);
    // HAL_PCD_EP_Open(&_stm_pcd, ep->ep_desc->bEndpointAddress, ep->ep_desc->wMaxPacketSize, ep->ep_desc->bmAttributes);
    return RT_EOK;
}

static rt_err_t _ep_disable(uep_t ep)
{
    rt_kprintf("_ep_disable\n");
    RT_ASSERT(ep != RT_NULL);
    RT_ASSERT(ep->ep_desc != RT_NULL);
    // HAL_PCD_EP_Close(&_stm_pcd, ep->ep_desc->bEndpointAddress);
    return RT_EOK;
}

static rt_size_t _ep_read(rt_uint8_t address, void *buffer)
{
    rt_kprintf("_ep_read\n");
    rt_size_t size = 0;
    RT_ASSERT(buffer != RT_NULL);
    return size;
}

static rt_size_t _ep_read_prepare(rt_uint8_t address, void *buffer, rt_size_t size)
{
    rt_kprintf("_ep_read_prepare\n");
    int result;
    USBD_Read(address, buffer, size, NULL);
    if (result = USB_STATUS_OK)
    {
        return size;
    }
    return 0;
}

static rt_err_t _ep0_send_status(void)
{
    rt_kprintf("_ep0_send_status\n");
    USBD_Write(0, NULL, 0, NULL); // Send Status to Host
    // HAL_PCD_EP_Transmit(&_stm_pcd, 0x00, NULL, 0);
    return RT_EOK;
}

static rt_size_t _ep_write(rt_uint8_t address, void *buffer, rt_size_t size)
{
    rt_kprintf("_ep_write\n");
    int result;
    result = USBD_Write(address, buffer, size, NULL);
    if (result = USB_STATUS_OK)
    {
        return size;
    }
    return 0;
}

static rt_err_t _suspend(void)
{
    return RT_EOK;
}

static rt_err_t _wakeup(void)
{
    return RT_EOK;
}

// Set the callback functions (see src/cdc.c)
USBD_Callbacks_TypeDef callbacks = {
    .usbReset = NULL,
    .usbStateChange = cdcStateChangeEvent, // Called when the device changes state
    .setupCmd = cdcSetupCmd,               // Called on each setup request from the host
    .isSelfPowered = NULL,
    .sofInt = NULL,
};

// Set the initialization struct descriptors (see src/descriptors.c)
const USBD_Init_TypeDef usbInitStruct = {
    .deviceDescriptor = &USBDESC_deviceDesc,
    .configDescriptor = USBDESC_configDesc,
    .stringDescriptors = USBDESC_strings,
    .numberOfStrings = sizeof(USBDESC_strings) / sizeof(void *),
    .callbacks = &callbacks,
    .bufferingMultiplier = USBDESC_bufferingMultiplier,
    .reserved = 0,
};

static rt_err_t _init(rt_device_t device)
{
    rt_kprintf("_init\n");
    // Initialize and start USB device stack
    USBD_Init(&usbInitStruct);
    return RT_EOK;
}

const static struct udcd_ops _udc_ops =
    {
        _set_address,
        _set_config,
        _ep_set_stall,
        _ep_clear_stall,
        _ep_enable,
        _ep_disable,
        _ep_read_prepare,
        _ep_read,
        _ep_write,
        _ep0_send_status,
        _suspend,
        _wakeup,
};

int rt_hw_usbd_init(void)
{
    rt_memset((void *)&_efm32_udc, 0, sizeof(struct udcd));
    _efm32_udc.parent.type = RT_Device_Class_USBDevice;
    _efm32_udc.parent.init = _init;
    _efm32_udc.ops = &_udc_ops;

    /* Register endpoint infomation */
    _efm32_udc.ep_pool = _ep_pool;
    _efm32_udc.ep0.id = &_ep_pool[0];
    rt_device_register((rt_device_t)&_efm32_udc, "EM_usbd", 0);
    rt_usb_device_init();
    // rt_usb_vcom_init();
    return 0;
}

#endif