/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include "mmu.h"
#include "rtthread.h"

#ifdef __CC_ARM
void mmu_setttbase(rt_uint32_t i)
{
    register rt_uint32_t value;

   /* Invalidates all TLBs.Domain access is selected as
    * client by configuring domain access register,
    * in that case access controlled by permission value
    * set by page table entry
    */
    value = 0;
    __asm
    {
        mcr p15, 0, value, c8, c7, 0
    }

    value = 0x55555555;
    __asm
    {
        mcr p15, 0, value, c3, c0, 0
        mcr p15, 0, i, c2, c0, 0
    }
}

void mmu_set_domain(rt_uint32_t i)
{
    __asm
    {
        mcr p15,0, i, c3, c0,  0
    }
}

void mmu_enable()
{
    register rt_uint32_t value;

    __asm
    {
        mrc p15, 0, value, c1, c0, 0
        orr value, value, #0x01
        mcr p15, 0, value, c1, c0, 0
    }
}

void mmu_disable()
{
    register rt_uint32_t value;

    __asm
    {
        mrc p15, 0, value, c1, c0, 0
        bic value, value, #0x01
        mcr p15, 0, value, c1, c0, 0
    }
}

void mmu_enable_icache()
{
    register rt_uint32_t value;

    __asm
    {
        mrc p15, 0, value, c1, c0, 0
        orr value, value, #0x1000
        mcr p15, 0, value, c1, c0, 0
    }
}

void mmu_enable_dcache()
{
    register rt_uint32_t value;

    __asm
    {
        mrc p15, 0, value, c1, c0, 0
        orr value, value, #0x04
        mcr p15, 0, value, c1, c0, 0
    }
}

void mmu_disable_icache()
{
    register rt_uint32_t value;

    __asm
    {
        mrc p15, 0, value, c1, c0, 0
        bic value, value, #0x1000
        mcr p15, 0, value, c1, c0, 0
    }
}

void mmu_disable_dcache()
{
    register rt_uint32_t value;

    __asm
    {
        mrc p15, 0, value, c1, c0, 0
        bic value, value, #0x04
        mcr p15, 0, value, c1, c0, 0
    }
}

void mmu_enable_alignfault()
{
    register rt_uint32_t value;

    __asm
    {
        mrc p15, 0, value, c1, c0, 0
        orr value, value, #0x02
        mcr p15, 0, value, c1, c0, 0
    }
}

void mmu_disable_alignfault()
{
    register rt_uint32_t value;

    __asm
    {
        mrc p15, 0, value, c1, c0, 0
        bic value, value, #0x02
        mcr p15, 0, value, c1, c0, 0
    }
}

void mmu_clean_invalidated_cache_index(int index)
{
    __asm
    {
        mcr p15, 0, index, c7, c14, 2
    }
}

void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(CACHE_LINE_SIZE - 1);

    while(ptr < buffer + size)
    {
        __asm
        {
            MCR p15, 0, ptr, c7, c14, 1
        }
        ptr += CACHE_LINE_SIZE;
    }
}

void mmu_clean_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        __asm
        {
            MCR p15, 0, ptr, c7, c10, 1
        }
        ptr += CACHE_LINE_SIZE;
    }
}

void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        __asm
        {
            MCR p15, 0, ptr, c7, c6, 1
        }
        ptr += CACHE_LINE_SIZE;
    }
}

void mmu_invalidate_tlb()
{
    register rt_uint32_t value;

    value = 0;
    __asm
    {
        mcr p15, 0, value, c8, c7, 0
    }
}

void mmu_invalidate_icache()
{
    register rt_uint32_t value;

    value = 0;

    __asm
    {
        mcr p15, 0, value, c7, c5, 0
    }
}


void mmu_invalidate_dcache_all()
{
    register rt_uint32_t value;

    value = 0;

    __asm
    {
        mcr p15, 0, value, c7, c6, 0
    }
}
#elif defined(__GNUC__)
void mmu_setttbase(register rt_uint32_t i)
{
    //register rt_uint32_t value;

   /* Invalidates all TLBs.Domain access is selected as
    * client by configuring domain access register,
    * in that case access controlled by permission value
    * set by page table entry
    */
#if 0
    value = 0;
    asm ("mcr p15, 0, %0, c8,c7, 0"::"r"(value));

    value = 0x55555555;
    asm ("mcr p15, 0, %0, c3,c0, 0"::"r"(value));
    asm ("mcr p15, 0, %0, c2,c0, 0"::"r"(i));
#endif

    asm (
        "mrc p15,0,r0,c1,c0,0 \r\n"
        "mov r1,#0x800000  \r\n"
        "orr r0,r0,r1    @ disable Subpage AP bits  \r\n"
        "mcr p15,0,r0,c1,c0,0 @ write value back  \r\n"
        "mov r0,#0x0  \r\n"
        "mcr p15,0,r0,c2,c0,2 @ W  \r\n"
    );
    asm("mcr p15,0,%0,c2,c0,0"::"r"(i));
}

