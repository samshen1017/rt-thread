/* ********************************************************************************
 * 
 * Copyright (C) 2019 - 2020 Shanghai 3H Inc. All rights reserved.
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * File: dev_nand.c
 * File Created: Wednesday, 19th February 2020 09:46:56
 * Author: Sam.Shen (samshen1017@sina.com)
 * 
 * Last Modified: Thursday, 23rd July 2020 05:07:06
 * Modified By: Sam.Shen (samshen1017@sina.com>)
 * 
 * Automatically generated; DO NOT EDIT.
 * 
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	---------------------------------------------------------
 * ********************************************************************************/

#include "dev_nand.h"
#include <drivers/mtd_nand.h>
#include "board.h"
#if defined(RT_USING_DFS)
#include "dfs_uffs.h"
#include "dfs_posix.h"
#endif

#if defined(RT_USING_EXT_NAND)
#define NANDFLASH_DEBUG
#ifdef NANDFLASH_DEBUG
#define nand_debug(format, args...) rt_kprintf(format, ##args)
#else
#define nand_debug(format, args...)
#endif

static struct efm32gg_nandflash _nand;

#if defined(RT_USING_PM)
#define REQUEST_PM                               \
    do                                           \
    {                                            \
        rt_pm_request(PM_SLEEP_MODE_NONE);       \
        rt_pm_run_enter(PM_RUN_MODE_HIGH_SPEED); \
    } while (0)

#define RELEASE_PM                              \
    do                                          \
    {                                           \
        rt_pm_release(PM_SLEEP_MODE_NONE);      \
        rt_pm_run_enter(PM_RUN_MODE_LOW_SPEED); \
    } while (0)
#else
#define REQUEST_PM
#define RELEASE_PM
#endif

static void completion_callback(unsigned int channel, bool primary, void *user)
{
    rt_completion_done(&_nand.completion);
}

static DMA_CB_TypeDef dmaCb =
    {
        completion_callback,
        RT_NULL,
        0,
};

static const DMA_CfgChannel_TypeDef chnCfg =
    {
        false,   /* Default priority */
        RT_TRUE, /* interrupt on transfer completion */
        0,       /* Memory-memory transfers */
        &dmaCb,  /* Transfer completion callback */
};

static DMA_CfgDescr_TypeDef descCfgWr =
    {
        dmaDataIncNone,
        dmaDataInc4,
        dmaDataSize4,
        dmaArbitrate1,
        0,
};

static DMA_CfgDescr_TypeDef descCfgRd =
    {
        dmaDataInc4,
        dmaDataIncNone,
        dmaDataSize4,
        dmaArbitrate1,
        0,
};

rt_inline void nand_cmd(rt_uint8_t cmd)
{
    *(volatile rt_uint8_t *)(EBI_NAND_BANK | NAND_CLE) = cmd;
}

rt_inline void nand_addr(rt_uint8_t addr)
{
    *(volatile rt_uint8_t *)(EBI_NAND_BANK | NAND_ALE) = addr;
}

rt_inline rt_uint8_t nand_read8(void)
{
    return (*((volatile rt_uint8_t *)(EBI_NAND_BANK)));
}

rt_inline void nand_write8(rt_uint8_t data)
{
    *((volatile rt_uint8_t *)(EBI_NAND_BANK)) = data;
}

rt_inline void waitReady(void)
{
    /* Wait for EBI idle in case of EBI writeBuffer is enabled */
    while (EBI->STATUS & EBI_STATUS_AHBACT)
    {
    }
    /* Wait on Ready/Busy pin to become high */
    while ((GPIO->P[NAND_READY_PORT].DIN & (1 << NAND_READY_PIN)) == 0)
    {
    }
}

rt_inline void writeProtect(bool enable)
{
    if (enable)
    {
        GPIO->P[NAND_WP_PORT].DOUTCLR = (1 << NAND_WP_PIN);
    }
    else
    {
        GPIO->P[NAND_WP_PORT].DOUTSET = (1 << NAND_WP_PIN);
    }
}

