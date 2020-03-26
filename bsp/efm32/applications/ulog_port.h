#ifndef ULOG_PORT_H
#define ULOG_PORT_H
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include <stdio.h>
#include <string.h>

#define LOG_MAX_LENTH 300
#define LOG_OLD_FILE_NAME "/ulog_old.txt"
#define LOG_NEW_FILE_NAME "/ulog.txt"
#define LOG_TEMP_FILE_NAME "/temp.txt"

int ulog_flash_backend_init(void);

#endif