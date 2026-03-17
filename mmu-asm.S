//Reused from lab

@ Must carefully read B2 of the armv6 manual.  You cannot rely on "well it
@ worked on my test case": mistakes cause stale data, which may or may not
@ show up in your test case (likely not) despite being wildly broken.
@
@ Note: Rd is allegedly a read-only parameter to these instructions, but
@ the manual says SBZ (should be zero).  I think (and the linux source seems
@ to think) that this means we have to explicitly set it to 0.  Not setting
@ Rd=0 is an easy mistake to make.  Perhaps is worth building in (Linux does
@ not, but) --- the cost of an extra mov is negligible given our other 
@ overheads.
@
@ Alot of people put these instructions in inline assembly, wrapped up by 
@ function calls (e.g., cs107e's MMU code).  This practice is dangerous. 
@ For example, if you are flushing caches, you cannot be sure that the 
@ function return, etc does not then reload them, esp w.r.t. the BTB.  I 
@ think the only safe approach is to put directly in assembly so you are 
@ guaranteed no branch, indirect jump, load/store happens in between.
@
@ A lot of MMU code out there appears pretty broken b/c they don't do enough
@ flushing operations and/or rely on gcc code generation to not do the above.

#include "rpi-asm.h"
#include "armv6-coprocessor-asm.h"


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ many page numbers are in 
@    armv6-coprocessor-asm.h
@ and the other armv6-*.h files

// just read the cache type: use C code to figure out if it is unified
// b6-13, b6-14
MK_FN(get_cache_type)
    CACHE_TYPE_RD(r0) // b6-13: read-only cache type register
    //asm_not_implemented()
    bx lr

// void reset(void)
// clear the dcache, icache, tlbs
MK_FN(mmu_reset)

    // b staff_mmu_reset
    //invalidate dcache, icache, unified tlb, dsb, flush btb, prefetch flush

    //d cache before i cache due to the B2-20 stuff
    //don't clean data cache as shown on B2-20 due to lab README note

    //seems to be no reasoning for icache after dcache, tlb
    //B2-22 and B2-20 says that we need a dsb after dcache, icache, and tlb are set
    //""The execution of an Instruction cache maintenance operation is only guaranteed to be complete after the execution of a DSB barrier."" B2-22
    //"The completion of a TLB maintenance operation is only guaranteed to be completed by the execution of a DSB instruction."B2-22

    //B2-24 B2.7.5 for the btb and prefetch flush

    mov r0, #0 // rd sbz

    INV_DCACHE(r0)//b6-21 inv dcache first (b2-20 ordering)
    INV_ICACHE(r0)// b6-21 then icache; 4x+11nops for armv6 errata
    INV_TLB(r0)// b4-45 inv unified tlb after caches

    DSB(r0)// b2-22 wait for cache+tlb inv to complete
    DSB(r0)// b2-22 extra dsb per staff (conservative)

    FLUSH_BTB(r0)// b2-24 btb not asid-tagged, must flush
    DSB(r0) // b2-24 b2.7.5 dsb before prefetch flush
    //mcr p15, 0, %0, c7, c5, 4// old: broken c inline-asm syntax in .S
    PREFETCH_FLUSH(r0) // b2-24 b2.7.5: btb flush visible after this

    bx lr

// void cp15_domain_ctrl_wr(uint32_t dom_reg);
// NOTE:
//   - definitely need to do prefetch flush 
//   - I don't *think* we need the full cp15 sync: flush_btb, 
//     dsb, prefetch flush.
MK_FN(cp15_domain_ctrl_wr)
    //b staff_cp15_domain_ctrl_wr

    DOMAIN_CTRL_WR(r0) // b4-42 write domain access ctrl (c3,c0,0)
    mov r0, #0
    PREFETCH_FLUSH(r0)// b2-24 b2.7.6 cp15 write needs prefetch flush
    // staff decompilation confirmed no btb flush needed for domain writes

    //asm_not_implemented()
    bx lr

// void cp15_set_procid_ttbr0(uint32_t proc_and_asid, fld_t *pt);
// sequence from b2-25
// r0 = proc_and_asid: bits [7:0] = asid (1..63), bits [31:8] = procid
// r1 = pt: page table base (must be 16kb-aligned, b4-41)
MK_FN(cp15_set_procid_ttbr0)
    //b staff_cp15_set_procid_ttbr0

    mov r2, #0 // rd sbz

    DSB(r2) // b2-25 pending stores done before asid switch

    ASID_SET(r2)// b2-25 step 1 asid=0 first, no old tlb entries fire
    PREFETCH_FLUSH(r2)// arm1176 3-129 isb required after contextidr write

    TTBR0_SET(r1) // b4-41:new page table (16kb-aligned)
    TTBR1_SET(r2)// b4-41 zero out ttbr1, only using ttbr0
    TTBR_BASE_CTRL_WR(r2)// b4-41ttbc=0 so n=0, all va uses ttbr0

    FLUSH_BTB(r2)// b2-24 2.7.5: btb not asid-tagged, flush before new asid
    PREFETCH_FLUSH(r2)// b2-24 b2.7.5: btb flush only visible after this

    ASID_SET(r0)// b2-25 step 2: activate new address space
    FLUSH_BTB(r2)// b2-24: flush btb for new asid context
    PREFETCH_FLUSH(r2)// b2-24 b2.7.5 / arm1176 3-129: isb after contextidr

    //asm_not_implemented()
    bx lr

// void mmu_disable_set_asm(cp15_ctrl_reg1_t c);
MK_FN(mmu_disable_set_asm)

    mov r1, #0
    //b staff_mmu_disable_set_asm

    //condition check, condition check
    //takes in cp15_ctrl_reg1_t C


    //first clean and invalidate dcache
    //dsb
    //turn off MMU
    //prefetch flush - B2.7.6


    // B2.7.2""The execution of a Data (or unified) cache maintenance operation by Set/Way is not necessarily visible to other observers within the system until a DSB operation has been executed.""
    //flush btb B2-24 2.7.5
    //dsb from B2.7.5
    //prefetch flush as in B2.7.2 B2-22

    //do icache after mmu disabled because we swap to physical address space from virtual address space
    //ARM1176 6.4.1 ""Disable and invalidate the Instruction Cache for the corresponding world. You can then re-enable the Instruction Cache when you enable the MMU."

    @ note: this piece of code is mapped into the same place in both
    @ MMU-enabled and MMU-disabled so BTB and Prefetch entries work identically
    @ to both MMU-enable and disable.   Without this requirement I believe is
    @ very hard to get right (perhaps impossible?) since we cannot flush
    @ the BTB and prefetch and switch MMU on/off atomically.  thus, would
    @ flush, but instructions could get fetched before we could disable,
    @ defeating the purpose.

    CLEAN_INV_DCACHE(r1)// arm1176 6.9 clean dcache before mmu off (inaccessible after)
    DSB(r1)// b2-22 wait for clean to complete

    CONTROL_REG1_WR(r0) // b4-39/40 disable mmu (r0 has mmu_enabled=0)
    PREFETCH_FLUSH(r1)// b2-24 b2.7.6 cp15 write needs prefetch flush

    INV_ICACHE(r1)// arm1176 6.4.1 inv icache after mmu off (va->pa switch)

    FLUSH_BTB(r1)// b2-24 b2.7.5 btb not asid-tagged, stale in phys space
    DSB(r1)// b2-24 b2.7.5 dsb before prefetch flush
    PREFETCH_FLUSH(r1)// b2-24 b2.7.5 btb flush visible after this

    //asm_not_implemented()
    bx lr

// void mmu_enable_set_asm(cp15_ctrl_reg1_t c);
MK_FN(mmu_enable_set_asm)
    //b staff_mmu_enable_set_asm

    //asm_not_implemented()

    mov r1, #0x0 //set up operand
    //disable icache first arm1176, 6.4.1 step 3 B4-6
    CONTROL_REG1_RD(r2)
    bic r2, r2, #0x1000 // b4-40: clear bit 12 (icache enable) before mmu on
    CONTROL_REG1_WR(r2)

    PREFETCH_FLUSH(r1) //prefetch flush B2.7.6 B2-24 CP15 change needs prefetch flush
    //INVALIDATE ICACHE arm1176 6.4.1 step 3 B4.2.3
    INV_ICACHE(r1)// arm1176 6.4.1 step 3 inv icache after disabling it

    //B2-22 B2.7.2
    DSB(r1)// b2-22 icache inv complete before enabling mmu

    //enable MMU
    CONTROL_REG1_WR(r0) //arm1176 6.4.1 step 4

    //B2-24 prefetch flush 2.7.6 since MMU enable only visiable after prefetch flush
    PREFETCH_FLUSH(r1) // b2-24 b2.7.6 mmu enable visible after this

    //B2-24 2.7.5 have to flush BTB after enabling MMU
    FLUSH_BTB(r1)//b2-24 b2.7.5: btb stale in new va space

    //B2-24 2.7.5 BTB invalidation only useful after prefetch flush
    PREFETCH_FLUSH(r1)//b2-24 b2.7.5 btb flush visible after this

    bx lr

MK_FN(mmu_sync_pte_mods)

//B2-23
//STR rx, [Page table entry] ;
//Clean line [Page table entry]
//DSB ; ensures visibility of the data cleaned from the D Cache
//Invalidate TLB entry by MVA [page address]
//Invalidate BTB
//DSB ; ensure completion of the Invalidate TLB
//PrefetchFlush
    //b staff_mmu_sync_pte_mods

    mov r2, #0// rd sbz

    CLEAN_INV_DCACHE(r2)// b2-23 step 2: new pte must be visible to hw walker
    INV_ICACHE(r2)// test 4 requires icache flush on pte mods

    DSB(r2)// b2-23 step 3 / b2-22: wait for dcache+icache done

    INV_TLB(r2)// b2-23 step 4:remove stale tlb entry for old pte
    FLUSH_BTB(r2)// b2-23 step 5 / b2-24: btb stale after pte change

    DSB(r2)// b2-23 step 6/ b2-22: waitfor tlb+btb done
    PREFETCH_FLUSH(r2)// b2-23 step 7 / b2-24 b2.7.5:all ops visible

    bx lr

@ void cp15_ctrl_reg1_wr(cp15_ctrl_reg1_t r);
@ b4-40: write control register 1
MK_FN(cp15_ctrl_reg1_wr)
    CONTROL_REG1_WR(r0)
    bx lr
