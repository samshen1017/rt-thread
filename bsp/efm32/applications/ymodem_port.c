#include "ymodem_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define DEFAULT_DOWNLOAD_PART "/usr/firmware"
#define DEFAULT_DOWNLOAD_PART "/"

#if defined(RT_USING_RYM) && defined(RT_USING_DFS)
#include "ymodem.h"

/* 下载目录 */
static char *recv_path = DEFAULT_DOWNLOAD_PART;
/* 下载文件描述符 */
static FILE *recv_f = RT_NULL;
/* 全局变量 */
static rt_uint32_t update_file_total_size, update_file_cur_size;

static enum rym_code ymodem_on_begin(struct rym_ctx *ctx, rt_uint8_t *buf, rt_size_t len)
{
    char *file_name, *file_size;
    char file_str[96] = {0};

    file_name = (char *)&buf[0];
    file_size = (char *)&buf[rt_strlen(file_name) + 1];
    update_file_total_size = atol(file_size);
    update_file_cur_size = 0;

    sprintf(file_str, "%s/%s", recv_path, file_name);
    recv_f = fopen(file_str, "w");
    if (recv_f == NULL)
    {
        rt_kprintf("%s fopen failed.\n", file_str);
        /* if fail then end session */
        return RYM_CODE_CAN;
    }
    return RYM_CODE_ACK;
}

static enum rym_code ymodem_on_data(struct rym_ctx *ctx, rt_uint8_t *buf, rt_size_t len)
{
    int ret = fwrite(buf, 1, len, recv_f);
    if (ret != len)
    {
        rt_kprintf("ymodem_on_data failed.\n");
        /* if fail then end session */
        return RYM_CODE_CAN;
    }
    return RYM_CODE_ACK;
}

static enum rym_code ymodem_on_end(struct rym_ctx *ctx, rt_uint8_t *buf, rt_size_t len)
{
    int ret = fwrite(buf, 1, len, recv_f);
    if (ret != len)
    {
        rt_kprintf("ymodem_on_end failed.\n");
        /* if fail then end session */
        return RYM_CODE_CAN;
    }
    fflush(recv_f);
    fclose(recv_f);
    recv_f = NULL;
    return RYM_CODE_ACK;
}

void ymodem_start(rt_device_t dev)
{
    struct rym_ctx rctx;
    rt_kprintf("Please select the file and use Ymodem to send.\n");

    if (!rym_recv_on_device(&rctx, dev, RT_DEVICE_OFLAG_RDWR, ymodem_on_begin, ymodem_on_data, ymodem_on_end, RT_TICK_PER_SECOND))
    {
        rt_kprintf("Download file to flash success.\n");
    }
    else
    {
        rt_thread_delay(RT_TICK_PER_SECOND);
        rt_kprintf("Update file failed.\n");
    }
}

static void ymodem_ota(rt_uint8_t argc, void **argv)
{
    rt_device_t device = rt_console_get_device();
    if (device == RT_NULL)
    {
        rt_kprintf("rt_console_get_device failed.\n");
        return;
    }
    ymodem_start(device);
}
MSH_CMD_EXPORT(ymodem_ota, Ymodem download file to flash chip);
#endif