void mmu_set_domain(register rt_uint32_t i)
{
    asm ("mcr p15,0, %0, c3, c0,  0": :"r" (i));
}

void mmu_enable()
{
    register rt_uint32_t i;

    /* read control register */
    asm ("mrc p15, 0, %0, c1, c0, 0":"=r" (i));

    i |= 0x1;

    /* write back to control register */
    asm ("mcr p15, 0, %0, c1, c0, 0": :"r" (i));
}

void mmu_disable()
{
    register rt_uint32_t i;

    /* read control register */
    asm ("mrc p15, 0, %0, c1, c0, 0":"=r" (i));

    i &= ~0x1;

    /* write back to control register */
    asm ("mcr p15, 0, %0, c1, c0, 0": :"r" (i));
}

void mmu_enable_icache()
{
    register rt_uint32_t i;

    /* read control register */
    asm ("mrc p15, 0, %0, c1, c0, 0":"=r" (i));

    i |= (1 << 12);

    /* write back to control register */
    asm ("mcr p15, 0, %0, c1, c0, 0": :"r" (i));
}

void mmu_enable_dcache()
{
    register rt_uint32_t i;

    /* read control register */
    asm ("mrc p15, 0, %0, c1, c0, 0":"=r" (i));

    i |= (1 << 2);

    /* write back to control register */
    asm ("mcr p15, 0, %0, c1, c0, 0": :"r" (i));
}

void mmu_disable_icache()
{
    register rt_uint32_t i;

    /* read control register */
    asm ("mrc p15, 0, %0, c1, c0, 0":"=r" (i));

    i &= ~(1 << 12);

    /* write back to control register */
    asm ("mcr p15, 0, %0, c1, c0, 0": :"r" (i));
}

void mmu_disable_dcache()
{
    register rt_uint32_t i;

    /* read control register */
    asm ("mrc p15, 0, %0, c1, c0, 0":"=r" (i));

    i &= ~(1 << 2);

    /* write back to control register */
    asm ("mcr p15, 0, %0, c1, c0, 0": :"r" (i));
}

void mmu_enable_alignfault()
{
    register rt_uint32_t i;

    /* read control register */
    asm ("mrc p15, 0, %0, c1, c0, 0":"=r" (i));

    i |= (1 << 1);

    /* write back to control register */
    asm ("mcr p15, 0, %0, c1, c0, 0": :"r" (i));
}

void mmu_disable_alignfault()
{
    register rt_uint32_t i;

    /* read control register */
    asm ("mrc p15, 0, %0, c1, c0, 0":"=r" (i));

    i &= ~(1 << 1);

    /* write back to control register */
    asm ("mcr p15, 0, %0, c1, c0, 0": :"r" (i));
}

void mmu_clean_invalidated_cache_index(int index)
{
    asm ("mcr p15, 0, %0, c7, c14, 2": :"r" (index));
}

void mmu_clean_invalidated_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(CACHE_LINE_SIZE - 1);

    while(ptr < buffer + size)
    {
        asm ("mcr p15, 0, %0, c7, c14, 1": :"r" (ptr));
        ptr += CACHE_LINE_SIZE;
    }
}


void mmu_clean_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        asm ("mcr p15, 0, %0, c7, c10, 1": :"r" (ptr));
        ptr += CACHE_LINE_SIZE;
    }
}

void mmu_invalidate_dcache(rt_uint32_t buffer, rt_uint32_t size)
{
    unsigned int ptr;

    ptr = buffer & ~(CACHE_LINE_SIZE - 1);

    while (ptr < buffer + size)
    {
        asm ("mcr p15, 0, %0, c7, c6, 1": :"r" (ptr));
        ptr += CACHE_LINE_SIZE;
    }
}

void mmu_invalidate_tlb()
{
    asm ("mcr p15, 0, %0, c8, c7, 0": :"r" (0));
}

