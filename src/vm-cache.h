#ifndef __VM_CACHE_H__
#define __VM_CACHE_H__
/*
 * vm-cache.h —minimal MMU + cache setup to speed up DMA
 */
#include "rpi.h"

//from VM lab
void mmu_init(void);
void mmu_enable(void);
void mmu_disable(void);
int  mmu_is_enabled(void);
void mmu_set_ctx(uint32_t pid, uint32_t asid, void *pt);
void domain_access_ctrl_set(uint32_t d);
void mmu_sync_pte_mods(void);

//ARMv6 1MB section descriptor(ARMB4-27)
//builds 32 bitsection descriptor
static inline uint32_t
mk_section_desc(uint32_t pa, uint32_t tex_c_b, uint32_t ap, uint32_t dom) {
    uint32_t desc = 0;
    desc |= (pa & 0xFFF00000);//[31:20] section base
    desc |= (ap & 0x3) << 10;//[11:10] AP
    desc |= ((ap >> 2) & 0x1)<<15; //[15] APX
    desc |= (dom & 0xF) <<5; //[8:5] domain
    desc |= ((tex_c_b >> 2) & 0x7)<< 12;//[14:12] TEX
    desc |= ((tex_c_b >> 1) &0x1)<< 3; //[3] C
    desc |= (tex_c_b & 0x1)<<2; //[2]B
    desc |= 0x2;//[1:0] =section
    return desc;
}

//TEX:C:B encodings (ARM B4-10)
#define VM_DEVICE 0//TEX=000,C=0,B=0: strongly ordered
#define VM_UNCACHED 4//TEX=001,C=0,B=0: normal uncached
#define VM_WB_NOALLOC 3//TEX=000,C=1,B=1: write-back, no write-alloc
#define VM_WT_NOALLOC 2//TEX=000,C=1,B=0: write-through, no write-alloc
#define VM_WB_ALLOC ((1<<2)|1|2)//TEX=001,C=1,B=1: write-back, write-alloc

#define VM_AP_RW_PRIV  0x1//AP=01,APX=0:kernel r/w

static uint32_t page_table[4096] __attribute__((aligned(16384)));//16KB aligned

static inline void vm_cache_setup(void) {
    //identity-mapped 1MB section page table
    //fill the page table with the appropriate section descriptors
    for (unsigned i =0; i<4096;i++) {
        uint32_t pa =i<<20;
        if (pa >= 0x20000000&&pa<0x20300000){
            //BCM peripherals:device memory
            page_table[i] = mk_section_desc(pa, VM_DEVICE, VM_AP_RW_PRIV,1);
        } else if (pa < 0x20000000){
            //normal memory: cacheable
            page_table[i]=mk_section_desc(pa, VM_WB_NOALLOC, VM_AP_RW_PRIV, 1);
        } else {
            page_table[i]=0;//unmapped (fault)
        }
    }
    mmu_init();
    domain_access_ctrl_set(0x1 << 2); //domain 1=client (bits [3:2])
    mmu_set_ctx(1, 1, page_table);
    mmu_enable();
    //enable caches+branch prediction(SCTLR bits)
    {
        uint32_t c;
        asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(c));
        c |= (1 << 2);//C
        c |= (1 << 3);//W
        c |= (1 << 11);//Z
        c |= (1 << 12);//I
        asm volatile("mcr p15, 0, %0, c1, c0, 0" :: "r"(c));
    }
}

//invalidate D-cache lines
static inline void dcache_inv_region(const void *addr, unsigned nbytes) {
    uint32_t start = (uint32_t)addr & ~31u;
    uint32_t end   = ((uint32_t)addr + nbytes + 31u) & ~31u;
    for (uint32_t a = start; a < end; a += 32)
        asm volatile("mcr p15, 0, %0, c7, c6, 1" :: "r"(a)); //MVA invalidate
    asm volatile("mcr p15, 0, %0, c7, c10, 4" :: "r"(0)); //DSB
}
#endif
