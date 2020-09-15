#ifndef YMODEM4USB_H
#define YMODEM4USB_H

#include "rtthread.h"

#define YM_ENABLE_CRC
#define YM_PATH_SIZE 24
#define YM_FILE_SIZE 24
#define YM_BUFFER_SIZE 1152

/* 协议声明 */
typedef enum
{
    YM_CODE_NONE = 0x00,
    YM_CODE_SOH = 0x01,
    YM_CODE_STX = 0x02,
    YM_CODE_EOT = 0x04,
    YM_CODE_ACK = 0x06,
    YM_CODE_NAK = 0x15,
    YM_CODE_CAN = 0x18,
    YM_CODE_C = 0x43,
} YM_CODE;

/* 错误反馈 */
typedef enum
{
    /* timeout on handshake */
    YM_ERR_TMO = 0x00,
    /* wrong code, wrong SOH, STX etc. */
    YM_ERR_CODE = 0x01,
    /* wrong sequence number */
    YM_ERR_SEQ = 0x02,
    /* wrong CRC checksum */
    YM_ERR_CRC = 0x04,
    /* not enough data received */
    YM_ERR_DSZ = 0x06,
    /* the transmission is aborted by user */
    YM_ERR_CAN = 0x15,
    /* wrong answer, wrong ACK or C */
    YM_ERR_ACK = 0x18,
    /* transmit file invalid */
    YM_ERR_FILE = 0x43,
} YM_ERR;

/* 状态 */
typedef enum
{
    YM_STAGE_NONE,
    /* set when C is send */
    YM_STAGE_ESTABLISHING,
    /* set when we've got the packet 0 and sent ACK and second C */
    YM_STAGE_ESTABLISHED,
    /* set when the sender respond to our second C and recviever got a real data packet. */
    YM_STAGE_TRANSMITTING,
    /* set when the sender send a EOT */
    YM_STAGE_FINISHING,
    /* set when transmission is really finished, i.e., after the NAK, C, final NULL packet stuff. */
    YM_STAGE_FINISHED,
} YM_STAGE;

typedef struct
{
    char filePath[YM_PATH_SIZE];
    char fileName[YM_FILE_SIZE];
    rt_uint8_t rx_buf[YM_BUFFER_SIZE];
    rt_sem_t sem;
    rt_thread_t th;
    YM_STAGE stage;
} ym_ctx;

rt_err_t ymodem4usb_start(const char *path);

#endif
