.section trampolinesec

.global trampoline
.global user_trap_entry
.global user_trap_ret_end

trampoline:

user_trap_entry:
    csrrw a0, sscratch, a0
    sd x1, 0*8(a0)
    sd x2, 1*8(a0)
    sd x3, 2*8(a0)
    sd x4, 3*8(a0)
    sd x5, 4*8(a0)
    sd x6, 5*8(a0)
    sd x7, 6*8(a0)
    sd x8, 7*8(a0)
    sd x9, 8*8(a0)
    # sd x10, 9*8(a0)
    sd x11, 10*8(a0)
    sd x12, 11*8(a0)
    sd x13, 12*8(a0)
    sd x14, 13*8(a0)
    sd x15, 14*8(a0)
    sd x16, 15*8(a0)
    sd x17, 16*8(a0)
    sd x18, 17*8(a0)
    sd x19, 18*8(a0)
    sd x20, 19*8(a0)
    sd x21, 20*8(a0)
    sd x22, 21*8(a0)
    sd x23, 22*8(a0)
    sd x24, 23*8(a0)
    sd x25, 24*8(a0)
    sd x26, 25*8(a0)
    sd x27, 26*8(a0)
    sd x28, 27*8(a0)
    sd x29, 28*8(a0)
    sd x30, 29*8(a0)
    sd x31, 30*8(a0)

    # store the last reg a0
    mv a1, a0
    csrrw a0, sscratch, a0
    sd a0, 9*8(a1)

    # all regs stored, prepare to call handler

    # store spec for ret
    csrr a0, sepc
    sd a0, 31*8(a1)

    # restore tp
    ld tp, 32*8(a1)

    # set up kernel_trap_handler
    ld a0, 33*8(a1)
    csrw stvec, a0

    # to call user_trap_handler
    ld ra, 34*8(a1)

    # load kstack
    ld sp, 35*8(a1)
    li a0, 4096
    add sp, sp, a0

    # turn into kernel page table
    ld a0, 36*8(a1)
    csrw satp, a0
    sfence.vma zero, zero
    ret

user_trap_ret_end:
    # call with argument: user_trap_ret_end(proc_page_table)
    # a0 = prco_page_table
    csrw satp, a0
    sfence.vma zero, zero

    csrrw a0, sscratch, a0

    # store user a0 in sscratch
    ld a1, 9*8(a0)
    csrw sscratch, a1

    ld x1, 0*8(a0)
    ld x2, 1*8(a0)
    ld x3, 2*8(a0)
    ld x4, 3*8(a0)
    ld x5, 4*8(a0)
    ld x6, 5*8(a0)
    ld x7, 6*8(a0)
    ld x8, 7*8(a0)
    ld x9, 8*8(a0)
    # ld x10, 9*8(a0) # a0 shouldn't be restore here
    ld x11, 10*8(a0)
    ld x12, 11*8(a0)
    ld x13, 12*8(a0)
    ld x14, 13*8(a0)
    ld x15, 14*8(a0)
    ld x16, 15*8(a0)
    ld x17, 16*8(a0)
    ld x18, 17*8(a0)
    ld x19, 18*8(a0)
    ld x20, 19*8(a0)
    ld x21, 20*8(a0)
    ld x22, 21*8(a0)
    ld x23, 22*8(a0)
    ld x24, 23*8(a0)
    ld x25, 24*8(a0)
    ld x26, 25*8(a0)
    ld x27, 26*8(a0)
    ld x28, 27*8(a0)
    ld x29, 28*8(a0)
    ld x30, 29*8(a0)
    ld x31, 30*8(a0)
    csrrw a0, sscratch, a0

    sret