static uint8_t readStatus(void)
{
    nand_cmd(NAND_RDSTATUS_CMD);

    return (nand_read8());
}

static void dmaRead(rt_uint8_t *dst, rt_uint32_t size)
{
    rt_uint32_t timeout;

    RT_ASSERT(dst != RT_NULL);
    RT_ASSERT(size != 0);

    timeout = size * 5;
    rt_completion_init(&_nand.completion);
    REQUEST_PM;

    if (((rt_uint32_t)dst & 1) || (size & 1))
    {
        /* Byte transfer */
        descCfgRd.dstInc = dmaDataInc1;
        descCfgRd.size = dmaDataSize1;
    }
    else if ((((rt_uint32_t)dst & 3) == 0) && ((size & 3) == 0))
    {
        /* Word transfer */
        descCfgRd.dstInc = dmaDataInc4;
        descCfgRd.size = dmaDataSize4;
        size /= 4;
    }
    else if ((((rt_uint32_t)dst & 3) == 2) || ((size & 3) == 2))
    {
        /* Halfword transfer */
        descCfgRd.dstInc = dmaDataInc2;
        descCfgRd.size = dmaDataSize2;
        size /= 2;
    }

    DMA_CfgDescr(EFM32GG_EBI_DMACH, RT_TRUE, (void *)&descCfgRd);
    DMA_ActivateAuto(EFM32GG_EBI_DMACH, true, dst, (void *)NAND_DATA, size - 1);
    if (rt_completion_wait(&_nand.completion, timeout) != RT_EOK)
    {
        nand_debug("nand read timeout\n");
    }
    RELEASE_PM;
}

static void dmaWrite(const rt_uint8_t *src, rt_uint32_t size)
{
    rt_uint32_t timeout;

    RT_ASSERT(src != RT_NULL);
    RT_ASSERT(size != 0);

    timeout = size * 5;
    rt_completion_init(&_nand.completion);
    REQUEST_PM;

    if (((rt_uint32_t)src & 1) || (size & 1))
    {
        /* Byte transfer */
        descCfgWr.srcInc = dmaDataInc1;
        descCfgWr.size = dmaDataSize1;
    }
    else if ((((rt_uint32_t)src & 3) == 0) && ((size & 3) == 0))
    {
        /* Word transfer */
        descCfgWr.srcInc = dmaDataInc4;
        descCfgWr.size = dmaDataSize4;
        size /= 4;
    }
    else if ((((rt_uint32_t)src & 3) == 2) || ((size & 3) == 2))
    {
        /* Halfword transfer */
        descCfgWr.srcInc = dmaDataInc2;
        descCfgWr.size = dmaDataSize2;
        size /= 2;
    }

    DMA_CfgDescr(EFM32GG_EBI_DMACH, true, (void *)&descCfgWr);
    DMA_ActivateAuto(EFM32GG_EBI_DMACH, true, (void *)NAND_DATA, (void *)src, size - 1);
    if (rt_completion_wait(&_nand.completion, timeout) != RT_EOK)
    {
        nand_debug("nand write timeout\n");
    }
    RELEASE_PM;
}

static void reset(void)
{
    nand_cmd(NAND_RST_CMD);
    waitReady();
}