void mmu_invalidate_icache()
{
    asm ("mcr p15, 0, %0, c7, c5, 0": :"r" (0));
}

void mmu_invalidate_dcache_all()
{
    asm ("mcr p15, 0, %0, c7, c6, 0": :"r" (0));
}
#endif

/* level1 page table */
//static volatile unsigned int _page_table[4*1024] __attribute__((aligned(16*1024)));
#define ARM1176_GCC_ALIGN(bits) __attribute__((aligned(1UL<<bits)))
#define ARM1176_MMU_TTB_ALIGNMENT 14
static volatile unsigned int _page_table[4096]
    ARM1176_GCC_ALIGN(ARM1176_MMU_TTB_ALIGNMENT) __attribute__ ((section(".nocache_buffer"))) = { 0x000011E2UL };

void mmu_setmtt(rt_uint32_t vaddrStart, rt_uint32_t vaddrEnd, rt_uint32_t paddrStart, rt_uint32_t attr)
{
    volatile rt_uint32_t *pTT;
    volatile int i,nSec;
    pTT=(rt_uint32_t *)_page_table+(vaddrStart>>20);
    nSec=(vaddrEnd>>20)-(vaddrStart>>20);
    for(i=0;i<=nSec;i++)
    {
        *pTT = attr |(((paddrStart>>20)+i)<<20);
        pTT++;
    }
}


void rt_hw_cpu_dump_page_table(rt_uint32_t *ptb)
{
    int i;
    int fcnt = 0;

    rt_kprintf("page table@%p\n", ptb);
    for (i = 0; i < 1024*4; i++)
    {
        rt_uint32_t pte1 = ptb[i];
        if ((pte1 & 0x3) == 0)
        {
            rt_kprintf("%03x: ", i);
            fcnt++;
            if (fcnt == 16)
            {
                rt_kprintf("fault\n");
                fcnt = 0;
            }
            continue;
        }
        if (fcnt != 0)
        {
            rt_kprintf("fault\n");
            fcnt = 0;
        }

        rt_kprintf("%03x: %08x: ", i, pte1);
        if ((pte1 & 0x3) == 0x3)
        {
            rt_kprintf("LPAE\n");
        }
        else if ((pte1 & 0x3) == 0x1)
        {
            rt_kprintf("pte,ns:%d,domain:%d\n",
                       (pte1 >> 3) & 0x1, (pte1 >> 5) & 0xf);
            /*
             *rt_hw_cpu_dump_page_table_2nd((void*)((pte1 & 0xfffffc000)
             *                               - 0x80000000 + 0xC0000000));
             */
        }
        else if (pte1 & (1 << 18))
        {
            rt_kprintf("super section,ns:%d,ap:%x,xn:%d,texcb:%02x\n",
                       (pte1 >> 19) & 0x1,
                       ((pte1 >> 13) | (pte1 >> 10))& 0xf,
                       (pte1 >> 4) & 0x1,
                       ((pte1 >> 10) | (pte1 >> 2)) & 0x1f);
        }
        else
        {
            rt_kprintf("section,ns:%d,ap:%x,"
                       "xn:%d,texcb:%02x,domain:%d\n",
                       (pte1 >> 19) & 0x1,
                       ((pte1 >> 13) | (pte1 >> 10))& 0xf,
                       (pte1 >> 4) & 0x1,
                       (((pte1 & (0x7 << 12)) >> 10) |
                        ((pte1 &        0x0c) >>  2)) & 0x1f,
                       (pte1 >> 5) & 0xf);
        }
    }
}

void rt_hw_mmu_init(struct mem_desc *mdesc, rt_uint32_t size)
{
    /* disable I/D cache */
    mmu_disable_dcache();
    mmu_disable_icache();
    mmu_disable();
    mmu_invalidate_tlb();

    /* set page table */
    for (; size > 0; size--)
    {
        mmu_setmtt(mdesc->vaddr_start, mdesc->vaddr_end,
            mdesc->paddr_start, mdesc->attr);
        mdesc++;
    }

    /* set MMU table address */
    mmu_setttbase((rt_uint32_t)_page_table);

    /* enables MMU */
    mmu_enable();

    /* enable Instruction Cache */
    mmu_enable_icache();

    /* enable Data Cache */
    mmu_enable_dcache();

    mmu_invalidate_icache();
    mmu_invalidate_dcache_all();
}




