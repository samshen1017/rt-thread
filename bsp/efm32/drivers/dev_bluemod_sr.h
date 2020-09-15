/*
 * 已函数库的方式调用不太好，不符合驱动规范，后续需要改进（以rt_device_t实现）
 */

#ifndef DEV_BLUEMOD_SR_H
#define DEV_BLUEMOD_SR_H

#include <rtthread.h>
#include <rtdevice.h>

void bluemodSR_Open(void);                                  //open
void bluemodSR_Close(void);                                 //close
rt_size_t bluemodSR_Write(const char *data, rt_size_t len); //write

void bluemodSR_SetConnectCallback(void (*cb)(void));
void bluemodSR_SetDisconnectCallback(void (*cb)(void));
void bluemodSR_SetReceiveCallback(void (*cb)(const char *data, rt_size_t size));
rt_err_t bluemodSR_SetBCName(const char *bname); // io_ctrl
rt_err_t bluemodSR_GetVersion(char *buffer);

rt_err_t rt_hw_bluemodSR_init(void);

#endif