rt_err_t nand_data_correct(rt_uint32_t generatedEcc,
                           rt_uint32_t readEcc,
                           uint8_t *data)
{
#define ECC_MASK28 0x0FFFFFFF /* 28 valid ECC parity bits. */
#define ECC_MASK 0x05555555   /* 14 ECC parity bits.       */

    rt_uint32_t count, bitNum, byteAddr;
    rt_uint32_t mask;
    rt_uint32_t syndrome;
    rt_uint32_t eccP;  /* 14 even ECC parity bits. */
    rt_uint32_t eccPn; /* 14 odd ECC parity bits.  */

    syndrome = (generatedEcc ^ readEcc) & ECC_MASK28;

    if (syndrome == 0)
        return (RT_MTD_EOK); /* No errors in data. */

    eccPn = syndrome & ECC_MASK;       /* Get 14 odd parity bits.  */
    eccP = (syndrome >> 1) & ECC_MASK; /* Get 14 even parity bits. */

    if ((eccPn ^ eccP) == ECC_MASK) /* 1-bit correctable error ? */
    {
        bitNum = (eccP & 0x01) | ((eccP >> 1) & 0x02) | ((eccP >> 2) & 0x04);

        byteAddr = ((eccP >> 6) & 0x001) |
                   ((eccP >> 7) & 0x002) |
                   ((eccP >> 8) & 0x004) |
                   ((eccP >> 9) & 0x008) |
                   ((eccP >> 10) & 0x010) |
                   ((eccP >> 11) & 0x020) |
                   ((eccP >> 12) & 0x040) |
                   ((eccP >> 13) & 0x080) |
                   ((eccP >> 14) & 0x100) |
                   ((eccP >> 15) & 0x200) |
                   ((eccP >> 16) & 0x400);

        data[byteAddr] ^= 1 << bitNum;

        return RT_MTD_EOK;
    }

    /* Count number of one's in the syndrome. */
    count = 0;
    mask = 0x00800000;
    while (mask)
    {
        if (syndrome & mask)
            count++;
        mask >>= 1;
    }

    if (count == 1) /* Error in the ECC itself. */
        return -RT_MTD_EECC;

    return -RT_MTD_EECC; /* Unable to correct data. */

#undef ECC_MASK
#undef ECC_MASK28
}

/**
 * @brief Check for ID
 * @param device: Pointer to MTD_NAND device
 * @return RT_EOK - The nand flash is K9F2G08U0B
 *         RT_ERROR - Can not support this flash
 */
static rt_err_t nandflash_readid(struct rt_mtd_nand_device *device)
{
    rt_uint16_t id = 0;
    waitReady();
    reset();
    nand_cmd(NAND_RDID_CMD);
    nand_addr(0);
    id = nand_read8();
    id |= (nand_read8() << 8);
    if (id == NAND_ID_K9F2G08U0B)
    {
        rt_kprintf("nand: k9f2g08u0b\n");
        return (RT_EOK);
    }
    else
    {
        rt_kprintf("unknown nand\n");
        return (RT_ERROR);
    }
}

/**
 * @brief Read a page from nand flash
 * @param device: Pointer to MTD_NAND device
 * @param page: Relative page offset of a logical partition
 * @param data: Buffer of main data
 * @param data_len: The length you want to read
 * @param spare: Buffer of spare data
 * @param spare_len: The length you want to read
 * @return RT_MTD_EOK - No error occurs
 *         -RT_MTD_EECC - The ECC error can not be fixed
 */
