#include "ulog_port.h"
#include <stdlib.h>
#define LOG_LVL LOG_LVL_DBG
#include <ulog.h>
#include <rthw.h>

static int read_line(void)
{
    int lines = 0;
    char one_line[200];
    FILE *f = fopen(LOG_NEW_FILE_NAME, "r");
    while (!feof(f))
    {
        if (fgets(one_line, sizeof(one_line), f) != NULL) //读到一个换行符就视为一行
        {
            lines++;
        }
    }
    fclose(f);
    return lines;
}

static void write_line(const char *buf, size_t length)
{
    FILE *f = fopen(LOG_NEW_FILE_NAME, "a");
    fwrite(buf, sizeof(char), length, f);
    fclose(f);
}

static void change_over_file(void)
{
    remove(LOG_OLD_FILE_NAME);
    rename(LOG_NEW_FILE_NAME, LOG_OLD_FILE_NAME);
    FILE *f = fopen(LOG_NEW_FILE_NAME, "w");
    fclose(f);
}

static void delete_line(int n)
{
    int lines = 0;
    char buf[200];
    FILE *f = fopen(LOG_NEW_FILE_NAME, "r");
    FILE *fp = fopen(LOG_TEMP_FILE_NAME, "w");
    while (!feof(f))
    {
        lines++;
        if (lines == n)
        {
            fgets(buf, sizeof(buf), f); //指针偏移
        }
        else
        {
            if (fgets(buf, sizeof(buf), f) != NULL)
            {
                fprintf(fp, "%s", buf);
            }
        }
    }
    fclose(f);
    fclose(fp);
    remove(LOG_NEW_FILE_NAME);
    rename(LOG_TEMP_FILE_NAME, LOG_NEW_FILE_NAME);
}

static void read_line_test(void)
{
    int n = read_line();
    rt_kprintf("共%d行\n", n);
}

static void clear_ulog(void)
{
    FILE *f = fopen(LOG_OLD_FILE_NAME, "w");
    FILE *fp = fopen(LOG_NEW_FILE_NAME, "w");
    fclose(f);
    fclose(fp);
}

static void ulog_example(void)
{
    for (int i = 0; i < 100; i++)
    {
        LOG_D("LOG_D(%d)", i);
        // LOG_D("LOG_D(%d): RT-Thread is an open source IoT operating system from China.", i);
        // LOG_I("LOG_I(%d): RT-Thread is an open source IoT operating system from China.", i);
        // LOG_W("LOG_W(%d): RT-Thread is an open source IoT operating system from China.", i);
        // LOG_E("LOG_E(%d): RT-Thread is an open source IoT operating system from China.", i);
        // ulog_d("test", "ulog_d(%d): RT-Thread is an open source IoT operating system from China.", i);
        // ulog_i("test", "ulog_i(%d): RT-Thread is an open source IoT operating system from China.", i);
        // ulog_w("test", "ulog_w(%d): RT-Thread is an open source IoT operating system from China.", i);
        // ulog_e("test", "ulog_e(%d): RT-Thread is an open source IoT operating system from China.", i);
    }
}

static void ulog_single(void)
{
    int i = 0;
    LOG_W("LOG_W(%d): RT-Thread is an open source IoT operating system from China.", i);
}

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(read_line_test, read_line test);
MSH_CMD_EXPORT(write_line, write_line test);
MSH_CMD_EXPORT(clear_ulog, clear_ulog test);
MSH_CMD_EXPORT(delete_line, delete_line test);
MSH_CMD_EXPORT(ulog_example, ulog_example test);
MSH_CMD_EXPORT(ulog_single, ulog_single test);
#endif

/* 定义FLASH后端设备 */
static struct ulog_backend flash;
/* FLASH后端输出函数 */
void ulog_flash_backend_output(struct ulog_backend *backend, rt_uint32_t level, const char *tag, rt_bool_t is_raw, const char *log, size_t len)
{
    /* 输出日志到flash */
    int lines = read_line();
    if (lines >= LOG_MAX_LENTH)
    {
        rt_kprintf("%d over lenth\n", lines);
        change_over_file();
    }
    write_line(log, len);
}

/* 控制台后端初始化 */
int ulog_flash_backend_init(void)
{
    FILE *f = fopen(LOG_OLD_FILE_NAME, "r");
    if (f == NULL)
    {
        f = fopen(LOG_OLD_FILE_NAME, "w");
    }
    fclose(f);
    FILE *fp = fopen(LOG_NEW_FILE_NAME, "r");
    if (fp == NULL)
    {
        fp = fopen(LOG_NEW_FILE_NAME, "w");
    }
    fclose(fp);
    /* 设定输出函数 */
    flash.output = ulog_flash_backend_output;
    /* 注册后端 */
    ulog_backend_register(&flash, "flash", RT_TRUE);
    return 0;
}
