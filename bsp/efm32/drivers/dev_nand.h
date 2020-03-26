/* ********************************************************************************
 * 
 * Copyright (C) 2019 - 2020 Shanghai 3H Inc. All rights reserved.
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * File: dev_nand.h
 * File Created: Wednesday, 19th February 2020 09:46:44
 * Author: Sam.Shen (samshen1017@sina.com)
 * 
 * Last Modified: Wednesday, 4th March 2020 01:27:45
 * Modified By: Sam.Shen (samshen1017@sina.com>)
 * 
 * Automatically generated; DO NOT EDIT.
 * 
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 * ********************************************************************************/

#ifndef DEV_NAND_H
#define DEV_NAND_H

#include <rtdevice.h>

#define EFM32GG_EBI_DMACH (5) /* ebi dma channel */

#define _BANK0 (0x80000000)
#define EBI_NAND_BANK (_BANK0)
#define NAND_ALE (1 << 6) /* PA5:AD14 -> A6 */
#define NAND_CLE (1 << 7) /* PA6:AD15 -> A7 */
#define NAND_DATA (EBI_NAND_BANK)

#define NAND_READY_PORT gpioPortD
#define NAND_READY_PIN (11)
#define NAND_WP_PORT gpioPortD
#define NAND_WP_PIN (12)

/* nandflash command */
#define NAND_RDA_CMD 0x00
#define NAND_RDB_CMD 0x30
#define NAND_RDID_CMD 0x90
#define NAND_RDSTATUS_CMD 0x70
#define NAND_PAGEPROG1_CMD 0x80
#define NAND_PAGEPROG2_CMD 0x10
#define NAND_RDACPB_CMD 0x00
#define NAND_RDBCPB_CMD 0x35
#define NAND_CPBPROG1_CMD 0x85
#define NAND_CPBPROG2_CMD 0x10
#define NAND_BLOCKERASE1_CMD 0x60
#define NAND_BLOCKERASE2_CMD 0xD0
#define NAND_RST_CMD 0xFF

#define NAND_ID_K9F2G08U0B 0xDAEC

/* nandflash confg */
#define PAGES_PER_BLOCK 64
#define PAGE_DATA_SIZE 2048
#define PAGE_OOB_SIZE 64
#define NAND_MARK_SPARE_OFFSET 4

struct efm32gg_nandflash
{
    struct rt_completion completion; /* completion event */
    struct rt_mutex lock;            /* mutex */
    rt_uint8_t id[5];                /* nand id */
};

void rt_hw_mtd_nand_init(void);
void uffs_auto_mount(void);
#endif //DEV_NAND_H