static rt_err_t nandflash_readpage(struct rt_mtd_nand_device *device, rt_off_t page, rt_uint8_t *data, rt_uint32_t data_len, rt_uint8_t *spare, rt_uint32_t spare_len)
{
    rt_uint32_t genecc;
    rt_err_t result;
    rt_uint8_t readecc[4];

    result = RT_MTD_EOK;
    RT_ASSERT(data_len <= PAGE_DATA_SIZE);
    RT_ASSERT(spare_len <= PAGE_OOB_SIZE);

    /* nand_debug("nand read[%d,%d,%d]\n",page,data_len,spare_len); */

    page = page + device->block_start * device->pages_per_block;
    if (page / device->pages_per_block > device->block_end)
    {
        return -RT_EIO;
    }

    rt_mutex_take(&_nand.lock, RT_WAITING_FOREVER);

    /* Read main data area */
    if (data && data_len)
    {
        /* The first READ command */
        nand_cmd(NAND_RDA_CMD);
        /* Read from the start of a page */
        nand_addr(0);
        nand_addr(0);
        /*Addressing page */
        nand_addr((uint8_t)page);
        nand_addr((uint8_t)(page >> 8));
        nand_addr((uint8_t)(page >> 16));
        nand_cmd(NAND_RDB_CMD);
        waitReady();
        /* Start generate ECC */
        EBI_StartNandEccGen();
        dmaRead(data, data_len);
        /* Stop generate ECC and get it */
        genecc = EBI_StopNandEccGen();

        /* Compare ECC when read a whole page */
        if (data_len == PAGE_DATA_SIZE)
        {
            rt_uint32_t ecc;
            /* Read ECC that save in spare area */
            readecc[0] = nand_read8();
            readecc[1] = nand_read8();
            readecc[2] = nand_read8();
            readecc[3] = nand_read8();

            ecc = (readecc[3] << 24) | (readecc[2] << 16) | (readecc[1] << 8) | readecc[0];

            /* skip the erased page */
            if (genecc == 0x00000000 && ecc == 0xffffffff)
                result = RT_MTD_EOK;
            else
            {
                result = nand_data_correct(genecc, ecc, data);
                if (result != RT_MTD_EOK)
                {
                    rt_kprintf("ecc correct error: genecc 0x%08x, ecc 0x%08x\n", genecc, ecc);
                }
            }

            if (spare && spare_len)
                goto _next;
        }
    }
    /* Read spare data area */
    if (spare && spare_len)
    {
        nand_cmd(NAND_RDA_CMD);
        /* Read from spare[4] */
        nand_addr(4);
        nand_addr(8);
        /* Addressing page */
        nand_addr((uint8_t)page);
        nand_addr((uint8_t)(page >> 8));
        nand_addr((uint8_t)(page >> 16));
        nand_cmd(NAND_RDB_CMD);

        waitReady();
    _next:
        dmaRead(&spare[4], spare_len - 4);
        rt_memcpy(spare, readecc, 4);
    }

    rt_mutex_release(&_nand.lock);

    return (result);
}

/**
 * @brief Write a page to nand flash
 * @param device: Pointer to MTD_NAND device
 * @param page: Relative page offset of a logical partition
 * @param data: Buffer of main data
 * @param data_len: The length you want to write
 * @param spare: Buffer of spare data
 * @param spare_len: The length you want to write
 * @return RT_MTD_EOK - No error occurs
 *         -RT_MTD_EIO - Programming fail
 */
rt_err_t nandflash_writepage(struct rt_mtd_nand_device *device, rt_off_t page, const rt_uint8_t *data, rt_uint32_t data_len, const rt_uint8_t *spare, rt_uint32_t spare_len)
{
    rt_uint32_t status;
    rt_uint32_t genecc;
    rt_err_t result;

    RT_ASSERT(data_len <= PAGE_DATA_SIZE);
    RT_ASSERT(spare_len <= PAGE_OOB_SIZE);
    result = RT_MTD_EOK;

    /* nand_debug("nand write[%d,%d,%d]\n",page,data_len,spare_len); */

    page = page + device->block_start * device->pages_per_block;
    if (page / device->pages_per_block > device->block_end)
    {
        return -RT_EIO;
    }

    rt_mutex_take(&_nand.lock, RT_WAITING_FOREVER);

    writeProtect(false);

    /* Programming main data area */
    if (data && data_len)
    {
        nand_cmd(NAND_PAGEPROG1_CMD);

        nand_addr(0);
        nand_addr(0);
        nand_addr((rt_uint8_t)page);
        nand_addr((rt_uint8_t)(page >> 8));
        nand_addr((rt_uint8_t)(page >> 16));
        waitReady();

        EBI_StartNandEccGen();
        dmaWrite(data, data_len);
        waitReady();
        genecc = EBI_StopNandEccGen();

        /* Write ECC to spare area only when data_len is a whole page */
        if (data_len == PAGE_DATA_SIZE)
        {
            /* Write ECC to spare area of nand */
            nand_write8(genecc);
            nand_write8(genecc >> 8);
            nand_write8(genecc >> 16);
            nand_write8(genecc >> 24);
            if (spare && spare_len)
                goto _next;
        }

        nand_cmd(NAND_PAGEPROG2_CMD);
        waitReady();

        status = readStatus();
        if ((status & 0x01) != 0)
        {
            result = -RT_MTD_EIO;
            goto _exit;
        }
    }

    /* Programming spare data area */
    if (spare && spare_len)
    {
        nand_cmd(NAND_PAGEPROG1_CMD);
        /* start from spare[4] */
        nand_addr(4);
        nand_addr(8);
        nand_addr((rt_uint8_t)page);
        nand_addr((rt_uint8_t)(page >> 8));
        nand_addr((rt_uint8_t)(page >> 16));
        waitReady();
    _next:
        dmaWrite(&spare[4], spare_len - 4);
        nand_cmd(NAND_PAGEPROG2_CMD);
        waitReady();

        status = readStatus();
        if ((status & 0x01) != 0)
        {
            result = -RT_MTD_EIO;
        }
    }

_exit:

    writeProtect(true);
    rt_mutex_release(&_nand.lock);
    return result;
}

/**
 * @brief Erase a block
 * @param device: Pointer to MTD_NAND device
 * @param block: Relative block offset of a logical partition
 * @return RT_MTD_EOK - Erase successfully
 *         -RT_MTD_EIO - Erase fail
 */
static rt_err_t nandflash_eraseblock(struct rt_mtd_nand_device *device, rt_uint32_t block)
{
    rt_err_t result;
    rt_uint32_t page;

    /* add the start blocks */
    block = block + device->block_start;

    page = block * PAGES_PER_BLOCK;
    result = RT_MTD_EOK;

    rt_mutex_take(&_nand.lock, RT_WAITING_FOREVER);

    writeProtect(false);

    nand_cmd(NAND_BLOCKERASE1_CMD);
    /* Coloumn address, bit 8 is not used, implicitely defined by NAND_RDA_CMD. */
    nand_addr((rt_uint8_t)page);
    nand_addr((rt_uint8_t)(page >> 8));
    nand_addr((rt_uint8_t)(page >> 16));
    nand_cmd(NAND_BLOCKERASE2_CMD);

    waitReady();

    if ((readStatus() & 0x01) != 0)
        result = -RT_MTD_EIO;

    writeProtect(true);

    rt_mutex_release(&_nand.lock);

    return (result);
}

// static rt_err_t nandflash_checkblock(struct rt_mtd_nand_device *device, rt_uint32_t block)
// {
//     return (RT_EOK);
// }

// static rt_err_t nandflash_mark_badblock(struct rt_mtd_nand_device *device, rt_uint32_t block)
// {
//     return (RT_EOK);
// }

static rt_err_t nandflash_pagecopy(struct rt_mtd_nand_device *device, rt_off_t src_page, rt_off_t dst_page)
{
    rt_err_t result;

    src_page = src_page + device->block_start * device->pages_per_block;
    dst_page = dst_page + device->block_start * device->pages_per_block;

    rt_mutex_take(&_nand.lock, RT_WAITING_FOREVER);
    writeProtect(false);

    nand_cmd(NAND_RDACPB_CMD);

    nand_addr(0);
    nand_addr(0);
    nand_addr((rt_uint8_t)src_page);
    nand_addr((rt_uint8_t)(src_page >> 8));
    nand_addr((rt_uint8_t)(src_page >> 16));

    nand_cmd(NAND_RDBCPB_CMD);
    waitReady();

    nand_cmd(NAND_CPBPROG1_CMD);
    nand_addr(0);
    nand_addr(0);
    nand_addr((rt_uint8_t)dst_page);
    nand_addr((rt_uint8_t)(dst_page >> 8));
    nand_addr((rt_uint8_t)(dst_page >> 16));
    nand_cmd(NAND_CPBPROG2_CMD);

    waitReady();

    result = (readStatus() & 0x01) ? -RT_MTD_EIO : RT_MTD_EOK;

    writeProtect(true);

    rt_mutex_release(&_nand.lock);

    return (result);
}

static const struct rt_mtd_nand_driver_ops ops = {
    nandflash_readid,
    nandflash_readpage,
    nandflash_writepage,
    nandflash_pagecopy,
    nandflash_eraseblock,
    RT_NULL,
    RT_NULL,
};
// nandflash_checkblock,
// nandflash_mark_badblock,

struct rt_mtd_nand_device _partition[3];

void rt_hw_mtd_nand_init(void)
{
    /* WP and R/B */
    GPIO_PinModeSet(NAND_WP_PORT, NAND_WP_PIN, gpioModePushPull, 0);    /* active low write-protect */
    GPIO_PinModeSet(NAND_READY_PORT, NAND_READY_PIN, gpioModeInput, 0); /* ready/busy */
    nandflash_readid(RT_NULL);
    DMA_CfgChannel(EFM32GG_EBI_DMACH, (void *)&chnCfg);
    rt_mutex_init(&_nand.lock, "nand", RT_IPC_FLAG_FIFO);
    rt_memset(&_nand.id, 0x00, 5);

    /* register nand0 */
    _partition[0].page_size = PAGE_DATA_SIZE;
    _partition[0].pages_per_block = PAGES_PER_BLOCK;
    _partition[0].plane_num = 2;
    _partition[0].oob_size = PAGE_OOB_SIZE;
    _partition[0].oob_free = 60;
    _partition[0].block_start = 0;
    _partition[0].block_end = 64;

    _partition[0].block_total = _partition[0].block_end - _partition[0].block_start;
    _partition[0].ops = &ops;

    rt_mtd_nand_register_device("nand0", &_partition[0]);

    /* register nand1 */
    _partition[1].page_size = PAGE_DATA_SIZE;
    _partition[1].pages_per_block = PAGES_PER_BLOCK;
    _partition[1].plane_num = 2;
    _partition[1].oob_size = PAGE_OOB_SIZE;
    _partition[1].oob_free = 60;
    _partition[1].block_start = 64;
    _partition[1].block_end = 1984;

    _partition[1].block_total = _partition[1].block_end - _partition[1].block_start;
    _partition[1].ops = &ops;

    rt_mtd_nand_register_device("nand1", &_partition[1]);
}

#if defined(RT_USING_DFS)
void uffs_init(void)
{
    rt_err_t ret;
    /* dfs init */
    ret = dfs_init();
    if (ret != 0)
    {
        rt_kprintf("dfs_init failed!\n");
    }
    dfs_uffs_init();
    if (ret != 0)
    {
        rt_kprintf("dfs_uffs_init failed!\n");
    }
}
FINSH_FUNCTION_EXPORT(uffs_init, uffs init);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(uffs_init, uffs init);
#endif

void uffs_mount(void)
{
    rt_err_t ret;
    /* mount root part */
    ret = dfs_mount("nand0", "/", "uffs", 0, 0);
    if (ret != RT_EOK)
    {
        dfs_mkfs("uffs", "nand0");
        ret = dfs_mount("nand0", "/", "uffs", 0, 0);
    }
    if (ret == RT_EOK)
    {
        rt_kprintf("Mount nand0 to /, Done!\n");
    }
    else
    {
        rt_kprintf("Mount nand0 failed.\n");
    }
    DIR *usr_path = opendir("/usr");
    if (usr_path == RT_NULL)
    {
        mkdir("/usr", 0);
        rt_kprintf("make a directory: '/usr'.\n");
    }
    else
    {
        closedir(usr_path);
    }

    /* mount boot part */
    ret = dfs_mount("nand1", "/usr", "uffs", 0, 0);
    if (ret != RT_EOK)
    {
        dfs_mkfs("uffs", "nand1");
        ret = dfs_mount("nand1", "/usr", "uffs", 0, 0);
    }
    if (ret == RT_EOK)
    {
        rt_kprintf("Mount nand1 to /usr, Done!\n");
    }
    else
    {
        rt_kprintf("Mount nand1 failed.\n");
    }
}
FINSH_FUNCTION_EXPORT(uffs_mount, uffs mount);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(uffs_mount, uffs mount);
#endif

void uffs_auto_mount(void)
{
    uffs_init();
    uffs_mount();
}
#endif

void nid(void)
{
    nandflash_readid(0);
}
FINSH_FUNCTION_EXPORT(nid, get nand id);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(nid, get nand id);
#endif

void nwrite(void)
{
    int i;
    rt_uint32_t partition = 0;
    int page = 0;
    rt_uint8_t spare[64];
    rt_uint8_t *data_ptr;
    struct rt_mtd_nand_device *device;

    if (partition >= 3)
        return;
    device = &_partition[partition];

    data_ptr = (rt_uint8_t *)rt_malloc(PAGE_DATA_SIZE);
    if (data_ptr == RT_NULL)
    {
        rt_kprintf("no memory.\n");
        return;
    }

    /* Need random data to test ECC */
    for (i = 0; i < PAGE_DATA_SIZE; i++)
        data_ptr[i] = 0x00;
    rt_memset(spare, 0xdd, sizeof(spare));
    nandflash_writepage(device, page, data_ptr, PAGE_DATA_SIZE, spare, sizeof(spare));
    rt_free(data_ptr);
}
FINSH_FUNCTION_EXPORT(nwrite, nand write test);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(nwrite, nand write test);
#endif

void nread(void)
{
    int i;
    rt_uint32_t partition = 0;
    int page = 0;
    rt_uint8_t spare[64];
    rt_uint8_t *data_ptr;
    struct rt_mtd_nand_device *device;

    if (partition >= 3)
        return;
    device = &_partition[partition];
    data_ptr = (rt_uint8_t *)rt_malloc(PAGE_DATA_SIZE);
    if (data_ptr == RT_NULL)
    {
        rt_kprintf("no memory\n");
        return;
    }

    rt_memset(spare, 0, sizeof(spare));
    rt_memset(data_ptr, 0, PAGE_DATA_SIZE);
    nandflash_readpage(device, page, data_ptr, PAGE_DATA_SIZE, spare, sizeof(spare));
    for (i = 0; i < 512; i++)
    {
        rt_kprintf("0x%X,", data_ptr[i]);
    }

    rt_kprintf("\n spare\n");
    for (i = 0; i < sizeof(spare); i++)
    {
        rt_kprintf("0x%X,", spare[i]);
    }
    rt_kprintf("\n\n");

    /* release memory */
    rt_free(data_ptr);
}
FINSH_FUNCTION_EXPORT(nread, nand read test);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(nread, nand read test);
#endif

void nerase_part(rt_uint32_t partition)
{
    rt_uint32_t index;
    struct rt_mtd_nand_device *device;

    if (partition >= 3)
    {
        rt_kprintf("outof partition.\n");
        return;
    }
    device = &_partition[partition];
    for (index = 0; index < device->block_total; index++)
    {
        nandflash_eraseblock(device, index);
    }
    rt_kprintf("erase _parttion[%d] finish(blk_total: %d).\n", partition, device->block_total);
}
void nerase_all(void)
{
    nerase_part(0);
    nerase_part(1);
}

FINSH_FUNCTION_EXPORT(nerase_all, nand erase all);
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(nerase_all, nand erase all);
#endif

#endif /* RT_USING_ETX_NAND */
